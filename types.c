#include "types.h"
#include <stdlib.h>

struct ftm_config *alloc_ftm_config() {
    struct ftm_config *config = malloc(sizeof(struct ftm_config));
    config->device_index = 0;
    config->peer_count = 0;
    return config;
}

void free_ftm_config(struct ftm_config *config) {
    free(config);
    config = NULL;
}

struct ftm_peer_attr *alloc_ftm_peer() {
    struct ftm_peer_attr *peer = malloc(sizeof(struct ftm_peer_attr));
    peer->preamble = NL80211_PREAMBLE_DMG;
    peer->num_bursts_exp = 15;
    peer->burst_period = 0;
    peer->burst_duration = 15;
    peer->ftms_per_burst = 0;
    peer->num_ftmr_retries = 3;
    peer->trigger_based = 0;
    return peer;
}

void free_ftm_peer(struct ftm_peer_attr *peer) {
    free(peer);
    peer = NULL;
}
