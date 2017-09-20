#include "hal_chipid.h"
#include "hal_location.h"
#include "hal_analogif.h"
#include "hal_trace.h"

enum HAL_CHIP_METAL_ID_T BOOT_BSS_LOC metal_id;

uint32_t WEAK BOOT_TEXT_FLASH_LOC read_hw_metal_id(void)
{
    return HAL_CHIP_METAL_ID_0;
}

uint32_t read_hw_rf_id(void);


#if 0
void  hal_chipid_init(void)
{
    metal_id = read_hw_metal_id();
}
#endif

void BOOT_TEXT_FLASH_LOC hal_chipid_init(void)
{
    hal_analogif_accessmode_set(HAL_ANALOGIF_MODE_FULLACCESS);
    metal_id = read_hw_metal_id();
    if(metal_id>=HAL_CHIP_METAL_ID_2)
    {
        uint32_t rf_id= read_hw_rf_id();
        if(rf_id == 0x4)
            metal_id = HAL_CHIP_METAL_ID_4;
    }
#ifdef __SBC_FUNC_IN_ROM__
    if(metal_id < HAL_CHIP_METAL_ID_4)
        ASSERT(0,"SBC IN ROM ONLY AFTER CHIP ID 4");
#endif
    hal_analogif_accessmode_set(HAL_ANALOGIF_MODE_PROTECTACCESS);
}


enum HAL_CHIP_METAL_ID_T BOOT_TEXT_SRAM_LOC hal_get_chip_metal_id(void)
{
#ifdef FPGA
    metal_id = HAL_CHIP_METAL_ID_2;  
#endif
    return metal_id;
}

