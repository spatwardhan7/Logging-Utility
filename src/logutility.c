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

#define NSEC_PER_SEC 1000000000
#define TIMER_INTERVAL_NS 1000000000  

const char * DM_ERROR_MSG =   "<DM_LOG_ERROR>   :";
const char * DM_WARNING_MSG = "<DM_LOG_WARNING> :";
const char * DM_TRACE_MSG =   "<DM_LOG_TRACE>   :";
const char * DM_INFO_MSG =    "<DM_LOG_INFO>    :";

static std::queue<char*> logQueue;
static pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE *fp;
static pthread_t consumer_t;

static int createTimer(timer_t *timer, int signo)
{
    struct sigevent sev;

    /* Create the timer */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_value.sival_ptr = timer;
    sev.sigev_signo = signo;

    if (timer_create(CLOCK_REALTIME, &sev, timer) == -1) 
    {
        printf("%s\n", strerror(errno));
        return 1;
    }

    return 0;
}
static int startTimerRep(timer_t *timer, unsigned long long time)
{
    int ret;
    struct itimerspec its;

    if (!timer) 
    {
        return 1;
    }

    its.it_value.tv_sec = time / NSEC_PER_SEC;
    its.it_value.tv_nsec = time % NSEC_PER_SEC;

    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    ret = timer_settime(*timer, 0, &its, NULL);

    return ret;
}
static void flushQueue()
{
	pthread_mutex_lock(&mutex);	
    while(!logQueue.empty())
    {
        char *str = logQueue.front(); 
	    logQueue.pop();
        if(LOG_TO_FILE == 1)
        {
            fprintf(fp,"%s\n",str);
        }
        if(LOG_TO_CONSOLE == 1)
            printf("%s\n",str);    
    }
    fflush(fp);	
    pthread_mutex_unlock(&mutex);	    
}
static void * consumer(void *arg)
{
    sigset_t waitset;
    siginfo_t info;

    int result;
    printf("Consumer Thread Started\n");

    sigaddset(&waitset, SIGUSR1);

    while(1)
    {
        result = sigwaitinfo( &waitset, &info );
        if( result == SIGUSR1 )
        {
            //fprintf(stderr,"sigwaitinfo() SUCCESS returned for signal %d\n",info.si_signo );
            flushQueue();
        }
        else
        {
            if(errno == EINTR) continue;
            fprintf( stderr,"sigwait() function failed error number %d\n", errno );
        }

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

int initLogger()
{    
    
    timer_t timerid;
    if(LOG_TO_FILE == 1)
    {
        if(openLogFile())
            return INIT_LOGGER_FAILED;
    }

    pthread_attr_t attr;
    sigset_t waitset;
    sigemptyset(&waitset);
    sigaddset( &waitset, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &waitset, NULL);

    if(createThread(&consumer_t,&attr,&consumer))
        return INIT_LOGGER_FAILED;


    if(createTimer(&timerid,SIGUSR1))
        return INIT_LOGGER_FAILED;

    if(startTimerRep(&timerid,TIMER_INTERVAL_NS))
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
        if(pthread_kill(consumer_t,SIGUSR1))
            fprintf(stderr,"PTHREAD_KILL FAILED\n");
    }
}

