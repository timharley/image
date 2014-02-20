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

void png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    struct mem_buffer* p=(struct mem_buffer*)png_get_io_ptr(png_ptr);
    if(p->read_index + length > p->size)
        abort_("libcompress.decompress.png_read_data: Tried to read past the end of a buffer!");
    memcpy(data, p->buffer + p->read_index, length);
    p->read_index = p->read_index + length;
}

void png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
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
        abort_("libcompress.compress.png_write_data: Failed to allocate buffer!");


    /* copy new bytes to end of buffer */
    memcpy(p->buffer + p->size, data, length);
    p->size = nsize;
}

/* The libpng specification specifies that if providing your own write_fn,
you need to provide a flush fn too, but this never gets called.*/
void png_flush(png_structp png_ptr) { }

// Pack the underlying tensor data from a Tensor into a PNG string.
// We don't attempt to save any space/work if the Tensor describes an odd view of a storage
// We assume that the most 2 contiguous dimensions of the storage describe an image, 
// i.e. That a k x m x n Tensor describes k images.
// We collapse any higher dimensions whilst compressing a tensor with >2 dimensions.
// this is equivalent to vertically 'stacking' each image in the Tensor.
// This means that e.g. a 3 x m x n colour image as loaded by image.load will be i
// compressed completely differently by this function compared to it's representation in file.
static THByteStorage * libcompress_pack_png_string(THByteTensor * image_tensor)
{
    struct mem_buffer compressed_image;
    compressed_image.buffer = NULL;
    compressed_image.size = 0;
    compressed_image.read_index = 0;

    //libpng needs each row to be contiguous.
    //We also assume every thing is contiguous when populating row_pointers below.
    //If the tensor is contiguous, this doesn't do any extra work.
    //Note: we need to free this later.
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
        if (!write_ptr) abort_("libcompress.compress: couldn't create png_write_struct");

        png_infop write_info_ptr = png_create_info_struct(write_ptr);
        if (!write_info_ptr) abort_("libcompress.compress: couldn't create png_info_struct");

        png_set_write_fn(write_ptr, &compressed_image, png_write_data, png_flush);

        png_set_IHDR(write_ptr, write_info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, 
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        png_write_info(write_ptr, write_info_ptr);
        png_write_image(write_ptr, row_pointers);
        png_destroy_write_struct(&write_ptr, &write_info_ptr);
        free(row_pointers);
    }

    //Can now free (or reduce the reference count of) our contiguous tensor.
    THByteTensor_free(tensorc);

    //The byte storage now assumes control of the memory buffer.
    THByteStorage * png_string = THByteStorage_newWithData(compressed_image.buffer, compressed_image.size);
    return png_string;
}

static THByteTensor * libcompress_unpack_png_string(THByteStorage * packed_tensor, THLongStorage * tensor_dimensions)
{
    //Set up struct to allow libpng to read from the THByteStorage of compressed data
    struct mem_buffer compressed_image;
    compressed_image.buffer = packed_tensor->data;
    compressed_image.size = packed_tensor->size;
    compressed_image.read_index = 0;

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);

    // the code in this if statement gets called if libpng encounters an error
    if (setjmp(png_jmpbuf(png_ptr))) {
        abort_("libcompress.decompress: libpng error.");
    }

    // init png reading from out buffer
    png_set_read_fn(png_ptr, &compressed_image, png_read_data);

    // read all the info up to the image data
    png_read_info(png_ptr, info_ptr);
    int width, height, bit_depth, colour_type;
    png_uint_32 png_width, png_height;
    png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height, &bit_depth, &colour_type, 0, 0, 0);
    width = png_width;
    height = png_height;

    //Check that the packed data has the expected number of bytes
    int expected_size = 1;
    for(int i = 0; i < tensor_dimensions->size; ++i)
        expected_size *= tensor_dimensions->data[i];

    if(width * height != expected_size)
        abort_("libcompress.decompress: Packed tensor size does not match expected size.");

    //Create our tensor, and write directory in to it's storage with libpng
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
