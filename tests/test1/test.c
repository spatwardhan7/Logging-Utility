#include <stdio.h>
#include <stdlib.h>
#include "logutility.h"


struct thread_info {    /* Used as argument to thread_start() */
           pthread_t thread_id;        /* ID returned by pthread_create() */
           int       thread_num;       /* Application-defined thread # */
};


static void * thread_start(void *arg)
{
    struct thread_info *tinfo =(struct thread_info*) arg;
    char detail[]= "Detail string ";
    int count = 0;
    int LOG_TYPE = 0;

    while(1)
    {
         sleep(1);
            
         LOG_TYPE = count %4;
         if(LOG_TYPE == 0 && count <15)
            LOG_TYPE = 1; 

	     DMLog((DMLogLevel)LOG_TYPE,"Thread %d : LOG TYPE %d in test %d and str %s", tinfo->thread_num,(DMLogLevel)LOG_TYPE,count,detail);
         count++;

    }
    return NULL;
}


int main(int argc, char* argv[])
{
	
    int num_threads = 1; 
    struct thread_info *tinfo;
    pthread_attr_t attr;

    tinfo = (struct thread_info*)calloc(num_threads, sizeof(struct thread_info));

    int s;
    int tnum;	

    pthread_attr_init(&attr);     

    for (tnum = 0; tnum < num_threads; tnum++) 
    {
         tinfo[tnum].thread_num = tnum + 1;
        

         /* The pthread_create() call stores the thread ID into
            corresponding element of tinfo[] */

          s = pthread_create(&tinfo[tnum].thread_id, &attr,
                                  &thread_start, &tinfo[tnum]);
           
    }

    if(initLogger())
        return 1; 

    
    //DMLog(DM_LOG_WARNING,"Initial Test message version 0.000001 %d",5);


    while(1)
    {}
    
    return 0;
}
