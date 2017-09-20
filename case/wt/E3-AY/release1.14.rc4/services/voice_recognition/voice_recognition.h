#ifndef __VOICE_RECOGNITION_H__
#define __VOICE_RECOGNITION_H__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

int speech_recognition_init(void* (* alloc_ext)(int));
int speech_recognition_reset(void);
uint32_t speech_recognition_more_data(int16_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
