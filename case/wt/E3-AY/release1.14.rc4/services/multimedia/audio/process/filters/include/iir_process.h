#ifndef __IIR_PROCESS_H__
#define __IIR_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "hal_aud.h"

#define IIR_PARAM_NUM                       (6)

typedef struct {
    float gain;
    float fc;
    float Q;
} IIR_PARAM_T;

typedef struct {
    float   gain0;      // left gain: it is usually negative, used to prevent saturation
    float   gain1;      // right gain: it is usually negative, used to prevent saturation
    int     num;        // iir number
    IIR_PARAM_T param[IIR_PARAM_NUM];
} IIR_CFG_T;


typedef struct {
    float   gain;
    float coefs[6];
    float history[4];
} IIR_MONO_CFG_T;

int iir_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_BITS_T sample_bits);
int iir_run(uint8_t *buf, uint32_t len);
int iir_close(void);
int iir_run_mono_16bits(IIR_MONO_CFG_T *cfg, int16_t *buf, int len);
int iir_run_mono_15bits(IIR_MONO_CFG_T *cfg, int16_t *buf, int len);
int iir_run_mono_algorithmgain(float gain, int16_t *buf, int len);

/**
 * @brief      set all iir cfg
 *
 * @param[in]  cfg      
 *
 * @return      0
 */
int iir_set_cfg(const IIR_CFG_T *cfg);

/**
 * @brief      set gain and iir num, used with iir_set_param and iir_set_coef
 *
 * @param[in]  gain0  left gain: it is usually negative, used to prevent saturation
 * @param[in]  gain1  right gain: it is usually negative, used to prevent saturation
 * @param[in]  num    iir number 
 *
 * @return     0
 */
int iir_set_gain_and_num(float gain0, float gain1, int num);

/**
 * @brief      set each iir param directly, used after iir_set_gain_and_num
 *
 * @param[in]  index  The index of cascade connection iir
 * @param[in]  param  The parameter
 *
 * @return     0
 */
int iir_set_param(uint8_t index, const IIR_PARAM_T *param);

/**
 * @brief      set each iir coef directly, used after iir_set_gain_and_num  
 *
 * @param[in]  index  The index of cascade connection iir
 * @param[in]  a      denominator, a[3]
 * @param[in]  b      numerator, b[3]
 *
 * @return     0
 */
int iir_set_coef(uint8_t index, const float *a, const float *b);

#ifdef __cplusplus
}
#endif

#endif