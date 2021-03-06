#include <zlib.h>
#include "compression.h"

namespace Compression
{
    int compress(const u8* src, int srclen, u8* dest, int destlen)
    {
        z_stream stream;

        stream.next_in=(Bytef*)src;
        stream.avail_in = srclen;
        stream.next_out=(Bytef*)dest;
        stream.avail_out = destlen;
        stream.data_type = Z_BINARY;

        stream.zalloc = NULL;
        stream.zfree = NULL;

        deflateInit(&stream, Z_DEFAULT_COMPRESSION);
        deflate(&stream, Z_SYNC_FLUSH);
        deflateEnd(&stream);

        return stream.total_out;
        // TODO: error returns
    }

    void decompress(const u8* src, int srclen, u8* dest, int destlen)
    {
        z_stream stream;

        stream.next_in=(Bytef*)src;
        stream.avail_in = srclen;
        stream.next_out=(Bytef*)dest;
        stream.avail_out = destlen;
        stream.data_type = Z_BINARY;

        stream.zalloc = NULL;
        stream.zfree = NULL;

        inflateInit(&stream);
        inflate(&stream, Z_SYNC_FLUSH);
        inflateEnd(&stream);

        // TODO: error returns
    }
}
