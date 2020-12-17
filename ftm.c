#include "ftm.h"

struct nl80211_state {
    struct nl_sock *nl_sock;
    int nl80211_id;
};

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
                         void *arg) {
    struct nlmsghdr *nlh = (struct nlmsghdr *)err - 1;
    int len = nlh->nlmsg_len;
    struct nlattr *attrs;
    struct nlattr *tb[3 + 1];
    int *ret = arg;
    int ack_len = sizeof(*nlh) + sizeof(int) + sizeof(*nlh);

    if (err->error > 0) {
        fprintf(stderr,
                "ERROR: received positive netlink error code %d\n",
                err->error);
        *ret = -EPROTO;
    } else {
        *ret = err->error;
    }

    if (!(nlh->nlmsg_flags & 0x200))
        return NL_STOP;

    if (!(nlh->nlmsg_flags & 0x100))
        ack_len += err->msg.nlmsg_len - sizeof(*nlh);

    if (len <= ack_len)
        return NL_STOP;

    attrs = (void *)((unsigned char *)nlh + ack_len);
    len -= ack_len;

    nla_parse(tb, 3, attrs, len, NULL);
    if (tb[1]) {
        len = strnlen((char *)nla_data(tb[1]),
                      nla_len(tb[1]));
        fprintf(stderr, "kernel reports: %*s\n", len,
                (char *)nla_data(tb[1]));
    }

    return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg) {
    int *ret = arg;
    *ret = 0;
    return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg) {
    int *ret = arg;
    *ret = 0;
    return NL_STOP;
}

static int no_seq_check(struct nl_msg *msg, void *arg) {
    return NL_OK;
}

static int nl80211_init(struct nl80211_state *state) {  // from iw
    int err;

    state->nl_sock = nl_socket_alloc();
    if (!state->nl_sock) {
        fprintf(stderr, "Failed to allocate netlink socket.\n");
        return -ENOMEM;
    }

    if (genl_connect(state->nl_sock)) {
        fprintf(stderr, "Failed to connect to generic netlink.\n");
        err = -ENOLINK;
        goto out_handle_destroy;
    }

    err = nl_socket_set_buffer_size(state->nl_sock, 32 * 1024, 32 * 1024);

    if (err)
        return 1;
    err = 1;
    setsockopt(nl_socket_get_fd(state->nl_sock), 270,
               1, &err, sizeof(err));

    state->nl80211_id = genl_ctrl_resolve(state->nl_sock, "nl80211");
    if (state->nl80211_id < 0) {
        fprintf(stderr, "nl80211 not found.\n");
        err = -ENOENT;
        goto out_handle_destroy;
    }

    return 0;

out_handle_destroy:
    nl_socket_free(state->nl_sock);
    return err;
}


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

    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
    struct nl_cb *s_cb = nl_cb_alloc(NL_CB_DEFAULT);

    if (!cb || !s_cb) {
        fprintf(stderr, "Fail to allocate callback\n");
        return 1;
    }

    nl_socket_set_cb(state->nl_sock, s_cb);

    err = nl_send_auto(state->nl_sock, msg);
    if (err < 0)
        return 1;
    
    err = 1;
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

    while (err > 0)
        nl_recvmsgs(state->nl_sock, cb);
    if (err == -1)
        fprintf(stderr, "Permission denied!\n");
    if (err < 0) {
        fprintf(stderr, "Received error code %d\n", err);
        return 1;
    }
    return 0;
nla_put_failure:
    return 1;
}

static int handle_ftm_result(struct nl_msg *msg, void *arg) {
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct ftm_results_wrap *results_wrap = arg;
    if (gnlh->cmd == NL80211_CMD_PEER_MEASUREMENT_COMPLETE) {
        *results_wrap->state = 0;
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

#define __FTM_GET(attr_idx, attr_name, type) \
    FTM_GET(ftm, resp_attr, attr_idx, attr_name, type)

        FTM_GET_ADDR(peer_tb[NL80211_PMSR_PEER_ATTR_ADDR], resp_attr);
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

static int listen_ftm_result(struct nl80211_state *state, struct ftm_results_wrap *results_wrap) {
    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT); // use NL_CB_DEBUG when debugging
    if (!cb)
        return 1;

    nl_socket_set_cb(state->nl_sock, cb);

    int status = 1;

    results_wrap->state = &status;
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, handle_ftm_result, results_wrap);

    while (status)
        nl_recvmsgs(state->nl_sock, cb);
    return 0;
nla_put_failure:
    return 1;
}

static void print_ftm_results(struct ftm_results_wrap *results, 
                              uint64_t attemp_idx) {
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

int ftm(struct ftm_config *config,
        void (*handler)(struct ftm_results_wrap *wrap, uint64_t),
        int attemps) {
    struct nl80211_state nlstate;
    int err = nl80211_init(&nlstate);
    if (err) {
        fprintf(stderr, "Fail to allocate socket!\n");
        return 1;
    }

    for (int i = 0; i < attemps; i++) {
        struct ftm_results_wrap *results_wrap =
            alloc_ftm_results_wrap(config->peer_count);

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
            handler(results_wrap, i);
        else
            print_ftm_results(results_wrap, i);
        
        free_ftm_results_wrap(results_wrap);
        continue;
    handle_free:
        free_ftm_results_wrap(results_wrap);
        return 1;
    }
    return 0;
}