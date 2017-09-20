#ifndef __AUDIO_RESAMPLER_H__
#define __AUDIO_RESAMPLER_H__

#define SAMPLE_NUM 128

#ifdef __cplusplus
extern "C" {
#endif

enum audio_resample_status_t{
    AUDIO_RESAMPLE_STATUS_SUCESS = 0,
    AUDIO_RESAMPLE_STATUS_CONTINUE,
    AUDIO_RESAMPLE_STATUS_FAILED,
};

enum resample_id_t{
	RESAMPLE_ID_32TO50P7 = 0,
	RESAMPLE_ID_32TO52P7,
	RESAMPLE_ID_44P1TO50P71,
	RESAMPLE_ID_44P1TO50P7,  //50p78
	RESAMPLE_ID_44P1TO51P88,  
	RESAMPLE_ID_44P1TO52P8,  
	RESAMPLE_ID_48TO50P74,
	RESAMPLE_ID_48TO50P7,    //50p78
	RESAMPLE_ID_48TO50P82,
	RESAMPLE_ID_48TO51P7,
	RESAMPLE_ID_48TO52P82,
	RESAMPLE_ID_NUM
};

//int audio_resample_need_buf_size(enum resample_id_t resample_id);
enum audio_resample_status_t audio_resample_flush_data(void);
enum audio_resample_status_t audio_resample_open(enum resample_id_t resample_id, void* (* alloc_ext)(int));
enum audio_resample_status_t audio_resample_cfg(short *OutBuf, int OutLen);
enum audio_resample_status_t audio_resample_run(short *InBuf, int InLen);
void audio_resample_close(void);

#ifdef __cplusplus
}
#endif

#endif
