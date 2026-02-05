/* stb_image_write - v1.16 - public domain - http://nothings.org/stb
   writes out PNG/BMP/TGA/JPEG/HDR images to C stdio - Sean Barrett 2010
                            no warranty implied; use at your own risk

   Before #including,
       #define STB_IMAGE_WRITE_IMPLEMENTATION
   in the file that you want to have the implementation.

   Will probably not work correctly with unusual stride values.

   ABOUT:
      This header file is a library for writing images to C stdio or a callback.

      The PNG output is not optimal; it is stripped down to be smaller and
      easier to integrate. Compression runs about 20% slower than libpng but
      is about 10% smaller.

   LICENSE
      Public domain.

*/

#ifndef STB_IMAGE_WRITE_H
#define STB_IMAGE_WRITE_H

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);

#ifdef __cplusplus
}
#endif

#ifdef STB_IMAGE_WRITE_IMPLEMENTATION

#ifndef STBIW_MALLOC
#include <stdlib.h>
#define STBIW_MALLOC(sz)    malloc(sz)
#define STBIW_FREE(p)       free(p)
#endif

#ifndef STBIW_MEMMOVE
#include <string.h>
#define STBIW_MEMMOVE(a,b,sz) memmove(a,b,sz)
#endif

#ifndef STBIW_ASSERT
#include <assert.h>
#define STBIW_ASSERT(x) assert(x)
#endif

typedef unsigned int stbiw_uint32;
typedef unsigned char stbi_uc;

static void stbiw__writefv(FILE *f, const char *fmt, va_list v);
static void stbiw__write3(FILE *f, unsigned char a, unsigned char b, unsigned char c);
static void stbiw__writef(FILE *f, const char *fmt, ...);

static void stbiw__writefv(FILE *f, const char *fmt, va_list v)
{
   while (*fmt) {
      switch (*fmt++) {
         case ' ': break;
         case '1': { unsigned char x = (unsigned char)va_arg(v, int); fputc(x,f); break; }
         case '2': { int x = va_arg(v,int); unsigned char b[2];
                     b[0] = (unsigned char) x; b[1] = (unsigned char) (x>>8);
                     fwrite(b,2,1,f); break; }
         case '4': { stbiw_uint32 x = va_arg(v,int); unsigned char b[4];
                     b[0]=(unsigned char)x; b[1]=(unsigned char)(x>>8);
                     b[2]=(unsigned char)(x>>16); b[3]=(unsigned char)(x>>24);
                     fwrite(b,4,1,f); break; }
         default:
            return;
      }
   }
}

static void stbiw__writef(FILE *f, const char *fmt, ...)
{
   va_list v;
   va_start(v, fmt);
   stbiw__writefv(f, fmt, v);
   va_end(v);
}

static void stbiw__write3(FILE *f, unsigned char a, unsigned char b, unsigned char c)
{
   unsigned char arr[3];
   arr[0] = a; arr[1] = b; arr[2] = c;
   fwrite(arr, 3, 1, f);
}

static stbiw_uint32 stbiw__crc32_tab[256];
static int stbiw__crc32_init;

static stbiw_uint32 stbiw__crc32(stbiw_uint32 crc, const unsigned char *buffer, int len)
{
   stbiw_uint32 c = ~crc;
   int n;
   if (!stbiw__crc32_init) {
      for (n=0; n<256; n++) {
         stbiw_uint32 x = (stbiw_uint32)n;
         int k;
         for (k=0; k<8; k++)
            x = x & 1 ? (x >> 1) ^ 0xedb88320u : x >> 1;
         stbiw__crc32_tab[n] = x;
      }
      stbiw__crc32_init = 1;
   }
   for (n=0; n<len; n++)
      c = stbiw__crc32_tab[(c ^ buffer[n]) & 0xff] ^ (c >> 8);
   return ~c;
}

static stbiw_uint32 stbiw__crc32_big(const unsigned char *buffer, int len)
{
   return stbiw__crc32(0, buffer, len);
}

static void stbiw__png_write_chunk(FILE *f, stbiw_uint32 len, stbiw_uint32 tag, const unsigned char *data)
{
   unsigned char length[4];
   unsigned char tag_buf[4];
   unsigned char crc_buf[4];
   stbiw_uint32 crc;

   length[0] = (unsigned char)(len >> 24);
   length[1] = (unsigned char)(len >> 16);
   length[2] = (unsigned char)(len >> 8);
   length[3] = (unsigned char)(len);
   fwrite(length, 4, 1, f);

   tag_buf[0] = (unsigned char)(tag >> 24);
   tag_buf[1] = (unsigned char)(tag >> 16);
   tag_buf[2] = (unsigned char)(tag >> 8);
   tag_buf[3] = (unsigned char)(tag);
   fwrite(tag_buf, 4, 1, f);

   if (data)
      fwrite(data, len, 1, f);

   crc = stbiw__crc32_big(tag_buf, 4);
   if (data)
      crc = stbiw__crc32(crc, data, len);

   crc_buf[0] = (unsigned char)(crc >> 24);
   crc_buf[1] = (unsigned char)(crc >> 16);
   crc_buf[2] = (unsigned char)(crc >> 8);
   crc_buf[3] = (unsigned char)(crc);
   fwrite(crc_buf, 4, 1, f);
}

static int stbiw__png_write_scanlines(FILE *f, int w, int h, int comp, const void *data, int stride)
{
   int y;
   const unsigned char *src = (const unsigned char *) data;
   int bytes_per_pixel = comp;
   int row_bytes = w * bytes_per_pixel;
   int zlib_max = row_bytes * h + h;
   unsigned char *zlib_data = (unsigned char *) STBIW_MALLOC(zlib_max);
   unsigned char *zlib_ptr = zlib_data;
   if (!zlib_data) return 0;

   for (y = 0; y < h; ++y) {
      *zlib_ptr++ = 0;
      memcpy(zlib_ptr, src + y * stride, row_bytes);
      zlib_ptr += row_bytes;
   }

   unsigned char *compressed = (unsigned char *) STBIW_MALLOC(zlib_max + 6);
   if (!compressed) {
      STBIW_FREE(zlib_data);
      return 0;
   }

   int zlib_len = 0;
   {
      unsigned int a = 1, b = 0;
      for (int i = 0; i < zlib_ptr - zlib_data; ++i) {
         a = (a + zlib_data[i]) % 65521;
         b = (b + a) % 65521;
      }
      stbiw_uint32 adler = (b << 16) | a;

      compressed[zlib_len++] = 0x78;
      compressed[zlib_len++] = 0x01;

      int remaining = (int)(zlib_ptr - zlib_data);
      int offset = 0;
      while (remaining > 0) {
         int block = remaining > 65535 ? 65535 : remaining;
         int final = remaining <= 65535;
         compressed[zlib_len++] = (unsigned char) final;
         compressed[zlib_len++] = (unsigned char) (block & 255);
         compressed[zlib_len++] = (unsigned char) ((block >> 8) & 255);
         compressed[zlib_len++] = (unsigned char) (~block & 255);
         compressed[zlib_len++] = (unsigned char) ((~block >> 8) & 255);
         memcpy(compressed + zlib_len, zlib_data + offset, block);
         zlib_len += block;
         offset += block;
         remaining -= block;
      }
      compressed[zlib_len++] = (unsigned char)(adler >> 24);
      compressed[zlib_len++] = (unsigned char)(adler >> 16);
      compressed[zlib_len++] = (unsigned char)(adler >> 8);
      compressed[zlib_len++] = (unsigned char)(adler);
   }

   stbiw__png_write_chunk(f, zlib_len, 0x49444154, compressed);

   STBIW_FREE(zlib_data);
   STBIW_FREE(compressed);
   return 1;
}

int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes)
{
   FILE *f;
   int bit_depth = 8;
   unsigned char sig[8] = { 137,80,78,71,13,10,26,10 };
   unsigned char ihdr[13];
   int color_type;

   if (comp == 1) color_type = 0;
   else if (comp == 2) color_type = 4;
   else if (comp == 3) color_type = 2;
   else if (comp == 4) color_type = 6;
   else return 0;

   f = fopen(filename, "wb");
   if (!f) return 0;

   fwrite(sig, 1, 8, f);

   ihdr[0] = (unsigned char)(w >> 24);
   ihdr[1] = (unsigned char)(w >> 16);
   ihdr[2] = (unsigned char)(w >> 8);
   ihdr[3] = (unsigned char)(w);
   ihdr[4] = (unsigned char)(h >> 24);
   ihdr[5] = (unsigned char)(h >> 16);
   ihdr[6] = (unsigned char)(h >> 8);
   ihdr[7] = (unsigned char)(h);
   ihdr[8] = (unsigned char)bit_depth;
   ihdr[9] = (unsigned char)color_type;
   ihdr[10] = 0;
   ihdr[11] = 0;
   ihdr[12] = 0;

   stbiw__png_write_chunk(f, 13, 0x49484452, ihdr);

   if (!stbiw__png_write_scanlines(f, w, h, comp, data, stride_in_bytes)) {
      fclose(f);
      return 0;
   }

   stbiw__png_write_chunk(f, 0, 0x49454e44, NULL);

   fclose(f);
   return 1;
}

#endif /* STB_IMAGE_WRITE_IMPLEMENTATION */

#endif /* STB_IMAGE_WRITE_H */
