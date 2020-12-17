#ifndef _FTM_CONFIG_H
#define _FTM_CONFIG_H

#include "ftm.h"
struct ftm_config *parse_config_file(const char *file_name,
                                     const char *if_name);
#endif /*_FTM_CONFIG_H*/