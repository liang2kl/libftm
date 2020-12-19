#include "config.h"
#include "initiator.h"

struct ftm_results_stat {
    int64_t rtt_avg_stat;
    int rtt_measure_count;
};

int my_start_ftm(int argc, char **argv);