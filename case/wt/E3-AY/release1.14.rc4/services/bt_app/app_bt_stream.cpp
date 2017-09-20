//#include "mbed.h"
#include <stdio.h>
#include <assert.h>

#include "cmsis_os.h"
#include "tgt_hardware.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "analog.h"
#include "app_bt_stream.h"
#include "app_overlay.h"
#include "app_audio.h"
#include "app_utils.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"

#ifdef MEDIA_PLAYER_SUPPORT
#include "resources.h"
#include "app_media_player.h"
#endif
#ifdef __FACTORY_MODE_SUPPORT__
#include "app_factory_audio.h"
#endif

#include "app_ring_merge.h"

#include "bt_drv.h"
#include "bt_xtal_sync.h"
#include "besbt.h"
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
}
#include "sw_timer.h"

#include "cqueue.h"
#include "btapp.h"

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)
#include "audio_eq.h"
#include "res_audio_eq.h"
#endif

#include "app_bt_media_manager.h"

#include "string.h"
#include "hal_location.h"

enum PLAYER_OPER_T {
    PLAYER_OPER_START,
    PLAYER_OPER_STOP,
    PLAYER_OPER_RESTART,
};

#if (AUDIO_OUTPUT_VOLUME_DEFAULT < 1) || (AUDIO_OUTPUT_VOLUME_DEFAULT > 16)
#error "AUDIO_OUTPUT_VOLUME_DEFAULT out of range"
#endif
int8_t stream_local_volume = (AUDIO_OUTPUT_VOLUME_DEFAULT);

struct btdevice_volume *btdevice_volume_p;

uint32_t a2dp_audio_more_data(uint8_t *buf, uint32_t len);
int a2dp_audio_init(void);
int a2dp_audio_deinit(void);

enum AUD_SAMPRATE_T a2dp_sample_rate = AUD_SAMPRATE_48000;


int bt_sbc_player_setup(uint8_t freq)
{
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    static uint8_t sbc_samp_rate = 0xff;

    if (sbc_samp_rate == freq)
        return 0;

    switch (freq) {
        case A2D_SBC_IE_SAMP_FREQ_16:
        case A2D_SBC_IE_SAMP_FREQ_32:
        case A2D_SBC_IE_SAMP_FREQ_48:
            a2dp_sample_rate = AUD_SAMPRATE_48000;
            break;
        case A2D_SBC_IE_SAMP_FREQ_44:
            a2dp_sample_rate = AUD_SAMPRATE_44100;
            break;
        default:
            break;
    }

    af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
    stream_cfg->sample_rate = a2dp_sample_rate;
    af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
    sbc_samp_rate = freq;

    return 0;
}

void merge_stereo_to_mono_16bits(int16_t *src_buf, int16_t *dst_buf,  uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; i+=2) {
        dst_buf[i] = (src_buf[i]>>1) + (src_buf[i+1]>>1);
        dst_buf[i+1] = dst_buf[i];
    }
}


uint32_t bt_sbc_player_more_data(uint8_t *buf, uint32_t len)
{
#ifdef BT_XTAL_SYNC
    bt_xtal_sync();
#endif
    a2dp_audio_more_data(buf, len);
    app_ring_merge_more_data(buf, len);
#ifdef __AUDIO_OUTPUT_MONO_MODE__
    merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)buf, len/2);
#endif

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)
    audio_eq_run(buf, len);
#endif
    OS_NotifyEvm();
    return len;
}

int bt_sbc_player(enum PLAYER_OPER_T on, enum APP_SYSFREQ_FREQ_T freq)
{
    struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;
    uint8_t* bt_audio_buff = NULL;

    TRACE("bt_sbc_player work:%d op:%d freq:%d", isRun, on, freq);

    if ((isRun && on == PLAYER_OPER_START) || (!isRun && on == PLAYER_OPER_STOP))
        return 0;

    if (on == PLAYER_OPER_STOP || on == PLAYER_OPER_RESTART) {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        if (on == PLAYER_OPER_STOP) {
#ifndef FPGA
#ifdef BT_XTAL_SYNC
            osDelay(50);
            bt_term_xtal_sync();
#endif
#endif
            a2dp_audio_deinit();

// call after AUD_STREAM_PLAYBACK close, because fadeout
#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)
            audio_eq_close();
#endif
            app_overlay_unloadall();
            app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
            af_set_priority(osPriorityAboveNormal);
        }
    }

    if (on == PLAYER_OPER_START || on == PLAYER_OPER_RESTART) {
        af_set_priority(osPriorityHigh);
        if (freq < APP_SYSFREQ_52M) {
            freq = APP_SYSFREQ_52M;
        }
#ifdef __BT_ONE_BRING_TWO__		
        if (MEC(activeCons)>1){
            freq = APP_SYSFREQ_104M;
        }
#endif		
        stream_local_volume = btdevice_volume_p->a2dp_vol;
        app_audio_mempool_init();
        app_audio_mempool_get_buff(&bt_audio_buff, BT_AUDIO_BUFF_SIZE);
#if defined(__SW_IIR_EQ_PROCESS__)&&defined(__HW_FIR_EQ_PROCESS__)
        if (audio_eq_fir_cfg.len > 128){
            app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq>APP_SYSFREQ_104M?freq:APP_SYSFREQ_104M);
        }else{
            app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);
        }
#else
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);
#endif

        memset(&stream_cfg, 0, sizeof(stream_cfg));
        memset(bt_audio_buff, 0, BT_AUDIO_BUFF_SIZE);
        if (on == PLAYER_OPER_START) {
#ifndef FPGA
            app_overlay_select(APP_OVERLAY_A2DP);
#ifdef BT_XTAL_SYNC
            bt_init_xtal_sync();
#endif
#endif
#ifdef  __FPGA_FLASH__
            app_overlay_select(APP_OVERLAY_A2DP);
#endif
        }
        
        if (on == PLAYER_OPER_START) {
            a2dp_audio_init();
        }

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = a2dp_sample_rate;
#ifdef FPGA
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#endif
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.vol = stream_local_volume;
        stream_cfg.handler = bt_sbc_player_more_data;

        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        stream_cfg.data_size = BT_AUDIO_BUFF_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

// call after AUD_STREAM_PLAYBACK open, otherwise an unexpected error occured
// TODO: Move this to audioflinger
#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)
        audio_eq_init();
#ifdef __AUDIO_RESAMPLE__        
        audio_eq_open(AUD_SAMPRATE_50700, AUD_BITS_16);
#else
        audio_eq_open(AUD_SAMPRATE_44100, AUD_BITS_16);
#endif   
#ifdef __SW_IIR_EQ_PROCESS__
        audio_eq_iir_set_cfg((IIR_CFG_T *)&audio_eq_iir_cfg);
        
        // const float a[3] = {1.000000000000000, -1.916920687511884, 0.923083357923947};
        // const float b[3] = {0.961218710921220, -1.919998941382705, 0.958786393131897};
        // audio_eq_iir_set_gain_and_num(0.0, 0.0, 1);
        // audio_eq_iir_set_coef(0, a, b);

#endif

#ifdef __HW_FIR_EQ_PROCESS__
        audio_eq_fir_set_cfg((FIR_CFG_T *)&audio_eq_fir_cfg);
#endif
#endif
    }

    isRun = (on != PLAYER_OPER_STOP);
    return 0;
}


int voicebtpcm_pcm_audio_init(void);
uint32_t voicebtpcm_pcm_audio_data_come(uint8_t *buf, uint32_t len);
uint32_t voicebtpcm_pcm_audio_more_data(uint8_t *buf, uint32_t len);
int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len);
int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len);
uint8_t btapp_hfp_need_mute(void);

//( codec:mic-->btpcm:tx
// codec:mic
static uint32_t bt_sco_codec_capture_data(uint8_t *buf, uint32_t len)
{
    voicebtpcm_pcm_audio_data_come(buf, len);
    return len;
}

// btpcm:tx
static uint32_t bt_sco_btpcm_playback_data(uint8_t *buf, uint32_t len)
{
    get_voicebtpcm_p2m_frame(buf, len);
    if (btapp_hfp_need_mute()){
        memset(buf, 0, len);
    }
    return len;
}
//)

//( btpcm:rx-->codec:spk
// btpcm:rx
static uint32_t bt_sco_btpcm_capture_data(uint8_t *buf, uint32_t len)
{
    store_voicebtpcm_m2p_buffer(buf, len);

    return len;
}

// codec:spk
static int16_t *bt_sco_codec_playback_cache_ping = NULL;
static int16_t *bt_sco_codec_playback_cache_pang = NULL;
static enum AF_PP_T bt_sco_codec_playback_cache_pp = PP_PING;

static uint32_t bt_sco_codec_playback_data(uint8_t *buf, uint32_t len)
{
    int16_t *bt_sco_codec_playback_cache;

    if (bt_sco_codec_playback_cache_pp == PP_PING){
        bt_sco_codec_playback_cache_pp = PP_PANG;
        bt_sco_codec_playback_cache = bt_sco_codec_playback_cache_ping;
    }else{    
        bt_sco_codec_playback_cache_pp = PP_PING;
        bt_sco_codec_playback_cache = bt_sco_codec_playback_cache_pang;
    }

#ifdef BT_XTAL_SYNC
    bt_xtal_sync();
#endif

    if (bt_sco_codec_playback_cache){
        voicebtpcm_pcm_audio_more_data((uint8_t *)bt_sco_codec_playback_cache, len/2);        
        app_ring_merge_more_data((uint8_t *)bt_sco_codec_playback_cache, len/2);
        app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf, bt_sco_codec_playback_cache, len/2/2);
    }else{
        memset(buf, 0, len);
        app_ring_merge_more_data(buf, len);
    }

    return len;
}
//)

int bt_sco_player(bool on, enum APP_SYSFREQ_FREQ_T freq)
{
    struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;
    uint8_t * bt_audio_buff = NULL;
    
    TRACE("bt_sco_player work:%d op:%d freq:%d", isRun, on, freq);
//  osDelay(1);

    if (isRun==on)
        return 0;

    if (on){
        //bt_syncerr set to max(0x0a)
//        BTDIGITAL_REG_SET_FIELD(REG_BTCORE_BASE_ADDR, 0x0f, 0, 0x0f);
//        af_set_priority(osPriorityRealtime);
         stream_local_volume = btdevice_volume_p->hfp_vol;

#if defined(SPEECH_NOISE_SUPPRESS) && defined(SPEECH_ECHO_CANCEL)
#if  !defined(_SCO_BTPCM_CHANNEL_)
        if (freq < APP_SYSFREQ_104M) {
            freq = APP_SYSFREQ_104M;
        }
#elif defined(SPEECH_WEIGHTING_FILTER_SUPPRESS) || defined(SPEAKER_WEIGHTING_FILTER_SUPPRESS)
        if (freq < APP_SYSFREQ_78M) {
            freq = APP_SYSFREQ_78M;
        }
#else
        if (freq < APP_SYSFREQ_52M) {
            freq = APP_SYSFREQ_52M;
        }
#endif
#else
        if (freq < APP_SYSFREQ_52M) {
            freq = APP_SYSFREQ_52M;
        }
#endif
#ifdef __BT_ONE_BRING_TWO__		
        if (MEC(activeCons)>1){
            freq = APP_SYSFREQ_104M;
        }
#endif
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);

#ifndef FPGA
        app_overlay_select(APP_OVERLAY_HFP);
#ifdef BT_XTAL_SYNC
        bt_init_xtal_sync();
#endif
#endif
        app_audio_mempool_init();
        voicebtpcm_pcm_audio_init();
        memset(&stream_cfg, 0, sizeof(stream_cfg));
#ifndef _SCO_BTPCM_CHANNEL_        
        memset(&hf_sendbuff_ctrl, 0, sizeof(hf_sendbuff_ctrl));
#endif
        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
        stream_cfg.sample_rate = AUD_SAMPRATE_8000;
        stream_cfg.vol = stream_local_volume;
        stream_cfg.data_size = BT_AUDIO_SCO_BUFF_SIZE;

        // codec:mic
#ifdef FPGA
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#endif        
        app_audio_mempool_get_buff(&bt_audio_buff, BT_AUDIO_SCO_BUFF_SIZE);
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
        stream_cfg.handler = bt_sco_codec_capture_data;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        // codec:spk
        stream_cfg.data_size = BT_AUDIO_SCO_BUFF_SIZE*2;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        app_audio_mempool_get_buff(&bt_audio_buff, BT_AUDIO_SCO_BUFF_SIZE*2);
        bt_sco_codec_playback_cache_pp = PP_PING;
        app_audio_mempool_get_buff((uint8_t **)&bt_sco_codec_playback_cache_ping, BT_AUDIO_SCO_BUFF_SIZE*2/2/2);
        app_audio_mempool_get_buff((uint8_t **)&bt_sco_codec_playback_cache_pang, BT_AUDIO_SCO_BUFF_SIZE*2/2/2);
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = bt_sco_codec_playback_data;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

#ifdef _SCO_BTPCM_CHANNEL_
        stream_cfg.data_size = BT_AUDIO_SCO_BUFF_SIZE;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
        // btpcm:rx
        app_audio_mempool_get_buff(&bt_audio_buff, BT_AUDIO_SCO_BUFF_SIZE);
        stream_cfg.device = AUD_STREAM_USE_BT_PCM;
        stream_cfg.handler = bt_sco_btpcm_capture_data;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        af_stream_open(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE, &stream_cfg);

        // btpcm:tx
        app_audio_mempool_get_buff(&bt_audio_buff, BT_AUDIO_SCO_BUFF_SIZE);
        stream_cfg.device = AUD_STREAM_USE_BT_PCM;
        stream_cfg.handler = bt_sco_btpcm_playback_data;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        af_stream_open(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK, &stream_cfg);

        af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
#endif
#ifdef FPGA
        app_bt_stream_volumeset(stream_local_volume);
#endif
        TRACE("bt_sco_player on");
    } else {
#ifndef FPGA
        osDelay(100);        //del by wanghaihui for discon two call the af_dma_irq_handler disappear on FPGA
#endif
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#ifdef _SCO_BTPCM_CHANNEL_
        af_stream_stop(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);

        af_stream_close(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
#endif
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        
        bt_sco_codec_playback_cache_ping = NULL;
        bt_sco_codec_playback_cache_pang = NULL;
		bt_sco_codec_playback_cache_pp = PP_PING;
#ifndef FPGA
#ifdef BT_XTAL_SYNC
        osDelay(50);
        bt_term_xtal_sync();
#endif
#endif
        TRACE("bt_sco_player off");
        app_overlay_unloadall();
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
 //       af_set_priority(osPriorityAboveNormal);

        //bt_syncerr set to default(0x07)
 //       BTDIGITAL_REG_SET_FIELD(REG_BTCORE_BASE_ADDR, 0x0f, 0, 0x07);
    }

    isRun=on;
    return 0;
}

APP_BT_STREAM_T gStreamplayer = APP_BT_STREAM_NUM;
int app_bt_stream_open(APP_AUDIO_STATUS* status)
{
    int nRet = -1;
    APP_BT_STREAM_T player = (APP_BT_STREAM_T)status->id;
    enum APP_SYSFREQ_FREQ_T freq = (enum APP_SYSFREQ_FREQ_T)status->freq;

    TRACE("app_bt_stream_open prev:%d cur:%d freq:%d", gStreamplayer, player, freq);

    if (gStreamplayer != APP_BT_STREAM_NUM){
        TRACE("Close prev bt stream before opening");
        nRet = app_bt_stream_close(gStreamplayer);
        if (nRet)
            return -1;
    }

    switch (player) {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            nRet = bt_sco_player(true, freq);
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            nRet = bt_sbc_player(PLAYER_OPER_START, freq);
            break;
#ifdef __FACTORY_MODE_SUPPORT__
        case APP_FACTORYMODE_AUDIO_LOOP:
            nRet = app_factorymode_audioloop(true, freq);
            break;
#endif
   #ifdef MEDIA_PLAYER_SUPPORT
        case APP_PLAY_BACK_AUDIO:
            nRet = app_play_audio_onoff(true, status);
            break;
    #endif
        default:
            nRet = -1;
            break;
    }

    if (!nRet)
        gStreamplayer = player;
#if MUTE_CTL_EN
	set_app_timer(TIMER_TYPE_SINGLE_SHOT,PA_DLY_TIME,spk_unmute);
#endif

    return nRet;
}

int app_bt_stream_close(APP_BT_STREAM_T player)
{
    int nRet = -1;
    TRACE("app_bt_stream_close prev:%d cur:%d", gStreamplayer, player);
//  osDelay(1);
#if MUTE_CTL_EN
	spk_mute();
#endif

    if (gStreamplayer != player)
        return -1;

    switch (player) {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            nRet = bt_sco_player(false, APP_SYSFREQ_32K);
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            nRet = bt_sbc_player(PLAYER_OPER_STOP, APP_SYSFREQ_32K);
            break;
#ifdef __FACTORY_MODE_SUPPORT__
        case APP_FACTORYMODE_AUDIO_LOOP:
            nRet = app_factorymode_audioloop(false, APP_SYSFREQ_32K);
            break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
       case APP_PLAY_BACK_AUDIO:
            nRet = app_play_audio_onoff(false, NULL);
            break;
#endif
        default:
            nRet = -1;
            break;
    }
    if (!nRet)
        gStreamplayer = APP_BT_STREAM_NUM;
    return nRet;
}

int app_bt_stream_setup(APP_BT_STREAM_T player, uint8_t status)
{
    int nRet = -1;

    TRACE("app_bt_stream_setup prev:%d cur:%d sample:%d", gStreamplayer, player, status);

    switch (player) {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            bt_sbc_player_setup(status);
            break;
        default:
            nRet = -1;
            break;
    }

    return nRet;
}

int app_bt_stream_restart(APP_AUDIO_STATUS* status)
{
    int nRet = -1;
    APP_BT_STREAM_T player = (APP_BT_STREAM_T)status->id;
    enum APP_SYSFREQ_FREQ_T freq = (enum APP_SYSFREQ_FREQ_T)status->freq;

    TRACE("app_bt_stream_restart prev:%d cur:%d freq:%d", gStreamplayer, player, freq);

    if (gStreamplayer != player)
        return -1;

    switch (player) {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            nRet = bt_sbc_player(PLAYER_OPER_RESTART, freq);
            break;
#ifdef __FACTORY_MODE_SUPPORT__
        case APP_FACTORYMODE_AUDIO_LOOP:
            break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
        case APP_PLAY_BACK_AUDIO:
            break;
#endif
        default:
            nRet = -1;
            break;
    }

    return nRet;
}
extern 	int app_volume_save(uint8_t volume);
void app_bt_stream_volumeup(void)
{
    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC))
    {
        TRACE("%s set audio volume", __func__);
        btdevice_volume_p->a2dp_vol++; 
        app_bt_stream_volumeset(btdevice_volume_p->a2dp_vol);   
    }

    if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM))
    {
        TRACE("%s set hfp volume", __func__);
        btdevice_volume_p->hfp_vol++;
        app_bt_stream_volumeset(btdevice_volume_p->hfp_vol);   
    }

    if (app_bt_stream_isrun(APP_BT_STREAM_NUM))
    {
        TRACE("%s set idle volume", __func__);
        btdevice_volume_p->a2dp_vol++;
    }

    if (btdevice_volume_p->a2dp_vol > TGT_VOLUME_LEVEL_15)
    {
        btdevice_volume_p->a2dp_vol = TGT_VOLUME_LEVEL_15;
#ifdef MEDIA_PLAYER_SUPPORT
     //   media_PlayAudio(AUD_ID_BT_WARNING,0);
     app_voice_report(APP_STATUS_INDICATION_MAXVOL,0);
#endif
    }

    if (btdevice_volume_p->hfp_vol > TGT_VOLUME_LEVEL_15)
    {
        btdevice_volume_p->hfp_vol = TGT_VOLUME_LEVEL_15;
#ifdef MEDIA_PLAYER_SUPPORT
	  app_voice_report(APP_STATUS_INDICATION_MAXVOL,0);
#endif
    }
    #ifndef FPGA
    nv_record_touch_cause_flush();
    #endif
	app_volume_save(btdevice_volume_p->a2dp_vol);
    TRACE("%s a2dp: %d", __func__, btdevice_volume_p->a2dp_vol);
    TRACE("%s hfp: %d", __func__, btdevice_volume_p->hfp_vol);
}

void app_bt_stream_volumedown(void)
{
    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC))
    {
        TRACE("%s set audio volume", __func__);
        btdevice_volume_p->a2dp_vol--; 
        app_bt_stream_volumeset(btdevice_volume_p->a2dp_vol);   
    }

    if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM))
    {
        TRACE("%s set hfp volume", __func__);
        btdevice_volume_p->hfp_vol--;
        app_bt_stream_volumeset(btdevice_volume_p->hfp_vol);   
    }

    if (app_bt_stream_isrun(APP_BT_STREAM_NUM))
    {
        TRACE("%s set idle volume", __func__);
        btdevice_volume_p->a2dp_vol--;
    }

    if (btdevice_volume_p->a2dp_vol < TGT_VOLUME_LEVEL_0)
    {
        btdevice_volume_p->a2dp_vol = TGT_VOLUME_LEVEL_0;
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_BT_WARNING,0);
#endif
    }

    if (btdevice_volume_p->hfp_vol < TGT_VOLUME_LEVEL_0)
    {
        btdevice_volume_p->hfp_vol = TGT_VOLUME_LEVEL_0;
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_BT_WARNING,0);
#endif
    }
    #ifndef FPGA
    nv_record_touch_cause_flush();
    #endif
	app_volume_save(btdevice_volume_p->a2dp_vol);

    TRACE("%s a2dp: %d", __func__, btdevice_volume_p->a2dp_vol);
    TRACE("%s hfp: %d", __func__, btdevice_volume_p->hfp_vol);
}

int app_bt_stream_volumeset(int8_t vol)
{
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    TRACE("app_bt_stream_volumeset vol=%d", vol);

    if (vol > TGT_VOLUME_LEVEL_15)
        vol = TGT_VOLUME_LEVEL_15;
    if (vol < TGT_VOLUME_LEVEL_0)
        vol = TGT_VOLUME_LEVEL_0;

    stream_local_volume = vol;
    if (!app_bt_stream_isrun(APP_PLAY_BACK_AUDIO)){
        af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
        stream_cfg->vol = vol;
        af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
    }
    return 0;
}

int app_bt_stream_local_volume_get(void)
{
    return stream_local_volume;
}

void app_bt_stream_a2dpvolume_reset(void)
{
     btdevice_volume_p->a2dp_vol = NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT ;
}

void app_bt_stream_hfpvolume_reset(void)
{
     btdevice_volume_p->hfp_vol = NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT;
}

void app_bt_stream_volume_ptr_update(uint8_t *bdAddr)
{
    static struct btdevice_volume stream_volume = {NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT,NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT};
	
    nvrec_btdevicerecord *record = NULL;

#ifndef FPGA

    if (!nv_record_btdevicerecord_find((BT_BD_ADDR *)bdAddr,&record))
		{
        btdevice_volume_p = &(record->device_vol);
        DUMP8("0x%02x ", bdAddr, BD_ADDR_SIZE);
        TRACE("%s a2dp_vol:%d hfp_vol:%d ptr:0x%x", __func__, btdevice_volume_p->a2dp_vol, btdevice_volume_p->hfp_vol,btdevice_volume_p);
    }else
#endif
    {
        btdevice_volume_p = &stream_volume;
        TRACE("%s default", __func__);
    }
}

struct btdevice_volume * app_bt_stream_volume_get_ptr(void)
{
    return btdevice_volume_p;
}

bool app_bt_stream_isrun(APP_BT_STREAM_T player)
{

    if (gStreamplayer == player)
        return true;
    else
        return false;
}

int app_bt_stream_closeall()
{
    TRACE("app_bt_stream_closeall");

    bt_sco_player(false, APP_SYSFREQ_32K);
    bt_sbc_player(PLAYER_OPER_STOP, APP_SYSFREQ_32K);

    #ifdef MEDIA_PLAYER_SUPPORT
    app_play_audio_onoff(false, NULL);
    #endif
    gStreamplayer = APP_BT_STREAM_NUM;
    return 0;
}

void app_bt_stream_copy_track_one_to_two_16bits(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; i++) {
        //dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = ((unsigned short)(src_buf[i])<<1);
        dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = src_buf[i];
    }
}

