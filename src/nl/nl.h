#ifndef _FTM_NL_H
#define _FTM_NL_H

#include <errno.h>
#include <net/if.h>
#include <netlink/attr.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>
#include <stdbool.h>
#include <linux/nl80211.h>

struct nl80211_state {
    struct nl_sock *nl_sock;
    int nl80211_id;
};

enum nl_send_msg_option { NL_SEND_MSG, NL_SET_NO_SEQ_CHECK };

struct nl_cb_arg {
    void *arg;
    int *state;
};

struct nl_cb_arg alloc_nl_cb_arg(void *arg);

int nl_handle_msg(struct nl80211_state *state, int type, struct nl_msg *msg,
                  nl_recvmsg_msg_cb_t handler, struct nl_cb_arg *arg);

int nl80211_init(struct nl80211_state *state);
#endif