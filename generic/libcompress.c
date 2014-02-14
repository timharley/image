#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/libcompress.c"
#else

/*
 * Pack image Tensors in to memory buffers using libpng
 */


static THTensor * libcompress_(pack_png_string)(THTensor * image_tensor)
{
    return image_tensor;
}

static THTensor * libcompress_(unpack_png_string)(THTensor * packed_tensor)
{
    return packed_tensor;
}

static int libcompress_(Main_pack)(lua_State *L) {
  THTensor *image_tensor = luaT_checkudata(L, 1, torch_Tensor);
  THTensor *packed_tensor = libcompress_(pack_png_string)(image_tensor);
  luaT_pushudata(L, packed_tensor, torch_Tensor);
  return 1;
}


static int libcompress_(Main_unpack)(lua_State *L) {
  THTensor *packed_tensor = luaT_checkudata(L, 1, torch_Tensor);
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
