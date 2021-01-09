#include "initiator.h"
#include <time.h>

#define SOL 299492458
#define RTT_TO_DIST(rtt) ((float)(rtt) * SOL / 1000000000000)
#define DIST_TO_RTT(dist) (dist * 1000000000000 / SOL)
#define RELATIVE_DIFF(ori, new) (abs((float)(new - ori) / ori))

static void custom_result_handler(struct ftm_results_wrap *results,
                                  int attempts, int attempt_idx, void *arg) {
    struct ftm_results_stat **stats = arg;
    int line_count = 0;
    for (int i = 0; i < results->count; i++) {
        struct ftm_resp_attr *resp = results->results[i];
        if (!resp) {
            fprintf(stderr, "Response %d does not exist!", i);
            return;
        }
        if (resp->flags[FTM_RESP_FLAG_rtt_avg]) {
            stats[i]->rtt_values[attempt_idx] = resp->rtt_avg;
        } else {
            stats[i]->rtt_values[attempt_idx] = 0;
        }
        if (resp->flags[FTM_RESP_FLAG_rtt_avg] && resp->rtt_avg) {
            const int threshold = 20;
            float rate = stats[i]->rtt_measure_count > threshold ? 0.1 : 0.3;
            if (stats[i]->rtt_measure_count <= 10 ||
                RELATIVE_DIFF(((float)stats[i]->rtt_avg_stat / stats[i]->rtt_measure_count),
                              resp->rtt_avg) <= rate) {
                stats[i]->rtt_avg_stat += resp->rtt_avg;
                stats[i]->rtt_measure_count++;
            }
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
        __FTM_PRINT(num_ftmr_attempts, u);
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

        printf("\n----Processed data----\n");
        float dist = 0;
        float corrected_dist = 0;
        int64_t rtt_corrected_value = 0;
        if (resp->flags[FTM_RESP_FLAG_rtt_avg]) {
            dist = RTT_TO_DIST(resp->rtt_avg);
            if (resp->flags[FTM_RESP_FLAG_rtt_correct]) {
                corrected_dist = RTT_TO_DIST(resp->rtt_avg + resp->rtt_correct);
            }
            if (resp->flags[FTM_RESP_FLAG_dist_truth]) {
                rtt_corrected_value = DIST_TO_RTT(dist - resp->dist_truth);
            }
        }
        printf("%-19s%.3f\n", "dist", dist);
        line_count += 3;
        if (stats[i]->rtt_measure_count) {
            int64_t rtt =
                stats[i]->rtt_avg_stat / stats[i]->rtt_measure_count;
            printf("%-19s%ld\n", "rtt_avg", rtt);
            printf("%-19s%-7.3f\n", "dist_avg", RTT_TO_DIST(rtt));
            line_count += 2;
        }
        
        if (resp->flags[FTM_RESP_FLAG_rtt_correct]) {
            printf("%-19s%.3f\n", "corrected_dist", corrected_dist);
            line_count++;
        }
        if (resp->flags[FTM_RESP_FLAG_rtt_correct] &&
            stats[i]->rtt_measure_count) {
            int64_t corrected_rtt =
                stats[i]->rtt_avg_stat / stats[i]->rtt_measure_count +
                resp->rtt_correct;
            printf("%-19s%ld\n", "corrected_rtt_avg", corrected_rtt);
            printf("%-19s%-7.3f\n", "corrected_dist_avg",
                   RTT_TO_DIST(corrected_rtt));
            line_count += 2;
        }
        if (resp->flags[FTM_RESP_FLAG_dist_truth]) {
            printf("%-19s%ld\n", "rtt_corrected_val", rtt_corrected_value);
            line_count += 1;
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
    /* parse the arguments */
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
    print_config(config);
    /* initialize our data */
    struct ftm_results_stat **stats =
        malloc(config->peer_count * sizeof(struct ftm_results_stat *));
    for (int i = 0; i < config->peer_count; i++) {
        stats[i] = malloc(sizeof(struct ftm_results_stat));
        stats[i]->rtt_avg_stat = 0;
        stats[i]->rtt_measure_count = 0;
        stats[i]->rtt_values = malloc(attempts * sizeof(int64_t));
    }

    /* 
     * start FTM using the config we created, our custom handler,
     * the attempt number we designated, and the pointer to our data
     */
    int err = ftm(config, custom_result_handler, attempts, stats);

    uint8_t *addr = config->peers[0]->mac_addr;
    unsigned char addr_str[25];
    sprintf(addr_str, "%02x:%02x:%02x:%02x:%02x:%02x",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    time_t timer = time(NULL);

    struct tm* tm_info;
    char time_buffer[60];
    tm_info = localtime(&timer);
    strftime(time_buffer, 60, "%Y-%m-%d %H:%M:%S", tm_info);

    char logfile_name[120];
    strcat(logfile_name, time_buffer);
    strcat(logfile_name, "_");
    strcat(logfile_name, addr_str);
    FILE *output = fopen(logfile_name, "w");
    for (int i = 0; i < attempts; i++) {
        fprintf(output, "%ld\n", stats[0]->rtt_values[i]);
    }
    if (err)
        fprintf(stderr, "FTM measurement failed!\n");

    /* clean up */
    for (int i = 0; i < config->peer_count; i++) {
        free(stats[i]);
    }
    free(stats);
    free_ftm_config(config);
    return err;
}
