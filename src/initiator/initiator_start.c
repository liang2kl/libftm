#include "initiator_start.h"

static int set_ftm_peer(struct nl_msg *msg, struct ftm_peer_attr *attr, int index) {
    struct nlattr *peer = nla_nest_start(msg, index);
    if (!peer)
        goto nla_put_failure;
    if (!attr->mac_addr) {
        fprintf(stderr, "No mac address data!\n");
        return 1;
    }
    NLA_PUT(msg, NL80211_PMSR_PEER_ATTR_ADDR, 6, attr->mac_addr);
    struct nlattr *req, *req_data, *ftm;
    req = nla_nest_start(msg, NL80211_PMSR_PEER_ATTR_REQ);
    if (!req)
        goto nla_put_failure;
    req_data = nla_nest_start(msg, NL80211_PMSR_REQ_ATTR_DATA);
    if (!req_data)
        goto nla_put_failure;
    ftm = nla_nest_start(msg, NL80211_PMSR_TYPE_FTM);
    if (!ftm)
        goto nla_put_failure;

#define __FTM_PUT(prefix, attr_idx, attr_name, type) \
    FTM_PUT(msg, attr, prefix, attr_idx, attr_name, type)
#define __FTM_PEER_PUT(attr_idx, attr_name, type) \
    FTM_PEER_PUT(msg, attr, attr_idx, attr_name, type)
#define __FTM_PEER_PUT_FLAG(attr_idx, attr_name) \
    FTM_PEER_PUT_FLAG(msg, attr, attr_idx, attr_name)

    __FTM_PEER_PUT(PREAMBLE, preamble, U32);
    __FTM_PEER_PUT(NUM_BURSTS_EXP, num_bursts_exp, U8);
    __FTM_PEER_PUT(BURST_PERIOD, burst_period, U16);
    __FTM_PEER_PUT(BURST_DURATION, burst_duration, U8);
    __FTM_PEER_PUT(FTMS_PER_BURST, ftms_per_burst, U8);
    __FTM_PEER_PUT(NUM_FTMR_RETRIES, num_ftmr_retries, U8);

    __FTM_PEER_PUT_FLAG(ASAP, asap);
    __FTM_PEER_PUT_FLAG(TRIGGER_BASED, trigger_based);

    nla_nest_end(msg, ftm);
    nla_nest_end(msg, req_data);
    nla_nest_end(msg, req);

    struct nlattr *chan = nla_nest_start(msg, NL80211_PMSR_PEER_ATTR_CHAN);
    if (!chan)
        goto nla_put_failure;

    __FTM_PUT(NL80211_ATTR_, CHANNEL_WIDTH, chan_width, U32);
    __FTM_PUT(NL80211_ATTR_, WIPHY_FREQ, center_freq, U32);

    nla_nest_end(msg, chan);
    nla_nest_end(msg, peer);
    return 0;
nla_put_failure:
    fprintf(stderr, "put failed!\n");
    return -1;
}

static int set_ftm_config(struct nl_msg *msg, struct ftm_config *config) {
    struct nlattr *pmsr = nla_nest_start(msg, NL80211_ATTR_PEER_MEASUREMENTS);
    if (!pmsr)
        return 1;
    struct nlattr *peers = nla_nest_start(msg, NL80211_PMSR_ATTR_PEERS);
    if (!peers)
        return 1;
    int peer_count = config->peer_count;
    for (int i = 0; i < peer_count; i++) {
        if (set_ftm_peer(msg, config->peers[i], i))
            return 1;
    }
    nla_nest_end(msg, peers);
    nla_nest_end(msg, pmsr);
    return 0;
}

static int start_ftm(struct nl80211_state *state,
                     struct ftm_config *config) {
    int err;
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Fail to allocate message!");
        return 1;
    }

    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, state->nl80211_id, 0, 0,
                NL80211_CMD_PEER_MEASUREMENT_START, 0);
    
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, config->interface_index);

    err = set_ftm_config(msg, config);
    if (err)
        return 1;
    nl_handle_msg(state, NL_SEND_MSG, msg, NULL, NULL);
    return 0;
nla_put_failure:
    nlmsg_free(msg);
    return 1;
}

static int handle_ftm_result(struct nl_msg *msg, void *arg) {
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    /* fetch pointer from nl_cb_arg */
    struct nl_cb_arg *cb_arg = arg;
    struct ftm_results_wrap *results_wrap = cb_arg->arg;
    if (gnlh->cmd == NL80211_CMD_PEER_MEASUREMENT_COMPLETE) {
        *cb_arg->state = 0;
        return -1;
    }
    if (gnlh->cmd != NL80211_CMD_PEER_MEASUREMENT_RESULT)
        return -1;

    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    int err;

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);

    if (!tb[NL80211_ATTR_COOKIE]) {
		printf("Peer measurements: no cookie!\n");
		return -1;
	}

    if (!tb[NL80211_ATTR_PEER_MEASUREMENTS]) {
		printf("Peer measurements: no measurement data!\n");
		return -1;
	}

    struct nlattr *pmsr[NL80211_PMSR_ATTR_MAX + 1];
    err = nla_parse_nested(pmsr, NL80211_PMSR_ATTR_MAX,
                           tb[NL80211_ATTR_PEER_MEASUREMENTS],
                           NULL);
    if (err)
        return -1;

    if (!pmsr[NL80211_PMSR_ATTR_PEERS]) {
        printf("Peer measurements: no peer data!\n");
        return -1;
    }

    struct nlattr *peer, **resp;
    int i;
    int index = 0;
    nla_for_each_nested(peer, pmsr[NL80211_PMSR_ATTR_PEERS], i) {
        struct nlattr *peer_tb[NL80211_PMSR_PEER_ATTR_MAX + 1];
        struct nlattr *resp[NL80211_PMSR_RESP_ATTR_MAX + 1];
        struct nlattr *data[NL80211_PMSR_TYPE_MAX + 1];
        struct nlattr *ftm[NL80211_PMSR_FTM_RESP_ATTR_MAX + 1];

        err = nla_parse_nested(peer_tb, NL80211_PMSR_PEER_ATTR_MAX, peer, NULL);
        if (err) {
            printf("Peer: failed to parse!\n");
            return 1;
        }
        if (!peer_tb[NL80211_PMSR_PEER_ATTR_ADDR]) {
            printf("Peer: no MAC address\n");
            return 1;
        }


        if (!peer_tb[NL80211_PMSR_PEER_ATTR_RESP]) {
            printf("No response!\n");
            return 1;
        }

        err = nla_parse_nested(resp, NL80211_PMSR_RESP_ATTR_MAX,
                               peer_tb[NL80211_PMSR_PEER_ATTR_RESP], NULL);
        if (err) {
            printf("Failed to parse response!\n");
            return 1;
        }

        err = nla_parse_nested(data, NL80211_PMSR_TYPE_MAX,
                               resp[NL80211_PMSR_RESP_ATTR_DATA], 
                               NULL);
        if (err)
            return 1;

        err = nla_parse_nested(ftm, NL80211_PMSR_FTM_RESP_ATTR_MAX,
                               data[NL80211_PMSR_TYPE_FTM], NULL);
        if (err)
            return 1;
        
        struct ftm_resp_attr *resp_attr = results_wrap->results[index];

        /* 
         * Find the correct ftm_result if the mac_addr does not match.
         * This seems not quite possible (because the peers are set in 
         * given order), but we do this just in case.
         */
        if (nla_memcmp(peer_tb[NL80211_PMSR_PEER_ATTR_ADDR],
                       resp_attr->mac_addr, 6) != 0) {
            uint8_t addr[6];
            nla_memcpy(addr, peer_tb[NL80211_PMSR_PEER_ATTR_ADDR], 6);
            bool found = false;
            for (int i = 0; i < results_wrap->count; i++) {
                resp_attr = results_wrap->results[i];
                if (memcmp(addr, resp_attr->mac_addr, 6) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr,
                        "Result for target"
                        "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx does not exist!\n",
                        addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
                continue;
            }
        }

#define __FTM_GET(attr_idx, attr_name, type) \
    FTM_GET(ftm, resp_attr, attr_idx, attr_name, type)

        __FTM_GET(FAIL_REASON, fail_reason, u32);
        __FTM_GET(BURST_INDEX, burst_index, u32);
        __FTM_GET(NUM_FTMR_ATTEMPTS, num_ftmr_attemps, u32);
        __FTM_GET(NUM_FTMR_SUCCESSES, num_ftmr_successes, u32);
        __FTM_GET(BUSY_RETRY_TIME, busy_retry_time, u32);
        __FTM_GET(NUM_BURSTS_EXP, num_bursts_exp, u8);
        __FTM_GET(BURST_DURATION, burst_duration, u8);
        __FTM_GET(FTMS_PER_BURST, ftms_per_burst, u8);
        __FTM_GET(RSSI_AVG, rssi_avg, s32);
        __FTM_GET(RSSI_SPREAD, rssi_spread, s32);
        __FTM_GET(RTT_AVG, rtt_avg, s64);
        __FTM_GET(RTT_VARIANCE, rtt_variance, u64);
        __FTM_GET(RTT_SPREAD, rtt_spread, u32);
        __FTM_GET(DIST_AVG, dist_avg, s64);
        __FTM_GET(DIST_VARIANCE, dist_variance, u64);
        __FTM_GET(DIST_SPREAD, dist_spread, u64);

        index++;
    };
    return 0;
}

static int listen_ftm_result(struct nl80211_state *state,
                             struct ftm_results_wrap *results_wrap) {
    struct nl_cb_arg arg = alloc_nl_cb_arg(results_wrap);
    int err = nl_handle_msg(state, NL_RECV_NO_SEQ_CHECK, NULL, handle_ftm_result,
                            &arg);
    return err;
}

static void print_ftm_results(struct ftm_results_wrap *results, 
                              uint attempts, uint attemp_idx, void *arg) {
    for (int i = 0; i < results->count; i++) {
        struct ftm_resp_attr *resp = results->results[i];
        if (!resp) {
            fprintf(stderr, "Response %d does not exist!", i);
            return;
        }
        printf("\nMEASUREMENT RESULT FOR TARGET #%d\n", i);

#define __FTM_PRINT(attr_name, specifier) \
    FTM_PRINT(resp, attr_name, specifier)

        FTM_PRINT_ADDR(resp);
        __FTM_PRINT(fail_reason, u);
        __FTM_PRINT(burst_index, u);
        __FTM_PRINT(num_ftmr_attemps, u);
        __FTM_PRINT(num_ftmr_successes, u);
        __FTM_PRINT(busy_retry_time, u);
        __FTM_PRINT(num_bursts_exp, u);
        __FTM_PRINT(burst_duration, u);
        __FTM_PRINT(ftms_per_burst, u);
        __FTM_PRINT(rssi_avg, d);
        __FTM_PRINT(rssi_spread, d);
        __FTM_PRINT(rtt_avg, ld);
        __FTM_PRINT(rtt_variance, lu);
        __FTM_PRINT(rtt_spread, lu);
        __FTM_PRINT(dist_avg, ld);
        __FTM_PRINT(dist_variance, lu);
        __FTM_PRINT(dist_spread, lu);
    }
}

int ftm(struct ftm_config *config, ftm_result_handler handler,
        int attempts, void *arg) {
    struct nl80211_state nlstate;
    int err = nl80211_init(&nlstate);
    if (err) {
        fprintf(stderr, "Fail to allocate socket!\n");
        return 1;
    }

    for (int i = 0; i < attempts; i++) {
        struct ftm_results_wrap *results_wrap =
            alloc_ftm_results_wrap(config);

        err = start_ftm(&nlstate, config);
        if (err) {
            fprintf(stderr, "Fail to start ftm!\n");
            goto handle_free;
        }

        err = listen_ftm_result(&nlstate, results_wrap);
        if (err) {
            fprintf(stderr, "Fail to listen!\n");
            goto handle_free;
        }

        if (handler)
            handler(results_wrap, attempts, i, arg);
        else
            print_ftm_results(results_wrap, attempts, i, NULL);

        free_ftm_results_wrap(results_wrap);
        continue;
    handle_free:
        free_ftm_results_wrap(results_wrap);
        return 1;
    }
    return 0;
}