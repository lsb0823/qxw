#ifndef NV_FACTORY_SECTION_H
#define NV_FACTORY_SECTION_H

#define ALIGN4 __attribute__((aligned(4)))
#define nvrec_dev_version   1
#define nvrec_dev_magic      0xba80

typedef struct{
    unsigned short magic;
    unsigned short version;
    unsigned int crc ;
    unsigned int reserved0;
    unsigned int reserved1;
}section_head_t;

typedef struct{
    unsigned char device_name[248+1] ALIGN4;
    unsigned char bt_address[8] ALIGN4;
    unsigned char ble_address[8] ALIGN4;
    unsigned char tester_address[8] ALIGN4;
    unsigned int  xtal_fcap ALIGN4;
}nv_factory_section_data_t;

typedef struct{
    section_head_t head;
    nv_factory_section_data_t data;
}nv_factory_section_t;

#ifdef __cplusplus
extern "C" {
#endif

int nv_factory_section_open(void);

nv_factory_section_data_t * nv_factory_section_get_data_ptr(void);

#ifdef __cplusplus
}
#endif
#endif
