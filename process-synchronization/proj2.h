#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <time.h>
#include <stdbool.h>
 

#define TO_MILISEC 1000
#define NECESSARY_ARGS 6
#define CMD_ARGS 5
#define DECIMAL_BASE 10
#define LOWER_LIMIT 0
#define UPPER_LIMIT 2000
#define PAGESIZE 4096



typedef struct shmem{
    int shmem_id_stat; // id of allocated shared memory for stats
    int *statistics; // array of shared variables
    FILE *file_p; //  file pointer
}shmem;

typedef struct semaphores{
    sem_t *writing; //semaphore for writing into a file                         
    sem_t *no_judge; //semaphore - judge                           
    sem_t *all_registered; // semaphore waiting for all immigigrants to be registred                             
    sem_t *judge_decision; //semaphore for decision
    sem_t *registration; // semaphore for registration queue
}semaphores;

void clean_shmem(shmem *info);
void clean_semaphores(semaphores *semaphore);
int init_shmem(shmem *info);
int init_semaphores(semaphores *semaphor);
int is_positive_integer(unsigned cmd_argument[], char *argv[]);
void immigrant(unsigned cmd_argument[], shmem *info, semaphores *semaphore, unsigned pos);
void judge(unsigned cmd_argument[], shmem *info, semaphores *semaphore);

enum cmd_arguments{PI,IG,JG,IT,JT}; // enum for command line argument
enum action_statistics{A,NE,NC,NB,JITB,IMM_ERROR,GEN_IMM}; //enum for shared memory variables


