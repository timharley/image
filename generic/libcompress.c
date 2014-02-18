#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/libcompress.c"
#else
#ifdef TH_REAL_IS_BYTE
/*
 * Pack image Tensors in to memory buffers using libpng
 */


static THByteStorage * libcompress_(pack_png_string)(THTensor * image_tensor)
{
    printf("%p\n", (void*)libcompress_(pack_png_string));
    printf("%lu\n", sizeof(real));

    struct mem_buffer compressed_image;
    compressed_image.buffer = NULL;
    compressed_image.size = 0;
    compressed_image.read_index = 0;

    THTensor *tensorc = THTensor_(newContiguous)(image_tensor);
    real *tensor_data = THTensor_(data)(tensorc);
   // if(tensorc->nDimension == 2)
    
    int width = tensorc->size[tensorc->nDimension-1];
    int height = 1;
    for(int i = 0; i < tensorc->nDimension-1; ++i)
        height = height*tensorc->size[i];

    printf("height = %d, width = %d\n", height, width);
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

        fprintf(stderr, "header:\n");
        png_write_info(write_ptr, write_info_ptr);
        fprintf(stderr, "Image:\n");
        png_write_image(write_ptr, row_pointers);
        fprintf(stderr, "Footer:\n");
        //png_write_end(write_ptr, NULL);
        char out_file_name[] = "/Users/tim.harley/git/pngtest/buffer_dmt.png";
        FILE *wfp = fopen(out_file_name, "wb");
        fwrite(compressed_image.buffer, 1, compressed_image.size, wfp);
        fclose(wfp);
    }
    THByteStorage * png_string = THByteStorage_newWithData(compressed_image.buffer, compressed_image.size);
    return png_string;
}

static THTensor * libcompress_(unpack_png_string)(THByteStorage * packed_tensor)
{
    printf("%p\n", (void*)libcompress_(unpack_png_string));

    //THStorage * image_data = THStorage_(newWithSize)(packed_tensor->size());
    //long size = {3, 512, 512};
    //THLongStorage * size_vec = THLongStorage_newWithData(size, 3);
    THTensor * unpacked_image_tensor = THTensor_(newWithSize3d)(3, 512, 512);
    //THLongStorage_free(size_vec);
    return unpacked_image_tensor;
}

static int libcompress_(Main_pack)(lua_State *L) {
  THTensor *image_tensor = luaT_checkudata(L, 1, torch_Tensor);
  THByteStorage *packed_tensor = libcompress_(pack_png_string)(image_tensor);
  luaT_pushudata(L, packed_tensor, "torch.ByteStorage");
  return 1;
}


static int libcompress_(Main_unpack)(lua_State *L) {
  THByteStorage *packed_tensor = luaT_checkudata(L, 1, "torch.ByteStorage");
  THTensor *image_tensor = libcompress_(unpack_png_string)(packed_tensor);
  luaT_pushudata(L, image_tensor, torch_Tensor);
  return 1;
}

static const luaL_reg libcompress_(Main__)[] =
{
  {"compress", libcompress_(Main_pack)},
  {"decompress", libcompress_(Main_unpack)},
  {NULL, NULL}
};

DLL_EXPORT int libcompress_(Main_init)(lua_State *L)
{
  luaT_pushmetatable(L, torch_Tensor);
  luaT_registeratname(L, libcompress_(Main__), "libcompress");
  return 1;
}

#endif
#endif
