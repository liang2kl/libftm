#ifndef _FTM_CONFIG_H
#define _FTM_CONFIG_H

#include "initiator.h"
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
 * modifing parse_config_file() in config.c.
 */

/**
 * parse_config_file - Parse ftm config file
 * 
 * @param file_name   relative or absolute path of the config file
 * @param if_name     interface name
 */
struct ftm_config *parse_config_file(const char *file_name,
                                     const char *if_name);
#endif /*_FTM_CONFIG_H*/