#ifndef _FTM_H
#define _FTM_H
#include <errno.h>
#include <net/if.h>
#include <netlink/attr.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nl80211.h"
#include "types.h"

int ftm(struct ftm_config *config,
        void (*handler)(struct ftm_results_wrap *wrap),
        int attemps);
        
#endif /* _FTM_H */