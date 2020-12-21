#ifndef _FTM_RESPONDER_H
#define _FTM_RESPONDER_H
#include "../nl/nl.h"

/**
 * ftm_start_responder - Start FTM responder
 * 
 * @param if_name   Interface name of the wireless card
 * 
 * @note
 * A running AP is needed to start a responder.
 */
int ftm_start_responder(const char* if_name);
#endif