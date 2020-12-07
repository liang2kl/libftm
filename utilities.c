#include <stdio.h>

void mac_addr_n2a(char *mac_addr, const unsigned char *arg) {
    int i, l;

    l = 0;
    for (i = 0; i < 6; i++) {
        if (i == 0) {
            sprintf(mac_addr + l, "%02x", arg[i]);
            l += 2;
        } else {
            sprintf(mac_addr + l, ":%02x", arg[i]);
            l += 3;
        }
    }
}
