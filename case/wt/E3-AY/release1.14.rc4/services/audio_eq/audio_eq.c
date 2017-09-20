/*******************************************************************************
** namer：	Audio_eq
** description:	eq
** version：V0.1
** author： yifengxiao
** modify：	2016.8.09
** todo: 	1. uart interface
**    		2. add CODEC_IRQ_USED
**          3. load coef
**          4. get buf from app_audio
*******************************************************************************/
#include "hal_timer.h"
#include "hal_trace.h"
#include "string.h"
#include "audio_eq.h"
#include "stdbool.h"

#ifdef __PC_CMD_UART__
#include "hal_cmd.h"

#ifdef __SW_IIR_EQ_PROCESS__
#define AUDIO_EQ_IIR_DYNAMIC    1
#endif

#ifdef __HW_FIR_EQ_PROCESS__
#define AUDIO_EQ_FIR_DYNAMIC    1
#endif

#endif


#if defined(AUDIO_EQ_IIR_DYNAMIC) || defined(AUDIO_EQ_FIR_DYNAMIC)
#define AUDIO_EQ_DYNAMIC        1
#endif

typedef signed int          audio_eq_sample_24bits_t;
typedef signed short int    audio_eq_sample_16bits_t;

typedef struct{
    enum AUD_BITS_T     sample_bits;
    enum AUD_SAMPRATE_T sample_rate;

#ifdef AUDIO_EQ_DYNAMIC
    bool update_cfg;
#endif

#ifdef AUDIO_EQ_IIR_DYNAMIC
    IIR_CFG_T iir_cfg;
#endif

#ifdef AUDIO_EQ_FIR_DYNAMIC
    FIR_CFG_T fir_cfg;
#endif
} AUDIO_EQ_T;

static AUDIO_EQ_T audio_eq;

#ifdef AUDIO_EQ_DYNAMIC
static int audio_eq_update_cfg_enable(bool en)
{
    audio_eq.update_cfg = en;

    return 0;
}

static int audio_eq_update_cfg(void)
{
#ifdef AUDIO_EQ_IIR_DYNAMIC
    iir_set_cfg(&audio_eq.iir_cfg);
#endif

#ifdef AUDIO_EQ_FIR_DYNAMIC
    fir_set_cfg(&audio_eq.fir_cfg);
#endif

    audio_eq_update_cfg_enable(false);

    return 0;
}
#endif

#ifdef __SW_IIR_EQ_PROCESS__
int audio_eq_iir_set_cfg(const IIR_CFG_T *cfg)
{
#ifdef AUDIO_EQ_IIR_DYNAMIC
    memcpy(&audio_eq.iir_cfg, cfg, sizeof(audio_eq.iir_cfg));
    audio_eq_update_cfg_enable(true);
#else
    iir_set_cfg(cfg);
#endif
    return 0;
}

int audio_eq_iir_set_gain_and_num(float gain0, float gain1, int num)
{
    iir_set_gain_and_num(gain0, gain1, num);

    return 0;
}

int audio_eq_iir_set_param(uint8_t index, const IIR_PARAM_T *param)
{
    iir_set_param(index, param);

    return 0;
}

int audio_eq_iir_set_coef(uint8_t index, const float *a, const float *b)
{
    iir_set_coef(index, a, b);

    return 0;
}
#endif

#ifdef __HW_FIR_EQ_PROCESS__
int audio_eq_fir_set_cfg(const FIR_CFG_T *cfg)
{
#ifdef AUDIO_EQ_FIR_DYNAMIC
    memcpy(&audio_eq.fir_cfg, cfg, sizeof(audio_eq.fir_cfg));
    audio_eq_update_cfg_enable(true);
#else
    fir_set_cfg(cfg);
#endif
    return 0;
}
#endif

int audio_eq_run(uint8_t *buf, uint32_t len)
{  
    int eq_len = 0;

    if(audio_eq.sample_bits == AUD_BITS_16)
    {
        eq_len = len/sizeof(audio_eq_sample_16bits_t); 
    } 
    else if(audio_eq.sample_bits == AUD_BITS_24)
    {
        eq_len = len/sizeof(audio_eq_sample_24bits_t); 
    }
    else
    {
        ASSERT(0, "[%s] bits(%s) is invalid", __func__, audio_eq.sample_bits);
    }

#ifdef __PC_CMD_UART__
    hal_cmd_run();
#endif

#ifdef AUDIO_EQ_DYNAMIC
    if(audio_eq.update_cfg)
    {
        audio_eq_update_cfg();
    }
#endif

//    int32_t s_time,e_time;
//    s_time = hal_sys_timer_get();

#ifdef __SW_IIR_EQ_PROCESS__
    iir_run(buf, eq_len);
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    fir_run(buf, eq_len);
#endif

#ifndef __HW_FIR_EQ_PROCESS__
    if(audio_eq.sample_bits == AUD_BITS_24)
    {
        audio_eq_sample_24bits_t *eq_buf = (audio_eq_sample_24bits_t *)buf;
        for (int i = 0; i < eq_len; i++)
        {
            eq_buf[i] = eq_buf[i]>>6;    // fir: 16bit
        } 
    }
#endif
//    e_time = hal_sys_timer_get();
//    TRACE("[%s] Sample len = %d, used time = %d ticks", __func__, eq_len, (e_time - s_time));
    return 0;
}

int audio_eq_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_BITS_T sample_bits)
{
    audio_eq.sample_rate = sample_rate;
    audio_eq.sample_bits = sample_bits;

#ifdef __SW_IIR_EQ_PROCESS__
    iir_open(sample_rate, sample_bits);
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    fir_open(sample_rate, sample_bits);
#endif

#ifdef __PC_CMD_UART__
    hal_cmd_open();
#endif

	return 0;
}

int audio_eq_close(void)
{
#ifdef __SW_IIR_EQ_PROCESS__
    iir_close();
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    fir_close();
#endif

#ifdef __PC_CMD_UART__
    hal_cmd_close();
#endif

    return 0;
}

#ifdef __PC_CMD_UART__
#ifdef AUDIO_EQ_IIR_DYNAMIC
int audio_eq_iir_callback(uint8_t *buf, uint32_t  len)
{
    IIR_CFG_T *rx_iir_cfg = NULL;

    rx_iir_cfg = (IIR_CFG_T *)buf;

    TRACE("[%s] left gain = %f, right gain = %f", __func__, rx_iir_cfg->gain0, rx_iir_cfg->gain1);

    for(int i=0; i<rx_iir_cfg->num; i++)
    {
        TRACE("[%s] iir[%d] gain = %f, f = %f, Q = %f", __func__, i, rx_iir_cfg->param[i].gain, rx_iir_cfg->param[i].fc, rx_iir_cfg->param[i].Q);
    }

    audio_eq_iir_set_cfg((const IIR_CFG_T *)rx_iir_cfg);

    return 0;
}
#endif

#ifdef __PC_CMD_UART__
int audio_tool_ping_callback(uint8_t *buf, uint32_t len)
{
    //TRACE("");
    return 0;
}
#endif

#ifdef AUDIO_EQ_FIR_DYNAMIC
// static void memcpy_int16_to_int16(int16_t *dest, const int16_t *src, int16_t num)
// {
//     int16_t i;
//     for (i = 0; i < num; i++){
//         dest[i] = src[i];
//     }
//     return;
// }

int audio_eq_fir_callback(uint8_t *buf, uint32_t  len)
{
    FIR_CFG_T *rx_fir_cfg = NULL;

    rx_fir_cfg = (FIR_CFG_T *)buf;

    TRACE("[%s] left gain = %d, right gain = %d", __func__, rx_fir_cfg->gain0, rx_fir_cfg->gain1);

    TRACE("[%s] len = %d, coef: %d, %d......%d, %d", __func__, rx_fir_cfg->len, rx_fir_cfg->coef[0], rx_fir_cfg->coef[1], rx_fir_cfg->coef[rx_fir_cfg->len-2], rx_fir_cfg->coef[rx_fir_cfg->len-1]);

    rx_fir_cfg->gain0 = 6;
    rx_fir_cfg->gain1 = 6;
    audio_eq_fir_set_cfg((const FIR_CFG_T *)rx_fir_cfg);

    return 0;
}
// int audio_eq_fir_callback(uint8_t *buf, uint32_t  len)
// {
//     uint32_t eq_len = 0;
//     int16_t *eq_buf = NULL;

//     eq_len = len / 2;
//     eq_buf = (int16_t *)buf;

//     TRACE("[%s] len = %d, buf = %d,%d......%d,%d", __func__, len, eq_buf[0], eq_buf[1], eq_buf[eq_len-2], eq_buf[eq_len-1]);

//     if(audio_eq.fir_cfg.len == eq_len)
//     {
//         memcpy_int16_to_int16(audio_eq.fir_cfg.coef, eq_buf, eq_len);
//         audio_eq_update_cfg_enable(true);
//         // audio_eq_fir_set_cfg(const FIR_CFG_T *cfg);
//     }
//     else
//     {
//         TRACE("[%s] audio_eq.coef_len(%d) != eq_len(%d)", __func__, audio_eq.fir_cfg.len, eq_len);
//     }

//     return 0;
// }
#endif
#endif

int audio_eq_init(void)
{
#ifdef __SW_IIR_EQ_PROCESS__
    iir_open(AUD_SAMPRATE_96000, AUD_BITS_16);
#endif

#ifdef __PC_CMD_UART__
    hal_cmd_init();
#ifdef AUDIO_EQ_IIR_DYNAMIC
    hal_cmd_register("iir_eq", audio_eq_iir_callback);
#endif

#ifdef AUDIO_EQ_FIR_DYNAMIC
    hal_cmd_register("fir_eq", audio_eq_fir_callback);
#endif
    hal_cmd_register("ping", audio_tool_ping_callback);
#endif

    return 0;
}