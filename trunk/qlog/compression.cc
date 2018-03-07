#include "compression.h"
#include "zlib.h"
#include "stdio.h"

#define COMPRESS_HEAD_LENGTH 5

namespace qlog {

compress_ret
q_compress(const char* source, long source_len,
        char* dest, long* dest_len,
        compress_type type)
{
    Bytef* z_dest = (Bytef*)dest + COMPRESS_HEAD_LENGTH;
    uLong z_dest_len = *dest_len - COMPRESS_HEAD_LENGTH;
    const Bytef* z_source = (const Bytef*)source;
    uLong z_source_len = (uLong)source_len;

    compress_ret ret = COMPRESS_OK;
    int z_ret;

    if (z_dest_len < compressBound(source_len))
        return COMPRESS_BUFFER_ERROR;
    if (!dest || !source || source_len < 0 ||
            *dest_len < 0)
        return COMPRESS_INVALID_PARAMETER;

    switch (type) {
        case Z_DEFLATE:
            z_ret = compress(z_dest, 
                    &z_dest_len,z_source,z_source_len);
            if (z_ret == Z_OK)
            {
                dest[0] = (char)type;
                for (int i = 1; i < COMPRESS_HEAD_LENGTH; i++)
                {
                    dest[i] = source_len % 256;
                    source_len /= 256;
                }
                *dest_len = z_dest_len + COMPRESS_HEAD_LENGTH;
            }
            else if (z_ret == Z_BUF_ERROR)
            {
                ret = COMPRESS_BUFFER_ERROR;
            }
            else
                ret = COMPRESS_FAILED;
            break;
        default:
            ret = COMPRESS_INVALID_TYPE;
    }
    return ret;
}

compress_ret
q_uncompress(const char* source, long source_len,
        char* dest, long* dest_len)
{
    compress_ret ret = COMPRESS_OK;

    if (!source || source_len <= COMPRESS_HEAD_LENGTH)
        return COMPRESS_INVALID_PARAMETER;

    compress_type type = (compress_type)source[0];
    uLong z_dest_len = 0;
    for (int i = 1; i < COMPRESS_HEAD_LENGTH; i++)
    {
        z_dest_len += (uLong)((unsigned char)source[i])<<8*(i-1);
    }

    //Customer could use dest==NULL or dest_len<=0 to check 
    //how large buffer they need to provide for 
    if (!dest || *dest_len <=0)
    {
        *dest_len = z_dest_len;
        return COMPRESS_OK;
    }

    if (*dest_len < (long)z_dest_len)
        return COMPRESS_BUFFER_ERROR;

    z_dest_len = *dest_len;
    Bytef* z_dest = (Bytef*)dest;
    const Bytef* z_source = (const Bytef*)source + COMPRESS_HEAD_LENGTH;
    uLong z_source_len = (uLong)source_len - COMPRESS_HEAD_LENGTH;
    int z_ret;

    switch (type) {
        case Z_DEFLATE:
            z_ret = uncompress(z_dest, &z_dest_len, 
                    z_source, z_source_len);

            if (z_ret == Z_OK)
                break;
            else if (z_ret == Z_BUF_ERROR)
            {
                ret = COMPRESS_BUFFER_ERROR;
            }
            else if (z_ret == Z_MEM_ERROR)
            {
                ret = COMPRESS_FAILED;
            } 
            else 
            {
                ret = COMPRESS_INVALID_FORMAT;
            }
            break;
        default:
            ret = COMPRESS_INVALID_FORMAT;
    }

    return ret;
}

long
q_compressBound(long source_len)
{
    return compressBound(source_len) + COMPRESS_HEAD_LENGTH;
}

} // namespace qlog

