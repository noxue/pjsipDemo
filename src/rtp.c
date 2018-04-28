//
// Created by noxue on 4/27/18.
//

#define THIS_FILE __FILE__

#include "rtp.h"
#include "utils.h"
#include "phone.h"

#include <pthread.h>

#include <pjmedia-codec.h>

/* This is a PJSIP module to be registered by application to handle
 * incoming requests outside any dialogs/transactions. The main purpose
 * here is to handle incoming INVITE request message, where we will
 * create a dialog and INVITE session for it.
 */
pjsip_module mod_siprtp = {
        NULL, NULL,			    /* prev, next.		*/
        { "mod-siprtpapp", 13 },	    /* Name.			*/
        -1,				    /* Id			*/
        PJSIP_MOD_PRIORITY_APPLICATION, /* Priority			*/
        NULL,			    /* load()			*/
        NULL,			    /* start()			*/
        NULL,			    /* stop()			*/
        NULL,			    /* unload()			*/
        &on_rx_request,		    /* on_rx_request()		*/
        NULL,			    /* on_rx_response()		*/
        NULL,			    /* on_tx_request.		*/
        NULL,			    /* on_tx_response()		*/
        NULL,			    /* on_tsx_state()		*/
};


void on_rx_rtp(void *user_data, void *pkt, pj_ssize_t size)
{
    media_stream_t *strm;
    pj_status_t status;
    const pjmedia_rtp_hdr *hdr;
    const void *payload;
    unsigned payload_len;

    strm = user_data;

    /* Discard packet if media is inactive */
    if (!strm->active)
        return;

    /* Check for errors */
    if (size < 0) {
        app_perror(THIS_FILE, "RTP recv() error", (pj_status_t)-size);
        return;
    }

    /* Decode RTP packet. */
    status = pjmedia_rtp_decode_rtp(&strm->in_sess,
                                    pkt, (int)size,
                                    &hdr, &payload, &payload_len);

    if (status != PJ_SUCCESS) {
        app_perror(THIS_FILE, "RTP decode error", status);
        return;
    }

    PJ_LOG(4,(THIS_FILE, "Rx seq=%d", pj_ntohs(hdr->seq)));

    u_int16_t buf[payload_len];
    for(int i=0; i<payload_len; i++) {
        buf[i] = pjmedia_alaw2linear( ((u_char *)payload)[i] );
    }

    fwrite(buf,payload_len*2,1,strm->recv_fd);

    /* Update the RTCP session. */
    pjmedia_rtcp_rx_rtp(&strm->rtcp, pj_ntohs(hdr->seq),
                        pj_ntohl(hdr->ts), payload_len);

    /* Update RTP session */
    pjmedia_rtp_session_update(&strm->in_sess, hdr, NULL);

}


pj_bool_t on_rx_request( pjsip_rx_data *rdata )
{
    /* Ignore strandled ACKs (must not send respone */
    if (rdata->msg_info.msg->line.req.method.id == PJSIP_ACK_METHOD)
        return PJ_FALSE;

    /* Respond (statelessly) any non-INVITE requests with 500  */
    if (rdata->msg_info.msg->line.req.method.id != PJSIP_INVITE_METHOD) {
        pj_str_t reason = pj_str("Unsupported Operation");
        pjsip_endpt_respond_stateless( app.sip_endpt, rdata,
                                       500, &reason,
                                       NULL, NULL);
        return PJ_TRUE;
    }

    /* Handle incoming INVITE */
//    process_incoming_call(rdata);

    /* Done */
    return PJ_TRUE;
}

/*
 * Destroy media.
 */
static void destroy_media()
{
    unsigned i;

    for (i=0; i<app.max_calls; ++i) {
        unsigned j;
        for (j=0; j<PJ_ARRAY_SIZE(app.call[0].media); ++j) {
            media_stream_t *m = &app.call[i].media[j];

            if(m->recv_fd){
                fclose(m->recv_fd);
                m->recv_fd=NULL;
            }


            if (m->transport) {
                pjmedia_transport_close(m->transport);
                m->transport = NULL;
            }
        }
    }

    if (app.med_endpt) {
        pjmedia_endpt_destroy(app.med_endpt);
        app.med_endpt = NULL;
    }
}

/*
 * Init media stack.
 */
pj_status_t init_media()
{
    unsigned	i, count;
    pj_uint16_t	rtp_port;
    pj_status_t	status;


    /* Initialize media endpoint so that at least error subsystem is properly
     * initialized.
     */
#if PJ_HAS_THREADS
    status = pjmedia_endpt_create(&app.cp.factory, NULL, 1, &app.med_endpt);
#else
    status = pjmedia_endpt_create(&app.cp.factory,
								  pjsip_endpt_get_ioqueue(app.sip_endpt),
								  0, &app.med_endpt);
#endif
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);


    /* Must register codecs to be supported */
#if defined(PJMEDIA_HAS_G711_CODEC) && PJMEDIA_HAS_G711_CODEC!=0
    pjmedia_codec_g711_init(app.med_endpt);
#endif

    /* RTP port counter */
    rtp_port = (pj_uint16_t)(app.rtp_start_port & 0xFFFE);

    /* Init media transport for all calls. */
    for (i=0, count=0; i<app.max_calls; ++i, ++count) {

        unsigned j;

        /* Create transport for each media in the call */
        for (j=0; j<PJ_ARRAY_SIZE(app.call[0].media); ++j) {
            /* Repeat binding media socket to next port when fails to bind
             * to current port number.
             */
            int retry;

            app.call[i].media[j].call_index = i;
            app.call[i].media[j].media_index = j;

            status = -1;
            for (retry=0; retry<100; ++retry,rtp_port+=2)  {
                media_stream_t *m = &app.call[i].media[j];

                status = pjmedia_transport_udp_create2(app.med_endpt,
                                                       "siprtp",
                                                       &app.local_addr,
                                                       rtp_port, 0,
                                                       &m->transport);
                if (status == PJ_SUCCESS) {
                    rtp_port += 2;
                    break;
                }
            }
        }

        if (status != PJ_SUCCESS)
            goto on_error;
    }

    /* Done */
    return PJ_SUCCESS;

    on_error:
    destroy_media();
    return status;
}




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


/*
 * This callback is called by media transport on receipt of RTCP packet.
 */
static void on_rx_rtcp(void *user_data, void *pkt, pj_ssize_t size)
{
    media_stream_t *strm;

    strm = user_data;

    /* Discard packet if media is inactive */
    if (!strm->active)
        return;

    /* Check for errors */
    if (size < 0) {
        app_perror(THIS_FILE, "Error receiving RTCP packet",(pj_status_t)-size);
        return;
    }

    /* Update RTCP session */
    pjmedia_rtcp_rx_rtcp(&strm->rtcp, pkt, size);
}



static void boost_priority(void)
{
#define POLICY	SCHED_FIFO
    struct sched_param tp;
    int max_prio;
    int policy;
    int rc;

    if (sched_get_priority_min(POLICY) < sched_get_priority_max(POLICY))
        max_prio = sched_get_priority_max(POLICY)-1;
    else
        max_prio = sched_get_priority_max(POLICY)+1;

    /*
     * Adjust process scheduling algorithm and priority
     */
    rc = sched_getparam(0, &tp);
    if (rc != 0) {
        app_perror( THIS_FILE, "sched_getparam error",
                    PJ_RETURN_OS_ERROR(rc));
        return;
    }
    tp.sched_priority = max_prio;

    rc = sched_setscheduler(0, POLICY, &tp);
    if (rc != 0) {
        app_perror( THIS_FILE, "sched_setscheduler error",
                    PJ_RETURN_OS_ERROR(rc));
    }

    PJ_LOG(4, (THIS_FILE, "New process policy=%d, priority=%d",
            policy, tp.sched_priority));

    /*
     * Adjust thread scheduling algorithm and priority
     */
    rc = pthread_getschedparam(pthread_self(), &policy, &tp);
    if (rc != 0) {
        app_perror( THIS_FILE, "pthread_getschedparam error",
                    PJ_RETURN_OS_ERROR(rc));
        return;
    }

    PJ_LOG(4, (THIS_FILE, "Old thread policy=%d, priority=%d",
            policy, tp.sched_priority));

    policy = POLICY;
    tp.sched_priority = max_prio;

    rc = pthread_setschedparam(pthread_self(), policy, &tp);
    if (rc != 0) {
        app_perror( THIS_FILE, "pthread_setschedparam error",
                    PJ_RETURN_OS_ERROR(rc));
        return;
    }

    PJ_LOG(4, (THIS_FILE, "New thread policy=%d, priority=%d",
            policy, tp.sched_priority));
}



/*
 * Media thread
 *
 * This is the thread to send and receive both RTP and RTCP packets.
 */
static int media_thread(void *arg)
{
    enum { RTCP_INTERVAL = 5000, RTCP_RAND = 2000 };
    media_stream_t *strm = arg;
    char packet[1500];
    unsigned msec_interval;
    pj_timestamp freq, next_rtp, next_rtcp;


    /* Boost thread priority if necessary */
    boost_priority();

    /* Let things settle */
    pj_thread_sleep(100);

    msec_interval = strm->samples_per_frame * 1000 / strm->clock_rate;
    pj_get_timestamp_freq(&freq);

    pj_get_timestamp(&next_rtp);
    next_rtp.u64 += (freq.u64 * msec_interval / 1000);

    next_rtcp = next_rtp;
    next_rtcp.u64 += (freq.u64 * (RTCP_INTERVAL+(pj_rand()%RTCP_RAND)) / 1000);


    while (!strm->thread_quit_flag) {
        pj_timestamp now, lesser;
        pj_time_val timeout;
        pj_bool_t send_rtp, send_rtcp;

        send_rtp = send_rtcp = PJ_FALSE;

        /* Determine how long to sleep */
        if (next_rtp.u64 < next_rtcp.u64) {
            lesser = next_rtp;
            send_rtp = PJ_TRUE;
        } else {
            lesser = next_rtcp;
            send_rtcp = PJ_TRUE;
        }

        pj_get_timestamp(&now);
        if (lesser.u64 <= now.u64) {
            timeout.sec = timeout.msec = 0;
            //printf("immediate "); fflush(stdout);
        } else {
            pj_uint64_t tick_delay;
            tick_delay = lesser.u64 - now.u64;
            timeout.sec = 0;
            timeout.msec = (pj_uint32_t)(tick_delay * 1000 / freq.u64);
            pj_time_val_normalize(&timeout);

            //printf("%d:%03d ", timeout.sec, timeout.msec); fflush(stdout);
        }

        /* Wait for next interval */
        //if (timeout.sec!=0 && timeout.msec!=0) {
        pj_thread_sleep(PJ_TIME_VAL_MSEC(timeout));
        if (strm->thread_quit_flag)
            break;
        //}

        pj_get_timestamp(&now);

        if (send_rtp || next_rtp.u64 <= now.u64) {
            /*
             * Time to send RTP packet.
             */
            pj_status_t status;
            const void *p_hdr;
            const pjmedia_rtp_hdr *hdr;
            pj_ssize_t size;
            int hdrlen;

            /* Format RTP header */
            status = pjmedia_rtp_encode_rtp( &strm->out_sess, strm->si.tx_pt,
                                             0, /* marker bit */
                                             strm->bytes_per_frame,
                                             strm->samples_per_frame,
                                             &p_hdr, &hdrlen);
            if (status == PJ_SUCCESS) {

                //PJ_LOG(4,(THIS_FILE, "\t\tTx seq=%d", pj_ntohs(hdr->seq)));

                hdr = (const pjmedia_rtp_hdr*) p_hdr;

                /* Copy RTP header to packet */
                pj_memcpy(packet, hdr, hdrlen);

                /* Zero the payload */
                pj_bzero(packet+hdrlen, strm->bytes_per_frame);
                for(int i=hdrlen; i<strm->bytes_per_frame; i++) {
                    packet[i] = pj_rand();
                }

                /* Send RTP packet */
                size = hdrlen + strm->bytes_per_frame;
                status = pjmedia_transport_send_rtp(strm->transport,
                                                    packet, size);
                if (status != PJ_SUCCESS)
                    app_perror(THIS_FILE, "Error sending RTP packet", status);

            } else {
                pj_assert(!"RTP encode() error");
            }

            /* Update RTCP SR */
            pjmedia_rtcp_tx_rtp( &strm->rtcp, (pj_uint16_t)strm->bytes_per_frame);

            /* Schedule next send */
            next_rtp.u64 += (msec_interval * freq.u64 / 1000);
        }


        if (send_rtcp || next_rtcp.u64 <= now.u64) {
            /*
             * Time to send RTCP packet.
             */
            void *rtcp_pkt;
            int rtcp_len;
            pj_ssize_t size;
            pj_status_t status;

            /* Build RTCP packet */
            pjmedia_rtcp_build_rtcp(&strm->rtcp, &rtcp_pkt, &rtcp_len);


            /* Send packet */
            size = rtcp_len;
            status = pjmedia_transport_send_rtcp(strm->transport,
                                                 rtcp_pkt, size);
            if (status != PJ_SUCCESS) {
                app_perror(THIS_FILE, "Error sending RTCP packet", status);
            }

            /* Schedule next send */
            next_rtcp.u64 += (freq.u64 * (RTCP_INTERVAL+(pj_rand()%RTCP_RAND)) /
                              1000);
        }
    }

    return 0;
}


/* Callback to be called when SDP negotiation is done in the call: */
void call_on_media_update( pjsip_inv_session *inv,
                                  pj_status_t status)
{
    call_t *call;
    media_stream_t *audio;
    const pjmedia_sdp_session *local_sdp, *remote_sdp;
    codec_t *codec_desc = NULL;
    unsigned i;

    call = inv->mod_data[mod_siprtp.id];
    audio = &call->media[0];

    /* If this is a mid-call media update, then destroy existing media */
    if (audio->thread != NULL)
        destroy_call_media(call->index);


    /* Do nothing if media negotiation has failed */
    if (status != PJ_SUCCESS) {
        app_perror(THIS_FILE, "SDP negotiation failed", status);
        return;
    }


    /* Capture stream definition from the SDP */
    pjmedia_sdp_neg_get_active_local(inv->neg, &local_sdp);
    pjmedia_sdp_neg_get_active_remote(inv->neg, &remote_sdp);

    status = pjmedia_stream_info_from_sdp(&audio->si, inv->pool, app.med_endpt,
                                          local_sdp, remote_sdp, 0);
    if (status != PJ_SUCCESS) {
        app_perror(THIS_FILE, "Error creating stream info from SDP", status);
        return;
    }

    /* Get the remainder of codec information from codec descriptor */
    if (audio->si.fmt.pt == app.audio_codec.pt)
        codec_desc = &app.audio_codec;
    else {
        /* Find the codec description in codec array */
        for (i=0; i<PJ_ARRAY_SIZE(audio_codecs); ++i) {
            if (audio_codecs[i].pt == audio->si.fmt.pt) {
                codec_desc = &audio_codecs[i];
                break;
            }
        }

        if (codec_desc == NULL) {
            PJ_LOG(3, (THIS_FILE, "Error: Invalid codec payload type"));
            return;
        }
    }

    audio->clock_rate = audio->si.fmt.clock_rate;
    audio->samples_per_frame = audio->clock_rate * codec_desc->ptime / 1000;
    audio->bytes_per_frame = codec_desc->bit_rate * codec_desc->ptime / 1000 / 8;


    pjmedia_rtp_session_init(&audio->out_sess, audio->si.tx_pt,
                             pj_rand());
    pjmedia_rtp_session_init(&audio->in_sess, audio->si.fmt.pt, 0);
    pjmedia_rtcp_init(&audio->rtcp, "rtcp", audio->clock_rate,
                      audio->samples_per_frame, 0);


    /* Attach media to transport */
    status = pjmedia_transport_attach(audio->transport, audio,
                                      &audio->si.rem_addr,
                                      &audio->si.rem_rtcp,
                                      sizeof(pj_sockaddr_in),
                                      &on_rx_rtp,
                                      &on_rx_rtcp);
    if (status != PJ_SUCCESS) {
        app_perror(THIS_FILE, "Error on pjmedia_transport_attach()", status);
        return;
    }

    /* Start media thread. */
    audio->thread_quit_flag = 0;
#if PJ_HAS_THREADS
    status = pj_thread_create( inv->pool, "media", &media_thread, audio,
                               0, 0, &audio->thread);
    if (status != PJ_SUCCESS) {
        app_perror(THIS_FILE, "Error creating media thread", status);
        return;
    }
#endif

    /* Set the media as active */
    audio->active = PJ_TRUE;
}
