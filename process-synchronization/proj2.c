
        /*****  IOS-projekt2-synchronizace procesu *****/

/***************************************************************/ 
/* proj2.c                                                     */ 
/* Autor: Lukas Foltyn                                         */ 
/* Fakulta: VUT - Fakulta informacnich technologii             */ 
/* Posledni zmena:15.4.2020                                    */ 
/***************************************************************/
   

#include "proj2.h"

int main(int argc, char **argv)
{    
    unsigned cmd_argument[CMD_ARGS];
    if(argc != NECESSARY_ARGS || !is_positive_integer(cmd_argument, argv)) //checking all the command line arguments
    {
        fprintf(stderr,"Wrong arguments!\n");
        return 1;
    }

    shmem info; //structer for shared memory
    semaphores semaphore; //structer for semaphores
    if(!init_shmem(&info)) 
    {
        clean_shmem(&info);
        fprintf(stderr, "Error: shared memory initialization.\n");
        return 1;
    }    
    if(!init_semaphores(&semaphore))
    {
        clean_semaphores(&semaphore); 
        fprintf(stderr, "Error: semaphores initialization.\n");
        return 1;

    }
    bool error_fork=false;           
    pid_t judge_process_id=fork();
    if(judge_process_id == 0) //judge
    {  
        judge(cmd_argument, &info, &semaphore);//calling judge
        exit(0);
    }
    else if(judge_process_id < 0) //error - forking the process
    {
        error_fork=true;
        goto EXIT_NO_WAIT;    
    }    
    
    pid_t generator_process_id=fork();
    if(generator_process_id == 0) //immigrant generator 
    {
        for(unsigned i=0; i<cmd_argument[PI];i++)
        {
            pid_t immigrant_process_id = fork();
            if(immigrant_process_id == 0) //immigrants
            {
                sem_wait(semaphore.writing);
                info.statistics[GEN_IMM]++; //counting generated immigrants
                sem_post(semaphore.writing);
                immigrant(cmd_argument,&info, &semaphore, i+1); //calling immigrant
            }
            else if(immigrant_process_id < 0)//error - forking the process 
            {  
                info.statistics[IMM_ERROR]=1;
                while(wait(NULL) > 0);
                exit(1);
            }
            if(cmd_argument[IG] != 0)
                usleep((rand()%cmd_argument[IG])*TO_MILISEC); //waiting before another loop    
        }
        while(wait(NULL) > 0);
        exit(0);        
            
    }
    else if(generator_process_id < 0) //error - forking the process
    { 
        info.statistics[IMM_ERROR]=1;
        error_fork=true;
        goto EXIT_WAIT_JUDGE;   
    }
    int exit_status; 
    
    waitpid(generator_process_id, &exit_status, 0);
    if(WEXITSTATUS(exit_status))
        error_fork=true;
    EXIT_WAIT_JUDGE:
    waitpid(judge_process_id, NULL, 0);
    EXIT_NO_WAIT:
    clean_shmem(&info);
    clean_semaphores(&semaphore);
    if(error_fork)
    {
        fprintf(stderr,"Error: duplicating processes.\n");
        return 1;         
    }
    return 0;   
}
void immigrant(unsigned cmd_argument[], shmem *info, semaphores *semaphore, unsigned pos)
{
    sem_wait(semaphore->writing);
        fprintf(info->file_p,"%d : IMM %d : starts\n",++info->statistics[A],pos);
        fflush(info->file_p);
    sem_post(semaphore->writing);
    
    sem_wait(semaphore->no_judge); //immigrant waiting in queue to get into building --> entering
    sem_wait(semaphore->writing);  
        fprintf(info->file_p,"%d : IMM %d : enters : %d : %d : %d\n",++info->statistics[A],pos,++info->statistics[NE],info->statistics[NC],++info->statistics[NB]);
        fflush(info->file_p);
    sem_post(semaphore->writing); 
    sem_post(semaphore->no_judge);
    
    sem_wait(semaphore->registration); //immigrant waiting in queue to be registered --> being checked in
    sem_wait(semaphore->writing);       
        fprintf(info->file_p,"%d : IMM %d : checks : %d : %d : %d\n",++info->statistics[A],pos,info->statistics[NE],++info->statistics[NC],info->statistics[NB]);
        fflush(info->file_p);
    sem_post(semaphore->writing);
       
    if(info->statistics[JITB] && info->statistics[NC] == info->statistics[NE])   
        sem_post(semaphore->all_registered); // if all immigrants registered --> judge gets sign --> confirmation is starting
    else
        sem_post(semaphore->registration); //else waiting for the rest of immigrants to be registered
    
    sem_wait(semaphore->judge_decision); //judgne has made decision --> immigrants can go for certificate
    
    sem_wait(semaphore->writing);
        fprintf(info->file_p,"%d : IMM %d : wants certificate : %d : %d : %d\n",++info->statistics[A],pos,info->statistics[NE],info->statistics[NC],info->statistics[NB]);
        fflush(info->file_p);
    sem_post(semaphore->writing);
    
    if(cmd_argument[IT] != 0)
        usleep((rand()%cmd_argument[IT])*TO_MILISEC); //time of getting the certificate
   
    sem_wait(semaphore->writing);
        fprintf(info->file_p,"%d : IMM %d : got certificate : %d : %d : %d\n",++info->statistics[A],pos,info->statistics[NE],info->statistics[NC],info->statistics[NB]);
        fflush(info->file_p);
    sem_post(semaphore->writing);

    sem_wait(semaphore->no_judge);//if judge is not in the bulding --> immigrant leaves
    sem_wait(semaphore->writing);
        fprintf(info->file_p,"%d : IMM %d : leaves : %d : %d : %d\n",++info->statistics[A],pos,info->statistics[NE],info->statistics[NC],--info->statistics[NB]);
        fflush(info->file_p);
    sem_post(semaphore->writing);
    sem_post(semaphore->no_judge);
          
    exit(0);
}
void judge(unsigned cmd_argument[], shmem *info, semaphores *semaphore)
{
    unsigned ready_for_certificate;
    unsigned confirmed = 0;
    while((cmd_argument[PI] != confirmed && !info->statistics[IMM_ERROR]) || (unsigned)info->statistics[GEN_IMM] != confirmed) //looping until every generated immigrant was confirmed
    {   
        sem_wait(semaphore->writing);    
            fprintf(info->file_p,"%d : JUDGE : wants to enter\n", ++info->statistics[A]);
            fflush(info->file_p);
        sem_post(semaphore->writing);
    
        sem_wait(semaphore->no_judge);//judge is getting into the building
        sem_wait(semaphore->registration);
        sem_wait(semaphore->writing);   
            fprintf(info->file_p,"%d : JUDGE : enters : %d : %d : %d\n", ++info->statistics[A], info->statistics[NE],info->statistics[NC],info->statistics[NB]);
            fflush(info->file_p);
            info->statistics[JITB]=1;
        sem_post(semaphore->writing);
        
        if(info->statistics[NE] > info->statistics[NC])//waiting for everyone in the building to register
        { 
            sem_wait(semaphore->writing);  
                fprintf(info->file_p,"%d : JUDGE : waits for imm : %d : %d : %d\n", ++info->statistics[A],info->statistics[NE],info->statistics[NC],info->statistics[NB]);
                fflush(info->file_p);
            sem_post(semaphore->writing);
            sem_post(semaphore->registration);
            sem_wait(semaphore->all_registered); // if everyone registered --> immigrant process opnes semaphore for judge to start confirmation
         }           
        
        sem_wait(semaphore->writing); // everyone in the building registered --> starting confirmation
            fprintf(info->file_p,"%d : JUDGE : starts confirmation : %d : %d : %d\n", ++info->statistics[A],info->statistics[NE],info->statistics[NC],info->statistics[NB]);
            fflush(info->file_p);
        sem_post(semaphore->writing);
    
        if(cmd_argument[JT] != 0)
            usleep((rand()%cmd_argument[JT])*TO_MILISEC); // making decision

        sem_wait(semaphore->writing);
            confirmed += info->statistics[NE];
            ready_for_certificate = info->statistics[NE]; 
            info->statistics[NE] = 0;
            info->statistics[NC] = 0;
            fprintf(info->file_p,"%d : JUDGE : ends confirmation : %d : %d : %d\n", ++info->statistics[A],info->statistics[NE],info->statistics[NC],info->statistics[NB]);
            fflush(info->file_p);
        sem_post(semaphore->writing);
        
        for(unsigned i=0; i<ready_for_certificate;i++) 
             sem_post(semaphore->judge_decision); // letting immigrants go for certificate
        if(cmd_argument[JT]!= 0)
            usleep((rand()%cmd_argument[JT])*TO_MILISEC);
        
        sem_wait(semaphore->writing);
            fprintf(info->file_p,"%d : JUDGE : leaves : %d : %d : %d\n", ++info->statistics[A],info->statistics[NE],info->statistics[NC],info->statistics[NB]);
            fflush(info->file_p);
            info->statistics[JITB]=0;
        sem_post(semaphore->writing);
        
        sem_post(semaphore->registration); 
        sem_post(semaphore->no_judge);//judge leaving the building
        
        if(cmd_argument[JG] != 0 && cmd_argument[PI] != confirmed)
            usleep((rand()%cmd_argument[JG])*TO_MILISEC);
    }
    sem_wait(semaphore->writing); //every generated immigrant was confirmed --> judge finishes
        fprintf(info->file_p,"%d : JUDGE : finishes\n", ++info->statistics[A]);
        fflush(info->file_p);
    sem_post(semaphore->writing);
}

                /***** argument checking *****/

int is_positive_integer(unsigned cmd_argument[], char *argv[])
{
    char *no_num_part;
    long int checked_value;
    for(short unsigned i=0;i<CMD_ARGS;i++)   
    {           
        checked_value=strtol(argv[i+1],&no_num_part,DECIMAL_BASE);
        if(!i) 
        {
            if(checked_value <= LOWER_LIMIT || strcmp(no_num_part,""))
                return 0;
        }
        else if(checked_value < LOWER_LIMIT || checked_value > UPPER_LIMIT ||strcmp(no_num_part,""))
            return 0;
        cmd_argument[i]=checked_value;
    }
    return 1;
}

              /***** shared memory - initialization *****/

int init_shmem(shmem *info)
{
    if((info->shmem_id_stat = shmget(IPC_PRIVATE, PAGESIZE, IPC_CREAT | 0666 )) == -1) //getting id of allocated shared memory
        return 0;
    if((info->statistics = (int*) shmat(info->shmem_id_stat,NULL,0)) == (void*) -1 || (info->file_p = fopen("proj2.out", "w")) == NULL) //attaching the shared memory to a pointer   
    {                                                                                                                                   //opening the file
        clean_shmem(info);
        return 0;
    }
    
    return 1;
}
            /***** cleaning shared memory *****/

void clean_shmem(shmem *info)
{
    shmdt(info->statistics);
    shmctl(info->shmem_id_stat, IPC_RMID, NULL);
    if(info->file_p != NULL)
        fclose(info->file_p);

}
            /***** cleaning semaphores *****/

void clean_semaphores(semaphores *semaphore)
{
    sem_unlink("/SemaphoreUno");
    sem_close(semaphore->writing);
    sem_unlink("/SemaphoreDeux");
    sem_close(semaphore->no_judge);
    sem_unlink("/SemaphoreTroi");
    sem_close(semaphore->all_registered);
    sem_unlink("/SemaphoreQuatre");
    sem_close(semaphore->judge_decision); 
    sem_unlink("/SemaphoreCinq");
    sem_close(semaphore->registration);        
}
    /***** semaphores - initialization *****/

int init_semaphores(semaphores *semaphore)
{
    if(((semaphore->writing = sem_open("/SemaphoreUno", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) ||
    ((semaphore->no_judge = sem_open("/SemaphoreDeux", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) ||
    ((semaphore->all_registered = sem_open("/SemaphoreTroi", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) ||
    ((semaphore->judge_decision = sem_open("/SemaphoreQuatre", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) ||
    ((semaphore->registration = sem_open("/SemaphoreCinq", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED))    
        return 0;
    return 1;
}
