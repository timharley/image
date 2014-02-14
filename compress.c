
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
#define compress_(NAME) TH_CONCAT_3(compress_, Real, NAME)

#include "generic/compress.c"
#include "THGenerateAllTypes.h"

DLL_EXPORT int luaopen_compress(lua_State *L)
{
  compress_FloatMain_init(L);
  compress_DoubleMain_init(L);
  compress_ByteMain_init(L);

  luaL_register(L, "compress.double", compress_DoubleMain__);
  luaL_register(L, "compress.float", compress_FloatMain__);
  luaL_register(L, "compress.byte", compress_ByteMain__);

  return 1;
}
