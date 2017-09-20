#ifndef __HAL_GPADC_H__
#define __HAL_GPADC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_GPADC_BAD_VALUE  0xFFFF

enum HAL_GPADC_CHAN_T {
	HAL_GPADC_CHAN_0 = 0,
	HAL_GPADC_CHAN_BATTERY = 1,
	HAL_GPADC_CHAN_2 = 2,
	HAL_GPADC_CHAN_3 = 3,
	HAL_GPADC_CHAN_4 = 4,
	HAL_GPADC_CHAN_5 = 5,
	HAL_GPADC_CHAN_6 = 6,
	HAL_GPADC_CHAN_ADCKEY = 7,
	HAL_GPADC_CHAN_QTY,
};

enum HAL_GPADC_ATP_T {
	HAL_GPADC_ATP_NULL = 0,
	HAL_GPADC_ATP_ONESHOT = 0,
	HAL_GPADC_ATP_122US,
	HAL_GPADC_ATP_1MS,
	HAL_GPADC_ATP_10MS,
	HAL_GPADC_ATP_100MS,
	HAL_GPADC_ATP_250MS,
	HAL_GPADC_ATP_500MS,
	HAL_GPADC_ATP_1S,
	HAL_GPADC_ATP_2S,
		
	HAL_GPADC_ATP_QTY,
};

typedef uint16_t HAL_GPADC_MV_T;


typedef void (*HAL_GPADC_EVENT_CB_T)(void *);

int hal_gpadc_open(enum HAL_GPADC_CHAN_T channel, enum HAL_GPADC_ATP_T atp, HAL_GPADC_EVENT_CB_T cb);

int hal_gpadc_close(enum HAL_GPADC_CHAN_T channel);

#ifdef __cplusplus
	}
#endif

#endif//__FMDEC_H__
