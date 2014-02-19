
#include <TH.h>
#include <luaT.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PNG_DEBUG 3
#include <png.h>

void _compress_abort_(const char * s, ...)
{
  va_list args;
  va_start(args, s);
  vfprintf(stderr, s, args);
  fprintf(stderr, "\n");
  va_end(args);
  abort();
}

struct mem_buffer
{
    unsigned char * buffer;
    size_t read_index, size;
};

void my_png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    struct mem_buffer* p=(struct mem_buffer*)png_get_io_ptr(png_ptr); /* was png_ptr->io_ptr */
    if(p->read_index + length > p->size)
        fprintf(stderr, "Tried to read past the end of a buffer!\n");
    memcpy(data, p->buffer + p->read_index, length);
    p->read_index = p->read_index + length;
}

void my_png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    /* with libpng15 next line causes pointer deference error; use libpng12 */
    struct mem_buffer* p=(struct mem_buffer*)png_get_io_ptr(png_ptr); /* was png_ptr->io_ptr */
    size_t nsize = p->size + length;

    /* allocate or grow buffer */
    if(p->buffer)
        p->buffer = (unsigned char*)realloc(p->buffer, nsize);
    else
        p->buffer = (unsigned char*)malloc(nsize);

    if(!p->buffer)
        png_error(png_ptr, "Write Error");


    /* copy new bytes to end of buffer */
    memcpy(p->buffer + p->size, data, length);
    p->size = nsize;
    printf("Buffer address = %p, size = %lu\n", p->buffer, nsize);
}

/* This is optional but included to show how png_set_write_fn() is called */
void my_png_flush(png_structp png_ptr)
{
    printf("Flushing...\n");
}

#define abort_ _compress_abort_
#define torch_(NAME) TH_CONCAT_3(torch_, Real, NAME)
#define torch_Tensor TH_CONCAT_STRING_3(torch., Real, Tensor)
#define libcompress_(NAME) TH_CONCAT_3(libcompress_, Real, NAME)

#include "generic/libcompress.c"
#include "THGenerateAllTypes.h"

DLL_EXPORT int luaopen_libcompress(lua_State *L)
{
  // libcompress_FloatMain_init(L);
  // libcompress_DoubleMain_init(L);
  // libcompress_ByteMain_init(L);

  //luaL_register(L, "libcompress.double", libcompress_DoubleMain__);
  //luaL_register(L, "libcompress.float", libcompress_FloatMain__);
  luaL_register(L, "libcompress.byte", libcompress_ByteMain__);

  return 1;
}
