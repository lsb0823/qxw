#ifndef __HAL_ANALOGIF_H__
#define __HAL_ANALOGIF_H__

enum HAL_ANALOGIF_MODE_T {
    HAL_ANALOGIF_MODE_FULLACCESS,
    HAL_ANALOGIF_MODE_PROTECTACCESS,
    
	HAL_ANALOGIF_ACCESS_NUM,
};

enum IOMUX_ISPI_ACCESS_T {
     IOMUX_ISPI_BT_RF = (1 << 0),
     IOMUX_ISPI_BT_PMU = (1 << 1),
     IOMUX_ISPI_BT_ANA = (1 << 2),
     IOMUX_ISPI_MCU_RF = (1 << 3),
     IOMUX_ISPI_MCU_PMU = (1 << 4),
     IOMUX_ISPI_MCU_ANA = (1 << 5),
};

#ifdef __cplusplus
extern "C" {
#endif


int hal_analogif_open(void);

int hal_analogif_accessmode_set(enum HAL_ANALOGIF_MODE_T mode);

int hal_analogif_reg_write(unsigned short reg, unsigned short val);

int hal_analogif_reg_read(unsigned short reg, unsigned short *val);


#ifdef __cplusplus
}
#endif

#endif

