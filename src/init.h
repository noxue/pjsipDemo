//
// Created by noxue on 4/27/18.
//


#ifndef PHONE_INIT_H
#define PHONE_INIT_H


#include <pjsip.h>
#include <pjmedia.h>
#include <pjsip_ua.h>
#include <pjlib-util.h>
#include <pjlib.h>
#include <phone.h>

pj_status_t app_logging_init(void);

pj_status_t init_sip();

void init_opts();

/*
 * Create SDP session for a call.
 */
pj_status_t create_sdp( pj_pool_t *pool,
                        call_t *call,
                        pjmedia_sdp_session **p_sdp);


#endif //PHONE_INIT_H
