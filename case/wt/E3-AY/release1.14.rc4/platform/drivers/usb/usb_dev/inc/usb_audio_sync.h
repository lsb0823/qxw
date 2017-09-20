#ifndef __USB_AUDIO_SYNC_H__
#define __USB_AUDIO_SYNC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

struct USB_AUDIO_STREAM_INFO_T {
    // 1 - capture stream; 0 - playback stream
    uint8_t capture;
    // 1 - buffer position can be adjusted in frame boundary; 0 - otherwise
    uint8_t frame_aligned;
    // Buffer size in samples
    uint32_t buffer_size;
    // Frame size in samples
    uint32_t frame_size;
    // Buffer adjusting threshold in samples
    uint16_t adj_buf_thresh;
    // Buffer adjusting step in samples
    uint16_t adj_buf_step;
    // Difference error threshold
    uint16_t err_thresh;
    // Difference synchronization threshold
    uint16_t sync_thresh;
};

void usb_audio_sync_reset(void);

int usb_audio_sync(uint32_t codec_pos, uint32_t *usb_pos_p, const struct USB_AUDIO_STREAM_INFO_T *info);

#ifdef __cplusplus
}
#endif

#endif
