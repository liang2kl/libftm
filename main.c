#include "ftm.h"
#include <stdio.h>

#define ATTEMPS 1000

int64_t rtt_avg_stat = 0;
int rtt_measure_count = 0;

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
    if (attemp_idx == ATTEMPS - 1)
        return;
    for (int i = 0; i < (FTM_RESP_FLAG_MAX + 2 + 2) * results->count; i++) {
        printf("\033[A\33[2K");
    }
}

int main(int argc, char **argv) {
    struct ftm_peer_attr *attr = alloc_ftm_peer();
    // required
    uint8_t mac_addr[6] = {0x0a, 0x83, 0xa1, 0x15, 0xbf, 0x50};
    FTM_PEER_SET_ATTR_ADDR(attr, mac_addr);
    FTM_PEER_SET_ATTR(attr, asap, 1);
    FTM_PEER_SET_ATTR(attr, center_freq, 2412);
    FTM_PEER_SET_ATTR(attr, chan_width, NL80211_CHAN_WIDTH_20);
    FTM_PEER_SET_ATTR(attr, preamble, NL80211_PREAMBLE_HT);

    // optional
    FTM_PEER_SET_ATTR(attr, ftms_per_burst, 5);
    FTM_PEER_SET_ATTR(attr, num_ftmr_retries, 5);

    struct ftm_peer_attr *peers[] = {attr};

    // the only interface to communicate with the tool
    struct ftm_config *config = alloc_ftm_config("wlp3s0", peers, 1);

    if (!config) {
        fprintf(stderr, "Fail to allocate config!\n");
        return 1;
    }

    // start FTM
    int err = ftm(config, custom_result_handler, ATTEMPS);
    if (err)
        printf("FTM measurement failed!\n");

    // clean up
    free_ftm_config(config);

    return 0;
}
