
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
#define abort_ _compress_abort_
#define torch_(NAME) TH_CONCAT_3(torch_, Real, NAME)
#define torch_Tensor TH_CONCAT_STRING_3(torch., Real, Tensor)
#define libcompress_(NAME) TH_CONCAT_3(libcompress_, Real, NAME)

#include "generic/libcompress.c"
#include "THGenerateAllTypes.h"

DLL_EXPORT int luaopen_libcompress(lua_State *L)
{
  libcompress_FloatMain_init(L);
  libcompress_DoubleMain_init(L);
  libcompress_ByteMain_init(L);

  luaL_register(L, "libcompress.double", libcompress_DoubleMain__);
  luaL_register(L, "libcompress.float", libcompress_FloatMain__);
  luaL_register(L, "libcompress.byte", libcompress_ByteMain__);

  return 1;
}
