//
// Created by noxue on 4/27/18.
//



#ifndef PHONE_UTILS_H
#define PHONE_UTILS_H

#include <pjsip.h>
#include <pjmedia.h>
#include <pjsip_ua.h>
#include <pjlib-util.h>
#include <pjlib.h>


int app_perror( const char *sender, const char *title,
                       pj_status_t status);




#endif //PHONE_UTILS_H
