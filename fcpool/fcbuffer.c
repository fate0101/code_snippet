#include "fcbuffer.h"
#include "fcallocator.h"

#include <string.h>

#define ALLOCATOR(x)       ((PM_ENV)x)
#define GET_TS_ALLCATOR(x) ALLOCATOR((x)->ins.allocator)

// 该宏不能在判断buffer有效前调用
#define GET_TS_REF(x)      (*((x)->ins.ref_cout_))

static size_t* iins_new_ref_cout(struct __string* ts) {
  return GET_TS_ALLCATOR(ts)->Allocate(GET_TS_ALLCATOR(ts), sizeof(size_t));
}

static void iins_delete_ref_cout(struct __string* ts) {
  GET_TS_ALLCATOR(ts)->Deallocate(GET_TS_ALLCATOR(ts), ts->ins.ref_cout_);
  ts->ins.ref_cout_ = NULL;
}

static void iins_realse_buffer(struct __string* ts) {
  if (NULL ==  ts->ins._00oo00_buffer)
    return;

  GET_TS_REF(ts)--;

  if (0 != GET_TS_REF(ts))
    return;

  // 释放旧的
  GET_TS_ALLCATOR(ts)->Deallocate(GET_TS_ALLCATOR(ts),
     ts->ins._00oo00_buffer);

  ts->ins._00oo00_buffer = NULL;

  // 释放旧的引用计数
  iins_delete_ref_cout(ts);
}

static size_t ins_Len(struct __string* ts) {
  return ts->ins.len;
}

static void ins_AppendBuffer(struct __string* ts, char* p, size_t s) {
  char* ns;
  if (NULL == ts->ins._00oo00_buffer) {
    ts->ins._00oo00_buffer = 
    GET_TS_ALLCATOR(ts)->Allocate(GET_TS_ALLCATOR(ts), s);
    memcpy(ts->ins._00oo00_buffer, p, s);

    // 构建计数器
    ts->ins.ref_cout_ = iins_new_ref_cout(ts);
    GET_TS_REF(ts) = 1;
  }
  else {
    if (GET_TS_REF(ts) <= 1) {
      ts->ins._00oo00_buffer = 
      GET_TS_ALLCATOR(ts)->Reallocate(GET_TS_ALLCATOR(ts), 
                                        ts->ins._00oo00_buffer, 
                                        ts->ins.len + s);
      memcpy(ts->ins._00oo00_buffer + ts->ins.len, p, s);
    }
    else {
      ns = GET_TS_ALLCATOR(ts)->Allocate(GET_TS_ALLCATOR(ts), ts->ins.len + s);
      if (ts->ins.len != 0)
        memcpy(ns, ts->ins._00oo00_buffer, ts->ins.len);

      memcpy(ns + ts->ins.len, p, s);

      ts->ins._00oo00_buffer = ns;

      // 减少老的引用计数，同时增加自身引用计数
      GET_TS_REF(ts)--;
      ts->ins.ref_cout_ = iins_new_ref_cout(ts);
      GET_TS_REF(ts) = 1;
    }
  }
  ts->ins.len = ts->ins.len + s;
}

static void ins_AppendString(struct __string* ts, char* p) {
  size_t s = strlen(p) + 1;
  ins_AppendBuffer(ts, p, s);
}

static void ins_AppendRune(struct __string* ts, char c) {
  ins_AppendBuffer(ts, &c, 1);
}

static void ins_Insert(struct __string* ts, size_t pos, char* p, size_t s) {
  char*  ns;
  size_t nl;
  size_t ls;
  if (NULL ==  ts->ins._00oo00_buffer || pos >= ts->ins.len) {
    ins_AppendBuffer(ts, p, s);
    return;
  }

  nl = s + ts->ins.len;
  ns = GET_TS_ALLCATOR(ts)->Allocate(GET_TS_ALLCATOR(ts), nl);
  if (0 != pos)
    memcpy(ns, ts->ins._00oo00_buffer, pos);

  memcpy(ns + pos, p, s);

  ls = nl - s - pos;
  if (0 != ls)
    memcpy(ns + pos + s, ts->ins._00oo00_buffer + pos, ls);

  if (GET_TS_REF(ts) <= 1) {
    GET_TS_ALLCATOR(ts)->Deallocate(GET_TS_ALLCATOR(ts),
      ts->ins._00oo00_buffer);   
  }
  else {
    // 如果有别人用，无需释放，修改引用计数器即可
    GET_TS_REF(ts)--;
    ts->ins.ref_cout_ = iins_new_ref_cout(ts);
    GET_TS_REF(ts) = 1;   
  }
  ts->ins._00oo00_buffer = ns;
  ts->ins.len = nl;
}

static int ins_FindBuffer(struct __string* ts, size_t pos, char* p, size_t s) {
  int rs = -1;
  size_t i  = pos;
  size_t ii;

  if (0 == ts->ins.len || pos + s > ts->ins.len)
    return rs;

  for (; i < ts->ins.len; i++) {
    if (ts->ins._00oo00_buffer[i] != p[0])
      continue;

    ii = 1;
    for (; ii < s; ii++) {
      if (ts->ins._00oo00_buffer[i + ii] != p[ii]) {
        break;
      }
    }
    if (ii >= s - 1) {
      rs = (int)i;
      break;
    }
  }

  return rs;
}

static int ins_FindRune(struct __string* ts, char c) {
  return ins_FindBuffer(ts, 0, &c, 1);
}

static size_t ins_Read(struct __string* ts, char* p, size_t s) {
  size_t rs = 0;
  size_t ls = 0;    // 剩余的
  char*  np = NULL; // 新的 buffer 指针

  if (0 == s || NULL == ts->ins._00oo00_buffer)
    return rs;

  if (s > ts->ins.len)
    s = ts->ins.len;
  
  memcpy(ts->ins._00oo00_buffer, p, s);
  
  rs = s;
  ls = ts->ins.len - s;

  if (0 != ls) {
      np = GET_TS_ALLCATOR(ts)->Allocate(GET_TS_ALLCATOR(ts), ls);
      memcpy(np, ts->ins._00oo00_buffer, ls);
  }

  if (GET_TS_REF(ts) <= 1) {
    // 只有自己用，直接释放掉就行了
    GET_TS_ALLCATOR(ts)->Deallocate(GET_TS_ALLCATOR(ts),
      ts->ins._00oo00_buffer);    
  }
  else {
    // 如果有别人用，无需释放，修改引用计数器即可
    GET_TS_REF(ts)--;
    ts->ins.ref_cout_ = iins_new_ref_cout(ts);
    GET_TS_REF(ts) = 1;
  }

  ts->ins._00oo00_buffer = np;
  ts->ins.len = ls;

  return rs;
}

static size_t ins_Write(struct __string* ts, char* p, size_t s) {
  if (0 == s)
    return 0;
  
  ins_AppendBuffer(ts, p, s);
  return s;
}

static const char* ins_GetBuffer(struct __string* ts) {
  return ts->ins._00oo00_buffer;
}

#ifdef MULTI_THREAD

static void iins_realse_buffer_lock(struct __string* ts) {
  FC_LOCK_BEGIN(ts);
  iins_realse_buffer(ts);
  FC_LOCK_END(ts);
}

static void ins_AppendBuffer_lock(struct __string* ts, char* p, size_t s) {
  FC_LOCK_BEGIN(ts);
  ins_AppendBuffer(ts, p, s);
  FC_LOCK_END(ts);
}

static void ins_AppendString_lock(struct __string* ts, char* p) {
  FC_LOCK_BEGIN(ts);
  ins_AppendString(ts, p);
  FC_LOCK_END(ts);
}

static void ins_AppendRune_lock(struct __string* ts, char c) {
  FC_LOCK_BEGIN(ts);
  ins_AppendRune(ts, c);
  FC_LOCK_END(ts);
}

static void ins_Insert_lock(struct __string* ts, size_t pos, char* p, size_t s) {
  FC_LOCK_BEGIN(ts);
  ins_Insert(ts, pos, p, s);
  FC_LOCK_END(ts);
}

static int ins_FindBuffer_lock(struct __string* ts, size_t pos, char* p, size_t s) {
  int ret;
  FC_LOCK_BEGIN(ts);
  ret = ins_FindBuffer(ts, pos, p, s);
  FC_LOCK_END(ts);
  return ret;
}

static int ins_FindRune_lock(struct __string* ts, char c) {
  int ret;
  FC_LOCK_BEGIN(ts);
  ret = ins_FindRune(ts, c);
  FC_LOCK_END(ts);
  return ret;
}

static size_t  ins_Len_lock(struct __string* ts) {
  size_t ret;
  FC_LOCK_BEGIN(ts);
  ret = ts->ins.len;
  FC_LOCK_END(ts);
  return ret;
}

static size_t ins_Read_lock(struct __string* ts, char* p, size_t s) {
  size_t ret;
  FC_LOCK_BEGIN(ts);
  ret = ins_Read(ts, p, s);
  FC_LOCK_END(ts);
  return ret;
}

static size_t  ins_Write_lock(struct __string* ts, char* p, size_t s) {
  size_t ret;
  FC_LOCK_BEGIN(ts);
  ret = ins_Write(ts, p, s);
  FC_LOCK_END(ts);
  return ret;
}
#endif

string_ptr N_ConstructorString(void* allocator, unsigned short flag) {
  #ifdef FC_ASSERT
    assert(NULL != allocator);
  #endif
  string_ptr ts = ALLOCATOR(allocator)->Allocate(allocator, sizeof(struct __string));
  memset(ts, 0, sizeof(struct __string));

  ts->ins.allocator = allocator;
  GET_TS_ALLCATOR(ts)->Incref(allocator);

#ifdef MULTI_THREAD
  if (flag & IN_MULTI_THREAD) {
    ts->AppendBuffer     = ins_AppendBuffer_lock;
    ts->AppendRune       = ins_AppendRune_lock;
    ts->Insert           = ins_Insert_lock;
    ts->FindBuffer       = ins_FindBuffer_lock;
    ts->FindRune         = ins_FindRune_lock;
    ts->Len              = ins_Len_lock;
    ts->Read             = ins_Read_lock;
    ts->Write            = ins_Write_lock;
    ts->AppendString     = ins_AppendString_lock;
    ts->GetBuffer_thread_unsafe = ins_GetBuffer;
  }                      
  else {                 
#endif                   
                         
    ts->AppendBuffer     = ins_AppendBuffer;
    ts->AppendRune       = ins_AppendRune;
    ts->Insert           = ins_Insert;
    ts->FindBuffer       = ins_FindBuffer;
    ts->FindRune         = ins_FindRune;
    ts->Len              = ins_Len;
    ts->Read             = ins_Read;
    ts->Write            = ins_Write;
    ts->AppendString     = ins_AppendString;
    ts->GetBuffer_thread_unsafe = ins_GetBuffer;
#ifdef MULTI_THREAD
  }
#endif
  return ts;
}

string_ptr N_ConstructorStringWithRune(void* allocator, unsigned short flag, char c) {
  string_ptr ts = N_ConstructorString(allocator, flag);
  ins_AppendRune(ts, c);
  return ts;
}

string_ptr N_ConstructorStringWithBuffer(void* allocator, unsigned short flag, char* p, size_t s) {
  string_ptr ts = N_ConstructorString(allocator, flag);
  ins_AppendBuffer(ts, p, s);
  return ts;
}

// 可能出现直接使用buffer的情况, 由于把 buffer作为一个首地址指针造成
// 可以安排多个buffer指针 <分别指向头尾 和 当前位置>, 提供实用性
// 这个函数为构建二级池提供可能, 目前没这样做，标记为不安全
string_ptr N_ConstructorStringWithSize_unsafe(void* allocator, unsigned short flag, size_t s) {
  string_ptr ts = N_ConstructorString(allocator, flag);
  ts->ins.ref_cout_ = iins_new_ref_cout(ts);
  GET_TS_REF(ts) = 1;
  ts->ins._00oo00_buffer = GET_TS_ALLCATOR(ts)->Allocate(GET_TS_ALLCATOR(ts), ts->ins.len + s);
  memset(ts->ins._00oo00_buffer, 0, s);
  return ts;
}

// 拷贝构造函数
string_ptr N_ConstructorStringWithString(string_ptr sts) {
  string_ptr ts;
  FC_LOCK_BEGIN(sts);
  ts = N_ConstructorString(GET_TS_ALLCATOR(sts), sts->ins.flag);

  // 如果没有buffer 就不需要拷贝了
  if (NULL == sts->ins._00oo00_buffer)
    goto __ret;

  // 采用写时拷贝
  ts->ins._00oo00_buffer = sts->ins._00oo00_buffer;
  ts->ins.len = sts->ins.len;
  ts->ins.ref_cout_ = sts->ins.ref_cout_;
  GET_TS_REF(ts)++;

__ret:
  FC_LOCK_END(sts);
  return ts;
}

void DeconstructionString(string_ptr ts) {
  F_COPY_MTL_AND_LCOK(ts, mtl);
  iins_realse_buffer(ts);
  GET_TS_ALLCATOR(ts)->Decref(GET_TS_ALLCATOR(ts));
  GET_TS_ALLCATOR(ts)->Deallocate(GET_TS_ALLCATOR(ts), ts);
  F_COPY_MTL_AND_UNLCOK(mtl);
}