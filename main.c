#include <stdio.h>

#include "ftm.h"
#include "config.h"


int64_t rtt_avg_stat = 0;
int rtt_measure_count = 0;
int attemps;

static void custom_result_handler(struct ftm_results_wrap *results,
                                  uint64_t attemp_idx) {
    for (int i = 0; i < results->count; i++) {
        struct ftm_resp_attr *resp = results->results[i];
        if (!resp) {
            fprintf(stderr, "Response %d does not exist!", i);
            return;
        }

        if (resp->flags[FTM_RESP_FLAG_rtt_avg]) {
            rtt_avg_stat += resp->rtt_avg;
            rtt_measure_count++;
        }

        printf("\nMEASUREMENT RESULT FOR TARGET #%d\n", i);
#define __FTM_PRINT(attr_name, specifier) \
    FTM_PRINT(resp, attr_name, specifier)

        FTM_PRINT_ADDR(resp);
        __FTM_PRINT(fail_reason, u);
        __FTM_PRINT(burst_index, u);
        __FTM_PRINT(num_ftmr_attemps, u);
        __FTM_PRINT(num_ftmr_successes, u);
        __FTM_PRINT(busy_retry_time, u);
        __FTM_PRINT(num_bursts_exp, u);
        __FTM_PRINT(burst_duration, u);
        __FTM_PRINT(ftms_per_burst, u);
        __FTM_PRINT(rssi_avg, d);
        __FTM_PRINT(rssi_spread, d);
        __FTM_PRINT(rtt_avg, ld);
        __FTM_PRINT(rtt_variance, lu);
        __FTM_PRINT(rtt_spread, lu);
        __FTM_PRINT(dist_avg, ld);
        __FTM_PRINT(dist_variance, lu);
        __FTM_PRINT(dist_spread, lu);

        printf("\n%-19s%ld\n", "rtt_avg_avg",
               rtt_avg_stat / rtt_measure_count);
    }
    if (attemp_idx == attemps - 1)
        return;
    for (int i = 0; i < (FTM_RESP_FLAG_MAX + 2 + 2) * results->count; i++) {
        printf("\033[A\33[2K");
    }
}


int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Parameter error!\n");
        return 1;
    }
    const char* if_name = argv[0];
    const char* file_name = argv[1];
    attemps = atoi(argv[2]);

    struct ftm_config *config = parse_config_file(file_name, if_name);
    if (!config) {
        fprintf(stderr, "Fail to parse config!\n");
        return 1;
    }

    // start FTM
    int err = ftm(config, custom_result_handler, attemps);
    if (err)
        printf("FTM measurement failed!\n");

    // clean up
    free_ftm_config(config);

    return 0;
}
