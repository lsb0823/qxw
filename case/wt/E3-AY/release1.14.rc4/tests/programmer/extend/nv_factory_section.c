#include "plat_types.h"
#include "export_fn_rom.h"
#include "utils.h"
#include "nv_factory_section.h"

#define crc32(crc, buf, len) __export_fn_rom.crc32(crc, buf, len)
static nv_factory_section_t *factory_section_p = NULL;
uint8_t factory_section_imagecache[4096];

int nv_factory_section_open(void)
{
    memcpy32((uint32_t *)factory_section_imagecache, (uint32_t *)0x6C0ff000, sizeof(factory_section_imagecache)/sizeof(uint32_t));
    factory_section_p = (nv_factory_section_t *)factory_section_imagecache;
    if (factory_section_p->head.magic != nvrec_dev_magic){
        factory_section_p = NULL;
        return -1;
    }
    if (factory_section_p->head.version != nvrec_dev_version){
        factory_section_p = NULL;
        return -1;
    }
    if (factory_section_p->head.crc != crc32(0,(unsigned char *)(&(factory_section_p->head.reserved0)),sizeof(nv_factory_section_t)-2-2-4)){
        factory_section_p = NULL;
        return -1;
    }
    return 0;
}

nv_factory_section_data_t * nv_factory_section_get_data_ptr(void)
{
    if (factory_section_p)
        return &(factory_section_p->data);
    else
        return NULL;
}

int nvrec_dev_get_xtal_fcap(unsigned int *xtal_fcap)
{
    if (factory_section_p){
        *xtal_fcap = factory_section_p->data.xtal_fcap;
        return 0;
    }else{
        return -1;
    }
}

