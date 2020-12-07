#ifndef ftm_types_h
#define ftm_types_h
#include <stdint.h>

struct ftm_target_attr {
    uint32_t freq;
    uint8_t bw;  // enum nl80211_ftm_bw
    uint32_t cntr_freq1;
    uint32_t cntr_freq2;
    uint8_t bssid[6];
    uint8_t num_of_burst_exp;
    uint16_t burst_period;
    uint8_t samples_per_burst;
    uint8_t retries;
    uint8_t burst_duration;
    uint64_t cookie;
    uint8_t preamble;  //enum nl80211_ftm_preamble
    uint8_t ftm_bw;
};

struct ftm_req_attr {
    uint8_t timeout;
    uint8_t mac_addr_template[6];
    uint8_t mac_addr_mask[6];
};

struct ftm_resp_attr {
    uint32_t burst_index;
    uint32_t frame_attemp_cnt;
    uint32_t frame_success_cnt;
    
};

struct pmsr_peer_attr {
    // missing NL80211_PMSR_PEER_ATTR_CHAN
    uint8_t mac_attr[6];
};

#endif