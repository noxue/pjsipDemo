//
// Created by noxue on 4/27/18.
//

#include "utils.h"


int app_perror( const char *sender, const char *title,
                pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];

    pj_strerror(status, errmsg, sizeof(errmsg));

    PJ_LOG(3,(sender, "%s: %s [code=%d]", title, errmsg, status));
    return 1;
}

