#ifndef _FTM_SCAN_H
#define _FTM_SCAN_H
#include <stdint.h>
#include "../nl/nl.h"

struct ftm_ap_attr {
    uint8_t mac_addr[6];
    uint32_t center_freq;
};

struct ftm_ap_attrs_wrap {
    struct ftm_ap_attr **attrs;
    int count;
};

typedef int (*ftm_ap_scan_handler_t)(struct ftm_ap_attr **attrs);

int scan_ap(ftm_ap_scan_handler_t handler, const char* if_name);
#endif /*_FTM_SCAN_H*/