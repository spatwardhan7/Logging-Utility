#ifndef LOGUTILITY_H_INCLUDED
#define LOGUTILITY_H_INCLUDED

#include <pthread.h>
#include <unistd.h>

#define INIT_LOGGER_OK       0
#define INIT_LOGGER_FAILED   1

#define N_SEC_TO_M_SEC       1000000

typedef enum
{
    DM_LOG_ERROR,
    DM_LOG_WARNING,
    DM_LOG_TRACE,
    DM_LOG_INFO
}DMLogLevel;


int initLogger();
void DMLog(DMLogLevel logLevel, char *format, ...);

#endif /* LOGUTILITY_H_INCLUDED*/
