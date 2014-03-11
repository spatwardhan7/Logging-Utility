#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <queue>
#include <string>

#include "logutility.h"
#include "config.h"

//JUST FOR GIT

const char * DM_ERROR_MSG =   "<DM_LOG_ERROR>   :";
const char * DM_WARNING_MSG = "<DM_LOG_WARNING> :";
const char * DM_TRACE_MSG =   "<DM_LOG_TRACE>   :";
const char * DM_INFO_MSG =    "<DM_LOG_INFO>    :";

static std::queue<char*> logQueue;
static pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE *fp;
static pthread_t aggressive_t;
static sigset_t waitset;
static siginfo_t info;

static void consumeFromQueue()
{
	pthread_mutex_lock(&mutex);	
    if(!logQueue.empty())
    {
        char *str = logQueue.front(); 
	    logQueue.pop();
        pthread_mutex_unlock(&mutex);
        if(LOG_TO_FILE == 1)
        {
            fprintf(fp,"%s\n",str);
            fflush(fp);
        }
        if(LOG_TO_CONSOLE == 1)
            printf("%s\n",str);    
    }
	else
    {  
        pthread_mutex_unlock(&mutex);
    }

}

static bool isQueueEmpty()
{
    bool bEmpty = false;
    pthread_mutex_lock  (&mutex);
    bEmpty = logQueue.empty() ? true : false;
    pthread_mutex_unlock(&mutex);
    return bEmpty;
}

static void * aggressiveConsumer(void *arg)
{

    // Run infinitely
    int result = 0;
    while(1)
    {
        result = sigwaitinfo( &waitset, &info );     
        if( result == 0 )
        {
            fprintf(stderr,"sigwaitinfo() SUCCESS returned for signal %d\n",info.si_signo );
            while(!isQueueEmpty())
            {         
                consumeFromQueue();
            }
        }
        else 
        {
            fprintf( stderr,"sigwait() function failed error number %d\n", errno );
        }
    }
    return NULL;
}


static void * periodicConsumer(void *arg)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = TIMER_MS * N_SEC_TO_M_SEC;

    while(1)
    {        
        consumeFromQueue();
        nanosleep(&ts,NULL);
    }
    return NULL;
}

static int openLogFile()
{
    if(LOG_TO_FILE == 1)
    {
        time_t t = time(NULL);
	    struct tm tm= *localtime(&t);	
        std::string buffer;
 
        sprintf((char*)buffer.c_str(),"%d-%d-%d_%d-%d-%d.txt", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

    	fp = fopen(buffer.c_str(),"w");
	    if(!fp)
        {
	        fprintf(stderr,"Failed to open log file: %s\n", strerror(errno));
               return 1;   
        }
    }
    return 0;
}

static int createThread(pthread_t *thread,pthread_attr_t *attr, void *(*start_routine) (void *))
{
        
    if(pthread_attr_init(attr))
    {
	    fprintf(stderr,"Failed to init logger thread: %s\n", strerror(errno));
        return INIT_LOGGER_FAILED;
    }    
    
    if(pthread_create(thread, attr,start_routine, NULL))
    {
	    fprintf(stderr,"Failed to create logger thread: %s\n", strerror(errno));
        return INIT_LOGGER_FAILED;
    }

    if(pthread_detach(*thread))
    {
	    fprintf(stderr,"Failed to detach thread: %s\n", strerror(errno));
        return INIT_LOGGER_FAILED;
    }

    return INIT_LOGGER_OK; 
}

void catcher( int sig ) {
    printf( "Signal catcher called for signal %d\n", sig );
}

static void setupSignal(struct sigaction sigact)
{
    sigemptyset( &sigact.sa_mask );
    sigact.sa_flags = 0;
    sigact.sa_handler = catcher;
    sigaction( SIGUSR1, &sigact, NULL );

    sigemptyset( &waitset );
    sigaddset( &waitset, SIGUSR1);

    pthread_sigmask( SIG_BLOCK, &waitset, NULL );
}

int initLogger()
{    
    struct sigaction sigact;
    setupSignal(sigact);

    if(LOG_TO_FILE == 1)
    {
        if(openLogFile())
            return INIT_LOGGER_FAILED; 
    }

    pthread_attr_t attr;
    pthread_t periodic_t;

    if(createThread(&periodic_t,&attr,&periodicConsumer))
        return INIT_LOGGER_FAILED;
   
    if(createThread(&aggressive_t,&attr,&aggressiveConsumer))
        return INIT_LOGGER_FAILED;

    return INIT_LOGGER_OK;
}

void DMLog(DMLogLevel logLevel, char *format, ...)
{
    va_list arglist;
    std::string tempstr;
    std::string logMsg;

    switch(logLevel)    
    {

	    case DM_LOG_ERROR:
                tempstr.append(DM_ERROR_MSG);
                break;

        case DM_LOG_WARNING:
                tempstr.append(DM_WARNING_MSG);
                break; 

        case DM_LOG_TRACE:
                tempstr.append(DM_TRACE_MSG);
                break;		

        case DM_LOG_INFO:
                tempstr.append(DM_INFO_MSG);
                break;

	    default:
		        break;
    }    

    tempstr.append(format);

    va_start( arglist, (char*)tempstr.c_str() );
    vsprintf( (char*) logMsg.c_str(),(char*)tempstr.c_str() , arglist);
    va_end( arglist );
  	
    pthread_mutex_lock(&mutex);
    logQueue.push((char*) logMsg.c_str());   
    pthread_mutex_unlock(&mutex);

    if(logLevel == DM_LOG_ERROR)
    {
        if(pthread_kill(aggressive_t,SIGUSR1))
            fprintf(stderr,"PTHREAD_KILL FAILED\n");
    }
}

