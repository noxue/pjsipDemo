//
// Created by noxue on 4/27/18.
//


#ifndef PHONE_H
#define PHONE_H



#include <stdio.h>

#include <pjsip.h>
#include <pjmedia.h>
#include <pjsip_ua.h>
#include <pjlib-util.h>
#include <pjlib.h>

#include "config.h"


typedef struct _codec{
    uint                pt;
    char                *name;
    uint                clock_rate;
    uint                bit_rate;
    uint                ptime;
    char                *description;
}codec_t;

typedef struct _media_stream{
    uint                    call_index;
    uint                    media_index;
    pjmedia_transport       *transport;

    pj_bool_t               active;

    pjmedia_stream_info     si;
    uint                    clock_rate;
    uint                    samples_per_frame;
    uint                    bytes_per_frame;

    /* RTP session: */
    pjmedia_rtp_session     out_sess;
    pjmedia_rtp_session     in_sess;

    pjmedia_rtcp_session    rtcp;

    /* thread: */
    pj_bool_t               thread_quit_flag;
    pj_thread_t             *thread;


    FILE                    *recv_fd;
}media_stream_t;

typedef struct _call{
    u_int               index;
    pjsip_inv_session   *inv;
    u_int               media_count;
    media_stream_t      media[1];
    pj_time_val         start_time;
    pj_time_val         response_time;
    pj_time_val         connect_time;

    pj_timer_entry      d_timer;        /* Disconnect timer */
} call_t;

typedef struct _App{

    char                *username;

    pjsip_endpoint      *sip_endpt;

    pj_bool_t           quit_flag;

    pj_bool_t           call_report;

    pj_caching_pool     cp;
    pj_pool_t           *pool;
    pj_thread_t         *sip_thread[1];
    pj_bool_t           thread_quit;
    uint                thread_count;

    int                 sip_port;
    int                 rtp_start_port;
    pj_str_t            local_addr;
    pj_str_t            local_uri;
    pj_str_t            local_contact;

    int                 app_log_level;
    int                 log_level;
    char                *log_filename;


    pj_str_t            url_to_call;

    pjmedia_endpt       *med_endpt;

    call_t              call[MAX_CALLS];
    uint                max_calls;

    uint                uac_calls;
    uint                duration;


    codec_t             audio_codec;

} app_t;

extern  app_t app;

/* Codec constants */
extern codec_t audio_codecs[5];

#endif //PHONE_MAIN_H
