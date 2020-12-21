#ifndef _FTM_INITIATOR_START_H
#define _FTM_INITIATOR_START_H

#include "../nl/nl.h"
#include "initiator_types.h"

/**
 * DOC: Start a fine timing measurement
 * 
 * The following section describes the API (and the only API needed) 
 * to start a fine timing measurement (FTM) and handle the results. 
 * Additional information of corresponding data types and methods to 
 * configure the request is provided in initiator_types.h.
 */

/**
 * typedef ftm_result_handler - Function type to handle FTM result
 * 
 * @wrap: FTM results, @see ftm_results_wrap
 * @attempt: Total attempts
 * @attempt_idx: The index of the current attempt, starting from 0
 * @arg: Pointer passed in when allocating ftm_config
 * 
 * @note
 * The results will be freed after calling this callback. Do not keep
 * a pointer to it.
 */
typedef void (*ftm_result_handler)(struct ftm_results_wrap *results,
                                   uint attempts, uint attempt_idx, void *arg);

/**
 * ftm - Start FTM with given config, receive response with given
 * callback, for given times
 * 
 * @param config    The config used to start FTM
 * @param handler   The callback to handle measurement results, can be NULL
 * @param attempts  How many times to measure distance
 * @param arg       Any pointer you want to pass to the handler
 * 
 * @return 0 on success, 1 on failure
 */
int ftm(struct ftm_config *config, ftm_result_handler handler,
        int attempts, void *arg);

/**
 * DOC: Lower-level APIs
 * 
 * Typically, the only API needed to start an FTM is the method ftm(). 
 * However, if you need to deal with netlink messages yourself, or have some 
 * specific requirements, you can use some convenient, lower-level APIs 
 * provided in the following section.
 */

/**
 * FTM_PUT - Set attribute from ftm_peer_attr
 * 
 * @param msg         the configuring netlink message
 * @param attr        ftm_peer_attr pointer
 * @param prefix      prefix of identifiers defined in nl80211.h, 
 *                    like NL80211_ATTR_
 * @param attr_idx    suffix of identifiers defined in nl80211.h, 
 *                    like CHANNEL_WIDTH
 * @param attr_name   variable name of ftm_peer_attr
 * @param type        data type of the attribute, capitalized, like U8
 * 
 * @note
 * Be sure that msg is in the correct hierarchy of nested attributes.
 */
#define FTM_PUT(msg, attr, prefix, attr_idx, attr_name, type)   \
    if (attr->flags[FTM_PEER_FLAG_##attr_name]) {               \
        NLA_PUT_##type(msg, prefix##attr_idx, attr->attr_name); \
    }

/**
 * FTM_PEER_PUT - Set attribute for an ftm peer
 * 
 * @see FTM_PUT
 * 
 * @note
 * This is a helper macro specified for setting attributes defined in
 * @enum l80211_peer_measurement_ftm_req.
 */
#define FTM_PEER_PUT(msg, attr, attr_idx, attr_name, type) \
    FTM_PUT(msg, attr, NL80211_PMSR_FTM_REQ_ATTR_, attr_idx, attr_name, type)

/**
 * FTM_PUT_FLAG - Set flag attributes from ftm_peer_attr
 * 
 * @param msg         the configuring netlink message
 * @param attr        ftm_peer_attr pointer
 * @param prefix      prefix of identifiers defined in nl80211.h
 * @param attr_idx    suffix of identifiers defined in nl80211.h
 * @param attr_name   variable name of ftm_peer_attr
 * 
 * @note
 * Be sure that msg is in the correct hierarchy of nested attributes. 
 * Flag attributes are defined in nl80211.h.
 */
#define FTM_PUT_FLAG(msg, attr, prefix, attr_idx, attr_name)         \
    if (attr->flags[FTM_PEER_FLAG_##attr_name] && attr->attr_name) { \
        NLA_PUT_FLAG(msg, prefix##attr_idx);                         \
    }

/**
 * FTM_PEER_PUT_FLAG - Set flag attributes for a peer
 * 
 * @see FTM_PUT_FLAG
 * This is a helper macro specified for setting attributes defined in
 * @enum l80211_peer_measurement_ftm_req.
 */
#define FTM_PEER_PUT_FLAG(msg, attr, attr_idx, attr_name) \
    FTM_PUT_FLAG(msg, attr, NL80211_PMSR_FTM_REQ_ATTR_, attr_idx, attr_name)

/**
 * FTM_RESP_SET_FLAG - Set flags for specific attribute
 * 
 * @note
 * Internal use only.
 */
#define FTM_RESP_SET_FLAG(attr, attr_name) \
    attr->flags[FTM_RESP_FLAG_##attr_name] = 1

/**
 * FTM_GET - Get attributes from given nlattr pointer and store in 
 * given resp_attr instance
 * 
 * @param ftm_attr    netlink attribute for NL80211_PMSR_TYPE_FTM
 * @param resp_attr   resp_attr pointer
 * @param attr_idx    suffix of identifiers
 * @param attr_name   variable name of ftm_resp_attr
 * @param type        data type of the attribute, lowercased, like u8
 * 
 * @note
 * Do not use this macro to get mac_addr. Use FTM_GET_ADDR instead.
 */
#define FTM_GET(ftm_attr, resp_attr, attr_idx, attr_name, type)         \
    if (ftm_attr[NL80211_PMSR_FTM_RESP_ATTR_##attr_idx]) {              \
        resp_attr->attr_name =                                          \
            nla_get_##type(ftm[NL80211_PMSR_FTM_RESP_ATTR_##attr_idx]); \
        FTM_RESP_SET_FLAG(resp_attr, attr_name);                        \
    }

/**
 * FTM_GET_ADDR - Copy mac address of the peer into given ftm_resp_attr 
 * instance
 * 
 * @param nl_attr     netlink attribute
 * @param resp_attr   ftm_resp_attr pointer
 */
#define FTM_GET_ADDR(nl_attr, resp_attr)             \
    if (nl_attr) {                                   \
        nla_memcpy(resp_attr->mac_addr, nl_attr, 6); \
        FTM_RESP_SET_FLAG(resp_attr, mac_addr);      \
    }

/**
 * FTM_PRINT - Print the given attribute from a response
 * 
 * @param resp        ftm_resp_attr pointer
 * @param attr_name   variable name of ftm_resp_attr
 * @param specifier   specifier used in printf() for the type, like 'd'
 * 
 * @note
 * Do not use this macro to print mac_addr. Use FTM_PRINT_ADDR instead.
 */
#define FTM_PRINT(resp, attr_name, specifier)             \
    do {                                                  \
        printf("%-19s", #attr_name);                      \
        if (resp->flags[FTM_RESP_FLAG_##attr_name]) {     \
            printf("%" #specifier "\n", resp->attr_name); \
        } else {                                          \
            printf("non-exist\n");                        \
        }                                                 \
    } while (0)

/**
 * FTM_GET_ADDR - Print the mac address of the peer
 * 
 * @param resp   ftm_resp_attr pointer
 */
#define FTM_PRINT_ADDR(resp)                                       \
    if (resp->flags[FTM_RESP_FLAG_mac_addr]) {                     \
        uint8_t *addr = resp->mac_addr;                            \
        printf("%-19s%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", \
               "mac_addr", addr[0], addr[1], addr[2], addr[3],     \
               addr[4], addr[5]);                                  \
    }

#endif /* _FTM_INITIATOR_START_H */