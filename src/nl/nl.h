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

/**
 * nl80211_init - Initialize socket and nl80211 identifier
 * 
 * @param state   nl80211_state pointer to be filled
 * 
 * @return 0 on success, 1 on failure
 */
int nl80211_init(struct nl80211_state *state);

/**
 * struct nl_cb_arg - Argument pass to the callback
 * 
 * @arg: Any pointer you want to pass to the callback
 * @state: Variable controlling the process of receiving netlink messages.
 * Must set it to 0 when you no longer receive messages from the
 * socket.
 * 
 * @note
 * In the handler, access the instance by casting arg to type nl_cb_arg *.
 */
struct nl_cb_arg {
    void *arg;
    int *state;
};

/**
 * allocate_nl_cb_arg - Create a nl_cb_arg instance with given pointer.
 * 
 * @param arg   Any pointer you want to pass to the callback
 */
struct nl_cb_arg alloc_nl_cb_arg(void *arg);

/**
 * nl_handle_msg - Start receiving or sending netlink message
 * 
 * @param state     nl80211_state instance created by nl80211_init()
 * @param msg       Netlink message to be sent, can be NULL if you don't need
 *                  to send any message.
 * @param handler   Callback to handle results. Can be NULL.
 * @param arg       nl_cb_arg to be sent to the callback. Can be NULL.
 * 
 * @return 0 on success, 1 on failure
 */
int nl_handle_msg(struct nl80211_state *state, struct nl_msg *msg,
                  nl_recvmsg_msg_cb_t handler, struct nl_cb_arg *arg);
#endif