//
// Created by noxue on 4/27/18.
//

#ifndef PHONE_SIP_H
#define PHONE_SIP_H

#include <phone.h>



/*
 * Make outgoing call.
 */
pj_status_t make_call(const pj_str_t *dst_uri);

/*
 * Destroy SIP
 */
void destroy_sip();
#endif //PHONE_SIP_H
