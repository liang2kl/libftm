#include "initiator.h"

#define SOL 299492458
#define RTT_TO_DIST(rtt) ((float)rtt * SOL / 1000000000000)

//static void custom_result_handler(struct ftm_results_wrap *results,
//                                  int attempts, int attempt_idx, void *arg) {
//    printf("\n #%d\n", attempt_idx);
//    for (int i = 0; i < results->count; i++) {
//        struct ftm_resp_attr *resp = results->results[i];
//        if (!resp) {
//            fprintf(stderr, "Response %d does not exist!", i);
//            return;
//        }
//        if (results->results[i]->flags[FTM_RESP_FLAG_rtt_avg]) {
//            printf("%f\n", RTT_TO_DIST(results->results[i]->rtt_avg));
//        }

//    }
//}

static void custom_result_handler(struct ftm_results_wrap *results,
                                  int attempts, int attempt_idx, void *arg) {
    for (int i = 0; i < results->count; i++) {
        struct ftm_resp_attr *resp = results->results[i];
        if (!resp) {
            fprintf(stderr, "Response %d does not exist!", i);
            return;
        }
        if (results->results[i]->flags[FTM_RESP_FLAG_rtt_avg]) {
            printf("%.2f\n", RTT_TO_DIST(results->results[i]->rtt_avg));
        } else {
            printf("NULL\n");
        }
    }
}
int my_start_ftm(int argc, char **argv) {
    /* parse the arguments */
    if (argc != 4 && argc != 3) {
        printf("Invalid arguments!\n");
        printf("Valid args: <if_name> <file_path> [<attemps>]\n");
        return 1;
    }
    const char *if_name = argv[1];
    const char *file_name = argv[2];
    int attempts = 1;
    if (argc == 4)
        attempts = atoi(argv[3]);

    /* generate config from config file */
    struct ftm_config *config = parse_config_file(file_name, if_name);
    if (!config) {
        fprintf(stderr, "Fail to parse config!\n");
        return 1;
    }

    /* 
     * start FTM using the config we created, our custom handler,
     * the attempt number we designated, and the pointer to our data
     */
    int err = ftm(config, custom_result_handler, attempts, NULL);
    if (err)
        fprintf(stderr, "FTM measurement failed!\n");
    
    free_ftm_config(config);
    return err;
}