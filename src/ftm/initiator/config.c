#include "config.h"
enum nl80211_chan_width str_to_bw(const char *str) {
#define BW_FROM_STR(des)                 \
    if (strcasecmp(str, #des)) {         \
        return NL80211_CHAN_WIDTH_##des; \
    }
    BW_FROM_STR(5);
    BW_FROM_STR(10);
    BW_FROM_STR(20);
    BW_FROM_STR(40);
    BW_FROM_STR(80);
    BW_FROM_STR(160);
    if (strcasecmp(str, "80+80")) {
        return NL80211_CHAN_WIDTH_80P80;
    }
    return NL80211_CHAN_WIDTH_20_NOHT;
}

int parse_peer_config(struct ftm_peer_attr *attr, char *str) {
    unsigned char addr[6];
    int res, consumed;
    char *bw = NULL, *pos, *tmp, *save_ptr, *delims = " \t\n";
    bool report_ap_tsf = false, preamble = false;
    unsigned int freq = 0, cf1 = 0, cf2 = 0;
    res = sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
                 &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5],
                 &consumed);

    if (res != 6) {
        printf("Invalid MAC address\n");
        return 1;
    }

    FTM_PEER_SET_ATTR_ADDR(attr, addr);

    str += consumed;
    pos = strtok_r(str, delims, &save_ptr);

#define INVALID_ATTR(attr_name)                    \
    if (*tmp) {                                    \
        printf("Invalid " #attr_name " value!\n"); \
        goto return_err;                           \
    }
#define __SET_ATTR(attr_name, value)               \
    do {                                           \
        FTM_PEER_SET_ATTR(attr, attr_name, value); \
        INVALID_ATTR(attr_name);                   \
    } while (0)

    while (pos) {
        if (strncmp(pos, "cf=", 3) == 0) {
            __SET_ATTR(center_freq, strtol(pos + 3, &tmp, 0));
        } else if (strncmp(pos, "bw=", 3) == 0) {
            bw = pos + 3;
            FTM_PEER_SET_ATTR(attr, center_freq, str_to_bw(bw));
        } else if (strncmp(pos, "cf1=", 4) == 0) {
            __SET_ATTR(center_freq_1,
                       strtol(pos + 4, &tmp, 0));
        } else if (strncmp(pos, "cf2=", 4) == 0) {
            __SET_ATTR(center_freq_2,
                       strtol(pos + 4, &tmp, 0));
        } else if (strncmp(pos, "bursts_exp=", 11) == 0) {
            __SET_ATTR(num_bursts_exp,
                       strtol(pos + 11, &tmp, 0));
        } else if (strncmp(pos, "burst_period=", 13) == 0) {
            __SET_ATTR(burst_period,
                       strtol(pos + 13, &tmp, 0));
        } else if (strncmp(pos, "retries=", 8) == 0) {
            __SET_ATTR(num_ftmr_retries,
                       strtol(pos + 8, &tmp, 0));
        } else if (strncmp(pos, "burst_duration=", 15) == 0) {
            __SET_ATTR(burst_duration,
                       strtol(pos + 15, &tmp, 0));
        } else if (strncmp(pos, "ftms_per_burst=", 15) == 0) {
            __SET_ATTR(ftms_per_burst,
                       strtol(pos + 15, &tmp, 0));
        } else if (strcmp(pos, "asap") == 0) {
            FTM_PEER_SET_ATTR(attr, asap, 1);
        } else if (strncmp(pos, "tb", 2) == 0) {
            FTM_PEER_SET_ATTR(attr, trigger_based, 1);
            FTM_PEER_SET_ATTR(attr, preamble, NL80211_PREAMBLE_HE);
            preamble = true;
        } else if (strncmp(pos, "rtt_correct=", 12) == 0) {
            __SET_ATTR(rtt_correct,
                       strtol(pos + 12, &tmp, 0));
        } else {
            printf("Unknown parameter %s\n", pos);
            return 1;
        }

        pos = strtok_r(NULL, delims, &save_ptr);
    }

    if (!preamble) {
        int preamble = -1;
        switch (str_to_bw(bw)) {
            case NL80211_CHAN_WIDTH_20_NOHT:
            case NL80211_CHAN_WIDTH_5:
            case NL80211_CHAN_WIDTH_10:
                preamble = NL80211_PREAMBLE_LEGACY;
                break;
            case NL80211_CHAN_WIDTH_20:
            case NL80211_CHAN_WIDTH_40:
                preamble = NL80211_PREAMBLE_HT;
                break;
            case NL80211_CHAN_WIDTH_80:
            case NL80211_CHAN_WIDTH_80P80:
            case NL80211_CHAN_WIDTH_160:
                preamble = NL80211_PREAMBLE_VHT;
                break;
            default:
                return 1;
        }
        FTM_PEER_SET_ATTR(attr, preamble, preamble);
    }
    return 0;
return_err:
    return 1;
}

struct ftm_config *parse_config_file(const char *file_name,
                                     const char *if_name) {
    FILE *file = fopen(file_name, "r");
    if (!file) {
        fprintf(stderr, "Fail to open file %s\n", file_name);
        return NULL;
    }
    /* placeholder here */
    unsigned int max_peer_count = 16;
    struct ftm_peer_attr **peers =
        malloc(max_peer_count * sizeof(struct ftm_peer_attr *));

    char line[255];
    int line_num;
    for (line_num = 1; fgets(line, sizeof(line), file); line_num++) {
        if (line_num > max_peer_count) {
            fprintf(stderr, "Peer count over %u, the rest will be ignored!\n",
                    max_peer_count);
            line_num--;
            break;
        }
        struct ftm_peer_attr *attr = alloc_ftm_peer();
        peers[line_num - 1] = attr;
        if (parse_peer_config(attr, line)) {
            printf("Invalid FTM configuration at line %d!\n",
                   line_num);
            return NULL;
        }
    }
    struct ftm_config *config = alloc_ftm_config(if_name, peers, line_num - 1);
    if (!config) {
        fprintf(stderr, "Fail to allocate config!\n");
    }
    return config;
return_err:
    fclose(file);
    return NULL;
}
