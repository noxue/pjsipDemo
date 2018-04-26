
#include <pjsua-lib/pjsua.h>

#define SIP_DOMAIN "192.168.4.101"
#define SIP_USER "110"
#define SIP_PASSWD "secret"
#define THIS_FILE "app"

//来电回调函数
/* Callback called by the library upon receiving incoming call */
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
                             pjsip_rx_data *rdata)
{
    pjsua_call_info ci;

    PJ_UNUSED_ARG(acc_id);
    PJ_UNUSED_ARG(rdata);

    //    获得呼叫信息
    pjsua_call_get_info(call_id, &ci);

    PJ_LOG(3,(THIS_FILE, "Incoming call from %.*s!!",
            (int)ci.remote_info.slen,
            ci.remote_info.ptr));

    //    自动应答呼叫
    /* Automatically answer incoming calls with 200/OK */
    pjsua_call_answer(call_id, 200, NULL, NULL);
}


//呼叫状态改变的回调函数
/* Callback called by the library when call's state has changed */
static void on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
    pjsua_call_info ci;

    PJ_UNUSED_ARG(e);

    pjsua_call_get_info(call_id, &ci);
    PJ_LOG(3,(THIS_FILE, "Call %d state=%.*s", call_id,
            (int)ci.state_text.slen,
            ci.state_text.ptr));
}


//媒体状态改变的回调函数
/* Callback called by the library when call's media state has changed */
static void on_call_media_state(pjsua_call_id call_id)
{
    pjsua_call_info ci;

    pjsua_call_get_info(call_id, &ci);

//    当媒体为激活时，连接呼叫和声音设备
    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
        // When media is active, connect call to sound device.
        pjsua_conf_connect(ci.conf_slot, 0);
        pjsua_conf_connect(0, ci.conf_slot);
    }
}

/*
 * main()
 *
 * argv[1] may contain URL to call.
 */
int main(int argc, char *argv[])
{
    pjsua_acc_id acc_id;
    pj_status_t status;

//  创建PJSIP
    /* Create pjsua first! */
    status = pjsua_create();
    if (status != PJ_SUCCESS) return status;


//    初始化PJSUA，设置回调函数
    /* Init pjsua */
    {
        pjsua_config cfg;
        pjsua_logging_config log_cfg;

        pjsua_config_default(&cfg);
        cfg.cb.on_incoming_call = &on_incoming_call;
        cfg.cb.on_call_media_state = &on_call_media_state;
        cfg.cb.on_call_state = &on_call_state;

        pjsua_logging_config_default(&log_cfg);
        log_cfg.console_level = 4;

        status = pjsua_init(&cfg, &log_cfg, NULL);
        if (status != PJ_SUCCESS) return status;
    }


//    创建PJSIP的传输端口
    /* Add UDP transport. */
    {
        pjsua_transport_config cfg;

        pjsua_transport_config_default(&cfg);
        cfg.port = 5060;
        status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, NULL);
        if (status != PJ_SUCCESS) return status;
    }

//    启动PJSIP
    /* Initialization is done, now start pjsua */
    status = pjsua_start();
    if (status != PJ_SUCCESS) return status;

//    设置SIP用户帐号
    /* Register to SIP server by creating SIP account. */
    {
        pjsua_acc_config cfg;

        pjsua_acc_config_default(&cfg);
        cfg.id = pj_str("sip:" SIP_USER "@" SIP_DOMAIN);
//        cfg.reg_uri = pj_str("sip:" SIP_DOMAIN);
        cfg.cred_count = 1;
//        cfg.cred_info[0].realm = pj_str(SIP_DOMAIN);
//        cfg.cred_info[0].scheme = pj_str("digest");
//        cfg.cred_info[0].username = pj_str(SIP_USER);
//        cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
//        cfg.cred_info[0].data = pj_str(SIP_PASSWD);


        status = pjsua_acc_add(&cfg, PJ_TRUE, &acc_id);
        if (status != PJ_SUCCESS) return status;
    }

    pjsua_set_no_snd_dev();

    pj_str_t uri = pj_str("sip:13758277505@192.168.4.101");
    status = pjsua_call_make_call(acc_id, &uri, 0, NULL, NULL, NULL);
    if (status != PJ_SUCCESS) return status;

//    循环等待
    /* Wait until user press "q" to quit. */
    for (;;) {
        char option[10];

        puts("Press 'h' to hangup all calls, 'q' to quit");
        if (fgets(option, sizeof(option), stdin) == NULL) {
            puts("EOF while reading stdin, will quit now..");
            break;
        }

        if (option[0] == 'q')
            break;

        if (option[0] == 'h')
            pjsua_call_hangup_all();
    }

    /* Destroy pjsua */
    pjsua_destroy();

    return 0;
}