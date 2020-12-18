#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include <net/if.h>
#include <string.h>

struct ftm_config *alloc_ftm_config(const char *interface_name,
                                    struct ftm_peer_attr **peers,
                                    int peer_count) {
    struct ftm_config *config = malloc(sizeof(struct ftm_config));
    signed long long devidx = if_nametoindex(interface_name);
    if (devidx == 0) {
        fprintf(stderr, "Fail to find device interface %s!\n", interface_name);
        return NULL;
    }
    config->interface_index = devidx;
    config->peers = peers;
    config->peer_count = peer_count;
    return config;
}

void free_ftm_config(struct ftm_config *config) {
    for (int i = 0; i < config->peer_count; i++) {
        if (config->peers[i]) {
            free(config->peers[i]);
        }
    }
    free(config);
    config = NULL;
}

struct ftm_peer_attr *alloc_ftm_peer() {
    struct ftm_peer_attr *peer = malloc(sizeof(struct ftm_peer_attr));
    memset(peer->flags, 0, sizeof(peer->flags));
    return peer;
}

struct ftm_results_wrap *alloc_ftm_results_wrap(struct ftm_config *config) {
    struct ftm_results_wrap *results_wrap =
        malloc(sizeof(struct ftm_results_wrap));
    results_wrap->results =
        malloc(config->peer_count * sizeof(struct ftm_resp_attr *));
    for (int i = 0; i < config->peer_count; i++) {
        results_wrap->results[i] = alloc_ftm_resp_attr();
        /* set mac_addr to the result */
        if (config->peers[i]->flags[FTM_PEER_FLAG_mac_addr]) {
            memcpy(results_wrap->results[i]->mac_addr,
                   config->peers[i]->mac_addr, 6);
        } else {
            fprintf(stderr,
                    "No mac address info for target #%d in config!\n", i);
            return NULL;
        }
        /* set rtt_correct to the result, identified by mac_addr */
        if (config->peers[i]->flags[FTM_PEER_FLAG_rtt_correct]) {
            results_wrap->results[i]->flags[FTM_RESP_FLAG_rtt_correct] = 1;
            results_wrap->results[i]->rtt_correct =
                config->peers[i]->rtt_correct;
        }
    }
    results_wrap->count = config->peer_count;
    return results_wrap;
};

void free_ftm_results_wrap(struct ftm_results_wrap * result_wrap) {
    for (int i = 0; i < result_wrap->count; i++) {
        if (result_wrap->results[i])
            free(result_wrap->results[i]);
    }
    free(result_wrap->results);
    free(result_wrap);
    result_wrap = NULL;
}

struct ftm_resp_attr *alloc_ftm_resp_attr() {
    struct ftm_resp_attr *resp_attr = malloc(sizeof(struct ftm_resp_attr));
    memset(resp_attr->flags, 0, sizeof(resp_attr->flags));
    return resp_attr;
};