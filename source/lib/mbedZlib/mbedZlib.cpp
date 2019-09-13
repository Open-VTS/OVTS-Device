/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Zip Compression library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/

#include "mbedZlib.h"



#define CHUNK 32


/** GUNZIP a from src buffer to dst buffer
 @param dst : destination buffer
 @param dst_length : pointer to destination buffer length
 @param src : source buffer
 @param src_length : source buffer length
 @return Z_OK on success, zlib error (<0) on failure
 */
int ZLib::unzip(unsigned char *dst, unsigned long *dst_length, unsigned char *src, unsigned long src_length)
{
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    stream.next_in = src;
    stream.avail_in = src_length;

    stream.next_out = dst;
    stream.avail_out = *dst_length;

    int rv = inflateInit2(&stream, 15 + 16);
    if (Z_OK == rv) {
        rv = inflate(&stream, Z_NO_FLUSH);
        if (Z_STREAM_END == rv) {
            inflateEnd(&stream);
            rv = Z_OK;
        }
    }

    if (Z_OK == rv) {
        *dst_length = stream.total_out;
    } else {
        *dst_length = 0;
    }

    return rv;
}

/** GUNZIP a from src buffer to dst buffer
 @param dst : destination buffer
 @param dst_length : pointer to destination buffer length
 @param src : source buffer
 @param src_length : source buffer length
 @return Z_OK on success, zlib error (<0) on failure
 */
int ZLib::zip(unsigned char *dst, unsigned long *dst_length, unsigned char *src, unsigned long src_length)
{
    z_stream        stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = src;
    stream.avail_in = src_length;

    stream.next_out = Z_NULL;
    stream.avail_out = 0;

    /* add 16 to MAX_WBITS to specify gzip format - it gets taken off again in defaultInit2 */
    int rv = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (Z_OK == rv) {
        unsigned long dst_bound = deflateBound(&stream, stream.avail_in) + 12; /* 12 bytes for the gzip header */
        if (dst_bound > *dst_length) {
            rv = Z_MEM_ERROR;
        } else {
            stream.next_out   = dst;
            stream.avail_out = dst_bound;
        }
    }
    if (Z_OK == rv) {
        gz_header        header;
        memset(&header, 0, sizeof(header));
        rv = deflateSetHeader(&stream, &header);
    }
    if (Z_OK == rv) {
        rv = deflate(&stream, Z_FINISH);
        if (Z_STREAM_END == rv) {
            rv = deflateEnd(&stream);
        }
    }
    if (Z_OK == rv) {
        *dst_length = stream.total_out;
    } else {
        *dst_length = 0;
    }

    return rv;
}


/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int def(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    // ret = deflateInit(&strm, level);
    ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            // assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        // assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    // assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}


int inf(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    // ret = inflateInit(&strm);
    ret = inflateInit2(&strm, 16 + MAX_WBITS);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            // assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}



int ZLib::compress_file(const char* src, const char* dest){
    FILE *infile = fopen(src, "r");
    FILE *outfile = fopen(dest, "w");
    int ret = def(infile, outfile, Z_DEFAULT_COMPRESSION);
    fclose(infile);
    fclose(outfile);
    return ret;
}

int ZLib::decompress_file(const char* src, const char* dest){
    FILE *infile = fopen(src, "r");
    FILE *outfile = fopen(dest, "w");
    int ret = inf(infile, outfile);
    fclose(infile);
    fclose(outfile);
    return ret;
}

