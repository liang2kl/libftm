#ifndef _FTM_INITIATOR_H
#define _FTM_INITIATOR_H
#include "initiator_config.h"
#include "initiator_start.h"

/**
 * DOC: Define your own data types and functions to start FTM and handle
 * the results
 * 
 * To make use of the API provided in /src/initiator/initiator_<suffix>.h, 
 * you are responsible for implementing a function to handle arguments from
 * the commandline, to create an ftm_config instance, and to start FTM. Also,
 * you might need to define some data types to handle the results. 
 * 
 * All the work mentioned above should be done in 'initiator.h', where you 
 * expose the API to the main function, and 'initiator.c', where you implement
 * your handlers. The original content in these two files shows an example of 
 * how to accomplish this work without diving deep into the implementation of 
 * the APIs.
 */

struct ftm_results_stat {
    int64_t rtt_avg_stat;
    int rtt_measure_count;
};

int my_start_ftm(int argc, char **argv);
#endif