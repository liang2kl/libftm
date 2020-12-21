#include "nl.h"

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

int valid_handler(struct nl_msg *msg, void *arg) {
    return NL_OK;
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

int nl80211_init(struct nl80211_state *state) {  // from iw
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

struct nl_cb_arg alloc_nl_cb_arg(void *arg) {
    struct nl_cb_arg cb_arg = {arg, NULL};
    return cb_arg;
}

int nl_handle_msg(struct nl80211_state *state, struct nl_msg *msg,
                  nl_recvmsg_msg_cb_t handler, struct nl_cb_arg *arg) {
    int err;
    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        fprintf(stderr, "Fail to allocate callback\n");
        return 1;
    }

    if (msg) {
        err = nl_send_auto(state->nl_sock, msg);
        if (err < 0)
            return 1;
    }

    err = 1;
    if (arg)
        arg->state = &err;

    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

    if (handler)
        nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, handler, arg);
    else
        nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);
    
    while (err > 0)
        nl_recvmsgs(state->nl_sock, cb);
    if (err < 0) {
        fprintf(stderr, "Command failed: %s (%d)\n", strerror(-err), err);
        return 1;
    }
    return 0;
}