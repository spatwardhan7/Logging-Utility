Logging Utility 

1) Structure
		The directory structure is as follows:
		/src
		/inc
		/tests

        inc folder contains include files of the logging utility
        config.h  - contains configurable parameters like LOG_TO_CONSOLE,
                   LOG_TO_FILE etc.
        logutility- contains functions and enums exposed to client applications
                    which would use this logging utility 
    -----------------------
		src folder consists of the library code.
        logutility.c - contains implementation code for functions defined in logutility.h
                       Also contains implementation code for helper functions required 
                       to carry out core functionality 
        Makefile     - use with make command to compile library 

    -----------------------
        tests folder consists of the client applications which use the logging utility

    -----------------------

2) Compiling and Executing: 
        To build logutility library: 
           1) go to /src
           2) make
        This will create logutility.a in same folder 
    -----------------------
        To run the test cases, go to the tests folder and inside a specific test folder
		
		For instance, inside test1:
		g++ test.c -o test -I../../inc ../../src/logutility.a -pthread -lrt			
		and then run ./test

