#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>
#include "nl80211.h"

struct ftm_config {
    int64_t device_index;
    int peer_count;
    struct ftm_peer_attr **peers;
};

struct ftm_peer_attr {
    uint8_t mac_addr[6];
    uint32_t chan_width;
    uint32_t center_freq;
    bool asap;
    uint32_t preamble;
    uint8_t num_bursts_exp;
    uint16_t burst_period;
    uint8_t burst_duration;
    uint8_t ftms_per_burst;
    uint8_t num_ftmr_retries;
    bool trigger_based;
};

struct ftm_resp_attr {
    uint32_t fail_reason;
    uint32_t burst_index;
    uint32_t num_ftmr_attemps;
    uint32_t num_ftmr_successes;
    uint32_t busy_retry_time;
    uint8_t num_bursts_exp;
    uint8_t burst_duration;
    uint8_t ftms_per_burst;
    int32_t rssi_avg;
    int32_t rssi_spread;
    int64_t rtt_avg;
    uint64_t rtt_variance;
    uint64_t rtt_spread;
    int64_t dist_avg;
    uint64_t dist_variance;
    uint64_t dist_spread;
};

struct ftm_results_wrap {
    struct ftm_resp_attr ** results;
    int count;
    int *state;
};

struct ftm_config *alloc_ftm_config();
void free_ftm_config(struct ftm_config *);
struct ftm_peer_attr *alloc_ftm_peer();
void free_ftm_peer(struct ftm_peer_attr *);
#endif /*_TYPES_H*/