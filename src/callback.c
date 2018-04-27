//
// Created by noxue on 4/27/18.
//

#include "callback.h"
#include "rtp.h"
#include "utils.h"
#include "phone.h"


#define THIS_FILE __FILE__


/* Destroy call's media */
static void destroy_call_media(unsigned call_index)
{
    media_stream_t *audio = &app.call[call_index].media[0];

    if (audio) {
        audio->active = PJ_FALSE;

        if (audio->thread) {
            audio->thread_quit_flag = 1;
            pj_thread_join(audio->thread);
            pj_thread_destroy(audio->thread);
            audio->thread = NULL;
            audio->thread_quit_flag = 0;
        }

        pjmedia_transport_detach(audio->transport, audio);
    }
}

static void hangup_call(unsigned index)
{
    pjsip_tx_data *tdata;
    pj_status_t status;

    if (app.call[index].inv == NULL)
        return;

    status = pjsip_inv_end_session(app.call[index].inv, 603, NULL, &tdata);
    if (status==PJ_SUCCESS && tdata!=NULL)
        pjsip_inv_send_msg(app.call[index].inv, tdata);
}

/* Callback timer to disconnect call (limiting call duration) */
static void timer_disconnect_call( pj_timer_heap_t *timer_heap,
                                   struct pj_timer_entry *entry)
{
    call_t *call = entry->user_data;

    PJ_UNUSED_ARG(timer_heap);

    entry->id = 0;
    hangup_call(call->index);
}


void call_on_state_changed( pjsip_inv_session *inv,
                            pjsip_event *e)
{
    printf("\n--------------------------------------------------------------\n");
      call_t *call = inv->mod_data[mod_siprtp.id];

    PJ_UNUSED_ARG(e);

    if (!call)
        return;

    if (inv->state == PJSIP_INV_STATE_DISCONNECTED) {

        pj_time_val null_time = {0, 0};

        if (call->d_timer.id != 0) {
            pjsip_endpt_cancel_timer(app.sip_endpt, &call->d_timer);
            call->d_timer.id = 0;
        }

        PJ_LOG(3,(THIS_FILE, "Call #%d disconnected. Reason=%d (%.*s)",
                call->index,
                inv->cause,
                (int)inv->cause_text.slen,
                inv->cause_text.ptr));

        if (app.call_report) {
            PJ_LOG(3,(THIS_FILE, "Call #%d statistics:", call->index));
//            print_call(call->index);
        }


        call->inv = NULL;
        inv->mod_data[mod_siprtp.id] = NULL;

        destroy_call_media(call->index);

        call->start_time = null_time;
        call->response_time = null_time;
        call->connect_time = null_time;

        ++app.uac_calls;

    } else if (inv->state == PJSIP_INV_STATE_CONFIRMED) {

        pj_time_val t;

        pj_gettimeofday(&call->connect_time);
        if (call->response_time.sec == 0)
            call->response_time = call->connect_time;

        t = call->connect_time;
        PJ_TIME_VAL_SUB(t, call->start_time);

        PJ_LOG(3,(THIS_FILE, "Call #%d connected in %d ms", call->index,
                PJ_TIME_VAL_MSEC(t)));

        if (app.duration != 0) {
            call->d_timer.id = 1;
            call->d_timer.user_data = call;
            call->d_timer.cb = &timer_disconnect_call;

            t.sec = app.duration;
            t.msec = 0;

            pjsip_endpt_schedule_timer(app.sip_endpt, &call->d_timer, &t);
        }

    } else if (	inv->state == PJSIP_INV_STATE_EARLY ||
                   inv->state == PJSIP_INV_STATE_CONNECTING) {

        if (call->response_time.sec == 0)
            pj_gettimeofday(&call->response_time);

    }
}


/* Callback to be called when dialog has forked: */
void call_on_forked(pjsip_inv_session *inv, pjsip_event *e)
{
    PJ_UNUSED_ARG(inv);
    PJ_UNUSED_ARG(e);

    PJ_TODO( HANDLE_FORKING );
}


