#include "ftm.h"


int main(int argc, char **argv) {
    struct ftm_peer_attr *attr = alloc_ftm_peer();
    // required
    uint8_t mac_addr[6] = {0x0a, 0x83, 0xa1, 0x15, 0xbf, 0x50};
    FTM_PEER_SET_ATTR(attr, mac_addr, mac_addr);
    FTM_PEER_SET_ATTR(attr, asap, 1);
    FTM_PEER_SET_ATTR(attr, center_freq, 2412);
    FTM_PEER_SET_ATTR(attr, chan_width, NL80211_CHAN_WIDTH_20);
    FTM_PEER_SET_ATTR(attr, preamble, NL80211_PREAMBLE_HT);

    // optional
    FTM_PEER_SET_ATTR(attr, ftms_per_burst, 5);
    FTM_PEER_SET_ATTR(attr, num_ftmr_retries, 5);

    struct ftm_peer_attr *peers[] = {attr};

    // the only interface to communicate with the tool
    struct ftm_config *config = alloc_ftm_config("wlp3s0", peers, 1);

    if (!config) {
        fprintf(stderr, "Fail to allocate config!\n");
        return 1;
    }

    // start FTM
    ftm(config, NULL, 1);

    // clean up
    free_ftm_config(config);

    return 0;
}
