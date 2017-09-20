//#include "mbed.h"
#include <stdio.h>
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "analog.h"
#include "bt_drv.h"
#include "app_audio.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"

extern "C" {
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "a2dp.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "hf.h"
#include "btalloc.h"
#include "hid.h"
}


#include "rtos.h"
#include "besbt.h"

#include "cqueue.h"
#include "btapp.h"
#include "apps.h"
#include "resources.h"
#include "app_bt_media_manager.h"
#include "tgt_hardware.h"


static int a2dp_volume_get(void);

static int a2dp_volume_set(U8 vol);


AvdtpCodec a2dp_avdtpcodec;
//A2dpStream a2dp_stream;
/* avrcp */
//AvrcpChannel avrcp_channel;
#if AVRCP_ADVANCED_CONTROLLER == XA_ENABLED
typedef struct {
    AvrcpAdvancedPdu pdu;
    uint8_t para_buf[40];
}APP_A2DP_AVRCPADVANCEDPDU;
osPoolId   app_a2dp_avrcpadvancedpdu_mempool = NULL;

osPoolDef (app_a2dp_avrcpadvancedpdu_mempool, 10, APP_A2DP_AVRCPADVANCEDPDU);

#define app_a2dp_avrcpadvancedpdu_mempool_init() do{ \
                                                    if (app_a2dp_avrcpadvancedpdu_mempool == NULL) \
                                                        app_a2dp_avrcpadvancedpdu_mempool = osPoolCreate(osPool(app_a2dp_avrcpadvancedpdu_mempool)); \
                                                  }while(0);

#define app_a2dp_avrcpadvancedpdu_mempool_calloc(buf)  do{ \
                                                        APP_A2DP_AVRCPADVANCEDPDU * avrcpadvancedpdu; \
                                                        avrcpadvancedpdu = (APP_A2DP_AVRCPADVANCEDPDU *)osPoolCAlloc(app_a2dp_avrcpadvancedpdu_mempool); \
                                                        buf = &(avrcpadvancedpdu->pdu); \
                                                        buf->parms = avrcpadvancedpdu->para_buf; \
                                                     }while(0);

#define app_a2dp_avrcpadvancedpdu_mempool_free(buf)  do{ \
                                                        osPoolFree(app_a2dp_avrcpadvancedpdu_mempool, buf); \
                                                     }while(0);
#endif

void get_value1_pos(U8 mask,U8 *start_pos, U8 *end_pos)
{
    U8 num = 0;
    
    for(U8 i=0;i<8;i++){
        if((0x01<<i) & mask){
            *start_pos = i;//start_pos,end_pos stands for the start and end position of value 1 in mask
            break;
        }
    }
    for(U8 i=0;i<8;i++){
        if((0x01<<i) & mask)
            num++;//number of value1 in mask
    }
    *end_pos = *start_pos + num - 1;
}
U8 get_valid_bit(U8 elements, U8 mask)
{
    U8 start_pos,end_pos;
    
    get_value1_pos(mask,&start_pos,&end_pos);
//    TRACE("!!!start_pos:%d,end_pos:%d\n",start_pos,end_pos);
    for(U8 i = start_pos; i <= end_pos; i++){
        if((0x01<<i) & elements){
            elements = ((0x01<<i) | (elements & (~mask)));
            break;
        }
    }
    return elements;
}


struct BT_DEVICE_T  app_bt_device;

#if AVRCP_ADVANCED_CONTROLLER == XA_ENABLED
void a2dp_init(void)
{
    for(uint8_t i=0; i<BT_DEVICE_NUM; i++)
    {
        app_bt_device.a2dp_state[i]=0;
        app_bt_device.a2dp_streamming[i] = 0;
        app_bt_device.avrcp_get_capabilities_rsp[i] = NULL;
        app_bt_device.avrcp_control_rsp[i] = NULL;
        app_bt_device.avrcp_notify_rsp[i] = NULL;
        app_bt_device.avrcp_cmd1[i] = NULL;
        app_bt_device.avrcp_cmd2[i] = NULL;
        
    }

    app_a2dp_avrcpadvancedpdu_mempool_init();

    app_bt_device.a2dp_state[BT_DEVICE_ID_1]=0;
    app_bt_device.a2dp_play_pause_flag = 0;
    app_bt_device.curr_a2dp_stream_id= BT_DEVICE_ID_1;
}

void avrcp_callback(AvrcpChannel *chnl, const AvrcpCallbackParms *Parms)
{
    TRACE("avrcp_callback : chnl %p, Parms %p\n", chnl, Parms);
    TRACE("::Parms->event %d\n", Parms->event);
#ifdef __BT_ONE_BRING_TWO__
    enum BT_DEVICE_ID_T device_id = (chnl == &app_bt_device.avrcp_channel[0])?BT_DEVICE_ID_1:BT_DEVICE_ID_2;
#else
    enum BT_DEVICE_ID_T device_id = BT_DEVICE_ID_1;
#endif
    switch(Parms->event)
    {
        case AVRCP_EVENT_CONNECT_IND:
            TRACE("::AVRCP_EVENT_CONNECT_IND %d\n", Parms->event);
            AVRCP_ConnectRsp(chnl, 1);
            break;
        case AVRCP_EVENT_CONNECT:
            if(0)//(chnl->avrcpVersion >=0x103)
            {
                TRACE("::AVRCP_GET_CAPABILITY\n");
                if (app_bt_device.avrcp_cmd1[device_id] == NULL)
                    app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_cmd1[device_id]);
                AVRCP_CtGetCapabilities(chnl,app_bt_device.avrcp_cmd1[device_id],AVRCP_CAPABILITY_EVENTS_SUPPORTED);
            }            
            TRACE("::AVRCP_EVENT_CONNECT %x\n", chnl->avrcpVersion);
            break;
        case AVRCP_EVENT_DISCONNECT:
            TRACE("::AVRCP_EVENT_DISCONNECT");
            if (app_bt_device.avrcp_get_capabilities_rsp[device_id]){
                app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_get_capabilities_rsp[device_id]);
                app_bt_device.avrcp_get_capabilities_rsp[device_id] = NULL;
            }
            if (app_bt_device.avrcp_control_rsp[device_id]){
                app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_control_rsp[device_id]);
                app_bt_device.avrcp_control_rsp[device_id] = NULL;
            }
            if (app_bt_device.avrcp_notify_rsp[device_id]){
                app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_notify_rsp[device_id]);
                app_bt_device.avrcp_notify_rsp[device_id] = NULL;
            }
     
            if (app_bt_device.avrcp_cmd1[device_id]){
                app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_cmd1[device_id]);
                app_bt_device.avrcp_cmd1[device_id] = NULL;
            }       
            if (app_bt_device.avrcp_cmd2[device_id]){
                app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_cmd2[device_id]);
                app_bt_device.avrcp_cmd2[device_id] = NULL;
            }                   
            app_bt_device.volume_report[device_id] = 0;
            break;       
        case AVRCP_EVENT_RESPONSE:
            TRACE("::AVRCP_EVENT_ADV_RESPONSE op=%x,status=%x\n", Parms->advOp,Parms->status);
        
            break;
        case AVRCP_EVENT_PANEL_CNF:
            TRACE("::AVRCP_EVENT_PANEL_CNF %x,%x,%x",
                Parms->p.panelCnf.response,Parms->p.panelCnf.operation,Parms->p.panelCnf.press);
            break;
        case AVRCP_EVENT_ADV_TX_DONE:
            TRACE("::AVRCP_EVENT_ADV_TX_DONE op:%d\n", Parms->p.adv.txPdu->op);
            if (Parms->p.adv.txPdu->op == AVRCP_OP_GET_CAPABILITIES){
                if (app_bt_device.avrcp_get_capabilities_rsp[device_id] == Parms->p.adv.txPdu){
                    app_bt_device.avrcp_get_capabilities_rsp[device_id] = NULL;
                    app_a2dp_avrcpadvancedpdu_mempool_free(Parms->p.adv.txPdu);
                }
            }
#if 0
            if (Parms->p.adv.txPdu->op == AVRCP_OP_SET_ABSOLUTE_VOLUME){
                if (Parms->p.adv.txPdu->ctype != AVCTP_RESPONSE_INTERIM){
                    if (app_bt_device.avrcp_control_rsp[device_id] == Parms->p.adv.txPdu){
                        app_bt_device.avrcp_control_rsp[device_id] = NULL;
                        app_a2dp_avrcpadvancedpdu_mempool_free(Parms->p.adv.txPdu);
                    }
                }
            }
            if (Parms->p.adv.txPdu->op == AVRCP_OP_REGISTER_NOTIFY){
                if (Parms->p.adv.txPdu->ctype != AVCTP_RESPONSE_INTERIM){
                    if (Parms->p.adv.txPdu->parms[0] == AVRCP_EID_VOLUME_CHANGED){
                        app_bt_device.avrcp_notify_rsp[device_id] = NULL;
                        app_a2dp_avrcpadvancedpdu_mempool_free(Parms->p.adv.txPdu);
                    }
                }
            }
#endif

            break;
        case AVRCP_EVENT_ADV_RESPONSE:
            TRACE("::AVRCP_EVENT_ADV_RESPONSE op=%x,status=%x\n", Parms->advOp,Parms->status);
            if(Parms->advOp == AVRCP_OP_GET_CAPABILITIES && Parms->status == BT_STATUS_SUCCESS)
            {
                TRACE("::AVRCP eventmask=%x\n", Parms->p.adv.rsp.capability.info.eventMask);
                chnl->adv.rem_eventMask = Parms->p.adv.rsp.capability.info.eventMask;
                if(chnl->adv.rem_eventMask & AVRCP_ENABLE_PLAY_STATUS_CHANGED)
                {
                    TRACE("::AVRCP send notification\n");
                    if (app_bt_device.avrcp_cmd1[device_id] == NULL)
                        app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_cmd1[device_id]);
                    AVRCP_CtRegisterNotification(chnl,app_bt_device.avrcp_cmd1[device_id],AVRCP_EID_MEDIA_STATUS_CHANGED,0);

                }
                if(chnl->adv.rem_eventMask & AVRCP_ENABLE_PLAY_POS_CHANGED)
                {
                    if (app_bt_device.avrcp_cmd2[device_id] == NULL)
                        app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_cmd2[device_id]);
                
                    AVRCP_CtRegisterNotification(chnl,app_bt_device.avrcp_cmd2[device_id],AVRCP_EID_PLAY_POS_CHANGED,1);
                    
                }
            }
            else if(Parms->advOp == AVRCP_OP_REGISTER_NOTIFY && Parms->status == BT_STATUS_SUCCESS)
            {
                   if(Parms->p.adv.notify.event == AVRCP_EID_MEDIA_STATUS_CHANGED) 
                   {
                        TRACE("::ACRCP notify rsp playback states=%x",Parms->p.adv.notify.p.mediaStatus);
                   // app_bt_device.a2dp_state = Parms->p.adv.notify.p.mediaStatus;
                   }
                   else if(Parms->p.adv.notify.event == AVRCP_EID_PLAY_POS_CHANGED)
                  {
                        TRACE("::ACRCP notify rsp play pos =%x",Parms->p.adv.notify.p.position);
                  }
                   else if(Parms->p.adv.notify.event == AVRCP_EID_VOLUME_CHANGED){
                        TRACE("::ACRCP notify rsp volume =%x",Parms->p.adv.notify.p.volume);
                        a2dp_volume_set(Parms->p.adv.notify.p.volume);
                   }
            }                
            break;
        case AVRCP_EVENT_COMMAND:
#ifndef __AVRCP_EVENT_COMMAND_VOLUME_SKIP__
            TRACE("::AVRCP_EVENT_COMMAND ctype=%x,subunitype=%x\n", Parms->p.cmdFrame->ctype,Parms->p.cmdFrame->subunitType);
            TRACE("::AVRCP_EVENT_COMMAND subunitId=%x,opcode=%x\n", Parms->p.cmdFrame->subunitId,Parms->p.cmdFrame->opcode);
            TRACE("::AVRCP_EVENT_COMMAND operands=%x,operandLen=%x\n", Parms->p.cmdFrame->operands,Parms->p.cmdFrame->operandLen);
            TRACE("::AVRCP_EVENT_COMMAND more=%x\n", Parms->p.cmdFrame->more);
            if(Parms->p.cmdFrame->ctype == AVRCP_CTYPE_STATUS)
            {
                uint32_t company_id = *(Parms->p.cmdFrame->operands+2) + ((uint32_t)(*(Parms->p.cmdFrame->operands+1))<<8) + ((uint32_t)(*(Parms->p.cmdFrame->operands))<<16);
                TRACE("::AVRCP_EVENT_COMMAND company_id=%x\n", company_id);
                if(company_id == 0x001958)  //bt sig
                {
                    AvrcpOperation op = *(Parms->p.cmdFrame->operands+3);
                    uint8_t oplen =  *(Parms->p.cmdFrame->operands+6)+ ((uint32_t)(*(Parms->p.cmdFrame->operands+5))<<8);
                    TRACE("::AVRCP_EVENT_COMMAND op=%x,oplen=%x\n", op,oplen);
                    switch(op)
                    {
                        case AVRCP_OP_GET_CAPABILITIES:
                        {
                                uint8_t event = *(Parms->p.cmdFrame->operands+7);
                                if(event==AVRCP_CAPABILITY_COMPANY_ID)
                                {
                                    TRACE("::AVRCP_EVENT_COMMAND send support compay id");
                                }
                                else if(event == AVRCP_CAPABILITY_EVENTS_SUPPORTED)
                                {
                                    TRACE("::AVRCP_EVENT_COMMAND send support event transId:%d", Parms->p.cmdFrame->transId);
                                    chnl->adv.eventMask = AVRCP_ENABLE_VOLUME_CHANGED;   ///volume control
                                    if (app_bt_device.avrcp_get_capabilities_rsp[device_id] == NULL)
                                        app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_get_capabilities_rsp[device_id]);
                                    app_bt_device.avrcp_get_capabilities_rsp[device_id]->transId = Parms->p.cmdFrame->transId;
                                    app_bt_device.avrcp_get_capabilities_rsp[device_id]->ctype = AVCTP_RESPONSE_IMPLEMENTED_STABLE;
                                    TRACE("::AVRCP_EVENT_COMMAND send support event transId:%d", app_bt_device.avrcp_get_capabilities_rsp[device_id]->transId);
                                    AVRCP_CtGetCapabilities_Rsp(chnl,app_bt_device.avrcp_get_capabilities_rsp[device_id],AVRCP_CAPABILITY_EVENTS_SUPPORTED,chnl->adv.eventMask);
                                }
                                else
                                {
                                    TRACE("::AVRCP_EVENT_COMMAND send error event value");
                                }
                        }
                        break;
                    }
                    
                }
                
            }else if(Parms->p.cmdFrame->ctype == AVCTP_CTYPE_CONTROL){        
                TRACE("::AVRCP_EVENT_COMMAND AVCTP_CTYPE_CONTROL\n");
                DUMP8("%02x ", Parms->p.cmdFrame->operands, Parms->p.cmdFrame->operandLen);
                if (Parms->p.cmdFrame->operands[3] == AVRCP_OP_SET_ABSOLUTE_VOLUME){
                    TRACE("::AVRCP_EID_VOLUME_CHANGED transId:%d\n", Parms->p.cmdFrame->transId);
                    a2dp_volume_set(Parms->p.cmdFrame->operands[7]);                    
                    if (app_bt_device.avrcp_control_rsp[device_id] == NULL)
                        app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_control_rsp[device_id]);
                    app_bt_device.avrcp_control_rsp[device_id]->transId = Parms->p.cmdFrame->transId;
                    app_bt_device.avrcp_control_rsp[device_id]->ctype = AVCTP_RESPONSE_ACCEPTED;
                    DUMP8("%02x ", Parms->p.cmdFrame->operands, Parms->p.cmdFrame->operandLen);
                    AVRCP_CtAcceptAbsoluteVolume_Rsp(chnl, app_bt_device.avrcp_control_rsp[device_id], Parms->p.cmdFrame->operands[7]);
                }
            }else if (Parms->p.cmdFrame->ctype == AVCTP_CTYPE_NOTIFY){
                BtStatus status;
                TRACE("::AVRCP_EVENT_COMMAND AVCTP_CTYPE_NOTIFY\n");
                DUMP8("%02x ", Parms->p.cmdFrame->operands, Parms->p.cmdFrame->operandLen);
                if (Parms->p.cmdFrame->operands[7] == AVRCP_EID_VOLUME_CHANGED){
                    TRACE("::AVRCP_EID_VOLUME_CHANGED transId:%d\n", Parms->p.cmdFrame->transId);
                    if (app_bt_device.avrcp_notify_rsp[device_id] == NULL)
                        app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_notify_rsp[device_id]);
                    app_bt_device.avrcp_notify_rsp[device_id]->transId = Parms->p.cmdFrame->transId;
                    app_bt_device.avrcp_notify_rsp[device_id]->ctype = AVCTP_RESPONSE_INTERIM;
                    app_bt_device.volume_report[device_id] = AVCTP_RESPONSE_INTERIM;
                    status = AVRCP_CtGetAbsoluteVolume_Rsp(chnl, app_bt_device.avrcp_notify_rsp[device_id], a2dp_volume_get());
                    TRACE("::AVRCP_EVENT_COMMAND AVRCP_EID_VOLUME_CHANGED nRet:%x\n",status);
                }
            }
#endif             
            break;
        case AVRCP_EVENT_ADV_NOTIFY:
            if(Parms->p.adv.notify.event == AVRCP_EID_VOLUME_CHANGED)
            {
                    TRACE("::ACRCP notify  vol =%x",Parms->p.adv.notify.p.volume);
                    AVRCP_CtRegisterNotification(chnl,app_bt_device.avrcp_notify_rsp[device_id],AVRCP_EID_VOLUME_CHANGED,0);
            }
           else if(Parms->p.adv.notify.event == AVRCP_EID_MEDIA_STATUS_CHANGED) 
           {
                TRACE("::ACRCP notify  playback states=%x",Parms->p.adv.notify.p.mediaStatus);
                if (app_bt_device.avrcp_cmd1[device_id] == NULL)
                    app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_cmd1[device_id]);
                AVRCP_CtRegisterNotification(chnl,app_bt_device.avrcp_cmd1[device_id],AVRCP_EID_MEDIA_STATUS_CHANGED,0);
           // app_bt_device.a2dp_state = Parms->p.adv.notify.p.mediaStatus;
           }
           else if(Parms->p.adv.notify.event == AVRCP_EID_PLAY_POS_CHANGED)
          {
                TRACE("::ACRCP notify  play pos =%x",Parms->p.adv.notify.p.position);
                if (app_bt_device.avrcp_cmd2[device_id] == NULL)
                    app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_cmd2[device_id]);
                AVRCP_CtRegisterNotification(chnl,app_bt_device.avrcp_cmd2[device_id],AVRCP_EID_PLAY_POS_CHANGED,1);
          }  

            
            break;
    }
}
#else
void a2dp_init(void)
{
    for(uint8_t i=0; i<BT_DEVICE_NUM; i++)
    {
        app_bt_device.a2dp_state[i]=0;
    }

    app_bt_device.a2dp_state[BT_DEVICE_ID_1]=0;
    app_bt_device.a2dp_play_pause_flag = 0;
    app_bt_device.curr_a2dp_stream_id= BT_DEVICE_ID_1;
}

void avrcp_callback(AvrcpChannel *chnl, const AvrcpCallbackParms *Parms)
{
    TRACE("avrcp_callback : chnl %p, Parms %p\n", chnl, Parms);
    TRACE("::Parms->event %d\n", Parms->event);
    switch(Parms->event)
    {
        case AVRCP_EVENT_CONNECT_IND:
            TRACE("::AVRCP_EVENT_CONNECT_IND %d\n", Parms->event);
            AVRCP_ConnectRsp(chnl, 1);
            break;
        case AVRCP_EVENT_CONNECT:
            TRACE("::AVRCP_EVENT_CONNECT %d\n", Parms->event);
            break;
        case AVRCP_EVENT_RESPONSE:
            TRACE("::AVRCP_EVENT_RESPONSE %d\n", Parms->event);
            
            break;
        case AVRCP_EVENT_PANEL_CNF:
            TRACE("::AVRCP_EVENT_PANEL_CNF %x,%x,%x",
                Parms->p.panelCnf.response,Parms->p.panelCnf.operation,Parms->p.panelCnf.press);
#if 0            
            if((Parms->p.panelCnf.response == AVCTP_RESPONSE_ACCEPTED) && (Parms->p.panelCnf.press == TRUE))
            {
                AVRCP_SetPanelKey(chnl,Parms->p.panelCnf.operation,FALSE);
            }
#endif            
            break;
    }
}
#endif
//void avrcp_init(void)
//{
//  hal_uart_open(HAL_UART_ID_0,NULL);
//    TRACE("avrcp_init...OK\n");
//}



const unsigned char a2dp_codec_elements[] =
{
    A2D_SBC_IE_SAMP_FREQ_48 | A2D_SBC_IE_SAMP_FREQ_44 | A2D_SBC_IE_CH_MD_STEREO | A2D_SBC_IE_CH_MD_JOINT,
    A2D_SBC_IE_BLOCKS_16 | A2D_SBC_IE_BLOCKS_12 | A2D_SBC_IE_SUBBAND_8 | A2D_SBC_IE_ALLOC_MD_L,
    A2D_SBC_IE_MIN_BITPOOL,
    BTA_AV_CO_SBC_MAX_BITPOOL
};

int store_sbc_buffer(unsigned char *buf, unsigned int len);
int a2dp_audio_sbc_set_frame_info(int rcv_len, int frame_num);


#if defined(__BT_RECONNECT__) && defined(__BT_A2DP_RECONNECT__)
struct BT_A2DP_RECONNECT_T bt_a2dp_reconnect;

extern "C" void cancel_a2dp_reconnect(void)
{
    if(bt_a2dp_reconnect.TimerNotifyFunc) {
        osTimerStop(bt_a2dp_reconnect.TimerID);
        osTimerDelete(bt_a2dp_reconnect.TimerID);
        bt_a2dp_reconnect.TimerNotifyFunc= 0;
    }
}

void reconnect_a2dp_stream(enum BT_DEVICE_ID_T stream_id)
{
    A2dpStream *Stream;

    TRACE("a2dp_reconnect:\n");
    Stream = bt_a2dp_reconnect.copyStream[stream_id];
    A2DP_OpenStream(Stream, &bt_a2dp_reconnect.copyAddr[stream_id]);
}

void a2dp_reconnect1(void)
{
    reconnect_a2dp_stream(BT_DEVICE_ID_1);
}

#ifdef __BT_ONE_BRING_TWO__
void a2dp_reconnect2(void)
{
    reconnect_a2dp_stream(BT_DEVICE_ID_2);
}
#endif

void a2dp_reconnect_timer_callback(void const *n) {
    if(bt_a2dp_reconnect.TimerNotifyFunc)
        bt_a2dp_reconnect.TimerNotifyFunc();
}
osTimerDef(a2dp_reconnect_timer, a2dp_reconnect_timer_callback);
#endif



void btapp_send_pause_key(enum BT_DEVICE_ID_T stream_id)
{
    TRACE("btapp_send_pause_key id = %x",stream_id);
    AVRCP_SetPanelKey(&(app_bt_device.avrcp_channel[stream_id]),AVRCP_POP_PAUSE,TRUE);
    AVRCP_SetPanelKey(&(app_bt_device.avrcp_channel[stream_id]),AVRCP_POP_PAUSE,FALSE);
 //   app_bt_device.a2dp_play_pause_flag = 0;
}


void btapp_a2dp_suspend_music(enum BT_DEVICE_ID_T stream_id)
{
    TRACE("btapp_a2dp_suspend_music id = %x",stream_id);

    btapp_send_pause_key(stream_id);
}

extern enum AUD_SAMPRATE_T a2dp_sample_rate;

#define A2DP_TIMESTAMP_DEBOUNCE_DURATION (1000)
#define A2DP_TIMESTAMP_MODE_SAMPLE_THRESHOLD (2000)

#define A2DP_TIMESTAMP_SYNC_LIMIT_CNT (200)
#define A2DP_TIMESTAMP_SYNC_TIME_THRESHOLD (50)
#define A2DP_TIMESTAMP_SYNC_SAMPLE_THRESHOLD (a2dp_sample_rate/1000*A2DP_TIMESTAMP_SYNC_TIME_THRESHOLD)

#define RICE_THRESHOLD
#define RICE_THRESHOLD

struct A2DP_TIMESTAMP_INFO_T{
    uint16_t rtp_timestamp;
    uint32_t loc_timestamp;
    uint16_t frame_num;
};

enum A2DP_TIMESTAMP_MODE_T{    
    A2DP_TIMESTAMP_MODE_NONE,
    A2DP_TIMESTAMP_MODE_SAMPLE,
    A2DP_TIMESTAMP_MODE_TIME,    
};

enum A2DP_TIMESTAMP_MODE_T a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_NONE;

struct A2DP_TIMESTAMP_INFO_T a2dp_timestamp_pre = {0,0,0};
bool a2dp_timestamp_parser_need_sync = false;
int a2dp_timestamp_parser_init(void)
{
    a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_NONE;
    a2dp_timestamp_pre.rtp_timestamp = 0;
    a2dp_timestamp_pre.loc_timestamp = 0;
    a2dp_timestamp_pre.frame_num = 0;
    a2dp_timestamp_parser_need_sync = false;
    return 0;
}

int a2dp_timestamp_parser_needsync(void)
{
    a2dp_timestamp_parser_need_sync = true;
    return 0;
}

int a2dp_timestamp_parser_run(uint16_t timestamp, uint16_t framenum)
{
    static int skip_cnt = 0;
    struct A2DP_TIMESTAMP_INFO_T curr_timestamp;
    int skipframe = 0;
    int16_t rtpdiff;
    int16_t locdiff;
    bool needsave_timestamp = true;

    curr_timestamp.rtp_timestamp = timestamp;
    curr_timestamp.loc_timestamp = hal_sys_timer_get();
    curr_timestamp.frame_num = framenum;

    switch(a2dp_timestamp_mode) {
        case A2DP_TIMESTAMP_MODE_NONE:            

//            TRACE("parser rtp:%d loc:%d num:%d prertp:%d preloc:%d\n", curr_timestamp.rtp_timestamp, curr_timestamp.loc_timestamp, curr_timestamp.frame_num,
//                                                   a2dp_timestamp_pre.rtp_timestamp, a2dp_timestamp_pre.loc_timestamp);       
            if (a2dp_timestamp_pre.rtp_timestamp){
                locdiff = curr_timestamp.loc_timestamp - a2dp_timestamp_pre.loc_timestamp;
                if (TICKS_TO_MS(locdiff) > A2DP_TIMESTAMP_DEBOUNCE_DURATION){                    
                    rtpdiff = curr_timestamp.rtp_timestamp - a2dp_timestamp_pre.rtp_timestamp;
                    if (ABS((int16_t)TICKS_TO_MS(locdiff)-rtpdiff)>A2DP_TIMESTAMP_MODE_SAMPLE_THRESHOLD){
                        a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_SAMPLE;
                        TRACE("A2DP_TIMESTAMP_MODE_SAMPLE\n");
                    }else{
                        a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_TIME;                        
                        TRACE("A2DP_TIMESTAMP_MODE_TIME\n");
                    }
                }else{
                    needsave_timestamp = false;
                }
            }
            break;
        case A2DP_TIMESTAMP_MODE_SAMPLE:
            if (a2dp_timestamp_parser_need_sync){
                skip_cnt++;                
                rtpdiff = curr_timestamp.rtp_timestamp - a2dp_timestamp_pre.rtp_timestamp;
                locdiff = curr_timestamp.loc_timestamp - a2dp_timestamp_pre.loc_timestamp;
                TRACE("A2DP_TIMESTAMP_MODE_SAMPLE SYNC diff:%d cnt:%d\n", ABS((int16_t)(TICKS_TO_MS(locdiff)*(a2dp_sample_rate/1000)) - rtpdiff), skip_cnt);
                if ((ABS((int16_t)(TICKS_TO_MS(locdiff)*(a2dp_sample_rate/1000)) - rtpdiff)) < (uint32_t)A2DP_TIMESTAMP_SYNC_SAMPLE_THRESHOLD){                    
                    TRACE("A2DP_TIMESTAMP_MODE_SAMPLE RESYNC OK cnt:%d\n", skip_cnt);
                    skip_cnt = 0;
                    a2dp_timestamp_parser_need_sync = false;                
                }else if (skip_cnt > A2DP_TIMESTAMP_SYNC_LIMIT_CNT){
                    TRACE("A2DP_TIMESTAMP_MODE_SAMPLE RESYNC FORCE END\n");
                    skip_cnt = 0;
                    a2dp_timestamp_parser_need_sync = false;                
                }else{
                    needsave_timestamp = false;
                    skipframe = 1;
                }
            }
            break;
        case A2DP_TIMESTAMP_MODE_TIME:
            if (a2dp_timestamp_parser_need_sync){
                skip_cnt++;
                rtpdiff = curr_timestamp.rtp_timestamp - a2dp_timestamp_pre.rtp_timestamp;
                locdiff = curr_timestamp.loc_timestamp - a2dp_timestamp_pre.loc_timestamp;                
                TRACE("A2DP_TIMESTAMP_MODE_TIME SYNC diff:%d cnt:%d\n", ABS((int16_t)TICKS_TO_MS(locdiff) - rtpdiff), skip_cnt);
                if (ABS((int16_t)TICKS_TO_MS(locdiff) - rtpdiff) < A2DP_TIMESTAMP_SYNC_TIME_THRESHOLD){
                    TRACE("A2DP_TIMESTAMP_MODE_TIME RESYNC OK cnt:%d\n", skip_cnt);
                    skip_cnt = 0;
                    a2dp_timestamp_parser_need_sync = false;                
                }else if (skip_cnt > A2DP_TIMESTAMP_SYNC_LIMIT_CNT){                
                    TRACE("A2DP_TIMESTAMP_MODE_TIME RESYNC FORCE END\n");
                    skip_cnt = 0;
                    a2dp_timestamp_parser_need_sync = false;                
                }else{            
                    needsave_timestamp = false;
                    skipframe = 1;
                }
            }
            break;
    }
    
    if (needsave_timestamp){
        a2dp_timestamp_pre.rtp_timestamp = curr_timestamp.rtp_timestamp;
        a2dp_timestamp_pre.loc_timestamp = curr_timestamp.loc_timestamp;
    }
    return skipframe;
}

static struct BT_DEVICE_ID_DIFF stream_id_flag;
#ifdef __BT_ONE_BRING_TWO__
void a2dp_stream_id_distinguish(A2dpStream *Stream)
{
    if(Stream == &app_bt_device.a2dp_stream[BT_DEVICE_ID_1]){
        stream_id_flag.id = BT_DEVICE_ID_1;
        stream_id_flag.id_other = BT_DEVICE_ID_2;
    }else if(Stream == &app_bt_device.a2dp_stream[BT_DEVICE_ID_2]){
        stream_id_flag.id = BT_DEVICE_ID_2;
        stream_id_flag.id_other = BT_DEVICE_ID_1;
    }
}
#endif


#if defined( __EARPHONE__) && defined(__BT_RECONNECT__)

#ifdef __BT_ONE_BRING_TWO__
extern BtDeviceRecord record2_copy;
extern uint8_t record2_avalible;
#endif

#endif

extern void app_bt_profile_connect_manager_a2dp(enum BT_DEVICE_ID_T id, A2dpStream *Stream, const A2dpCallbackParms *Info);

void a2dp_callback(A2dpStream *Stream, const A2dpCallbackParms *Info)
{
    int header_len = 0;
    AvdtpMediaHeader header;

#if defined(__BT_RECONNECT__)
    static AvdtpCodec   setconfig_codec;
    static u8 tmp_element[10];
#endif

//    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
#if 0 //def __BT_ONE_BRING_TWO__
    if((app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_ACTIVE) || (app_bt_device.hf_audio_state[BT_DEVICE_ID_2] == HF_AUDIO_CON)){
        switch(Info->event) {
            case A2DP_EVENT_STREAM_START_IND:
                TRACE("!!!HF_CALL_ACTIVE  A2DP_EVENT_STREAM_START_IND\n");
                A2DP_StartStreamRsp(&(app_bt_device.a2dp_stream[BT_DEVICE_ID_1]), A2DP_ERR_NO_ERROR);
                app_bt_device.curr_a2dp_stream_id = BT_DEVICE_ID_1;
                app_bt_device.a2dp_play_pause_flag = 1;
                return;
            case A2DP_EVENT_STREAM_STARTED:
                TRACE("!!!HF_CALL_ACTIVE  A2DP_EVENT_STREAM_STARTED\n");
                app_audio_sendrequest((uint8_t)APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                return;
            case A2DP_EVENT_STREAM_DATA_IND:
                return;
            default:
                break;
        }
    }
#endif

#ifdef __BT_ONE_BRING_TWO__
    a2dp_stream_id_distinguish(Stream);
#else
    stream_id_flag.id = BT_DEVICE_ID_1;
#endif

    switch(Info->event) {
        case A2DP_EVENT_AVDTP_CONNECT:
            TRACE("::A2DP_EVENT_AVDTP_CONNECT %d\n", Info->event);
            break;
        case A2DP_EVENT_STREAM_OPEN:
            TRACE("::A2DP_EVENT_STREAM_OPEN stream_id:%d, sample_rate codec.elements 0x%x\n", stream_id_flag.id,Info->p.configReq->codec.elements[0]);            
            a2dp_timestamp_parser_init();
            app_bt_profile_connect_manager_a2dp(stream_id_flag.id, Stream, (A2dpCallbackParms *)Info);
            app_bt_stream_volume_ptr_update((uint8_t *)Stream->stream.conn.remDev->bdAddr.addr);
//            app_bt_stream_a2dpvolume_reset();
            app_bt_device.sample_rate[stream_id_flag.id] = (Info->p.configReq->codec.elements[0] & A2D_SBC_IE_SAMP_FREQ_MSK);
            app_bt_device.a2dp_state[stream_id_flag.id] = 1;
#ifdef __BT_ONE_BRING_TWO__
            if(app_bt_device.a2dp_stream[stream_id_flag.id_other].stream.state != AVDTP_STRM_STATE_STREAMING){
                app_bt_device.curr_a2dp_stream_id = stream_id_flag.id;
            }
#endif
            AVRCP_Connect(&app_bt_device.avrcp_channel[stream_id_flag.id], &Stream->stream.conn.remDev->bdAddr);
            break;
        case A2DP_EVENT_STREAM_OPEN_IND:
            TRACE("::A2DP_EVENT_STREAM_OPEN_IND %d\n", Info->event);
            A2DP_OpenStreamRsp(Stream, A2DP_ERR_NO_ERROR, AVDTP_SRV_CAT_MEDIA_TRANSPORT);
            break;
        case A2DP_EVENT_STREAM_STARTED:
            TRACE("::A2DP_EVENT_STREAM_STARTED  stream_id:%d\n", stream_id_flag.id);            
            a2dp_timestamp_parser_init();
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC,stream_id_flag.id,MAX_RECORD_NUM);          
            app_bt_device.a2dp_streamming[stream_id_flag.id] = 1;
            break;
        case A2DP_EVENT_STREAM_START_IND:
            TRACE("::A2DP_EVENT_STREAM_START_IND stream_id:%d\n", stream_id_flag.id);

#ifdef __BT_ONE_BRING_TWO__
            A2DP_StartStreamRsp(&app_bt_device.a2dp_stream[stream_id_flag.id], A2DP_ERR_NO_ERROR);
            if(app_bt_device.a2dp_stream[stream_id_flag.id_other].stream.state != AVDTP_STRM_STATE_STREAMING){
                app_bt_device.curr_a2dp_stream_id = stream_id_flag.id;
            }
#else
            A2DP_StartStreamRsp(Stream, A2DP_ERR_NO_ERROR);
#endif
            app_bt_device.a2dp_play_pause_flag = 1;
            break;
        case A2DP_EVENT_STREAM_SUSPENDED:
            TRACE("::A2DP_EVENT_STREAM_SUSPENDED  stream_id:%d\n", stream_id_flag.id);
            a2dp_timestamp_parser_init();
#ifdef __BT_ONE_BRING_TWO__
            if(app_bt_device.a2dp_stream[stream_id_flag.id_other].stream.state == AVDTP_STRM_STATE_STREAMING){
                app_bt_device.curr_a2dp_stream_id = stream_id_flag.id_other;
                app_bt_device.a2dp_play_pause_flag = 1;
            }else{
                app_bt_device.a2dp_play_pause_flag = 0;
            }
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,stream_id_flag.id,0);
#else
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,BT_DEVICE_ID_1,0);
            app_bt_device.a2dp_play_pause_flag = 0;
#endif
            app_bt_device.a2dp_streamming[stream_id_flag.id] = 0;
            break;
        case A2DP_EVENT_STREAM_DATA_IND:
#ifdef __BT_ONE_BRING_TWO__
                ////play music of curr_a2dp_stream_id
                if(app_bt_device.curr_a2dp_stream_id  == stream_id_flag.id && 
                   app_bt_device.hf_audio_state[stream_id_flag.id]==HF_AUDIO_DISCON && 
                   app_bt_device.hf_audio_state[stream_id_flag.id_other]==HF_AUDIO_DISCON &&
                   app_bt_device.hfchan_callSetup[stream_id_flag.id_other] == HF_CALL_SETUP_NONE){
                    header_len = AVDTP_ParseMediaHeader(&header, Info->p.data);
                    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC) && (Stream->stream.state == AVDTP_STRM_STATE_STREAMING)){
                        if (a2dp_timestamp_parser_run(header.timestamp,(*(((unsigned char *)Info->p.data) + header_len)))){
                            TRACE("::A2DP_EVENT_STREAM_DATA_IND skip frame\n");
                        }else{
                            a2dp_audio_sbc_set_frame_info(Info->len - header_len - 1, (*(((unsigned char *)Info->p.data) + header_len)));
                            store_sbc_buffer(((unsigned char *)Info->p.data) + header_len + 1 , Info->len - header_len - 1);
                        }
                    }
                }
#else
                header_len = AVDTP_ParseMediaHeader(&header, Info->p.data);
                if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC) && (Stream->stream.state == AVDTP_STRM_STATE_STREAMING)){
                    if (a2dp_timestamp_parser_run(header.timestamp,(*(((unsigned char *)Info->p.data) + header_len)))){
                        TRACE("::A2DP_EVENT_STREAM_DATA_IND skip frame\n");
                    }else{
                        a2dp_audio_sbc_set_frame_info(Info->len - header_len - 1, (*(((unsigned char *)Info->p.data) + header_len)));
                        store_sbc_buffer(((unsigned char *)Info->p.data) + header_len + 1 , Info->len - header_len - 1);
                    }
                }
#endif
            break;
        case A2DP_EVENT_STREAM_CLOSED:
            TRACE("::A2DP_EVENT_STREAM_CLOSED stream_id:%d, reason = %x\n", stream_id_flag.id,Info->discReason);
            a2dp_timestamp_parser_init();            
            app_bt_profile_connect_manager_a2dp(stream_id_flag.id, Stream, (A2dpCallbackParms *)Info);
#ifdef __BT_ONE_BRING_TWO__          
            app_bt_device.curr_a2dp_stream_id = stream_id_flag.id_other;
            if(app_bt_device.a2dp_stream[stream_id_flag.id_other].stream.state == AVDTP_STRM_STATE_STREAMING){
         //       app_bt_device.curr_a2dp_stream_id = stream_id_flag.id_other;
                app_bt_device.a2dp_play_pause_flag = 1;   
            }     
            else
            {
                app_bt_device.a2dp_play_pause_flag = 0;   
            }
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,stream_id_flag.id,0);
#else
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,stream_id_flag.id,0);

#endif
            app_bt_device.a2dp_state[stream_id_flag.id] = 0;

#ifdef __BT_ONE_BRING_TWO__
        ///a2dp disconnect so check the other stream is playing or not
        if(app_bt_device.a2dp_stream[stream_id_flag.id_other].stream.state  != AVDTP_STRM_STATE_STREAMING)
        {
            app_bt_device.a2dp_play_pause_flag = 0;    
        }
#endif
            break;
#if defined(__BT_RECONNECT__)
        case A2DP_EVENT_CODEC_INFO:
            TRACE("::A2DP_EVENT_CODEC_INFO %d\n", Info->event);
            setconfig_codec.codecType = Info->p.codec->codecType;
            setconfig_codec.discoverable = Info->p.codec->discoverable;
            setconfig_codec.elemLen = Info->p.codec->elemLen;
            setconfig_codec.elements = tmp_element;
            setconfig_codec.elements[0] = (Info->p.codec->elements[0]) & (a2dp_codec_elements[0]);
            setconfig_codec.elements[1] = (Info->p.codec->elements[1]) & (a2dp_codec_elements[1]);

            if(Info->p.codec->elements[2] <= a2dp_codec_elements[2])
                setconfig_codec.elements[2] = a2dp_codec_elements[2];////[2]:MIN_BITPOOL
            else
                setconfig_codec.elements[2] = Info->p.codec->elements[2];

            if(Info->p.codec->elements[3] >= a2dp_codec_elements[3])
                setconfig_codec.elements[3] = a2dp_codec_elements[3];////[3]:MAX_BITPOOL
            else
                setconfig_codec.elements[3] = Info->p.codec->elements[3];

            ///////null set situation:
            if(setconfig_codec.elements[3] < a2dp_codec_elements[2]){
                setconfig_codec.elements[2] = a2dp_codec_elements[2];
                setconfig_codec.elements[3] = a2dp_codec_elements[3];
            }
            else if(setconfig_codec.elements[2] > a2dp_codec_elements[3]){
                setconfig_codec.elements[2] = a2dp_codec_elements[3];
                setconfig_codec.elements[3] = a2dp_codec_elements[3];
            }
            TRACE("!!!setconfig_codec.elements[2]:%d,setconfig_codec.elements[3]:%d\n",setconfig_codec.elements[2],setconfig_codec.elements[3]);    

            setconfig_codec.elements[0] = get_valid_bit(setconfig_codec.elements[0],A2D_SBC_IE_SAMP_FREQ_MSK);
            setconfig_codec.elements[0] = get_valid_bit(setconfig_codec.elements[0],A2D_SBC_IE_CH_MD_MSK);
            setconfig_codec.elements[1] = get_valid_bit(setconfig_codec.elements[1],A2D_SBC_IE_BLOCKS_MSK);
            setconfig_codec.elements[1] = get_valid_bit(setconfig_codec.elements[1],A2D_SBC_IE_SUBBAND_MSK);
            setconfig_codec.elements[1] = get_valid_bit(setconfig_codec.elements[1],A2D_SBC_IE_ALLOC_MD_MSK);
            break;
        case A2DP_EVENT_GET_CONFIG_IND:
            TRACE("::A2DP_EVENT_GET_CONFIG_IND %d\n", Info->event);
            A2DP_SetStreamConfig(Stream, &setconfig_codec, NULL);
            break;
#endif
    }
}

void a2dp_suspend_music_force(void)
{
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,stream_id_flag.id,0);
}

static int a2dp_volume_get(void)
{
    int vol = app_bt_stream_volume_get_ptr()->a2dp_vol;

    if (vol >= TGT_VOLUME_LEVEL_15){
        vol = 0x7f;
    }else{
        vol = 8*(vol-1);
    }
    
    if (vol > 115)
        vol = 0x7f;

    TRACE("get vol raw:%d %d/127", app_bt_stream_volume_get_ptr()->a2dp_vol, vol);

    return (vol);
}

int app_bt_stream_volumeset(int8_t vol);

void a2dp_volume_local_set(int8_t vol)
{
    app_bt_stream_volume_get_ptr()->a2dp_vol = vol;
    #ifndef FPGA
    nv_record_touch_cause_flush();
    #endif
}

static int a2dp_volume_set(U8 vol)
{
    int dest_vol;

    if(vol<8 && vol>5)vol = 8;  //rjb add --���iPhone��һ��������������״̬�£�����绰/����������ָ���������������
	
    dest_vol = (((int)vol&0x7f)<<4)/0x7f + 1;

    if (dest_vol > TGT_VOLUME_LEVEL_15)
        dest_vol = TGT_VOLUME_LEVEL_15;
    if (dest_vol < TGT_VOLUME_LEVEL_0)
        dest_vol = TGT_VOLUME_LEVEL_0;

    a2dp_volume_local_set(dest_vol);
    app_bt_stream_volumeset(dest_vol);

    TRACE("put vol raw:%d/127 %d", vol, dest_vol);

    return (vol);
}

void btapp_a2dp_report_speak_gain(void)
{
#if AVRCP_ADVANCED_CONTROLLER == XA_ENABLED
    uint8_t i;
    int vol = a2dp_volume_get();

    for(i=0; i<BT_DEVICE_NUM; i++)
    {
        TRACE("btapp_a2dp_report_speak_gain transId:%d a2dp_state:%d streamming:%d report:%02x\n", 
        app_bt_device.avrcp_notify_rsp[i]->transId,
        app_bt_device.a2dp_state[i],
        app_bt_device.a2dp_streamming[i],
        app_bt_device.volume_report[i]);
        OS_LockStack();
        if ((app_bt_device.a2dp_state[i] == 1) && 
            (app_bt_device.a2dp_streamming[i] == 1) &&
            (app_bt_device.volume_report[i] == AVCTP_RESPONSE_INTERIM)){
            app_bt_device.volume_report[i] = AVCTP_RESPONSE_CHANGED;
            TRACE("btapp_a2dp_report_speak_gain transId:%d\n", app_bt_device.avrcp_notify_rsp[i]->transId);
            if (app_bt_device.avrcp_notify_rsp[i] != NULL){
                app_bt_device.avrcp_notify_rsp[i]->ctype = AVCTP_RESPONSE_CHANGED;
                AVRCP_CtGetAbsoluteVolume_Rsp(&app_bt_device.avrcp_channel[i], app_bt_device.avrcp_notify_rsp[i], vol);
            }
        }
        OS_UnlockStack();
    }
#endif
}
