#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <ctype.h>

#include "oss.h"

#define SHMKEY	859047             /* Parent and child agree on common key.*/

void help(){
	printf("This is help!");
}


int main(int argc, char** argv){
	int sec, nano;
		
	if(argc < 3){
		perror("WORKER: Too few arguments!");
		exit(1);
	}

	sec = atoi(argv[1]);
        nano = atoi(argv[2]);

	int shmid = shmget(SHMKEY, sizeof(SimClock), 0666 | IPC_CREAT);
	if(shmid == -1){
		perror("WORKER: Failed to get the shared memory created by parent");
		exit(1);
	}

	SimClock *clock = shmat(shmid, NULL, 0);

	if(clock == (void*) -1){
		perror("WORKER: Failed to attach to shared memory");
		exit(1);
	}
	int startSec = clock->second;

	int targetSec = clock->second + sec;
	int targetNano = clock->nano + nano;

	printf("\n\nWORKER PID: %d\tPPID: %d\tSysClockS: %d\tSysClockN: %d\tTermTimeS: %d\tTermTimeNano: %d\t--Just Starting\n\n", getpid(), getppid(), clock->second, clock->nano, targetSec, targetNano);

	int prevSec = startSec;
	while(1){
		if(clock->second >= prevSec + 1){
			printf("\n\nWORKER PID: %d\tPPID: %d\tSysClockS: %d\tSysClockN: %d\tTermTimeS: %d\tTermTimeNano: %d\t--%d seconds have passed since starting\n\n", getpid(), getppid(), clock->second, clock->nano, targetSec, targetNano, (int) clock->second - startSec);
			prevSec = clock->second;
		}
		if(clock->second >= targetSec) break;
		else if(clock->second == targetSec && clock->nano >= targetNano) break;
	}
	
	printf("\n\nWORKER PID: %d\tPPID: %d\tSysClockS: %d\tSysClockN: %d\tTermTimeS: %d\tTermTimeNano: %d\t--Terminating\n\n", getpid(), getppid(), clock->second, clock->nano, targetSec, targetNano);

	
	return 0;
}


