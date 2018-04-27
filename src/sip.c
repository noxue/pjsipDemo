//
// Created by noxue on 4/27/18.
//

#include "sip.h"
#include "init.h"
#include "rtp.h"


pj_status_t make_call(const pj_str_t *dst_uri) {
    unsigned i;
    call_t *call;
    pjsip_dialog *dlg;
    pjmedia_sdp_session *sdp;
    pjsip_tx_data *tdata;
    pj_status_t status;


/* Find unused call slot */
    for (i=0; i<app.max_calls; ++i) {
        if (app.call[i].inv == NULL)
            break;
    }

    if (i == app.max_calls)
        return PJ_ETOOMANY;

    call = &app.call[i];

    /* Create UAC dialog */
    status = pjsip_dlg_create_uac( pjsip_ua_instance(),
                                   &app.local_uri,	/* local URI	    */
                                   &app.local_contact,	/* local Contact    */
                                   dst_uri,		/* remote URI	    */
                                   dst_uri,		/* remote target    */
                                   &dlg);		/* dialog	    */
    if (status != PJ_SUCCESS) {
        ++app.uac_calls;
        return status;
    }

    /* Create SDP */
    create_sdp( dlg->pool, call, &sdp);

    /* Create the INVITE session. */
    status = pjsip_inv_create_uac( dlg, sdp, 0, &call->inv);
    if (status != PJ_SUCCESS) {
        pjsip_dlg_terminate(dlg);
        ++app.uac_calls;
        return status;
    }


    /* Attach call data to invite session */
    call->inv->mod_data[mod_siprtp.id] = call;

    /* Mark start of call */
    pj_gettimeofday(&call->start_time);


    /* Create initial INVITE request.
     * This INVITE request will contain a perfectly good request and
     * an SDP body as well.
     */
    status = pjsip_inv_invite(call->inv, &tdata);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);


    /* Send initial INVITE request.
     * From now on, the invite session's state will be reported to us
     * via the invite session callbacks.
     */
    status = pjsip_inv_send_msg(call->inv, tdata);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);


    return PJ_SUCCESS;
}



void destroy_sip()
{
    unsigned i;

    app.thread_quit = 1;
    for (i=0; i<app.thread_count; ++i) {
        if (app.sip_thread[i]) {
            pj_thread_join(app.sip_thread[i]);
            pj_thread_destroy(app.sip_thread[i]);
            app.sip_thread[i] = NULL;
        }
    }

    if (app.sip_endpt) {
        pjsip_endpt_destroy(app.sip_endpt);
        app.sip_endpt = NULL;
    }

}