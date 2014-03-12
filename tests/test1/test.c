#include <stdio.h>
#include <stdlib.h>
#include "logutility.h"


struct thread_info {    /* Used as argument to thread_start() */
           pthread_t thread_id;        /* ID returned by pthread_create() */
           int       thread_num;       /* Application-defined thread # */
           int       num_threads;       /* Number of Threads */
};


static void * thread_start(void *arg)
{
    struct thread_info *tinfo =(struct thread_info*) arg;
    char detail[]= "Bad State";
    int count = 0;
    int LOG_TYPE = 0;
    time_t t;

    /* Intializes random number generator */
    srand((unsigned) time(&t));

    /* 
     * Generate all types of messages with different varargs
     */
    while(1)
    {
        /* 
         * In case of only 1 thread, generate messages sequentially
         * starting from DMLogLevel 0 to 3 to ensure correctness
         * Generate random messages in case there are more than 1 threads
         */
        LOG_TYPE = (tinfo->num_threads == 1 ? (count % 4) : ((rand()+tinfo->thread_num) % 4));
        sleep(2);
        switch(LOG_TYPE)
        {
            case DM_LOG_ERROR:
                DMLog((DMLogLevel)LOG_TYPE,"Thread %d: Error message in test %d and %s occurred", tinfo->thread_num,count+1,detail);
                break;
            
            case DM_LOG_WARNING:
                DMLog((DMLogLevel)LOG_TYPE,"Thread %d: Warning message in test %d", tinfo->thread_num,count+1);
                break;

            case DM_LOG_TRACE:
                DMLog((DMLogLevel)LOG_TYPE,"Thread %d: Trace message in test %d", tinfo->thread_num,count+1);
                break;

            case DM_LOG_INFO:
                DMLog((DMLogLevel)LOG_TYPE,"Thread %d: Info message in test %d", tinfo->thread_num, count+1);
                break;

            default:
                break;
        }
        
        count++;
        
    }
    return NULL;
}


int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("Usage: ./test <num_threads>\n");
        return 1;
    }

    int num_threads = atoi(argv[1]);

    /* Initialize Logger */
    if(initLogger() == INIT_LOGGER_FAILED)
        return 1;     
 
    struct thread_info *tinfo;
    pthread_attr_t attr;

    tinfo = (struct thread_info*)calloc(num_threads, sizeof(struct thread_info));

    if(!tinfo)
        return 1;

    int s;
    int tnum;
	
    /* Create Threads which generate messages to be logger */
    pthread_attr_init(&attr);     

    for (tnum = 0; tnum < num_threads; tnum++) 
    {
         tinfo[tnum].thread_num = tnum + 1;
         tinfo[tnum].num_threads = num_threads;
        
          s = pthread_create(&tinfo[tnum].thread_id, &attr,
                                  &thread_start, &tinfo[tnum]);       
          if(s != 0)
          {
              printf("Thread Creation Failed\n");
              return 1; 
          }
    }

    /* Call pthread join*/
    for (tnum = 0; tnum < num_threads; tnum++) 
    {
          s = pthread_join(tinfo[tnum].thread_id, NULL);       
    }
    
    return 0;
}
