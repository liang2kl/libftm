#include "ftm.h"
#include "utilities.c"

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
                         void *arg) {
    struct nlmsghdr *nlh = (struct nlmsghdr *)err - 1;
    int len = nlh->nlmsg_len;
    struct nlattr *attrs;
    struct nlattr *tb[NLMSGERR_ATTR_MAX + 1];
    int *ret = arg;
    int ack_len = sizeof(*nlh) + sizeof(int) + sizeof(*nlh);

    if (err->error > 0) {
        /*
		 * This is illegal, per netlink(7), but not impossible (think
		 * "vendor commands"). Callers really expect negative error
		 * codes, so make that happen.
		 */
        fprintf(stderr,
                "ERROR: received positive netlink error code %d\n",
                err->error);
        *ret = -EPROTO;
    } else {
        *ret = err->error;
    }

    if (!(nlh->nlmsg_flags & NLM_F_ACK_TLVS))
        return NL_STOP;

    if (!(nlh->nlmsg_flags & NLM_F_CAPPED))
        ack_len += err->msg.nlmsg_len - sizeof(*nlh);

    if (len <= ack_len)
        return NL_STOP;

    attrs = (void *)((unsigned char *)nlh + ack_len);
    len -= ack_len;

    nla_parse(tb, NLMSGERR_ATTR_MAX, attrs, len, NULL, NULL);
    if (tb[NLMSGERR_ATTR_MSG]) {
        len = strnlen((char *)nla_data(tb[NLMSGERR_ATTR_MSG]),
                      nla_len(tb[NLMSGERR_ATTR_MSG]));
        fprintf(stderr, "kernel reports: %*s\n", len,
                (char *)nla_data(tb[NLMSGERR_ATTR_MSG]));
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

static int (*registered_handler)(struct nl_msg *, void *);
// static void *registered_handler_data;

void register_handler(int (*handler)(struct nl_msg *, void *)) {
    registered_handler = handler;
    // registered_handler_data = data;
}

int valid_handler(struct nl_msg *msg, void *arg) {
    if (registered_handler)
        return registered_handler(msg, registered_handler_data);

    return NL_OK;
}

static int nl80211_init(struct nl80211_state *state) {  // from iw
    int err;

    // allocate socket
    state->nl_sock = nl_socket_alloc();
    if (!state->nl_sock) {
        fprintf(stderr, "Failed to allocate netlink socket.\n");
        return -ENOMEM;
    }

    // connect a Generic Netlink socket
    if (genl_connect(state->nl_sock)) {
        fprintf(stderr, "Failed to connect to generic netlink.\n");
        err = -ENOLINK;
        goto out_handle_destroy;
    }

    // set socket buffer size
    nl_socket_set_buffer_size(state->nl_sock, 8192, 8192);

    /* try to set NETLINK_EXT_ACK to 1, ignoring errors */
    err = 1;
    setsockopt(nl_socket_get_fd(state->nl_sock), SOL_NETLINK,
               NETLINK_EXT_ACK, &err, sizeof(err));

    // Resolves the Generic Netlink family name to the corresponding
    // numeric family identifier
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

static int handle_cmd(struct nl80211_state *state, int command,
                      int (*handler)(struct nl80211_state *,
                                     struct nl_msg *,
                                     int, char **),
                      int (*callback)(struct nl_msg *, void *),
                      int argc, char **argv) {
    int err;
    struct nl_cb *cb;
    struct nl_cb *s_cb;
    struct nl_msg *msg;

    msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "failed to allocate netlink message\n");
        return 2;
    }

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    s_cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb || !s_cb) {
        fprintf(stderr, "failed to allocate netlink callbacks\n");
        err = 2;
        goto out;
    }

    // set nl80211 and command (e.g NL80211_CMD_PEER_MEASUREMENT_START)
    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, state->nl80211_id, 0, 0, command, 0);

    if (handler) {
        int err = handler(state, msg, argc, argv);
        if (err)
            goto out;
    }

    // set callback for current socket
    nl_socket_set_cb(state->nl_sock, s_cb);

    err = nl_send_auto(state->nl_sock, msg);
    if (err < 0)
        goto out;

    if (callback)
        register_handler(callback);
    err = 1;

    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);

   /* Repeatedly calls nl_recv() or the respective replacement if provided
    * by the application (see nl_cb_overwrite_recv()) and parses the
    * received data as netlink messages. Stops reading if one of the
    * callbacks returns NL_STOP or nl_recv returns either 0 or a negative error code.
    */
    while (err > 0)
        nl_recvmsgs(state->nl_sock, cb);

out:
    nl_cb_put(cb);
    nl_cb_put(s_cb);
    nlmsg_free(msg);
    return err;
}

static int ftm_request_start(struct nl80211_state *state,
                       struct nl_msg *msg,
                       int argc,
                       char **argv) {
    int err;
    err = handle_cmd(state,
                     NL80211_CMD_PEER_MEASUREMENT_START,
                     ftm_request_send,
                     NULL,  // callback
                     argc,
                     argv);
    if (err)
        return err;
    
    prepare_result_cb();
    return 0;
}

static int prepare_result_cb() {
    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        fprintf(stderr, "failed to allocate netlink callbacks\n");
        return NULL;
    }

    int status = 1;
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, &status);
    // TODO
}

static int parse_ftm_result(struct nl_msg *msg, void *arg) {
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    nla_parse(tb, NL80211_ATTR_MAX,
              genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0),
              NULL, NULL);  // get all attributes at highest level
    if (!tb[NL80211_ATTR_COOKIE]) 
        return 1;  //no cookie
    if (!tb[NL80211_ATTR_PEER_MEASUREMENTS]) 
        return 2;  //no measurement data

    // Peer measurement attributes (enum nl80211_peer_measurement_attrs)
    struct nlattr *pmsr_attrs[NL80211_PMSR_ATTR_MAX + 1];
    int err = nla_parse_nested(pmsr_attrs, NL80211_PMSR_ATTR_MAX,
                               tb[NL80211_ATTR_PEER_MEASUREMENTS],
                               NULL, NULL);
    if (err)
        return 3;  
    
    struct nlattr *peer_attrs;//failed to parse measurement data
    int i;
    nla_for_each_nested(peer_attrs, pmsr_attrs[NL80211_PMSR_ATTR_PEERS], i) {
        // fetch attributes of each peer (enum nl80211_peer_measurement_peer_attrs)
        parse_pmsr_peer(peer_attrs);
    }
}

static void parse_pmsr_peer(struct nlattr *peer_attrs) {

    struct nlattr *tb[NL80211_PMSR_PEER_ATTR_MAX + 1];
    struct nlattr *resp_attrs[NL80211_PMSR_RESP_ATTR_MAX + 1];
    struct nlattr *data[NL80211_PMSR_TYPE_MAX + 1];
    char macbuf[6 * 3];
    int err;

    // TODO: save attributes
    err = nla_parse_nested(tb, NL80211_PMSR_PEER_ATTR_MAX, peer_attrs, NULL, NULL);
    if (err) {
        printf("  Peer: failed to parse!\n");
        return;
    }

    if (!tb[NL80211_PMSR_PEER_ATTR_ADDR]) {
        printf("  Peer: no MAC address\n");
        return;
    }

    mac_addr_n2a(macbuf, nla_data(tb[NL80211_PMSR_PEER_ATTR_ADDR]));
    printf("  Peer %s:", macbuf);

    if (!tb[NL80211_PMSR_PEER_ATTR_RESP]) {
        printf(" no response!\n");
        return;
    }

    err = nla_parse_nested(resp_attrs, NL80211_PMSR_RESP_ATTR_MAX,
                           tb[NL80211_PMSR_PEER_ATTR_RESP], NULL, NULL);
    if (err) {
        printf(" failed to parse response!\n");
        return;
    }

    if (resp_attrs[NL80211_PMSR_RESP_ATTR_STATUS])
        printf(" status=%d (%s)",
               nla_get_u32(resp_attrs[NL80211_PMSR_RESP_ATTR_STATUS]),
               pmsr_status(nla_get_u32(resp_attrs[NL80211_PMSR_RESP_ATTR_STATUS])));
    if (resp_attrs[NL80211_PMSR_RESP_ATTR_HOST_TIME])
        printf(" @%llu",
               (unsigned long long)nla_get_u64(resp_attrs[NL80211_PMSR_RESP_ATTR_HOST_TIME]));
    if (resp_attrs[NL80211_PMSR_RESP_ATTR_AP_TSF])
        printf(" tsf=%llu",
               (unsigned long long)nla_get_u64(resp_attrs[NL80211_PMSR_RESP_ATTR_AP_TSF]));
    if (resp_attrs[NL80211_PMSR_RESP_ATTR_FINAL])
        printf(" (final)");

    if (!resp_attrs[NL80211_PMSR_RESP_ATTR_DATA]) {
        printf(" - no data!\n");
        return;
    }

    printf("\n");

    nla_parse_nested(data, NL80211_PMSR_TYPE_MAX,
                     resp_attrs[NL80211_PMSR_RESP_ATTR_DATA], NULL, NULL);

    if (data[NL80211_PMSR_TYPE_FTM])
        parse_pmsr_ftm_data(data[NL80211_PMSR_TYPE_FTM]);
}

static void parse_pmsr_ftm_data(struct nlattr *data) {
    struct nlattr *ftm[NL80211_PMSR_FTM_RESP_ATTR_MAX + 1];

    printf("    FTM");
    nla_parse_nested(ftm, NL80211_PMSR_FTM_RESP_ATTR_MAX, data, NULL, NULL);

    if (ftm[NL80211_PMSR_FTM_RESP_ATTR_FAIL_REASON]) {
        printf(" failed: %s (%d)",
               ftm_fail_reason(nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_FAIL_REASON])),
               nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_FAIL_REASON]));
        if (ftm[NL80211_PMSR_FTM_RESP_ATTR_BUSY_RETRY_TIME])
            printf(" retry after %us",
                   nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_BUSY_RETRY_TIME]));
        printf("\n");
        return;
    }

    printf("\n");

#define PFTM(tp, attr, sign)                                     \
    do {                                                         \
        if (ftm[NL80211_PMSR_FTM_RESP_ATTR_##attr])              \
            printf("      " #attr ": %lld\n",                    \
                   (sign long long)nla_get_##tp(                 \
                       ftm[NL80211_PMSR_FTM_RESP_ATTR_##attr])); \
    } while (0)

    PFTM(u32, BURST_INDEX, unsigned);
    PFTM(u32, NUM_FTMR_ATTEMPTS, unsigned);
    PFTM(u32, NUM_FTMR_SUCCESSES, unsigned);
    PFTM(u8, NUM_BURSTS_EXP, unsigned);
    PFTM(u8, BURST_DURATION, unsigned);
    PFTM(u8, FTMS_PER_BURST, unsigned);
    PFTM(u32, RSSI_AVG, signed);
    PFTM(u32, RSSI_SPREAD, unsigned);
    PFTM(u64, RTT_AVG, signed);
    PFTM(u64, RTT_VARIANCE, unsigned);
    PFTM(u64, RTT_SPREAD, unsigned);
    PFTM(u64, DIST_AVG, signed);
    PFTM(u64, DIST_VARIANCE, unsigned);
    PFTM(u64, DIST_SPREAD, unsigned);

    if (ftm[NL80211_PMSR_FTM_RESP_ATTR_TX_RATE]) {
        char buf[100];

        parse_bitrate(ftm[NL80211_PMSR_FTM_RESP_ATTR_TX_RATE],
                      buf, sizeof(buf));
        printf("      TX bitrate: %s\n", buf);
    }

    if (ftm[NL80211_PMSR_FTM_RESP_ATTR_RX_RATE]) {
        char buf[100];

        parse_bitrate(ftm[NL80211_PMSR_FTM_RESP_ATTR_RX_RATE],
                      buf, sizeof(buf));
        printf("      RX bitrate: %s\n", buf);
    }

    if (ftm[NL80211_PMSR_FTM_RESP_ATTR_LCI])
        iw_hexdump("      LCI",
                   nla_data(ftm[NL80211_PMSR_FTM_RESP_ATTR_LCI]),
                   nla_len(ftm[NL80211_PMSR_FTM_RESP_ATTR_LCI]));

    if (ftm[NL80211_PMSR_FTM_RESP_ATTR_CIVICLOC])
        iw_hexdump("      civic location",
                   nla_data(ftm[NL80211_PMSR_FTM_RESP_ATTR_CIVICLOC]),
                   nla_len(ftm[NL80211_PMSR_FTM_RESP_ATTR_CIVICLOC]));
}

static int ftm_request_send(struct nl80211_state *state, struct nl_msg *msg,
                            int argc, char **argv) {  // from iw
    struct nlattr *pmsr, *peers;
    const char *file;
    int err;

    file = argv[0];
    argc--;
    argv++;
    while (argc) {
        if (strncmp(argv[0], "randomise", 9) == 0 ||
            strncmp(argv[0], "randomize", 9) == 0) {
            err = parse_random_mac_addr(msg, argv[0] + 9);
            if (err)
                return err;
        } else if (strncmp(argv[0], "timeout=", 8) == 0) {
            char *end;

            NLA_PUT_U32(msg, NL80211_ATTR_TIMEOUT,
                        strtoul(argv[0] + 8, &end, 0));
            if (*end)
                return 1;
        } else {
            return 1;
        }

        argc--;
        argv++;
    }

    pmsr = nla_nest_start(msg, NL80211_ATTR_PEER_MEASUREMENTS);
    // enum nl80211_peer_measurement_attrs (nested)
    if (!pmsr)
        goto nla_put_failure;
    peers = nla_nest_start(msg, NL80211_PMSR_ATTR_PEERS);
    // the maximum number of peers measurements can be done 
    // with in a single request (u32)

    if (!peers) 
        goto nla_put_failure;

    // uint32_t max_number = nla_get_u32(peers);   // get
    // setup pmsr and peer here
    err = parse_ftm_config(msg, file);
    if (err)
        return err;

    nla_nest_end(msg, peers);
    nla_nest_end(msg, pmsr);

    return 0;

nla_put_failure:
    return -ENOBUFS;
}

int main() {
    struct nl80211_state nlstate;
    int err = nl80211_init(&nlstate);
    if (err) {
        return 1;
    }

    // TODO: design API for measurement
    char(*ftm_req_cmd)[] = {"<filename>",
                            "randomise[=<addr>/<mask>]",
                            "timeout=<seconds>"};
    handle_cmd(&nlstate, 0, ftm_request_start,
               NULL, 3, ftm_req_cmd);  // start ftm
}