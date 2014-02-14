#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/compress.c"
#else

/*
 * Pack image Tensors in to memory buffers using libpng
 */


static THTensor * compress_(pack_png_string)(THTensor * image_tensor)
{
    return image_tensor;
}

static THTensor * compress_(unpack_png_string)(THTensor * packed_tensor)
{
    return packed_tensor;
}

static int compress_(Main_pack)(lua_State *L) {
  THTensor *image_tensor = luaT_checkudata(L, 1, torch_Tensor);
  THTensor *packed_tensor = compress_(pack_png_string)(image_tensor);
  luaT_pushudata(L, packed_tensor, torch_Tensor);
  return 1;
}


static int compress_(Main_unpack)(lua_State *L) {
  THTensor *packed_tensor = luaT_checkudata(L, 1, torch_Tensor);
  THTensor *image_tensor = compress_(unpack_png_string)(packed_tensor);
  luaT_pushudata(L, image_tensor, torch_Tensor);
  return 1;
}

static const luaL_reg compress_(Main__)[] =
{
  {"compress", compress_(Main_pack)},
  {"decompress", compress_(Main_unpack)},
  {NULL, NULL}
};

DLL_EXPORT int compress_(Main_init)(lua_State *L)
{
  luaT_pushmetatable(L, torch_Tensor);
  luaT_registeratname(L, compress_(Main__), "compress");
  return 1;
}

#endif
