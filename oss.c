#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <stdbool.h>

#include "oss.h"

#define SHMKEY 859047 /* Parent and child agree on common key.*/

int CLOCK_RATE = 100000;
int NANO_MAX = 1.0e9;
int CLOCK_INTERVAL = (int) (1.0e9 / 100000);

bool timeout = false;

// Signal handler for timeout
void timeoutHandler(int signum) {
    printf("Timeout reached. Terminating...\n");
    timeout = true;
}

// Signal handler for Ctrl-C
void ctrlCHandler(int signum) {
    printf("Ctrl-C received. Terminating...\n");
    timeout = true;
}

void helpPrint()
{
	printf("This is help!");
}

struct PCB
{
	int occupied;
	pid_t pid;
	int startSeconds;
	int startNano;
};


int get_random(int min, int max){
	return rand() % (max + min -1) + 1;
}


void table_pretty_print(SimClock *clock, struct PCB *procTable, int proc){
		printf("OSS PID: %d\tSysClockS: %d\tSysClockNano: %d\n", getpid(), clock->second, clock->nano);
		printf("Process Table: \n");
		printf("Entry\t\tOccupied\t\tPID\t\tStartS\t\tStartN\t\n");

		for(int i=0; i < proc; i++){
				printf(
						"%d\t\t%d\t\t%d\t\t%d\t\t%d\t\n\n", 
						i,
						procTable[i].occupied,
						procTable[i].pid,
						procTable[i].startSeconds,
						procTable[i].startNano
					);
		}
}


void spawn_worker(SimClock *clock, struct PCB *pcb, int time_limit, int proc){

	char second_arg[10];
	char nano_arg[15];
	sprintf(second_arg, "%d", get_random(1, time_limit));
	sprintf(nano_arg, "%d", get_random(1, (int) NANO_MAX));

	pid_t child_pid = fork();
	if (child_pid == 0)
	{
		char *args[] = {"./worker", second_arg, nano_arg, NULL};
		execvp(args[0], args);
	}
	else
	{
		for(int i=0; i < proc; i++){
			if(pcb[i].pid == 0){ // can also check if occupied == 0 but this space may be replaced by new process - so just check with pid 
				struct PCB newEntry;
				newEntry.occupied = 1;
				newEntry.pid = child_pid;
				newEntry.startSeconds = clock->second;
				newEntry.startNano = clock->nano;

				pcb[i] = newEntry;
				break;
			}
		}
	}
}


void initializeProcTable(struct PCB *pcb, int size){
	struct PCB newEntry;
	newEntry.occupied = 0;
	newEntry.pid = 0;
	newEntry.startSeconds = 0;
	newEntry.startNano = 0;

	for(int i=0; i < size; i++){
		pcb[i] = newEntry;
	}
}


int main(int argc, char **argv)
{

	signal(SIGALRM, timeoutHandler); // Timeout signal
    	signal(SIGINT, ctrlCHandler);   // Ctrl-C signal

	alarm(1);

	int proc, simul, option, time_limit;

	while ((option = getopt(argc, argv, "hn:s:t:")) != -1)
	{
		switch (option)
		{
			case 'h':
				helpPrint();
				return 0;
			case 'n':
				proc = atoi(optarg);
				break;
			case 's':
				simul = atoi(optarg);
				break;
			case 't':
				time_limit = atoi(optarg);
				break;
			case '?':
				if (optopt == 'c')
					fprintf(stderr, "Option -%c requires an argument. \n", optopt);
				else if (isprint(optopt))
					fprintf(stderr, "Unkown %c option character. \n", optopt);
				else
					fprintf(stderr, "Unkown option character '\\x%x'. \n");
				return 1;
			default:
				helpPrint();
				return 0;
		}
	}

	struct PCB procTable[proc];
	initializeProcTable(procTable, proc);

	int shmid = shmget(SHMKEY, sizeof(SimClock), 0666 | IPC_CREAT);
	if (shmid == -1)
	{
		perror("Failed to create a shared memory!");
		exit(1);
	}

	SimClock *clock = shmat(shmid, NULL, 0);
	if (clock == (void *) -1)
	{
		perror("Failed to attack to shared memory!");
		exit(1);
	}

	int count_siml = 0, clock_counter = 0;

	// intialize system clock
	clock->nano = 0;
	clock->second = 0;

	for(int i=0; i < simul; i++){
		spawn_worker(clock, procTable, time_limit, proc);
		count_siml++;
	}

	int total_launched = count_siml;

	while (!timeout)   // wait for 60 sec in real life before terminating
	{	

		// increment clock here
		clock->second += (int) ((clock->nano + CLOCK_INTERVAL) /  NANO_MAX);
		clock->nano = CLOCK_INTERVAL * (clock_counter % CLOCK_RATE);
		clock_counter++;

		if ((clock->nano % (int) (NANO_MAX / 2)) == 0)
		{
			table_pretty_print(clock, procTable, proc);

		}


		int terminated_pid = waitpid(-1, NULL, WNOHANG);
		if (terminated_pid != 0)
		{
			for(int i=0; i < proc; i++){
				if(procTable[i].pid == terminated_pid){
					procTable[i].occupied = 0;
					break;
				}
			}
			count_siml--;
		}

		if (count_siml >= simul || total_launched > proc)
			continue;

		count_siml++;
		total_launched++;

		spawn_worker(clock, procTable, time_limit, proc);

	}

	// Do clean up
	for(int i=0; i < proc; i++){
		if(procTable[i].occupied == 1){
			kill(procTable[i].pid, SIGKILL);
		}
	}

	shmdt(clock);
	return 0;
}
