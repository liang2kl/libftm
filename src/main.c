#include <stdio.h>

#include "ftm/initiator/start.h"
#include "ftm/responder/responder.h"

int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("Arguments needed!\n");
        return 1;
    }
    const char *cmd = argv[1];
    if (strcmp(cmd, "start_responder") == 0) {
        if (argc != 3) {
            printf("Invalid arguments!\n");
            return 1;
        }
        int err = ftm_start_responder(argv[2]);
        if (err) {
            printf("Fail to start ftm responder!\n");
            return 1;
        }
    } else if (strcmp(cmd, "start_measurement") == 0) {
        argc--;
        argv++;
        int err = my_start_ftm(argc, argv);
        if (err) {
            printf("Fail to start ftm responder!\n");
            return 1;
        }
    } else {
        printf("Invalid arguments!\n");
        return 1;
    }
    return 0;
}