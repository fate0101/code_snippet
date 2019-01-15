#ifndef config_h
#define config_h

/*
  初始化 likely 和 unlikeyly
*/
#ifndef GCC
#define likely
#define unlikely
#endif

/*
  是否允许断言
*/
#ifndef FC_ASSERT
#define FC_ASSERT
#include <assert.h>
#endif

/*
  是否拥有默认初始化方案
*/
#ifndef FC_MEM_DEFAULT
#define FC_MEM_DEFAULT
#include <stdlib.h>
#endif

/*
  是否打印调试日志
*/
#define FC_MEM_DBG

/*
  来自 fcerr
*/
enum ERROR_CODE {
  F_OK = 0,
  F_FAILED = -1,
  F_UNSUPPORT = -2,
};

#define FC_SUCCESS(x) (x >= F_OK)

#endif  // config_h