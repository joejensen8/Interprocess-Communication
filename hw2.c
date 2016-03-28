#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define ALPHA_OFFSET 97
#define LETTERS 26
#define NUMBER_OF_MAPPERS 4
#define NUMBER_OF_REDUCERS 26

// To Do: 
// Error check all system calls
// closing all pipes?

int main (void) {
	int counts[LETTERS] = {0}; // stores letter counts
	int mapper_pipes[NUMBER_OF_MAPPERS][2]; // stores all the pipes to the mappers
	int reducer_pipes[NUMBER_OF_REDUCERS][2]; // stores all the pipes to reducers
	

	// need to make all the pipes before making the children...
	// make mapper pipes
	int j; // used to iterate for pipe loops
	for (j = 0; j < NUMBER_OF_MAPPERS; j++) {
		if (pipe(mapper_pipes[j]) == -1) {
			perror("pipe");
			exit(errno);
		}
	}
	// make reducer pipes
	for (j = 0; j < NUMBER_OF_REDUCERS; j++) {
		if (pipe(reducer_pipes[j]) == -1) {
			perror("pipe");
			exit(errno);
		}
	}



	char buffer[BUFFER_SIZE]; // buffer used when reading input
	FILE *input_file = fopen("input.txt", "r"); // the input file!	

	// sending lines of input to the mappers
	int mapper = 0;
	while (fgets(buffer, BUFFER_SIZE, input_file) > 0) {
		if (write(mapper_pipes[mapper][1], buffer, (strlen(buffer)+1)) == -1) {
			perror("write");
			exit(errno);
		}
		if (close(mapper_pipes[mapper][1]) == -1) { // close pipe from parent to mapper.	
			perror("close");
			exit(errno);
		}	
		mapper++;
	}
	fclose(input_file);



	// make the 4 children (mappers)
	int i, pid;
	for (i = 0; i < NUMBER_OF_MAPPERS; i++) {
		pid = fork();
   		if(pid < 0) {
        		perror("fork error");
			exit(errno);
    		} else if (pid == 0) {
			// read from pipe!
			char readbuffer[BUFFER_SIZE];
			int bytes = read(mapper_pipes[i][0], readbuffer, sizeof(readbuffer));
			if (bytes == -1) {
				perror("read");
				exit(errno);
			}
			int k = 0;
			// loop through buffer to inspect the letters
			for (k = 0; k < strlen(readbuffer); k++) {
				if (readbuffer[k] >= ALPHA_OFFSET && buffer[k] < ALPHA_OFFSET + LETTERS) {
					// send to reducer...
					int reducer = readbuffer[k] - ALPHA_OFFSET;
					char character[2] = {readbuffer[k], '\0'};
					if (write(reducer_pipes[reducer][1], character, 1) == -1) {
						perror("write");
						exit(errno);
					}
				}
			}		
			exit(0);  
    		} else  {
			// will wait until mappers are done.
			wait(NULL);
    		}
	}

	// This for loop closes pipes.
	for (i = 0; i < NUMBER_OF_REDUCERS; i++) {
		if (close(reducer_pipes[i][1]) == -1) {
			perror("close");
			exit(errno);
		}
	}
	
	// make 26 children (reducers)
	// for each iteration, a child is created (reducer) and the reducer reads input from the mappers, counting characters
	for (i = 0; i < NUMBER_OF_REDUCERS; i++) {
		pid = fork();
		if (pid < 0) {
			perror("fork error");
			exit(errno);
		} else if (pid == 0) {
			// Read input from mapper
			char readbuffer[BUFFER_SIZE];
			int bytes = read(reducer_pipes[i][0], readbuffer, sizeof(readbuffer));
			if (bytes == -1) {
				perror("read");
				exit(errno);
			}	
			int count = 0;
			int j = 0;
			// this following loop goes through the pipe input buffer to count only it's characters.
			for (j = 0; j < strlen(readbuffer); j++) {
				// check if the character is in fact the correct char
				if (readbuffer[j] == (ALPHA_OFFSET + i)) {
					count++;
				}
			}
			// print output - how many times the lower case letter occured in the input file.
			printf("count %c: %d\n", i + ALPHA_OFFSET, count); 
			exit(0);
		} else {
			wait(NULL);
		}
	}

	return 0;
}


