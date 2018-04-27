//
// Created by noxue on 4/27/18.
//

#include "init.h"
#include "utils.h"
#include "callback.h"
#include "rtp.h"

static FILE *log_file;

static void app_log_writer(int level, const char *buffer, int len){
    if (level <= app.app_log_level) {
        pj_log_write(level,buffer,len);
    }

    if (log_file) {
        pj_size_t count = fwrite(buffer, len, 1, log_file);
        PJ_UNUSED_ARG(count);
        fflush(log_file);
    }
}

pj_status_t app_logging_init(void) {

    pj_log_set_log_func(&app_log_writer);

    if (app.log_filename) {
        log_file = fopen(app.log_filename, "wt");
        if (log_file == NULL) {
            PJ_LOG(1,(__FILE__,"创建日志记录文件失败"));
            return -1;
        }
    }

    return PJ_SUCCESS;
}

void app_logging_shutdown(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}


pj_status_t init_sip() {

    pj_status_t  status;

    // init pjlib-util
    status = pjlib_util_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    // must create a pool factory before we allocate any memory
    pj_caching_pool_init(&app.cp, &pj_pool_factory_default_policy, 0);

    // create a pool for misc
    app.pool = pj_pool_create(&app.cp.factory, "app", 1000, 1000,NULL);

    // create the endpoint
    status = pjsip_endpt_create(&app.cp.factory, pj_gethostname()->ptr, &app.sip_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    // add udp transport
    {
        pj_sockaddr_in      addr;
        pjsip_host_port     addrname;
        pjsip_transport     *tp;

        pj_bzero(&addr, sizeof(addr));
        addr.sin_family = pj_AF_INET();
        addr.sin_addr.s_addr = 0;
        addr.sin_port = pj_htons((pj_uint16_t)app.sip_port);

        if (app.local_addr.slen) {
            addrname.host = app.local_addr;
            addrname.port = app.sip_port;

            status = pj_sockaddr_in_init(&addr, &app.local_addr,
                                         (pj_uint16_t)app.sip_port);
            if (status != PJ_SUCCESS) {
                app_perror(__FILE__,"无法解析ip地址信息",status);
                return status;
            }
        }

        status = pjsip_udp_transport_start(app.sip_endpt, &addr,
                                           (app.local_addr.slen ? &addrname:NULL),
                                           1, &tp);
        if (status != PJ_SUCCESS) {
            app_perror(__FILE__,"开始udp传输失败",status);
            return status;
        }

        PJ_LOG(3,(__FILE__, "SIP UDP listening on %.*s:%d",
                (int)tp->local_name.host.slen, tp->local_name.host.ptr,
                tp->local_name.port));
    }

    /**
     * init transaction layer
     * this will create/initialize transaction hash tables etc
     */
    status = pjsip_tsx_layer_init_module(app.sip_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    // initialize ua layer
    status = pjsip_ua_init_module(app.sip_endpt, NULL);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    // initialize 100rel support
    status = pjsip_100rel_init_module(app.sip_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /*  Init invite session module. */
    {
        pjsip_inv_callback inv_cb;

        /* Init the callback for INVITE session: */
        pj_bzero(&inv_cb, sizeof(inv_cb));
        inv_cb.on_state_changed = &call_on_state_changed;
        inv_cb.on_new_session = &call_on_forked;
        inv_cb.on_media_update = &call_on_media_update;


        /* Initialize invite session module:  */
        status = pjsip_inv_usage_init(app.sip_endpt, &inv_cb);
        PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);
    }



    // register our module to receive incomming requests.
    status = pjsip_endpt_register_module( app.sip_endpt, &mod_siprtp);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);


    /* Init calls */
    for (uint i=0; i<app.max_calls; ++i){
        app.call[i].index = i;

        char filename[100];
        snprintf(filename,100,"call-%d.pcm",i);
        if( (app.call[i].media[0].recv_fd = fopen(filename,"wb"))==NULL ) {
            printf("创建音频文件失败:%s\n",filename);
            exit(1);
        }
    }



    // Done
    return PJ_SUCCESS;
}





/*
 * Create SDP session for a call.
 */
pj_status_t create_sdp( pj_pool_t *pool,
                        call_t *call,
                        pjmedia_sdp_session **p_sdp)
{
    pj_time_val tv;
    pjmedia_sdp_session *sdp;
    pjmedia_sdp_media *m;
    pjmedia_sdp_attr *attr;
    pjmedia_transport_info tpinfo;
    media_stream_t *audio = &call->media[0];

    PJ_ASSERT_RETURN(pool && p_sdp, PJ_EINVAL);


    /* Get transport info */
    pjmedia_transport_info_init(&tpinfo);
    pjmedia_transport_get_info(audio->transport, &tpinfo);

    /* Create and initialize basic SDP session */
    sdp = pj_pool_zalloc (pool, sizeof(pjmedia_sdp_session));

    pj_gettimeofday(&tv);
    sdp->origin.user = pj_str("noxue");
    sdp->origin.version = sdp->origin.id = tv.sec + 2208988800UL;
    sdp->origin.net_type = pj_str("IN");
    sdp->origin.addr_type = pj_str("IP4");
    sdp->origin.addr = *pj_gethostname();
    sdp->name = pj_str("noxue");

    /* Since we only support one media stream at present, put the
     * SDP connection line in the session level.
     */
    sdp->conn = pj_pool_zalloc (pool, sizeof(pjmedia_sdp_conn));
    sdp->conn->net_type = pj_str("IN");
    sdp->conn->addr_type = pj_str("IP4");
    sdp->conn->addr = app.local_addr;


    /* SDP time and attributes. */
    sdp->time.start = sdp->time.stop = 0;
    sdp->attr_count = 0;

    /* Create media stream 0: */

    sdp->media_count = 1;
    m = pj_pool_zalloc (pool, sizeof(pjmedia_sdp_media));
    sdp->media[0] = m;

    /* Standard media info: */
    m->desc.media = pj_str("audio");
    m->desc.port = pj_ntohs(tpinfo.sock_info.rtp_addr_name.ipv4.sin_port);
    m->desc.port_count = 1;
    m->desc.transport = pj_str("RTP/AVP");

    /* Add format and rtpmap for each codec. */
    m->desc.fmt_count = 1;
    m->attr_count = 0;

    {
        pjmedia_sdp_rtpmap rtpmap;
        char ptstr[10];

        sprintf(ptstr, "%d", app.audio_codec.pt);
        pj_strdup2(pool, &m->desc.fmt[0], ptstr);
        rtpmap.pt = m->desc.fmt[0];
        rtpmap.clock_rate = app.audio_codec.clock_rate;
        rtpmap.enc_name = pj_str(app.audio_codec.name);
        rtpmap.param.slen = 0;

        pjmedia_sdp_rtpmap_to_attr(pool, &rtpmap, &attr);
        m->attr[m->attr_count++] = attr;
    }

    /* Add sendrecv attribute. */
    attr = pj_pool_zalloc(pool, sizeof(pjmedia_sdp_attr));
    attr->name = pj_str("sendrecv");
    m->attr[m->attr_count++] = attr;

#if 1
    /*
     * Add support telephony event
     */
    m->desc.fmt[m->desc.fmt_count++] = pj_str("121");
    /* Add rtpmap. */
    attr = pj_pool_zalloc(pool, sizeof(pjmedia_sdp_attr));
    attr->name = pj_str("rtpmap");
    attr->value = pj_str("121 telephone-event/8000");
    m->attr[m->attr_count++] = attr;
    /* Add fmtp */
    attr = pj_pool_zalloc(pool, sizeof(pjmedia_sdp_attr));
    attr->name = pj_str("fmtp");
    attr->value = pj_str("121 0-15");
    m->attr[m->attr_count++] = attr;
#endif

    /* Done */
    *p_sdp = sdp;

    return PJ_SUCCESS;
}



void init_opts() {
    static char ip_addr[PJ_INET_ADDRSTRLEN];
    static char local_uri[64];


    /* Get local IP address for the default IP address */
    {
        const pj_str_t *hostname;
        pj_sockaddr_in tmp_addr;

        hostname = pj_gethostname();
        pj_sockaddr_in_init(&tmp_addr, hostname, 0);
        pj_inet_ntop(pj_AF_INET(), &tmp_addr.sin_addr, ip_addr,
                     sizeof(ip_addr));
    }

    /* Init defaults */
    app.username="110";
    app.max_calls = 2;
    app.thread_count = 1;
    app.sip_port = 5080;
    app.rtp_start_port = RTP_START_PORT;
    app.local_addr = pj_str("192.168.4.111");
    app.log_level = 3;
    app.app_log_level = 4;
    app.log_filename = LOG_FILE;

    /* Default codecs: */
    app.audio_codec = audio_codecs[0];


    /* Build local URI and contact */
    pj_ansi_sprintf( local_uri, "sip:%s@%s:%d", app.username,app.local_addr.ptr, app.sip_port);
    app.local_uri = pj_str(local_uri);
    app.local_contact = app.local_uri;
}