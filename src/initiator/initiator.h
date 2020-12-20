#ifndef _INITIATOR_H
#define _INITIATOR_H
#include "initiator_config.h"
#include "initiator_start.h"

struct ftm_results_stat {
    int64_t rtt_avg_stat;
    int rtt_measure_count;
};

int my_start_ftm(int argc, char **argv);
#endif