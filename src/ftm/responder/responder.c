#include "responder.h"

int ftm_start_responder(const char *if_name) {
    struct nl80211_state nlstate;
    int err = 0;
    err = nl80211_init(&nlstate);
    if (err) {
        fprintf(stderr, "Fail to allocate socket!\n");
        return 1;
    }
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Fail to allocate message!");
        return 1;
    }
    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, nlstate.nl80211_id, 0, 0,
                NL80211_CMD_SET_BEACON, 0);

    signed long long devidx = if_nametoindex(if_name);
    if (devidx == 0) {
        fprintf(stderr, "Fail to find device interface %s!\n", if_name);
        return 1;
    }
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);
    struct nlattr *ftm = nla_nest_start(msg, NL80211_ATTR_FTM_RESPONDER);
    if (!ftm)
        return 1;
    nla_put_flag(msg, NL80211_FTM_RESP_ATTR_ENABLED);
    nla_nest_end(msg, ftm);
    err = nl_handle_msg(&nlstate, NL_SEND_MSG, msg, NULL, NULL, NULL);
    return err;
nla_put_failure:
    nlmsg_free(msg);
    return 1;
}
