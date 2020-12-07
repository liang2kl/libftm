#ifndef FTM_H
#define FTM_H

#include <errno.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>
#include <stdbool.h>
#include <stdint.h>
#include "ftm_types.h"
#include "nl80211.h"

#define 

int valid_handler(struct nl_msg *msg, void *arg);
void register_handler(int (*handler)(struct nl_msg *, void *));
struct nl80211_state {
    struct nl_sock *nl_sock;
    int nl80211_id;
};
#endif