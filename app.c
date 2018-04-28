//
// Created by noxue on 4/27/18.
//

#include <phone.h>
#include <utils.h>
#include <init.h>
#include <sip.h>
#include <logger.h>
#include <rtp.h>

/* For logging purpose. */
#define THIS_FILE   "sipecho.c"
#define PORT        5050
#define PORT_STR	":5050"

#define SAME_BRANCH	0
#define ACK_HAS_SDP	1


app_t   app;

codec_t audio_codecs[] = {
        { 8,  "PCMA", 8000, 64000, 20, "G.711 ALaw" },
        { 0,  "PCMU", 8000, 64000, 20, "G.711 ULaw" },
        { 3,  "GSM",  8000, 13200, 20, "GSM" },
        { 4,  "G723", 8000, 6400,  30, "G.723.1" },
        { 18, "G729", 8000, 8000,  20, "G.729" },
};


static pjsip_endpoint *sip_endpt;
static pj_bool_t       quit_flag;
static pjsip_dialog   *dlg;




/* Worker thread for SIP */
static int sip_worker_thread(void *arg)
{
    PJ_UNUSED_ARG(arg);

    while (!app.thread_quit) {
        pj_time_val timeout = {0, 10};
        pjsip_endpt_handle_events(app.sip_endpt, &timeout);
    }

    return 0;
}

int main(){

    pj_caching_pool cp;
    pj_thread_t *thread;
    pj_pool_t *pool;
    pj_status_t status;

    //初始化pj库
    status = pj_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    init_opts();

    // init logging
    status = app_logging_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);




    status = init_sip();
    if (status != PJ_SUCCESS) {
        app_perror(__FILE__, "初始化 sip 失败", status);
        destroy_sip();
        return -1;
    }

    /* Register module to log incoming/outgoing messages */
    pjsip_endpt_register_module(app.sip_endpt, &msg_logger);

    /* Init media */
    status = init_media();
    if (status != PJ_SUCCESS) {
        app_perror(THIS_FILE, "Media initialization failed", status);
        destroy_sip();
        return -1;
    }

    for (int i=0; i<app.thread_count; ++i) {
        pj_thread_create( app.pool, "app", &sip_worker_thread, NULL,
                          0, 0, &app.sip_thread[i]);
    }

    pj_str_t dest = pj_str("sip:13758277505@192.168.4.101");
    make_call(&dest);
//    make_call(&dest);


    for (;;) {
        char line[10];

        fgets(line, sizeof(line), stdin);

        switch (line[0]) {

            case 'q':
                goto on_quit;

            default:
                break;
        }
    }

    on_quit:

    pj_thread_join(app.sip_thread[0]);

    pjsip_endpt_destroy(app.sip_endpt);
    pj_caching_pool_destroy(&app.cp);
    pj_shutdown();
    return 0;

}

