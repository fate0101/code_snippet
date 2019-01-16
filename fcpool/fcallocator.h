#ifndef fcallocator_h
#define fcallocator_h

#include <stddef.h>
#include "config.h"

/*
  err handler
*/
typedef void (*ERR_HANDLER)();

#ifdef FC_MEM_DBG
/*
  debug out put
*/
typedef  void    (_cdecl *OUTPUTDBG)(const char* fmt, ...);
typedef  void*   (_cdecl *MALLOC)   (void* m_env, const char* flag, size_t s);
typedef  void    (_cdecl *FREE)     (void* m_env, const char* flag, void* p);
typedef  void*   (_cdecl *REALLOC)  (void* m_env, const char* flag, void* p, size_t ns);

#else
/*
  默认分配器
*/
typedef  void*   (_cdecl *MALLOC)  (size_t s);
typedef  void    (_cdecl *FREE)    (void* p);
typedef  void*   (_cdecl *REALLOC) (void* p, size_t ns);

#endif  // FC_MEM_DBG


// 内存标志
struct _block {
  size_t size;
};

#define BLOCK_SIZE sizeof(struct _block)

enum ALLOCATE {
  ALIGN = 16,                                // 避免过多的移动
  MAX_BYTES = 272,                           // MAX_PATH = 260
  NFREELISTS = 17                            // MAX_BYTES / ALIGN
};

#define MAX_BLOCK_IN_POOL_SIZE (MAX_BYTES - BLOCK_SIZE)
#define GET_SIZE_BY_POINTER(x) ((struct _block*)((char*)p - sizeof(struct _block)))->size;


union _Header {
  union _Header* free_list_link;
  char data[1];
};

typedef struct _M_ENV {
  size_t   check_size;                       // check

  void* (*Allocate)  (struct _M_ENV* m_env, size_t s);
  void  (*Deallocate)(struct _M_ENV* m_env, void* p);
  void* (*Reallocate)(struct _M_ENV* m_env, void* p, size_t ns);
  
  // 引用计数处理
  void (*Incref)(struct _M_ENV* m_env);
  void (*Decref)(struct _M_ENV* m_env);

  struct { 
    MALLOC         malloc_fn;                // 默认分配器
    FREE           free_fn;
    REALLOC        realloc_fn;

    ERR_HANDLER    alloc_failed_handler;     // 尝试恢复
    ERR_HANDLER    BSOD;                     // 错误处理

#ifdef FC_MEM_DBG
    OUTPUTDBG      putstring_fn;             // 调试输出
#endif

    F_STAT_LOCK_WITHOUT_SEM

    union _Header* free_list[NFREELISTS];
    union _Header* release_list;             // 用于释放 freelist chunk
    char*          start_free;
    char*          end_free;
    size_t         heap_size;                // 总容量计数器

    size_t         ref_cout_;                // 引用计数
  } ins;                                     // 内部
}M_ENV, *PM_ENV;

int   ConstructorAllocator(PM_ENV m_env);
void  DeconstructionAllocator(PM_ENV m_env);

// 用于非提领快速操作
#define FAllocate(x,y)        Allocate(&x,y)
#define FDeallocate(x,y)      Deallocate(&x,y)
#define FReallocate(x,y,z,i)  Reallocate(&x,y,z,i)

// Init And Des
#define FConstructorAllocator(x)     ConstructorAllocator(&x)
#define FDeconstructionAllocator(x)  DeconstructionAllocator(&x)


// 用于快速配置的宏
#ifdef FC_MEM_DBG

#define F_STAT_MALLOC(x,y)    void* _cdecl x(void* m_env, const char* flag, size_t y)
#define F_STAT_FREE(x,y)      void  _cdecl x(void* m_env, const char* flag, void* y)
#define F_STAT_REALLOC(x,y,z) void* _cdecl x(void* m_env, const char* flag, void* y, size_t z)

#define F_STAT_ARG(x,y)       PM_ENV x = m_env; const char* y = flag; do{x,y;}while(0)
#define F_SET_DBG(x,y)        do{if (NULL != (x)) (x)->ins.putstring_fn = y;}while(0)
#define F_DBG(x, ...)         do{if (NULL != (x) && NULL != (x)->ins.putstring_fn) (x)->ins.putstring_fn(__VA_ARGS__);}while(0)

#else

#define F_STAT_MALLOC(x,y)    void* _cdecl x(size_t y)
#define F_STAT_FREE(x,y)      void  _cdecl x(void* y)
#define F_STAT_REALLOC(x,y,z) void* _cdecl x(void* y, size_t z)

#define F_STAT_ARG(x,y)       PM_ENV x = NULL; const char* y = NULL; do{x,y;}while(0)
#define F_SET_DBG(x,y)
#define F_DBG(x, ...)
#endif  // FC_MEM_DBG

#endif  // fcallocator_h
