#include <stdio.h>

#include "initiator/initiator.h"


int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("Arguments needed!\n");
        return 1;
    }
    const char *cmd = argv[1];
    if (strcmp(cmd, "start_measurement") == 0) {
        argc--;
        argv++;
        int err = my_start_ftm(argc, argv);
        if (err) {
            return 1;
        }
    } else {
        printf("Invalid arguments!\n");
        return 1;
    }
    return 0;
}