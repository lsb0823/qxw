#include "usb_dev_desc.h"
#include "hal_usb.h"
#include "usb_descriptor.h"
#include "tgt_hardware.h"

#define USB_CDC_USB_REL                         0x0110

#ifndef USB_CDC_VENDOR_ID
#define USB_CDC_VENDOR_ID                       0xBE57
#endif
#ifndef USB_CDC_PRODUCT_ID
#define USB_CDC_PRODUCT_ID                      0x0101
#endif

#define USB_AUDIO_USB_REL                       0x0110 //0x0200

#ifndef USB_AUDIO_VENDOR_ID
#define USB_AUDIO_VENDOR_ID                     0xBE57
#endif
#ifndef USB_AUDIO_PRODUCT_ID
#define USB_AUDIO_PRODUCT_ID                    0x0201
#endif

//----------------------------------------------------------------
// USB device common string descriptions
//----------------------------------------------------------------

static const uint8_t stringLangidDescriptor[] = {
    0x04,               /*bLength*/
    STRING_DESCRIPTOR,  /*bDescriptorType 0x03*/
    0x09,0x04,          /*bString Lang ID - 0x0409 - English*/
};

static const uint8_t stringIconfigurationDescriptor[] = {
    0x06,               /*bLength*/
    STRING_DESCRIPTOR,  /*bDescriptorType 0x03*/
    '0',0,'1',0,        /*bString iConfiguration - 01*/
};

//----------------------------------------------------------------
// USB CDC device description and string descriptions
//----------------------------------------------------------------

const uint8_t * cdc_deviceDesc(uint8_t index)
{
    static const uint8_t deviceDescriptor[] = {
        18,                   // bLength
        1,                    // bDescriptorType
        LSB(USB_CDC_USB_REL), MSB(USB_CDC_USB_REL), // bcdUSB
        2,                    // bDeviceClass
        0,                    // bDeviceSubClass
        0,                    // bDeviceProtocol
        EP0_MAX_PACKET_SIZE,  // bMaxPacketSize0
        LSB(USB_CDC_VENDOR_ID), MSB(USB_CDC_VENDOR_ID), // idVendor
        LSB(USB_CDC_PRODUCT_ID), MSB(USB_CDC_PRODUCT_ID), // idProduct
        0x00, 0x01,           // bcdDevice
        STRING_OFFSET_IMANUFACTURER, // iManufacturer
        STRING_OFFSET_IPRODUCT, // iProduct
        STRING_OFFSET_ISERIAL, // iSerialNumber
        1                     // bNumConfigurations
    };
    return deviceDescriptor;
}

const uint8_t *cdc_stringDesc(uint8_t index)
{
    static const uint8_t stringImanufacturerDescriptor[] =
#ifdef USB_CDC_STR_DESC_MANUFACTURER
        USB_CDC_STR_DESC_MANUFACTURER;
#else
    {
        0x16,                                            /*bLength*/
        STRING_DESCRIPTOR,                               /*bDescriptorType 0x03*/
        'b',0,'e',0,'s',0,'t',0,'e',0,'c',0,'h',0,'n',0,'i',0,'c',0 /*bString iManufacturer - bestechnic*/
    };
#endif

    static const uint8_t stringIserialDescriptor[] =
#ifdef USB_CDC_STR_DESC_SERIAL
        USB_CDC_STR_DESC_SERIAL;
#else
    {
        0x16,                                                           /*bLength*/
        STRING_DESCRIPTOR,                                              /*bDescriptorType 0x03*/
        '2',0,'0',0,'1',0,'5',0,'1',0,'0',0,'0',0,'6',0,'.',0,'1',0,    /*bString iSerial - 20151006.1*/
    };
#endif

    static const uint8_t stringIinterfaceDescriptor[] =
#ifdef USB_CDC_STR_DESC_INTERFACE
        USB_CDC_STR_DESC_INTERFACE;
#else
    {
        0x08,
        STRING_DESCRIPTOR,
        'C',0,'D',0,'C',0,
    };
#endif

    static const uint8_t stringIproductDescriptor[] =
#ifdef USB_CDC_STR_DESC_PRODUCT
        USB_CDC_STR_DESC_PRODUCT;
#else
    {
        0x16,
        STRING_DESCRIPTOR,
        'C',0,'D',0,'C',0,' ',0,'D',0,'E',0,'V',0,'I',0,'C',0,'E',0
    };
#endif

    const uint8_t *data = NULL;
    
    switch (index)
    {
        case STRING_OFFSET_LANGID:
            data = stringLangidDescriptor;
            break;
        case STRING_OFFSET_IMANUFACTURER:
            data = stringImanufacturerDescriptor;
            break;
        case STRING_OFFSET_IPRODUCT:
            data = stringIproductDescriptor;
            break;
        case STRING_OFFSET_ISERIAL:
            data = stringIserialDescriptor;
            break;
        case STRING_OFFSET_ICONFIGURATION:
            data = stringIconfigurationDescriptor;
            break;
        case STRING_OFFSET_IINTERFACE:
            data = stringIinterfaceDescriptor;
            break;
    }
    
    return data;
}

//----------------------------------------------------------------
// USB audio device description and string descriptions
//----------------------------------------------------------------

const uint8_t * uaud_dev_desc(uint8_t index)
{
    static const uint8_t deviceDescriptor[] = {
        18,                   // bLength
        1,                    // bDescriptorType
        LSB(USB_AUDIO_USB_REL), MSB(USB_AUDIO_USB_REL), // bcdUSB
        0,                    // bDeviceClass
        0,                    // bDeviceSubClass
        0,                    // bDeviceProtocol
        EP0_MAX_PACKET_SIZE,  // bMaxPacketSize0
        LSB(USB_AUDIO_VENDOR_ID), MSB(USB_AUDIO_VENDOR_ID), // idVendor
        LSB(USB_AUDIO_PRODUCT_ID), MSB(USB_AUDIO_PRODUCT_ID), // idProduct
        0x00, 0x01,           // bcdDevice
        STRING_OFFSET_IMANUFACTURER, // iManufacturer
        STRING_OFFSET_IPRODUCT, // iProduct
        STRING_OFFSET_ISERIAL, // iSerialNumber
        1                     // bNumConfigurations
    };
    return deviceDescriptor;
}

const uint8_t *uaud_string_desc(uint8_t index)
{
    static const uint8_t stringImanufacturerDescriptor[] =
#ifdef USB_AUDIO_STR_DESC_MANUFACTURER
        USB_AUDIO_STR_DESC_MANUFACTURER;
#else
    {
        0x16,                                            /*bLength*/
        STRING_DESCRIPTOR,                               /*bDescriptorType 0x03*/
        'b',0,'e',0,'s',0,'t',0,'e',0,'c',0,'h',0,'n',0,'i',0,'c',0 /*bString iManufacturer - bestechnic*/
    };
#endif

    static const uint8_t stringIserialDescriptor[] =
#ifdef USB_AUDIO_STR_DESC_SERIAL
        USB_AUDIO_STR_DESC_SERIAL;
#else
    {
        0x16,                                                           /*bLength*/
        STRING_DESCRIPTOR,                                              /*bDescriptorType 0x03*/
        '2',0,'0',0,'1',0,'6',0,'0',0,'4',0,'0',0,'6',0,'.',0,'1',0,    /*bString iSerial - 20160406.1*/
    };
#endif

    static const uint8_t stringIinterfaceDescriptor[] =
#ifdef USB_AUDIO_STR_DESC_INTERFACE
        USB_AUDIO_STR_DESC_INTERFACE;
#else
    {
        0x0c,                           //bLength
        STRING_DESCRIPTOR,              //bDescriptorType 0x03
        'A',0,'u',0,'d',0,'i',0,'o',0   //bString iInterface - Audio
    };
#endif

    static const uint8_t stringIproductDescriptor[] =
#ifdef USB_AUDIO_STR_DESC_PRODUCT
        USB_AUDIO_STR_DESC_PRODUCT;
#else
    {
        0x16,                                                       //bLength
        STRING_DESCRIPTOR,                                          //bDescriptorType 0x03
        'B',0,'e',0,'s',0,'t',0,' ',0,'A',0,'u',0,'d',0,'i',0,'o',0 //bString iProduct - Best Audio
    };
#endif

    const uint8_t *data = NULL;

    switch (index)
    {
        case STRING_OFFSET_LANGID:
            data = stringLangidDescriptor;
            break;
        case STRING_OFFSET_IMANUFACTURER:
            data = stringImanufacturerDescriptor;
            break;
        case STRING_OFFSET_IPRODUCT:
            data = stringIproductDescriptor;
            break;
        case STRING_OFFSET_ISERIAL:
            data = stringIserialDescriptor;
            break;
        case STRING_OFFSET_ICONFIGURATION:
            data = stringIconfigurationDescriptor;
            break;
        case STRING_OFFSET_IINTERFACE:
            data = stringIinterfaceDescriptor;
            break;
    }

    return data;
}


