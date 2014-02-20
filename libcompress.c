
#include <TH.h>
#include <luaT.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PNG_DEBUG 3
#include <png.h>

#define byte unsigned char

void _compress_abort_(const char * s, ...)
{
  va_list args;
  va_start(args, s);
  vfprintf(stderr, s, args);
  fprintf(stderr, "\n");
  va_end(args);
  abort();
}

#define abort_ _compress_abort_

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
    struct mem_buffer* p=(struct mem_buffer*)png_get_io_ptr(png_ptr); 
    size_t nsize = p->size + length;

    /* allocate or grow buffer */
    // This is a really inefficient way of doing this!
    // We could grow by a fixed factor e.g 3.5 each time we need to reallocate, then realloc down to the correct size.
    if(p->buffer)
        p->buffer = (unsigned char*)realloc(p->buffer, nsize);
    else
        p->buffer = (unsigned char*)malloc(nsize);

    if(!p->buffer)
        png_error(png_ptr, "Write Error");


    /* copy new bytes to end of buffer */
    memcpy(p->buffer + p->size, data, length);
    p->size = nsize;
    //printf("Buffer address = %p, size = %lu\n", p->buffer, nsize);
}

/* This is optional but included to show how png_set_write_fn() is called */
void my_png_flush(png_structp png_ptr)
{
    printf("Flushing...\n");
}


static THByteStorage * libcompress_pack_png_string(THByteTensor * image_tensor)
{
    struct mem_buffer compressed_image;
    compressed_image.buffer = NULL;
    compressed_image.size = 0;
    compressed_image.read_index = 0;

    //libpng needs each row to be contiguous.
    //We also assume every thing is contiguous when populating row_pointers below.
    THByteTensor * tensorc = THByteTensor_newContiguous(image_tensor);
    byte * tensor_data = THByteTensor_data(tensorc);
    
    //A 2D tensor is an image, so we can just compress it.
    //We collapse any higher dimensional tensor to 2D 
    //equivalent to stacking each 2D plane to give a very tall image.
    int width = tensorc->size[tensorc->nDimension-1];
    int height = 1;
    for(int i = 0; i < tensorc->nDimension-1; ++i)
        height = height*tensorc->size[i];

    png_bytep * row_pointers = (png_bytep *)malloc(height * sizeof(png_bytep));
    const int row_stride = width;
    for(int i = 0; i < height; ++i)
        row_pointers[i] = &tensor_data[tensorc->storageOffset + i*row_stride];

    {
        png_structp write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        //if (!write_ptr) return 0;

        png_infop write_info_ptr = png_create_info_struct(write_ptr);
        //if (!write_info_ptr)
        //{
        //    png_destroy_write_struct(&write_ptr, (png_infopp)NULL);
        //    return 0;
        //}
        
        png_set_write_fn(write_ptr, &compressed_image, my_png_write_data, my_png_flush);

        png_set_IHDR(write_ptr, write_info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, 
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        //fprintf(stderr, "header:\n");
        png_write_info(write_ptr, write_info_ptr);
        //fprintf(stderr, "Image:\n");
        png_write_image(write_ptr, row_pointers);
        //fprintf(stderr, "Footer:\n");
        //png_write_end(write_ptr, NULL);
        //char out_file_name[] = "/Users/tim.harley/git/pngtest/buffer_dmt.png";
        //FILE *wfp = fopen(out_file_name, "wb");
        //fwrite(compressed_image.buffer, 1, compressed_image.size, wfp);
        //fclose(wfp);
        png_destroy_write_struct(&write_ptr, &write_info_ptr);
        free(row_pointers);
    }
    //The byte storage now assumes control of the memory buffer.
    THByteStorage * png_string = THByteStorage_newWithData(compressed_image.buffer, compressed_image.size);
    return png_string;
}

static THByteTensor * libcompress_unpack_png_string(THByteStorage * packed_tensor, THLongStorage * tensor_dimensions)
{
    struct mem_buffer compressed_image;
    compressed_image.buffer = packed_tensor->data;
    compressed_image.size = packed_tensor->size;
    compressed_image.read_index = 0;
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);

    // the code in this if statement gets called if libpng encounters an error
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "error from libpng\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return 0;
    }

    // init png reading from out buffer
    png_set_read_fn(png_ptr, &compressed_image, my_png_read_data);

    // read all the info up to the image data
    png_read_info(png_ptr, info_ptr);
    int width, height, bit_depth, colour_type;
    png_uint_32 png_width, png_height;
    png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height, &bit_depth, &colour_type, 0, 0, 0);
    width = png_width;
    height = png_height;
    //printf("height = %d, width = %d\n", height, width);
    int expected_size = 1;
    for(int i = 0; i < tensor_dimensions->size; ++i)
        expected_size *= tensor_dimensions->data[i];

    if(width * height != expected_size)
        abort_("Packed tensor has unexpected dimensions");
    THByteTensor * unpacked_image_tensor = THByteTensor_newWithSize(tensor_dimensions, NULL);

    byte * tensor_data = THByteTensor_data(unpacked_image_tensor);
    png_bytep * row_pointers = (png_bytep *)malloc(height * sizeof(png_bytep));
    const int row_stride = width;
    for(int i = 0; i < height; ++i)
        row_pointers[i] = &tensor_data[unpacked_image_tensor->storageOffset + i*row_stride];

    png_read_image(png_ptr, row_pointers);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    free(row_pointers);

    return unpacked_image_tensor;
}

static int libcompress_Main_pack(lua_State *L) {
  THByteTensor *image_tensor = luaT_checkudata(L, 1, "torch.ByteTensor");
  THByteStorage *packed_tensor = libcompress_pack_png_string(image_tensor);
  luaT_pushudata(L, packed_tensor, "torch.ByteStorage");
  return 1;
}


static int libcompress_Main_unpack(lua_State *L) {
  THByteStorage *packed_tensor = luaT_checkudata(L, 1, "torch.ByteStorage");
  THLongStorage *tensor_dimensions = luaT_checkudata(L, 2, "torch.LongStorage");
  THByteTensor *image_tensor = libcompress_unpack_png_string(packed_tensor, tensor_dimensions);
  luaT_pushudata(L, image_tensor, "torch.ByteTensor");
  return 1;
}

static const luaL_reg libcompress__Main__[] =
{
  {"compress", libcompress_Main_pack},
  {"decompress", libcompress_Main_unpack},
  {NULL, NULL}
};

DLL_EXPORT int luaopen_libcompress(lua_State *L)
{
  luaL_register(L, "libcompress", libcompress__Main__);
  return 1;
}
