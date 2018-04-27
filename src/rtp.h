//
// Created by noxue on 4/27/18.
//

#ifndef PHONE_RTP_H
#define PHONE_RTP_H

#include <pjsip.h>
#include <pjmedia.h>
#include <pjsip_ua.h>
#include <pjlib-util.h>
#include <pjlib.h>



extern pjsip_module mod_siprtp;

pj_status_t init_media();


/*
 * This callback is called by media transport on receipt of RTP packet.
 */
void on_rx_rtp(void *user_data, void *pkt, pj_ssize_t size);


/* Callback to be called to handle incoming requests outside dialogs: */
pj_bool_t on_rx_request( pjsip_rx_data *rdata );


/* Callback to be called when SDP negotiation is done in the call: */
void call_on_media_update( pjsip_inv_session *inv,
                                  pj_status_t status);


#endif //PHONE_RTP_H
