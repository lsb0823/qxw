#include "plat_types.h"
#include "utils.h"

void memcpy32(uint32_t * dst, const uint32_t * src, size_t length)
{
    for (uint32_t i = 0; i < length;i++)
    {
        dst[i] = src[i];
    }
}

