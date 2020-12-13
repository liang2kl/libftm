#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>
#include "nl80211.h"

struct ftm_config {
    uint64_t interface_index;
    int peer_count;
    struct ftm_peer_attr **peers;
};

/* 
 * record what attributes are set
 * internal use only
 * 
 * Append other attrs by adding members in
 * @struct ftm_peer_attr (attr_name) and flags in
 * @enum ftm_peer_attr_flags (FTM_PEER_FLAG_##attr_name).
 * 
 * Then set via FTM_PEER_SET_ATTR(attr, attr_name, value).
 */
enum ftm_peer_attr_flags {
    FTM_PEER_FLAG_mac_addr,
    FTM_PEER_FLAG_chan_width,
    FTM_PEER_FLAG_center_freq,
    FTM_PEER_FLAG_asap,
    FTM_PEER_FLAG_preamble,
    FTM_PEER_FLAG_num_bursts_exp,
    FTM_PEER_FLAG_burst_period,
    FTM_PEER_FLAG_burst_duration,
    FTM_PEER_FLAG_ftms_per_burst,
    FTM_PEER_FLAG_num_ftmr_retries,
    FTM_PEER_FLAG_trigger_based,

    FTM_PEER_FLAG_MAX
};

#define FTM_PEER_SET_ATTR(attr, attr_name, value)   \
    do {                                            \
        attr->attr_name = value;                    \
        attr->flags[FTM_PEER_FLAG_##attr_name] = 1; \
    } while (0)

struct ftm_peer_attr {
    uint8_t *mac_addr;
    uint32_t chan_width;
    uint32_t center_freq;
    int asap;
    uint32_t preamble;
    uint8_t num_bursts_exp;
    uint16_t burst_period;
    uint8_t burst_duration;
    uint8_t ftms_per_burst;
    uint8_t num_ftmr_retries;
    int trigger_based;

    /* internal use */
    uint8_t flags[FTM_PEER_FLAG_MAX];
};

/* 
 * record what attributes exist
 * internal use only
 * 
 * Append other attrs by adding members in
 * @struct ftm_resp_attr (attr_name) and flags in
 * @enum ftm_resp_attr_flags (FTM_RESP_FLAG_##attr_name).
 * 
 * Then access via FTM_GET(attr_idx, attr_name, type)
 * as defined in ftm.c
 */
enum ftm_resp_attr_flags {
    FTM_RESP_FLAG_fail_reason,
    FTM_RESP_FLAG_burst_index,
    FTM_RESP_FLAG_num_ftmr_attemps,
    FTM_RESP_FLAG_num_ftmr_successes,
    FTM_RESP_FLAG_busy_retry_time,
    FTM_RESP_FLAG_num_bursts_exp,
    FTM_RESP_FLAG_burst_duration,
    FTM_RESP_FLAG_ftms_per_burst,
    FTM_RESP_FLAG_rssi_avg,
    FTM_RESP_FLAG_rssi_spread,
    FTM_RESP_FLAG_rtt_avg,
    FTM_RESP_FLAG_rtt_variance,
    FTM_RESP_FLAG_rtt_spread,
    FTM_RESP_FLAG_dist_avg,
    FTM_RESP_FLAG_dist_variance,
    FTM_RESP_FLAG_dist_spread,

    FTM_RESP_FLAG_MAX
};

#define FTM_RESP_SET_FLAG(attr, attr_name) \
    attr->flags[FTM_RESP_FLAG_##attr_name] = 1

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

    /* internal use */
    uint8_t flags[FTM_RESP_FLAG_MAX];
};

struct ftm_results_wrap {
    struct ftm_resp_attr ** results;
    int count;
    int *state;
};

struct ftm_config *alloc_ftm_config(const char *interface_name,
                                    struct ftm_peer_attr **peers,
                                    int peer_count);

void free_ftm_config(struct ftm_config *config);
struct ftm_peer_attr *alloc_ftm_peer();
struct ftm_results_wrap *alloc_ftm_results_wrap(int count);
void free_ftm_results_wrap(struct ftm_results_wrap *wrap);
struct ftm_resp_attr *alloc_ftm_resp_attr();
#endif /*_TYPES_H*/