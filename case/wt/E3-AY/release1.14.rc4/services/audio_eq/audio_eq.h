#ifndef __AUDIO_EQ_H__
#define __AUDIO_EQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "iir_process.h"
#include "fir_process.h"

int audio_eq_init(void);
int audio_eq_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_BITS_T sample_bits);
int audio_eq_run(uint8_t *buf, uint32_t len);
int audio_eq_close(void);

/**
 * @brief      set all iir cfg
 *
 * @param[in]  cfg      
 *
 * @return      0
 */
int audio_eq_iir_set_cfg(const IIR_CFG_T *cfg);

/**
 * @brief      set gain and iir num, used with iir_set_param and iir_set_coef
 *
 * @param[in]  gain0  left gain: it is usually negative, used to prevent saturation
 * @param[in]  gain1  right gain: it is usually negative, used to prevent saturation
 * @param[in]  num    iir number 
 *
 * @return     0
 */
int audio_eq_iir_set_gain_and_num(float gain0, float gain1, int num);

/**
 * @brief      set each iir param directly, used after audio_eq_iir_set_gain_and_num
 *
 * @param[in]  index  The index of cascade connection iir
 * @param[in]  param  The parameter
 *
 * @return     0
 */
int audio_eq_iir_set_param(uint8_t index, const IIR_PARAM_T *param);

/**
 * @brief      set each iir coef directly, used after audio_eq_iir_set_gain_and_num  
 *
 * @param[in]  index  The index of cascade connection iir
 * @param[in]  a      denominator, a[3]
 * @param[in]  b      numerator, b[3]
 *
 * @return     0
 */
int audio_eq_iir_set_coef(uint8_t index, const float *a, const float *b);

int audio_eq_fir_set_cfg(const FIR_CFG_T *cfg);

#ifdef __cplusplus
}
#endif

#endif