#include "initiator.h"

#define SOL 299492458
#define RTT_TO_DIST(rtt) ((float)rtt * SOL / 1000000000000)

static void custom_result_handler(struct ftm_results_wrap *results,
                                  uint attempts, uint attempt_idx, void *arg) {
    struct ftm_results_stat **stats = arg;
    uint line_count = 0;
    for (int i = 0; i < results->count; i++) {
        struct ftm_resp_attr *resp = results->results[i];
        if (!resp) {
            fprintf(stderr, "Response %d does not exist!", i);
            return;
        }

        if (resp->flags[FTM_RESP_FLAG_rtt_avg]) {
            stats[i]->rtt_avg_stat += resp->rtt_avg;
            stats[i]->rtt_measure_count++;
        }

        printf("\nMEASUREMENT RESULT FOR TARGET #%d\n", i);
        line_count += 2;
#define __FTM_PRINT(attr_name, specifier)      \
    do {                                       \
        FTM_PRINT(resp, attr_name, specifier); \
        line_count++;                          \
    } while (0)

        FTM_PRINT_ADDR(resp);
        line_count++;
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

        if (stats[i]->rtt_measure_count) {
            int64_t rtt =
                stats[i]->rtt_avg_stat / stats[i]->rtt_measure_count;
            printf("\n%-19s%ld\n", "rtt_avg_avg", rtt);
            printf("%-19s%-7.3f%s\n", "dist_avg_avg", RTT_TO_DIST(rtt), "m");
            line_count += 3;
        }

        if (resp->flags[FTM_RESP_FLAG_rtt_correct] &&
            stats[i]->rtt_measure_count) {
            int64_t corrected_rtt =
                stats[i]->rtt_avg_stat / stats[i]->rtt_measure_count +
                resp->rtt_correct;
            printf("\n%-19s%ld\n", "corrected_rtt", corrected_rtt);
            printf("%-19s%-7.3f%s\n", "corrected_dist",
                   RTT_TO_DIST(corrected_rtt), "m");
            line_count += 3;
        }
    }
    if (attempt_idx == attempts - 1)
        return;

    /* flush output */
    for (int i = 0; i < line_count; i++) {
        printf("\033[A\033[2K");
    }
}

int my_start_ftm(int argc, char **argv) {
    if (argc != 4 && argc != 3) {
        printf("Invalid arguments!\n");
        printf("Valid args: <if_name> <file_path> [<attemps>]\n");
        return 1;
    }
    const char *if_name = argv[1];
    const char *file_name = argv[2];
    int attempts = 1;
    if (argc == 4)
        attempts = atoi(argv[3]);

    /* generate config from config file */
    struct ftm_config *config = parse_config_file(file_name, if_name);
    if (!config) {
        fprintf(stderr, "Fail to parse config!\n");
        return 1;
    }

    /* initialize our data */
    struct ftm_results_stat **stats =
        malloc(config->peer_count * sizeof(struct ftm_results_stat *));
    for (int i = 0; i < config->peer_count; i++) {
        stats[i] = malloc(sizeof(struct ftm_results_stat));
        stats[i]->rtt_avg_stat = 0;
        stats[i]->rtt_measure_count = 0;
    }

    /* 
     * start FTM using the config we created, our custom handler,
     * the attempt number we designated, and the pointer to our data
     */
    int err = ftm(config, custom_result_handler, attempts, stats);
    if (err)
        printf("FTM measurement failed!\n");

    /* clean up */
    for (int i = 0; i < config->peer_count; i++) {
        free(stats[i]);
    }
    free(stats);
    free_ftm_config(config);
    return 0;
}