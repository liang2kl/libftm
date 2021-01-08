#include "scan.h"

static int start_scan(struct nl80211_state *state, const char *if_name) {
    int err;
    struct nl_msg *msg = init_nl_msg_with_if(if_name, state->nl80211_id);
    if (!msg) {
        fprintf(stderr, "Fail to create message for scanning!");
        return 1;
    }
    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, state->nl80211_id, 0, 0,
                NL80211_CMD_TRIGGER_SCAN, 0);
    err = nl_sock_handle(state->nl_sock, msg, NULL, NULL);
    return err;
}

int handle_scan_result(struct nl_msg *msg, void *arg) {
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    /* fetch pointer from nl_cb_arg */
    struct nl_cb_arg *cb_arg = arg;
    struct ftm_ap_attrs_wrap *attrs_wrap = cb_arg->arg;
    if (gnlh->cmd == NL80211_CMD_NEW_SCAN_RESULTS) {
        *cb_arg->state = 0;
    }
    
}

static int **listen_scan_result(struct nl80211_state *state, const char *if_name, struct ftm_ap_attrs_wrap *attrs_wrap) {
    struct nl_cb_arg arg = alloc_nl_cb_arg(attrs_wrap);
    int err;
    struct nl_msg *msg = init_nl_msg_with_if(if_name, state->nl80211_id);
    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, state->nl80211_id, 0, 0,
                NL80211_CMD_GET_SCAN, 0);
    err = nl_sock_handle(state, NULL, handle_scan_result, &arg);
    if (err) {
        fprintf(stderr, "Fail to listen scan result!\n");
    }
    return err;
}

int scan_ap(ftm_ap_scan_handler_t handler, const char* if_name) {
    struct nl80211_state nlstate;
    int err = nl80211_init(&nlstate);
    if (err) {
        fprintf(stderr, "Fail to allocate socket!\n");
        return 1;
    }
    err = start_scan(&nlstate, if_name);
    if (err) {
        fprintf(stderr, "Fail to start scanning!\n");
        return 1;
    }
    struct ftm_ap_attr **attrs = handle_scan_result(&nlstate);
    if (!attrs) {
        return 1;
    }
    if (!handler) {
        fprintf(stderr, "No handler provided to handle scan result!\n");
        return 1;
    }
    handler(attrs);
    return 0;
}