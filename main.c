#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netlink/netlink.h>   //lots of netlink functions
#include <netlink/genl/genl.h> //genl_connect, genlmsg_put
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h> //genl_ctrl_resolve
#include <linux/nl80211.h>     //NL80211 definitions


struct nl80211_state {
    struct nl_sock *nl_sock;
    int nl80211_id;
};

static int nl80211_init(struct nl80211_state *state) {
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

    nl_socket_set_buffer_size(state->nl_sock, 8192, 8192);

    /* try to set NETLINK_EXT_ACK to 1, ignoring errors */
    err = 1;
    setsockopt(nl_socket_get_fd(state->nl_sock), SOL_NETLINK,
               NETLINK_EXT_ACK, &err, sizeof(err));

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

int main() {
    struct nl80211_state nlstate;
    int err = nl80211_init(&nlstate);
    if (err) {
        return 1;
    }

    struct nl_cb *cb1;
    struct nl_cb *cb2;
    struct nl_msg *msg;
    msg = nlmsg_alloc();    // init netlink message
    
    cb1 = nl_cb_alloc(NL_CB_DEFAULT);
    cb2 = nl_cb_alloc(NL_CB_DEFAULT);

    if (!cb1 || !cb2) {
        fprintf(stderr, "Failed to allocate netlink callback.\n");
        nl_close(nlstate.nl_sock);
        nl_socket_free(nlstate.nl_sock);
        return -1;
    }

}