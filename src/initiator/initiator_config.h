#ifndef _FTM_INITIATOR_CONFIG_H
#define _FTM_INITIATOR_CONFIG_H

#include "initiator_types.h"
#include <stdio.h>
#include "../nl/nl.h"
/**
 * DOC: Format of ftm config file
 * 
 * Each line in the file represents a target, with the following format:
 * <addr> 
 * bw=<[20|40|80|80+80|160]> 
 * cf=<center_freq> 
 * [cf1=<center_freq1>] 
 * [cf2=<center_freq2>] 
 * [ftms_per_burst=<samples per burst>] 
 * [asap] 
 * [bursts_exp=<num of bursts exponent>] 
 * [burst_period=<burst period>] 
 * [retries=<num of retries>] 
 * [burst_duration=<burst duration>] 
 * [tb]
 * [rtt_correct=<rtt to be compensated>]
 * 
 * @note
 * Each peer must take only one line, although the doc above seperates the 
 * attributes into different lines. Append more configuration options by 
 * modifing parse_peer_config() in config.c.
 */

/**
 * parse_config_file - Parse ftm config file
 * 
 * @param file_name   relative or absolute path of the config file
 * @param if_name     interface name
 * 
 * @return a valid ftm_config pointer on success, NULL on failure.
 */
struct ftm_config *parse_config_file(const char *file_name,
                                     const char *if_name);

#define CONFIG_PRINT(peer, name, spec)         \
    do {                                         \
        printf("%-19s", #name);                  \
        if (peer->flags[FTM_PEER_FLAG_##name]) { \
            printf("%" #spec "\n", peer->name);  \
        } else {                                 \
            printf("default\n");                 \
        }                                        \
    } while (0)

void print_config(struct ftm_config *config);
#endif /*_FTM_CONFIG_H*/