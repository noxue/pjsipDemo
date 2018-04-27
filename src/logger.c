//
// Created by noxue on 4/27/18.
//

#include "logger.h"

#define THIS_FILE __FILE__


/* Notification on incoming messages */
static pj_bool_t logger_on_rx_msg(pjsip_rx_data *rdata)
{
    PJ_LOG(4,(THIS_FILE, "RX %d bytes %s from %s:%d:\n"
                         "%s\n"
                         "--end msg--",
            rdata->msg_info.len,
            pjsip_rx_data_get_info(rdata),
            rdata->pkt_info.src_name,
            rdata->pkt_info.src_port,
            rdata->msg_info.msg_buf));

    /* Always return false, otherwise messages will not get processed! */
    return PJ_FALSE;
}

/* Notification on outgoing messages */
static pj_status_t logger_on_tx_msg(pjsip_tx_data *tdata)
{

    /* Important note:
     *	tp_info field is only valid after outgoing messages has passed
     *	transport layer. So don't try to access tp_info when the module
     *	has lower priority than transport layer.
     */

    PJ_LOG(4,(THIS_FILE, "TX %d bytes %s to %s:%d:\n"
                         "%s\n"
                         "--end msg--",
            (tdata->buf.cur - tdata->buf.start),
            pjsip_tx_data_get_info(tdata),
            tdata->tp_info.dst_name,
            tdata->tp_info.dst_port,
            tdata->buf.start));

    /* Always return success, otherwise message will not get sent! */
    return PJ_SUCCESS;
}

/* The module instance. */
pjsip_module msg_logger = {
        NULL, NULL,				/* prev, next.		*/
        { "mod-siprtp-log", 14 },		/* Name.		*/
        -1,					/* Id			*/
        PJSIP_MOD_PRIORITY_TRANSPORT_LAYER-1,/* Priority	        */
        NULL,				/* load()		*/
        NULL,				/* start()		*/
        NULL,				/* stop()		*/
        NULL,				/* unload()		*/
        &logger_on_rx_msg,			/* on_rx_request()	*/
        &logger_on_rx_msg,			/* on_rx_response()	*/
        &logger_on_tx_msg,			/* on_tx_request.	*/
        &logger_on_tx_msg,			/* on_tx_response()	*/
        NULL,				/* on_tsx_state()	*/

};