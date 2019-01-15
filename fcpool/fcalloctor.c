#include "fcalloctor.h"

#include <string.h>

#ifdef FC_MEM_DBG
#ifdef FC_MEM_DEFAULT

#include <stdio.h>
#include <stdarg.h>

// mf 计数器
size_t mf_count = 0;

#endif  // FC_MEM_DEFAULT
#endif  // FC_MEM_DBG

static void* ins_malloc(PM_ENV m_env, size_t s)
{
  ERR_HANDLER my_handler;
  void* ret;

  for (;;) {                                                     // 不断尝试释放、分配、再释放、再分配...
    my_handler = m_env->ins.alloc_failed_handler;
    if (NULL == my_handler) {                                    // 无法处理, 退出应用或当机
      m_env->ins.BSOD();
    }

    (*my_handler)();                                             // 调用指定函数，尝试释放内存

#ifdef FC_MEM_DBG
    ret = m_env->ins.malloc_fn(m_env, __FUNCTION__, s);          // 再次尝试分配内存
#else
    ret = m_env->ins.malloc_fn(s);                               // 再次尝试分配内存
#endif  // FC_MEM_DBG

    if (ret) return(ret);
  }
}

static void* ins_realloc(PM_ENV m_env, void* p, size_t ns)
{
  ERR_HANDLER my_handler;
  void* ret;

  for (;;) {
    my_handler = m_env->ins.alloc_failed_handler;
    if (NULL == my_handler) {
       m_env->ins.BSOD();
    }

    (*my_handler)();

#ifdef FC_MEM_DBG
    ret = m_env->ins.realloc_fn(m_env, __FUNCTION__, p, ns);
#else
    ret = m_env->ins.realloc_fn(p, ns);
#endif  // FC_MEM_DBG
    
    if (ret) return(ret);
  }
}

// 一级分配器
// 一级配置器直接调用默认分配器
// 在内存分配失败时,调用 ins_malloc 尝试恢复或退出
static void* malloc_s(PM_ENV m_env, size_t s) {
  void* ret;

#ifdef FC_MEM_DBG
  ret = m_env->ins.malloc_fn(m_env, __FUNCTION__, s);          // 再次尝试分配内存
#else
  ret = m_env->ins.malloc_fn(s);                               // 再次尝试分配内存
#endif  // FC_MEM_DBG

  if (NULL == ret) ret = ins_malloc(m_env, s);
  return ret;
}

// 同 malloc_s()
static void* realloc_s(PM_ENV m_env, void* p, size_t ns) {
  void* ret;
  
#ifdef FC_MEM_DBG
  ret = m_env->ins.realloc_fn(m_env, __FUNCTION__, p, ns);
#else
  ret = m_env->ins.realloc_fn(p, ns);
#endif  // FC_MEM_DBG

  if (NULL == ret) ret = ins_realloc(m_env, p, ns);
  return ret;
}

// 一级配置器直接使用默认释放器
static void free_s(PM_ENV m_env, void* p) {
  // unref
  m_env;

#ifdef FC_MEM_DBG
  m_env->ins.free_fn(m_env, __FUNCTION__, p);
#else
  m_env->ins.free_fn(p);
#endif  // FC_MEM_DBG

}

// 对齐
static size_t round_up(size_t s) {
  return (((s)+(size_t)ALIGN - 1) & ~((size_t)ALIGN - 1));
}

// 寻求下标
static  size_t freelist_index(size_t s) {
  return (((s)+(size_t)ALIGN - 1) / (size_t)ALIGN - 1);
}

static char* chunk_alloc(PM_ENV m_env, size_t s, size_t* headers) {
  char* ret;
  // 需求的总量为 块大小 * 块个数
  size_t total_bytes = s * (*headers);
  
  // 池内剩余空间
  size_t left_bytes  = m_env->ins.end_free - m_env->ins.start_free;

  if (left_bytes >= total_bytes) {
    // 剩余空间足够，可以满足 headers 个的内存分配
    ret = m_env->ins.start_free;
    m_env->ins.start_free += total_bytes;
    return ret;
  }
  else if (left_bytes >= s) {
    // 剩余空间不足， 但是能满足至少一个块分配
    // 那么能分配多少块就分配多少块
    *headers = left_bytes / s;
    total_bytes = s * (*headers);
    ret = m_env->ins.start_free;
    m_env->ins.start_free += total_bytes;
    return ret;
  }
  else {
    // 内存池余量不足
    // 新增内存: 需求量的2倍 + 随着配置次数增加而愈来愈大的附加量
    size_t get_bytes =
      2 * total_bytes + round_up(m_env->ins.heap_size >> 4);

    if (left_bytes > 0) {
      // 尝试将残余碎片利用起来
      union _Header** fl =  
        // 找一个合适的顶点
        m_env->ins.free_list + freelist_index(left_bytes);

        // 按照 dealloctor 的方式进行回收
        ((union _Header*)m_env->ins.start_free)->free_list_link = *fl;
        *fl = (union _Header*)m_env->ins.start_free;
    }

    // 使用默认分配器配置添加新的内存块
    union _Header* h = 
      (union _Header*)

#ifdef FC_MEM_DBG
      m_env->ins.malloc_fn(m_env, __FUNCTION__, get_bytes + sizeof(union _Header));          // 再次尝试分配内存
#else
      m_env->ins.malloc_fn(get_bytes + sizeof(union _Header));                               // 再次尝试分配内存
#endif  // FC_MEM_DBG

    if (likely(NULL != h)) {
      if (likely(m_env->ins.release_list))
        h->free_list_link = m_env->ins.release_list;
      else 
        h->free_list_link = NULL;

      m_env->ins.release_list = h;
      m_env->ins.start_free = (char*)h + sizeof(union _Header);
    }
    else {
      m_env->ins.start_free = NULL;
    }

    // 默认分配器也配置失败了
    if (NULL == m_env->ins.start_free) {
      union _Header** fl;
      union _Header*  hf;

      // 检查当前的 free_list 的块是否能合并出大块
      for (size_t i = s;
            i <= (size_t)MAX_BYTES;
            i += (size_t)ALIGN) {
        fl = m_env->ins.free_list + freelist_index(i);
		hf = *fl;
        if (NULL != hf) {
          *fl = hf->free_list_link;
          m_env->ins.start_free = (char*)hf;
          m_env->ins.end_free   = m_env->ins.start_free + i;

          return chunk_alloc(m_env, s, headers);
        }
      }
      // 实在找不到了
      m_env->ins.end_free = NULL;
      // 调用第一级分配器, 尝试异常处理机制
      m_env->ins.start_free = (char*)malloc_s(m_env, get_bytes);
      
      // 这里会出现两种结果，其一是通过异常恢复解决问题
      // 二是进入 BSOD ，尴尬的退出
    }

    // 修正 heap_size 和 end_free
    m_env->ins.heap_size += get_bytes;
    m_env->ins.end_free  = m_env->ins.start_free + get_bytes;

    // 修正 headers
    return chunk_alloc(m_env, s, headers);
  }
}

static void* refill(PM_ENV m_env, size_t s) {
  size_t headers = 20;
  char* chunk = chunk_alloc(m_env, s, &headers);
  union _Header** fl;
  union _Header*  ret;
  union _Header*  cur;
  union _Header*  next;

  // 如果只获得了一块，直接返回
  if (unlikely(1 == headers)) return chunk;

  fl =
    m_env->ins.free_list + freelist_index(s);

  ret = (union _Header*) chunk;

  // 修正 chunk, 将其串接

  // 指向第二块
  *fl = next = (union _Header*)(chunk + s);

  for (size_t i = 1; /* nop */; i++) {
    cur = next;
    // 偏移一个 block ，即可到达下一块
    next = (union _Header*)((char*)next + s);

    if (headers - 1 == i) {
      cur->free_list_link = NULL;
      break;
    }
    cur->free_list_link = next;
  }

  // 返回第一块
  return ret;
}

// 如果 s 是0， 至少会分配 BLOCK_SIZE 的空间
static void* ins_Allocate(PM_ENV m_env, size_t s) {
  void* ret;
  s += BLOCK_SIZE;

  if (s > (size_t)MAX_BYTES) {
    ret = malloc_s(m_env, s);
  }
  else {
    union _Header** fl = 
      m_env->ins.free_list + freelist_index(s);

#ifdef MUTIL_THREAD
    // lock
    m_env->ins.lock(m_env->ins.ctx);
#endif  // FC_MEM_MUTIL_THREAD

    union _Header* h = *fl;

    if (NULL == h) {
      ret = refill(m_env, round_up(s));
    }
    else {
      *fl = h->free_list_link;
      ret = h;
    }

#ifdef MUTIL_THREAD
    // lock release
    m_env->ins.unlock(m_env->ins.ctx);
#endif  // FC_MEM_MUTIL_THREAD

  }

  // 在块头设置大小，用于回收
  ((struct _block*)ret)->size = s;
  ret = (void*)((char*)ret + BLOCK_SIZE);

  return ret;
}

static void ins_Deallocate(PM_ENV m_env, void* p) {

  struct _block* b = (struct _block*)((char*)p - BLOCK_SIZE);

  // 根据 size 识别是否分配在池内
  if (b->size > (size_t)MAX_BYTES)
    free_s(m_env, b);
  else {

    union _Header**  fl
      = m_env->ins.free_list + freelist_index(b->size);
    union _Header* h = (union _Header*)b;

#ifdef FC_MEM_MUTIL_THREAD
    // lock
    m_env->ins.lock(m_env->ins.ctx);
#endif  // FC_MEM_MUTIL_THREAD

    h->free_list_link = *fl;
    *fl = h;

#ifdef FC_MEM_MUTIL_THREAD
    // lock release
    m_env->ins.unlock(m_env->ins.ctx);
#endif  // FC_MEM_MUTIL_THREAD

  }
}

static void* ins_Reallocate(PM_ENV m_env, void* p, size_t ns) {
  void* ret;
  size_t cs;
  struct _block* b = (struct _block*)((char*)p - BLOCK_SIZE);

  // 如果旧的大小大于池内粒度且新的大于最大池内粒度，则通过一级分配器分配
  if (b->size > (size_t)MAX_BYTES && ns > (size_t)MAX_BYTES) {
    b->size = ns;
    return(realloc_s(m_env, b, ns));
  }

  // 如果新分配内存小于 block size, 直接返回
  if (round_up(b->size) == round_up(ns)) return(p);

  // 否则通过二级分配器重新分配, 二级分配器会间接调用一级分配器
  ret = ins_Allocate(m_env, ns);
  cs = ns > b->size ? b->size : ns;
  memcpy(ret, p, cs);
  // 回收旧的
  ins_Deallocate(m_env, p);
  return ret;
}


// 默认的分配器

#ifdef FC_MEM_DEFAULT

static void ins_alloc_failed_handler_d() {
  /*
    do nothing
  */
  return;
}

static void ins_BSOD_d() {
  /*
    do nothing
  */
  return;
}

static F_STAT_MALLOC(ins_malloc_d, s) {
  F_STAT_ARG(d, f);
  F_DBG(d, "%s 0x%016X\r\n", f, s);

#ifdef FC_MEM_DBG
  mf_count++;
#endif

  return malloc(s);
}

static F_STAT_REALLOC(ins_realloc_d, p, ns) {
  F_STAT_ARG(d, f);
  F_DBG(d, "%s 0x%16X\r\n", f, ns);
  return realloc(p, ns);
}

static F_STAT_FREE(ins_free_d, p) {
  F_STAT_ARG(d, f);
  F_DBG(d, "%s \r\n", f);

#ifdef FC_MEM_DBG
  mf_count--;
#endif

  free(p);
}

#endif  // FC_MEM_DEFAULT

#ifdef FC_MEM_DBG
static void inc_output_d(const char* fmt, ...) {
	va_list vArgList;
	va_start(vArgList, fmt);
	vprintf(fmt, vArgList);
	va_end(vArgList);
}
#endif  // FC_MEM_DBG


int InitAllocator(PM_ENV m_env) {

#ifdef FC_MEM_DEFAULT
  if (m_env->check_size == 0) {
    memset(m_env, 0, sizeof(m_env));
    m_env->check_size     = sizeof(m_env);
    m_env->ins.malloc_fn  = ins_malloc_d;
    m_env->ins.realloc_fn = ins_realloc_d;
    m_env->ins.free_fn    = ins_free_d;


#ifdef FC_MEM_MUTIL_THREAD
    // no define
#endif  // FC_MEM_MUTIL_THREAD
   
	F_SET_DBG(m_env, inc_output_d);


    // error handler
    m_env->ins.BSOD                 = ins_BSOD_d;
    m_env->ins.alloc_failed_handler = ins_alloc_failed_handler_d;
    
  }
  else 
#endif  // FC_MEM_DEFAULT

  if (m_env->check_size != sizeof(M_ENV)) {
    return F_FAILED;
  }

  m_env->Allocate   = ins_Allocate;
  m_env->Deallocate = ins_Deallocate;
  m_env->Reallocate = ins_Reallocate;

#ifdef FC_ASSERT
  assert(NULL != m_env->ins.malloc_fn);
  assert(NULL != m_env->ins.free_fn);
  assert(NULL != m_env->ins.realloc_fn);

  assert(NULL != m_env->ins.BSOD);

  // 不强制要求具备恢复功能
  // assert(NULL != m_env->ins.alloc_failed_handler);

#ifdef FC_MEM_MUTIL_THREAD
  assert(NULL != m_env->ins.lock);
  assert(NULL != m_env->ins.unlock);
#endif  // FC_MEM_MUTIL_THREAD

#ifdef FC_MEM_DBG
  assert(NULL != m_env->ins.putstring_fn);
#endif  // FC_MEM_DBG

#endif  // FC_ASSERT

  return F_OK;
}

void DestoryAllocator(PM_ENV m_env) {

  for (union _Header* next =
        m_env->ins.release_list;
        next != NULL;
        /* nop */) {
    void* p = next;
    next    = next->free_list_link;

#ifdef FC_MEM_DBG
    m_env->ins.free_fn(m_env, __FUNCTION__, p);
#else
    m_env->ins.free_fn(p);
#endif  // FC_MEM_DBG

  }

  F_DBG(m_env, "mf_count : %d\r\n", mf_count);
}
