//
// Created by noxue on 4/27/18.
//

#ifndef PHONE_CALLBACK_H
#define PHONE_CALLBACK_H


#include <pjsip-ua/sip_inv.h>

/* Callback to be called when invite session's state has changed: */
void call_on_state_changed( pjsip_inv_session *inv,
                                   pjsip_event *e);


/* Callback to be called when dialog has forked: */
void call_on_forked(pjsip_inv_session *inv, pjsip_event *e);

#endif //PHONE_CALLBACK_H
