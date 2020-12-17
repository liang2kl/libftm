#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "nl80211.h"

struct ftm_config {
    uint64_t interface_index;
    int peer_count;
    struct ftm_peer_attr **peers;
};

/**
 * enum ftm_peer_attr_flags - Record what attributes are set
 * 
 * @note
 * Internal use only. Modification is needed if you add additional attribute
 * into @struct ftm_peer_attr. Please keep the suffix the same as the variable
 * name in struct ftm_peer_attr.
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

    /* keep last */
    FTM_PEER_FLAG_MAX
};

/**
 * struct ftm_peer_attr - Attributes for a peer
 * 
 * @mac_addr: defined by @NL80211_PMSR_PEER_ATTR_ADDR
 * @chan_width: defined by @NL80211_ATTR_CHANNEL_WIDTH
 * @center_freq: defined by @NL80211_ATTR_WIPHY_FREQ
 * other attrs: defined in @enum nl80211_peer_measurement_ftm_req
 * @flags: whether an attribute is set. Identifiers are defined 
 * in @enum ftm_peer_attr_flags.
 * 
 * @note
 * Append additional attrs by adding members in
 * @struct ftm_peer_attr (attr_name) and flags in
 * @enum ftm_peer_attr_flags (FTM_PEER_FLAG_##attr_name),
 * then set via FTM_PEER_SET_ATTR(attr, attr_name, value).
 */
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

    /* internal use */
    uint8_t flags[FTM_PEER_FLAG_MAX];
};

/**
 * enum ftm_resp_attr_flags - Record what attributes exist
 * 
 * @note
 * Internal use only. Modification is needed if you add additional attribute
 * into @struct ftm_resp_attr. Please keep the suffix the same as the variable
 * name in struct ftm_resp_attr.
 */
enum ftm_resp_attr_flags {
    FTM_RESP_FLAG_mac_addr,
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

    /* keep last */
    FTM_RESP_FLAG_MAX
};

/**
 * struct ftm_resp_attr - Attributes in FTM result
 * 
 * @mac_addr: mac address of the target 
 * other variables are defined in @enum nl80211_peer_measurement_ftm_resp
 * 
 * @note
 * Append other attrs by adding members in @struct ftm_resp_attr (attr_name)
 * and flags in @enum ftm_resp_attr_flags (FTM_RESP_FLAG_##attr_name),
 * then access via FTM_GET as defined in ftm.h
 */
struct ftm_resp_attr {
    uint8_t mac_addr[6];
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

/**
 * FTM_PEER_SET_ATTR - Set attribute for given ftm_peer_attr instance
 * 
 * @param attr        ftm_peer_attr pointer
 * @param attr_name   variable name of designated attribute
 * @param value       the value to set
 * 
 * @note
 * Do not set mac_addr via this macro. Use FTM_PEER_SET_ATTR_ADDR instead.
 */
#define FTM_PEER_SET_ATTR(attr, attr_name, value)   \
    do {                                            \
        attr->attr_name = value;                    \
        attr->flags[FTM_PEER_FLAG_##attr_name] = 1; \
    } while (0)

/**
 * FTM_PEER_SET_ATTR_ADDR - Set mac address for given ftm_peer_attr instance
 * 
 * @param attr       ftm_peer_attr pointer
 * @param mac_addr   an array containing six u8 numbers.
 * 
 * @note
 * This is the only legal way to set a peer's mac address.
 */
#define FTM_PEER_SET_ATTR_ADDR(attr, addr)       \
    do {                                         \
        memcpy(attr->mac_addr, addr, 6);         \
        attr->flags[FTM_PEER_FLAG_mac_addr] = 1; \
    } while (0)

/**
 * alloc_ftm_config - Allocate a new ftm config with given configurations
 * 
 * @param interface_name   Interface name for wireless card
 * @param peers            Array of attributes of different peers
 * @param peer_count       Number of peers
 * 
 * @note
 * Each of peers must be allocated using alloc_ftm_peer().
 */
struct ftm_config *alloc_ftm_config(const char *interface_name,
                                    struct ftm_peer_attr **peers,
                                    int peer_count);

/**
 * free_ftm_config - Free the allocated ftm config
 * 
 * @param config   ftm config to be freed
 * 
 * @note
 * Call this function only when the config is no longer used.
 */
void free_ftm_config(struct ftm_config *config);

/**
 * alloc_ftm_peer - Allocate a new peer attribute
 * 
 * @note
 * The allocated instance will be automatically freed when calling 
 * free_ftm_results_wrap with associated ftm_results_wrap pointer. 
 * Don't free it on your own.
 */
struct ftm_peer_attr *alloc_ftm_peer();

/**
 * alloc_ftm_resp_attr: Allocate a new ftm_resp_attr instance.
 * 
 * @note
 * This is for internal use. You don't need to allocate a
 * response attribute on your own.
 */
struct ftm_resp_attr *alloc_ftm_resp_attr();

/**
 * alloc_ftm_results_wrap - Allocate an ftm_results_wrap container 
 * to store FTM results with given peer count.
 * 
 * @param count   Number of peers
 * 
 * @note
 * The count should be equal to the number of ftm_peer_attr defined
 * in ftm_config. This is for internal use. You don't need to allocate
 * on your own.
 */
struct ftm_results_wrap *alloc_ftm_results_wrap(int count);

/**
 * free_ftm_results_wrap - Free the allocated results wrap
 * 
 * @note
 * This function will free all the associated ftm_resp_attr pointers
 * and the pointer to the array itself.This is for internal use. 
 * You don't need to free on your own.
 */
void free_ftm_results_wrap(struct ftm_results_wrap *wrap);
#endif /*_TYPES_H*/