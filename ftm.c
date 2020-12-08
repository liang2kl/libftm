#include <errno.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>
#include <net/if.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nl80211.h"
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

		 /* This is illegal, per netlink(7), but not impossible (think
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
    err = nl_socket_set_buffer_size(state->nl_sock, 8192, 8192);

    if (err)
        return 1;
    /* try to set NETLINK_EXT_ACK to 1, ignoring errors */
    err = 1;
    setsockopt(nl_socket_get_fd(state->nl_sock), 270,
               11, &err, sizeof(err));

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

static int set_ftm_peer(struct nl_msg *msg, int index) {
    struct nlattr *peer = nla_nest_start(msg, index);
    uint8_t mac_addr[6] = {0x0a, 0x83, 0xa1, 0x15, 0xbf, 0x50}; // placeholder
    NLA_PUT(msg, NL80211_PMSR_PEER_ATTR_ADDR, 6, mac_addr);
    // set attributes
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

    NLA_PUT_U8(msg, NL80211_PMSR_FTM_REQ_ATTR_NUM_FTMR_RETRIES, 5);
    NLA_PUT_FLAG(msg, NL80211_PMSR_FTM_REQ_ATTR_ASAP);
    nla_nest_end(msg, ftm);
    nla_nest_end(msg, req_data);
    nla_nest_end(msg, req);

    struct nlattr *chan;
    chan = nla_nest_start(msg, NL80211_PMSR_PEER_ATTR_CHAN);
    if (!chan)
        goto nla_put_failure;

    NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, 20);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, 2412);

    nla_nest_end(msg, chan);
    nla_nest_end(msg, peer);
    return 0;
nla_put_failure:
    return -1;
}

static int set_ftm_config(struct nl_msg *msg) {
    struct nlattr *pmsr = nla_nest_start(msg, NL80211_ATTR_PEER_MEASUREMENTS);
    if (!pmsr)
        return 1;
    struct nlattr *peers = nla_nest_start(msg, NL80211_PMSR_ATTR_PEERS);
    if (!peers)
        return 1;
    set_ftm_peer(msg, 1);
    nla_nest_end(msg, peers);
    nla_nest_end(msg, pmsr);
    return 0;
}

static int start_ftm(struct nl80211_state *state) {
    int err;
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg)
        return 1;

    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, state->nl80211_id, 0, 0,
                NL80211_CMD_PEER_MEASUREMENT_START, 0);
    
    signed long long devidx = if_nametoindex("wlp3s0");
    
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);
    err = set_ftm_config(msg);
    if (err)
        return 1;

    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEBUG);
    struct nl_cb *s_cb = nl_cb_alloc(NL_CB_DEBUG);

    nl_socket_set_cb(state->nl_sock, s_cb);

    err = nl_send_auto(state->nl_sock, msg);


    err = 1;
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    while (err > 0)
        nl_recvmsgs(state->nl_sock, cb);
    if (err < 0) return 1;
    return 0;
nla_put_failure:
    printf("fail to set up message!");
    return 1;
}

static int handle_ftm_result(struct nl_msg *msg, void *arg) {
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    printf("%d\n",gnlh->cmd == NL80211_CMD_PEER_MEASUREMENT_RESULT);
    if (gnlh->cmd != NL80211_CMD_PEER_MEASUREMENT_RESULT)
        return -1;

    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    int err;
    err = nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
                    genlmsg_attrlen(gnlh, 0), NULL);
    if (err)
        goto nla_put_failure;
    struct nlattr *pmsr[NL80211_PMSR_ATTR_MAX + 1];
    err = nla_parse_nested(pmsr, NL80211_PMSR_ATTR_MAX,
                           tb[NL80211_ATTR_PEER_MEASUREMENTS],
                           NULL);
    if (err)
        goto nla_put_failure;

    struct nlattr *peer, **resp;
    int i;
    nla_for_each_nested(peer, pmsr[NL80211_PMSR_ATTR_PEERS], i) {
        nla_parse_nested(resp, NL80211_PMSR_RESP_ATTR_MAX,
                         tb[NL80211_PMSR_PEER_ATTR_RESP], NULL);
        struct nlattr *peer_tb[NL80211_PMSR_PEER_ATTR_MAX + 1];
        struct nlattr *resp[NL80211_PMSR_RESP_ATTR_MAX + 1];
        struct nlattr *data[NL80211_PMSR_TYPE_MAX + 1];
        struct nlattr *ftm[NL80211_PMSR_FTM_RESP_ATTR_MAX + 1];
        err = nla_parse_nested(peer_tb, NL80211_PMSR_PEER_ATTR_MAX,
                               peer, NULL);
        if (err)
            goto nla_put_failure;
        err = nla_parse_nested(resp, NL80211_PMSR_RESP_ATTR_MAX,
                               peer_tb[NL80211_PMSR_PEER_ATTR_RESP], 
                               NULL);
        if (err)
            goto nla_put_failure;

        err = nla_parse_nested(data, NL80211_PMSR_TYPE_MAX,
                               resp[NL80211_PMSR_RESP_ATTR_DATA], 
                               NULL);
        if (err)
            goto nla_put_failure;

        err = nla_parse_nested(ftm, NL80211_PMSR_FTM_RESP_ATTR_MAX,
                               data[NL80211_PMSR_TYPE_FTM], NULL);
        if (err)
            goto nla_put_failure;

        int64_t dist;

        if (ftm[NL80211_PMSR_FTM_RESP_ATTR_DIST_AVG])
            dist = nla_get_s64(ftm[NL80211_PMSR_FTM_RESP_ATTR_DIST_AVG]);
        printf("Measurement result: %ld", dist);
    };
    return 0;
nla_put_failure:
    return 1;
}

static int listen_ftm_result(struct nl80211_state *state) {
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg)
        return 1;
    genlmsg_put(msg, 0, NL_AUTO_SEQ, state->nl80211_id, 0, 0,
                NL80211_CMD_PEER_MEASUREMENT_RESULT, 0);

    // signed long long devidx = if_nametoindex("wlp3s0");
    // NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);

    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEBUG);
    if (!cb)
        return 1;

    nl_socket_set_cb(state->nl_sock, cb);
    nl_send_auto(state->nl_sock, msg);

    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, handle_ftm_result, NULL);

    while (1)
        nl_recvmsgs(state->nl_sock, cb);
nla_put_failure:
    return 1;
}

int main() {
    struct nl80211_state nlstate;
    int err = nl80211_init(&nlstate);
    if (err) {
    	printf("fail to allocate socket\n");
        return 1;
    }

    err = start_ftm(&nlstate);
    if (err) {
        printf("fail to start ftm\n");
        return 1;
    }

    err = listen_ftm_result(&nlstate);
    if (err) {
        printf("fail to listen\n");
        return 1;
    }

    return 0;
}
