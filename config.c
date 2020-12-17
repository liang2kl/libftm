#include "config.h"
enum nl80211_chan_width str_to_bw(const char *str) {
	static const struct {
		const char *name;
		unsigned int val;
	} bwmap[] = {
		{ .name = "5", .val = NL80211_CHAN_WIDTH_5, },
		{ .name = "10", .val = NL80211_CHAN_WIDTH_10, },
		{ .name = "20", .val = NL80211_CHAN_WIDTH_20, },
		{ .name = "40", .val = NL80211_CHAN_WIDTH_40, },
		{ .name = "80", .val = NL80211_CHAN_WIDTH_80, },
		{ .name = "80+80", .val = NL80211_CHAN_WIDTH_80P80, },
		{ .name = "160", .val = NL80211_CHAN_WIDTH_160, },
	};
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(bwmap); i++) {
		if (strcasecmp(bwmap[i].name, str) == 0)
			return bwmap[i].val;
	}

	return NL80211_CHAN_WIDTH_20_NOHT;
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
        unsigned char addr[ETH_ALEN];
        int res, consumed;
        char *bw = NULL, *pos, *tmp, *save_ptr, *delims = " \t\n";
        bool report_ap_tsf = false, preamble = false;
        unsigned int freq = 0, cf1 = 0, cf2 = 0;
        char *str = line;
        res = sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
                     &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5],
                     &consumed);

        if (res != ETH_ALEN) {
            printf("Invalid MAC address\n");
            return NULL;
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
                __SET_ATTR(center_freq, str_to_bw(pos + 3));
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
            } else {
                printf("Unknown parameter %s\n", pos);
                return NULL;
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
                    return NULL;
            }
            FTM_PEER_SET_ATTR(attr, preamble, preamble);
        }
    }
    struct ftm_config *config = alloc_ftm_config(if_name, peers, line_num);
    if (!config) {
        fprintf(stderr, "Fail to allocate config!\n");
    }
    return config;
return_err:
    fclose(file);
    return NULL;
}
