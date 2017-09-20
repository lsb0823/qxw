#include "mbed.h"
// Standard C Included Files
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "rtos.h"
#include "SDFileSystem.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "cqueue.h"
#include "app_audio.h"
#include "tgt_hardware.h"

//#define SPEAKER_ALGORITHMGAIN (2.0f)
#define SPEECH_ALGORITHMGAIN (4.0f)

#if defined(SPEECH_WEIGHTING_FILTER_SUPPRESS) || defined(SPEAKER_WEIGHTING_FILTER_SUPPRESS) || defined(SPEAKER_ALGORITHMGAIN) || defined(SPEECH_ALGORITHMGAIN) || defined(HFP_1_6_ENABLE)
#include "iir_process.h"
#endif

// BT
extern "C" {
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "hf.h"
#include "plc.h"
#ifdef  VOICE_DETECT
#include "webrtc_vad_ext.h"
#endif
}

//#define SPEECH_PLC


#define VOICEBTPCM_TRACE(s,...)
//TRACE(s, ##__VA_ARGS__)

/* voicebtpcm_pcm queue */
#define VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH (120)
#define VOICEBTPCM_PCM_TEMP_BUFFER_SIZE (VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH*2)
#define VOICEBTPCM_PCM_QUEUE_SIZE (VOICEBTPCM_PCM_TEMP_BUFFER_SIZE*4)
CQueue voicebtpcm_p2m_queue;
CQueue voicebtpcm_m2p_queue;

static enum APP_AUDIO_CACHE_T voicebtpcm_cache_m2p_status = APP_AUDIO_CACHE_QTY;
static enum APP_AUDIO_CACHE_T voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_QTY;

#ifdef VOICE_DETECT
#define VAD_SIZE (VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH*2)
#define VAD_CNT_MAX (30)
#define VAD_CNT_MIN (-10)
#define VAD_CNT_INCREMENT (1)
#define VAD_CNT_DECREMENT (1)

static VadInst  *g_vadmic = NULL;
static uint16_t *vad_buff;
static uint16_t vad_buf_size = 0;
static int vdt_cnt = 0;
#endif


#if defined (SPEECH_WEIGHTING_FILTER_SUPPRESS)
static IIR_MONO_CFG_T mic_cfg[] = {
                                     {1.f,
                                     {1.00000000, -1.86616866, 0.87658642,
                                     0.94810458, -1.87117017, 0.92348033},
                                     {0.f, 0.f, 0.f}},
                                     {1.f,
                                     {1.00000000, -0.73499401, 0.61896342, 
                                     0.88532841, -0.73499401, 0.73363501},
                                     {0.f, 0.f, 0.f}},
                                     {1.f,
                                     {1.00000000, -1.04498871, 0.68793235, 
                                     0.92216834, -1.04498871, 0.76576402},
                                     {0.f, 0.f, 0.f}}
                                    };
/*
HP
a= 1.00000000, -1.86616866, 0.87658642, b= 0.94810458, -1.87117017, 0.92348033, Q=8.000000e-01, f0=60
a= 1.00000000, -1.79827159, 0.82094837, b= 0.92289343, -1.80915859, 0.88716794, Q=8.000000e-01, f0=90
a= 1.00000000, -1.73010321, 0.76910598, b= 0.89822518, -1.74882823, 0.85215577, Q=8.000000e-01, f0=120
a= 1.00000000, -1.66190643, 0.72086961, b= 0.87412777, -1.69021434, 0.81843393, Q=8.000000e-01, f0=150 
a= 1.00000000, -1.54874191, 0.64797877, b= 0.83528658, -1.59638499, 0.76504911, Q=8.000000e-01, f0=200
a= 1.00000000, -1.43687641, 0.58373222, b= 0.79814194, -1.50738110, 0.71508559, Q=8.000000e-01, f0=250 
a= 1.00000000, -1.37064339, 0.54899424, b= 0.77667822, -1.45626868, 0.68669074, Q=8.000000e-01, f0=280 
a= 1.00000000, -1.32692505, 0.52731913, b= 0.76271162, -1.42313318, 0.66839939, Q=8.000000e-01, f0=300 
a= 1.00000000, -1.11447076, 0.43500626, b= 0.69692498, -1.26835814, 0.58419391, Q=8.000000e-01, f0=400 
a= 1.00000000, -0.91365056, 0.36563348, b= 0.63760583, -1.13064514, 0.51103307, Q=8.000000e-01, f0=500 
a= 1.00000000, -0.72537664, 0.31470058, b= 0.58425226, -1.00830791, 0.44751705, Q=8.000000e-01, f0=600
a= 1.00000000, -0.54971399, 0.27854201, b= 0.53629269, -0.89962041, 0.39234289, Q=8.000000e-01, f0=700 
a= 1.00000000, -0.38622682, 0.25421117, b= 0.49315048, -0.80294146, 0.34434605, Q=8.000000e-01, f0=800 
*/
#endif

#if defined (SPEAKER_WEIGHTING_FILTER_SUPPRESS)
static IIR_MONO_CFG_T spk_cfg[] = {1,
                         {1.000000000000000, -1.849861221773838, 0.860188009219491,
                          0.952640907278933, -1.854374582739788, 0.903033740974608},
                         {0.f, 0.f, 0.f}};
#endif

/*
{1.f,
 {1.000000000000000, -1.749673879263390, 0.849865759304125,
  0.902579925106905, -1.799706752231840, 0.897252961228774},
 {0.f, 0.f, 0.f}};
*/

static void copy_one_track_to_two_track_16bits(uint16_t *src_buf, uint16_t *dst_buf, uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; ++i) {
        //dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = ((unsigned short)(src_buf[i])<<1);
        dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = (src_buf[i]);
    }
}
void merge_two_track_to_one_track_16bits(uint8_t chnlsel, uint16_t *src_buf, uint16_t *dst_buf,  uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; i+=2) {
        dst_buf[i/2] = src_buf[i + chnlsel];
    }
}

//playback flow
//bt-->store_voicebtpcm_m2p_buffer-->decode_voicebtpcm_m2p_frame-->audioflinger playback-->speaker
//used by playback, store data from bt to memory
int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len)
{
    int size;
    unsigned int avail_size;

    LOCK_APP_AUDIO_QUEUE();
    avail_size = APP_AUDIO_AvailableOfCQueue(&voicebtpcm_m2p_queue);
    if (len <= avail_size){
        APP_AUDIO_EnCQueue(&voicebtpcm_m2p_queue, buf, len);
    }else{
        VOICEBTPCM_TRACE( "spk buff overflow %d/%d", len, avail_size);
        APP_AUDIO_DeCQueue(&voicebtpcm_m2p_queue, 0, len - avail_size);
        APP_AUDIO_EnCQueue(&voicebtpcm_m2p_queue, buf, len);
    }
    size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_m2p_queue);
    UNLOCK_APP_AUDIO_QUEUE();

    if (size > (VOICEBTPCM_PCM_QUEUE_SIZE/2)) {
        voicebtpcm_cache_m2p_status = APP_AUDIO_CACHE_OK;
    }

    VOICEBTPCM_TRACE("m2p :%d/%d", len, size);

    return 0;
}

//used by playback, decode data from memory to speaker
int decode_voicebtpcm_m2p_frame(unsigned char *pcm_buffer, unsigned int pcm_len)
{
    int r = 0, got_len = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    LOCK_APP_AUDIO_QUEUE();
    r = APP_AUDIO_PeekCQueue(&voicebtpcm_m2p_queue, pcm_len - got_len, &e1, &len1, &e2, &len2);
    UNLOCK_APP_AUDIO_QUEUE();

    if (r){
        VOICEBTPCM_TRACE( "spk buff underflow");
    }

    if(r == CQ_OK) {
        //app_audio_memcpy_16bit(pcm_buffer + got_len, e1, len1/2);
        if (len1){
//            copy_one_trace_to_two_track_16bits((uint16_t *)e1, (uint16_t *)(pcm_buffer + got_len), len1/2);
            app_audio_memcpy_16bit((short *)(pcm_buffer), (short *)e1, len1/2);
            LOCK_APP_AUDIO_QUEUE();
            APP_AUDIO_DeCQueue(&voicebtpcm_m2p_queue, 0, len1);
            UNLOCK_APP_AUDIO_QUEUE();
            got_len += len1;
        }
        if (len2) {
            //app_audio_memcpy_16bit(pcm_buffer + got_len, e2, len2);
//            copy_one_trace_to_two_track_16bits((uint16_t *)e2, (uint16_t *)(pcm_buffer + got_len), len2/2);
            app_audio_memcpy_16bit((short *)(pcm_buffer + len1), (short *)e2, len2/2);
            LOCK_APP_AUDIO_QUEUE();
            APP_AUDIO_DeCQueue(&voicebtpcm_m2p_queue, 0, len2);
            UNLOCK_APP_AUDIO_QUEUE();
            got_len += len2;
        }
    }

    return got_len;
}

//capture flow
//mic-->audioflinger capture-->store_voicebtpcm_p2m_buffer-->get_voicebtpcm_p2m_frame-->bt
//used by capture, store data from mic to memory
int store_voicebtpcm_p2m_buffer(unsigned char *buf, unsigned int len)
{
    int size;
    unsigned int avail_size = 0;
    LOCK_APP_AUDIO_QUEUE();
//    merge_two_trace_to_one_track_16bits(0, (uint16_t *)buf, (uint16_t *)buf, len>>1);
//    r = APP_AUDIO_EnCQueue(&voicebtpcm_p2m_queue, buf, len>>1);
    avail_size = APP_AUDIO_AvailableOfCQueue(&voicebtpcm_p2m_queue);
    if (len <= avail_size){
        APP_AUDIO_EnCQueue(&voicebtpcm_p2m_queue, buf, len);
    }else{
        VOICEBTPCM_TRACE( "mic buff overflow %d/%d", len, avail_size);
        APP_AUDIO_DeCQueue(&voicebtpcm_p2m_queue, 0, len - avail_size);
        APP_AUDIO_EnCQueue(&voicebtpcm_p2m_queue, buf, len);
    }
    size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue);
    UNLOCK_APP_AUDIO_QUEUE();

    VOICEBTPCM_TRACE("p2m :%d/%d", len, size);

    return 0;
}

//used by capture, get the memory data which has be stored by store_voicebtpcm_p2m_buffer()
int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len)
{
    int r = 0, got_len = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
//    int size;

    if (voicebtpcm_cache_p2m_status == APP_AUDIO_CACHE_CACHEING){
        app_audio_memset_16bit((short *)buf, 0, len/2);
        got_len = len;
    }else{
        LOCK_APP_AUDIO_QUEUE();
//        size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue);
        r = APP_AUDIO_PeekCQueue(&voicebtpcm_p2m_queue, len - got_len, &e1, &len1, &e2, &len2);
        UNLOCK_APP_AUDIO_QUEUE();

//        VOICEBTPCM_TRACE("p2m :%d/%d", len, APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue));

        if(r == CQ_OK) {
            if (len1){
                app_audio_memcpy_16bit((short *)buf, (short *)e1, len1/2);
                LOCK_APP_AUDIO_QUEUE();
                APP_AUDIO_DeCQueue(&voicebtpcm_p2m_queue, 0, len1);
                UNLOCK_APP_AUDIO_QUEUE();
                got_len += len1;
            }
            if (len2 != 0) {
                app_audio_memcpy_16bit((short *)(buf+got_len), (short *)e2, len2/2);
                got_len += len2;
                LOCK_APP_AUDIO_QUEUE();
                APP_AUDIO_DeCQueue(&voicebtpcm_p2m_queue, 0, len2);
                UNLOCK_APP_AUDIO_QUEUE();
            }
        }else{
            VOICEBTPCM_TRACE( "mic buff underflow");
            app_audio_memset_16bit((short *)buf, 0, len/2);
            got_len = len;
            voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_CACHEING;
        }
    }

    return got_len;
}

#if defined (SPEECH_PLC)
static struct PlcSt *speech_lc;
#endif

#if defined( SPEECH_ECHO_CANCEL ) || defined( SPEECH_NOISE_SUPPRESS ) || defined(SPEAKER_NOISE_SUPPRESS )

#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"

#define SPEEX_BUFF_SIZE (VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH)

static short *e_buf;

#if defined( SPEECH_ECHO_CANCEL )
static SpeexEchoState *st=NULL;
static short *ref_buf;
static short *echo_buf;
#endif

#if defined( SPEECH_NOISE_SUPPRESS )
static SpeexPreprocessState *den=NULL;
#endif

#if defined( SPEAKER_NOISE_SUPPRESS )
static SpeexPreprocessState *den_voice = NULL;
#endif

#endif

#if 0
void get_mic_data_max(short *buf, uint32_t len)
{
    int max0 = -32768, min0 = 32767, diff0 = 0;
    int max1 = -32768, min1 = 32767, diff1 = 0;

    for(uint32_t i=0; i<len/2;i+=2)
    {
        if(buf[i+0]>max0)
        {
            max0 = buf[i+0];
        }

        if(buf[i+0]<min0)
        {
            min0 = buf[i+0];
        }

        if(buf[i+1]>max1)
        {
            max1 = buf[i+1];
        }

        if(buf[i+1]<min1)
        {
            min1 = buf[i+1];
        }
    }
    TRACE("min0 = %d, max0 = %d, diff0 = %d, min1 = %d, max1 = %d, diff1 = %d", min0, max0, max0 - min0, min1, max1, max1 - min1);
}
#endif

#ifdef VOICE_DETECT
int16_t voicebtpcm_vdt(uint8_t *buf, uint32_t len)
{
        int16_t ret = 0;
        uint16_t buf_size = 0;
        uint16_t *p_buf = NULL;
         if(len == 120){
            	vad_buf_size += VAD_SIZE/2; //VAD BUFF IS UINT16
            	if(vad_buf_size < VAD_SIZE){
            		app_audio_memcpy_16bit((short *)vad_buff , buf, VAD_SIZE/2);
            		return ret;
            	}
                	app_audio_memcpy_16bit((short *)(vad_buff + VAD_SIZE), buf, VAD_SIZE/2);
                 p_buf = vad_buff;
                 buf_size = VAD_SIZE;
         }else if(len == 160){
                 p_buf = (uint16_t *)buf;
                 buf_size = VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH;
         }else
                 return 0;

	ret = WebRtcVad_Process((VadInst*)g_vadmic, 8000, (WebRtc_Word16 *)p_buf, buf_size);
	vad_buf_size = 0;
         return ret;
}
#endif	

//used by capture, store data from mic to memory
uint32_t voicebtpcm_pcm_audio_data_come(uint8_t *buf, uint32_t len)
{
	int16_t ret = 0;
	bool vdt = false;
//	int32_t loudness,gain,max1,max2;	
//    uint32_t stime, etime;
//    static uint32_t preIrqTime = 0;

//  VOICEBTPCM_TRACE("%s enter", __func__);
//    stime = hal_sys_timer_get();
    //get_mic_data_max((short *)buf, len);
    int size = 0;

#if defined( SPEECH_ECHO_CANCEL ) || defined( SPEECH_NOISE_SUPPRESS )
    {

 //       int i;
        short *buf_p=(short *)buf;

//        uint32_t lock = int_lock();
#if defined (SPEECH_ECHO_CANCEL)
    //       for(i=0;i<120;i++)
    //       {
    //           ref_buf[i]=buf_p[i];
    //       }
 //       app_audio_memcpy_16bit((short *)ref_buf, buf_p, SPEEX_BUFF_SIZE);
        speex_echo_cancellation(st, buf_p, echo_buf, e_buf);
//#else
    //        for(i=0;i<120;i++)
    //        {
    //            e_buf[i]=buf_p[i];
    //        }
//        app_audio_memcpy_16bit((short *)e_buf, buf_p, SPEEX_BUFF_SIZE);
#else
    e_buf = buf_p;
#endif
#ifdef VOICE_DETECT
    ret = voicebtpcm_vdt((uint8_t*)e_buf,  len);
	if(ret){
	if(vdt_cnt < 0)
		vdt_cnt = 3;
	else
    		vdt_cnt += ret*VAD_CNT_INCREMENT*3;
    }else{
        // not detect voice
    	vdt_cnt -=VAD_CNT_DECREMENT;
    }
	if (vdt_cnt > VAD_CNT_MAX)
		vdt_cnt = VAD_CNT_MAX;
	if (vdt_cnt < VAD_CNT_MIN)
		vdt_cnt = VAD_CNT_MIN;
	if (vdt_cnt > 0)
		vdt =  true;
#endif
#if defined (SPEECH_NOISE_SUPPRESS)
#ifdef VOICE_DETECT
		float gain;
		if (vdt){
			gain = 31.f;
			speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC_FIX_GAIN, &gain);
		}else{
			gain = 1.f;
			speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC_FIX_GAIN, &gain);
		}
#endif
#ifdef NOISE_SUPPRESS_ADAPTATION
{
    int32_t l2;
    float gain;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_GET_AGC_GAIN_RAW, &gain);
    if (gain>3.f){        
        l2 = -21;
        speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &l2);
    }else{  
        l2 = -9;
        speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &l2);
    }
}
#endif
    speex_preprocess_run(den, e_buf);
#endif
//        int_unlock(lock);
    //        for(i=0;i<120;i++)
    //        {
    //            buf_p[i]=e_buf[i];
    //        }
    //    app_audio_memcpy_16bit((short *)buf_p, e_buf, sizeof(e_buf)/2);


#if defined (SPEECH_WEIGHTING_FILTER_SUPPRESS)
        for (uint8_t i=0; i<sizeof(mic_cfg)/sizeof(IIR_MONO_CFG_T);i++){
            iir_run_mono_16bits(&mic_cfg[i], (int16_t *)e_buf, len/2);
        }
#endif
#if defined (SPEECH_ALGORITHMGAIN)
        iir_run_mono_algorithmgain(SPEECH_ALGORITHMGAIN, (int16_t *)e_buf, len/2);
#endif
        LOCK_APP_AUDIO_QUEUE();
        store_voicebtpcm_p2m_buffer((unsigned char *)e_buf, SPEEX_BUFF_SIZE<<1);
        size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue);
        UNLOCK_APP_AUDIO_QUEUE();
    }
#else
//       etime = hal_sys_timer_get();
//  VOICEBTPCM_TRACE("\n******total Spend:%d Len:%dn", TICKS_TO_MS(etime - stime), len);
#if defined (SPEECH_WEIGHTING_FILTER_SUPPRESS)
    for (uint8_t i=0; i<sizeof(mic_cfg)/sizeof(IIR_MONO_CFG_T);i++){
        iir_run_mono_16bits(&mic_cfg[i], (int16_t *)buf, len/2);
    }
#endif
#if defined (SPEECH_ALGORITHMGAIN)
    iir_run_mono_algorithmgain(SPEECH_ALGORITHMGAIN, (int16_t *)buf, len/2);
#endif
    LOCK_APP_AUDIO_QUEUE();
    store_voicebtpcm_p2m_buffer(buf, len);
    size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue);
    UNLOCK_APP_AUDIO_QUEUE();
#endif

    if (size > (VOICEBTPCM_PCM_QUEUE_SIZE/2)) {
        voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_OK;
    }

//    preIrqTime = stime;

    return len;
}


//used by playback, play data from memory to speaker
uint32_t voicebtpcm_pcm_audio_more_data(uint8_t *buf, uint32_t len)
{
    uint32_t l = 0;
//    uint32_t stime, etime;
//    static uint32_t preIrqTime = 0;

    if (voicebtpcm_cache_m2p_status == APP_AUDIO_CACHE_CACHEING){
        app_audio_memset_16bit((short *)buf, 0, len/2);
        l = len;
    }else{
#if defined (SPEECH_ECHO_CANCEL)
        {
//            int i;
        short *buf_p=(short *)buf;
//            for(i=0;i<120;i++)
//            {
//                echo_buf[i]=buf_p[i];
//            }
        app_audio_memcpy_16bit((short *)echo_buf, buf_p, SPEEX_BUFF_SIZE);

        }
#endif

/*
         for(int i=0;i<120;i=i+2)
            {
               short *buf_p=(short *)buf;
                TRACE("%5d,%5d,", buf_p[i],buf_p[i+1]);
            hal_sys_timer_delay(2);
            }
*/


// stime = hal_sys_timer_get();
    l = decode_voicebtpcm_m2p_frame(buf, len);
    if (l != len){
        app_audio_memset_16bit((short *)buf, 0, len/2);
        voicebtpcm_cache_m2p_status = APP_AUDIO_CACHE_CACHEING;
    }

#if defined (SPEECH_PLC)
//stime = hal_sys_timer_get();
    speech_plc(speech_lc,(short *)buf,len);
//etime = hal_sys_timer_get();
//TRACE( "plc cal ticks:%d", etime-stime);
#endif

#ifdef AUDIO_OUTPUT_LR_BALANCE
    app_audio_lr_balance(buf, len, AUDIO_OUTPUT_LR_BALANCE);
#endif


    //    etime = hal_sys_timer_get();
    //  VOICEBTPCM_TRACE("%s irqDur:%03d Spend:%03d Len:%d ok:%d", __func__, TICKS_TO_MS(stime - preIrqTime), TICKS_TO_MS(etime - stime), len>>1, voicebtpcm_cache_m2p_status);

    //    preIrqTime = stime;
    }
#if defined (SPEAKER_WEIGHTING_FILTER_SUPPRESS)    
    for (uint8_t i=0; i<sizeof(spk_cfg)/sizeof(IIR_MONO_CFG_T);i++){
        iir_run_mono_16bits(&spk_cfg[i], (int16_t *)buf, len/2);
    }
#endif

#if defined (SPEAKER_ALGORITHMGAIN)
    iir_run_mono_algorithmgain(SPEAKER_ALGORITHMGAIN, (int16_t *)buf, len/2);
#endif

#if defined ( SPEAKER_NOISE_SUPPRESS )
    speex_preprocess_run(den_voice, (short *)buf);
#endif
    return l;
}

void* speex_get_ext_buff(int size)
{
    uint8_t *pBuff = NULL;
    if (size % 4){
        size = size + (4 - size % 4);
    }
    app_audio_mempool_get_buff(&pBuff, size);
    VOICEBTPCM_TRACE( "speex_get_ext_buff len:%d", size);
    return (void*)pBuff;
}

int voicebtpcm_pcm_audio_init(void)
{
    uint8_t *p2m_buff = NULL;
    uint8_t *m2p_buff = NULL;

#if defined( SPEECH_NOISE_SUPPRESS ) || defined( SPEAKER_NOISE_SUPPRESS )
    float gain, l;
    spx_int32_t l2;
    spx_int32_t max_gain;
#endif

    app_audio_mempool_get_buff(&p2m_buff, VOICEBTPCM_PCM_QUEUE_SIZE);
    app_audio_mempool_get_buff(&m2p_buff, VOICEBTPCM_PCM_QUEUE_SIZE);

    LOCK_APP_AUDIO_QUEUE();
    /* voicebtpcm_pcm queue*/
    APP_AUDIO_InitCQueue(&voicebtpcm_p2m_queue, VOICEBTPCM_PCM_QUEUE_SIZE, p2m_buff);
    APP_AUDIO_InitCQueue(&voicebtpcm_m2p_queue, VOICEBTPCM_PCM_QUEUE_SIZE, m2p_buff);
    UNLOCK_APP_AUDIO_QUEUE();

    voicebtpcm_cache_m2p_status = APP_AUDIO_CACHE_CACHEING;
    voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_CACHEING;
#if defined( SPEECH_ECHO_CANCEL ) || defined( SPEECH_NOISE_SUPPRESS ) || defined ( SPEAKER_NOISE_SUPPRESS )
    {
       #define NN VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH
       #define TAIL VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH
       int sampleRate = 8000;
       int leak_estimate = 16383;//Better 32767/2^n

#if defined( SPEECH_ECHO_CANCEL )
       e_buf = (short *)speex_get_ext_buff(SPEEX_BUFF_SIZE<<1);
       ref_buf = (short *)speex_get_ext_buff(SPEEX_BUFF_SIZE<<1);
       echo_buf = (short *)speex_get_ext_buff(SPEEX_BUFF_SIZE<<1);
       st = speex_echo_state_init(NN, TAIL, speex_get_ext_buff);
       speex_echo_ctl(st, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);
       speex_echo_ctl(st, SPEEX_ECHO_SET_FIXED_LEAK_ESTIMATE, &leak_estimate);
#endif

#if defined( SPEECH_NOISE_SUPPRESS )
       den = speex_preprocess_state_init(NN, sampleRate, speex_get_ext_buff);
#endif

#if defined ( SPEAKER_NOISE_SUPPRESS )
       den_voice = speex_preprocess_state_init(NN, sampleRate, speex_get_ext_buff);
#endif

#if defined( SPEECH_ECHO_CANCEL ) && defined( SPEECH_NOISE_SUPPRESS )
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_ECHO_STATE, st);
#endif
    }
#endif

#if defined( SPEECH_NOISE_SUPPRESS )
    l= 32000;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC_LEVEL, &l);

    l2 = 0;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC, &l2);

    gain = 0;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC_FIX_GAIN, &gain);

    max_gain = 24;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &max_gain);
    max_gain /=2;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_INIT_MAX, &max_gain);
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_VOC_DEC_RST_GAIN, &max_gain);

    l2 = -12;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &l2);

#endif

#if defined( SPEECH_ECHO_CANCEL )
    l2 = -39;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, &l2);

    l2 = -6;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE, &l2);
#endif

#if defined ( SPEAKER_NOISE_SUPPRESS )
    l= 32000;
    speex_preprocess_ctl(den_voice, SPEEX_PREPROCESS_SET_AGC_LEVEL, &l);

    l2 = 0;
    speex_preprocess_ctl(den_voice, SPEEX_PREPROCESS_SET_AGC, &l2);

    gain = 0;
    speex_preprocess_ctl(den_voice, SPEEX_PREPROCESS_SET_AGC_FIX_GAIN, &gain);

    max_gain = 20;
    speex_preprocess_ctl(den_voice, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &max_gain);

    l2 = -20;
    speex_preprocess_ctl(den_voice, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &l2);

    l = 0.3f;
    speex_preprocess_ctl(den_voice, SPEEX_PREPROCESS_SET_NOISE_PROB, &l);

#endif


#if defined (SPEECH_PLC)
    speech_lc = speech_plc_init(speex_get_ext_buff);
#endif

#ifdef VOICE_DETECT
    {
    	WebRtcVad_AssignSize(&vdt_cnt);
    	g_vadmic = (VadInst*)speex_get_ext_buff(vdt_cnt + 4);
    	WebRtcVad_Init((VadInst*)g_vadmic);
    	WebRtcVad_set_mode((VadInst*)g_vadmic, 2);
    	//one channel 320*2
    	vad_buff = (uint16_t *)speex_get_ext_buff(VAD_SIZE*2);
    	vad_buf_size = 0;
    	vdt_cnt = 0;
    }    
#endif	

#if defined (SPEECH_WEIGHTING_FILTER_SUPPRESS)
    for (uint8_t i=0; i<sizeof(mic_cfg)/sizeof(IIR_MONO_CFG_T);i++){
        mic_cfg[i].history[0] = 0;
        mic_cfg[i].history[1] = 0;
        mic_cfg[i].history[2] = 0;
        mic_cfg[i].history[3] = 0;
    }
#endif

#if defined (SPEAKER_WEIGHTING_FILTER_SUPPRESS)    
    for (uint8_t i=0; i<sizeof(spk_cfg)/sizeof(IIR_MONO_CFG_T);i++){
        spk_cfg[i].history[0] = 0;
        spk_cfg[i].history[1] = 0;
        spk_cfg[i].history[2] = 0;
        spk_cfg[i].history[3] = 0;
    }
#endif

    return 0;
}

