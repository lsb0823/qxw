#ifndef __RES_AUDIO_EQ_H__
#define __RES_AUDIO_EQ_H__

#ifdef __SW_IIR_EQ_PROCESS__
const IIR_CFG_T audio_eq_iir_cfg = {
    .gain0 = -2,
    .gain1 = -2,
    .num = 4,
    .param = {
        {1.2,   50.0,   0.7},
        {2.0,   150.0,  0.7},
        {1.5,   300.0, 0.7},
        {1.0,   2500.0, 0.3},
        //{12.0,   8000.0, 0.7}    
    }
};
#endif

#ifdef __HW_FIR_EQ_PROCESS__
const FIR_CFG_T audio_eq_fir_cfg = {
    .gain0 = 6,
    .gain1 = 6,
    .len = 128,
    .coef = {
        #include "res/eq/EQ_COEF.txt"
    }
};
#endif

#ifdef __cplusplus
	extern "C" {
#endif
		
#ifdef __cplusplus
	}
#endif
#endif
	
