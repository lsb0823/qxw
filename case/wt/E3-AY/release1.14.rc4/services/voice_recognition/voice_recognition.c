#include "cmsis_os.h"
#include "hal_trace.h"
#include "string.h"
#include "recognition.h"
#include "voice_recognition.h"


const mfcc_vct_t mode[] = {
    {
        .flag = 0,
        .frm_num = 50,
        .mfcc_frm = {
            #include "res/mfcc_out5_Fix.txt"
        }
    },
    {
        .flag = 0,
        .frm_num = 39,
        .mfcc_frm = {
            #include "res/mfcc_out6_Fix.txt"
        }
    },    
    {
        .flag = 0,
        .frm_num = 38,
        .mfcc_frm = {
            #include "res/mfcc_out8_Fix.txt"
        }
    },
    {
        .flag = 0,
        .frm_num = 28,
        .mfcc_frm = {
            #include "res/mfcc_out9_Fix.txt"
        }
    },
    {
        .flag = 0,
        .frm_num = 56,
        .mfcc_frm = {
            #include "res/mfcc_out15_Fix.txt"
        }
    },
    {
        .flag = 0,
        .frm_num = 47,
        .mfcc_frm = {
            #include "res/mfcc_out16_Fix.txt"
        }
    },
    {
        .flag = 0,
        .frm_num = 34,
        .mfcc_frm = {
            #include "res/mfcc_out20_Fix.txt"
        }
    },
    {
        .flag = 0,
        .frm_num = 46,
        .mfcc_frm = {
            #include "res/mfcc_out24_Fix.txt"
        }
    },
};


static mfcc_extract_t *mfcc_exec = NULL;

int speech_recognition_init(void* (* alloc_ext)(int))
{
    mfcc_exec = (mfcc_extract_t *)alloc_ext(sizeof(mfcc_extract_t));
    vad_init(&(mfcc_exec->vad));
    mfcc_init(&(mfcc_exec->mfcc));
    mfcc_exec->tag = 3;
    mfcc_exec->dis_thr = 4200;

    mfcc_exec->mode_num = sizeof(mode)/sizeof(mfcc_vct_t);
    for (uint8_t i = 0; i<mfcc_exec->mode_num; i++){
        mfcc_exec->mode[i] = (mfcc_vct_t *)&mode[i];
    }    
    TRACE("%s size:%d alloc_ext:0x%08x mfcc_exec:0x%08x",__func__, sizeof(mfcc_extract_t), alloc_ext, mfcc_exec);
    return 0;
}

uint32_t speech_recognition_more_data(int16_t *buf, uint32_t len)
{
    int recognition_idx;
    recognition_idx = speech_recognition_process(mfcc_exec, buf, len);
    switch (recognition_idx) {
        case 0:
        case 1:            
            TRACE("The recognition result is 'close', match with %d", recognition_idx);
            break;    
        case 2:
        case 3:
            TRACE("The recognition result is 'no', match with %d", recognition_idx);
            break;    
        case 4:
        case 5:
            TRACE("The recognition result is 'open', match with %d", recognition_idx);
            break;
        case 6: 
        case 7:
            TRACE("The recognition result is 'yes', match with %d", recognition_idx);
            break;  
        default:
            break;
    }
    return len;
}

