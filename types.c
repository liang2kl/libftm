#include "types.h"
#include <stdlib.h>

struct ftm_config *alloc_ftm_config(const char *interface_name,
                                    struct ftm_peer_attr **peers,
                                    int peer_count) {
    struct ftm_config *config = malloc(sizeof(struct ftm_config));
    config->interface_name = interface_name;
    config->peers = peers;
    config->peer_count = peer_count;
    return config;
}

void free_ftm_config(struct ftm_config *config) {
    for (int i = 0; i < config->peer_count; i++) {
        if (!config->peers[i])
            free(config->peers[i]);
    }
    free(config);
    config = NULL;
}

struct ftm_peer_attr *alloc_ftm_peer() {
    struct ftm_peer_attr *peer = malloc(sizeof(struct ftm_peer_attr));
    memset(peer->flags, 0, sizeof(peer->flags));
    return peer;
}

struct ftm_results_wrap *alloc_ftm_results_wrap(struct ftm_resp_attr **results,
                                                int count) {
    struct ftm_results_wrap *results_wrap = malloc(sizeof(struct ftm_results_wrap));
    results_wrap->results = results;
    results_wrap->count = count;
    return results_wrap;
};

void free_ftm_results_wrap(struct ftm_results_wrap * result_wrap) {
    for (int i = 0; i < result_wrap->count; i++) {
        if (!result_wrap->results[i])
            free(result_wrap->results[i]);
    }
    free(result_wrap);
    result_wrap = NULL;
}

struct ftm_resp_attr *alloc_ftm_resp_attr() {
    struct ftm_resp_attr *resp_attr = malloc(sizeof(struct ftm_resp_attr));
    memset(resp_attr->flags, 0, sizeof(resp_attr->flags));
    return resp_attr;
};