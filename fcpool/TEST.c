﻿#include "fcalloctor.h"


#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static void inc_output_d(const char* fmt, ...) {
  va_list vArgList;
  va_start(vArgList, fmt);
  vprintf(fmt, vArgList);
  va_end(vArgList);

}

static size_t mf_count = 0;

#ifdef FC_MEM_DBG
static void* ins_malloc_d(void* m_env, const char* flag, size_t s) {
  PM_ENV m_env_dbg = (PM_ENV)m_env;
  m_env_dbg->ins.putstring("%s 0x%08X\r\n", flag, s);
  mf_count++;
  return malloc(s);
}

static void* ins_realloc_d(void* m_env, const char* flag, void* p, size_t s) {
  PM_ENV m_env_dbg = (PM_ENV)m_env;
  m_env_dbg->ins.putstring("%s 0x%08X\r\n", flag, s);
  return realloc(p, s);
}

static void ins_free_d(void* m_env, const char* flag, void* p) {
  PM_ENV m_env_dbg = (PM_ENV)m_env;
  m_env_dbg->ins.putstring("%s \r\n", flag);
  mf_count--;
  free(p);
}
#else
static void* ins_malloc_d(size_t s) {
  mf_count++;
  return malloc(s);
}

static void* ins_realloc_d(void* p, size_t s) {
  return realloc(p, s);
}

static void ins_free_d(void* p) {
  mf_count--;
  free(p);
}
#endif

void BSOD() {
  printf("error!\r\n");
  system("pause");
  exit(-1);
}

// 测试外部导入分配器环境
// 由于 dbg 环境下，
// malloc_fn free_fn realloc_fn
// 具备调试信息
// 故需要写两份
void Test1() {


  M_ENV m_env = { sizeof(m_env) };

  m_env.ins.malloc_fn   = ins_malloc_d;
  m_env.ins.free_fn     = ins_free_d;
  m_env.ins.realloc_fn  = ins_realloc_d;
  m_env.ins.BSOD        = BSOD;

#ifdef FC_MEM_DBG
  m_env.ins.putstring   = inc_output_d;
#endif

  if (!FC_SUCCESS(FInitAllocator(m_env))) {
    printf("init error\r\n");
    return;
  }

  for (size_t i = 0; i < 10000; i++) {

    // -8 将全部分配到池内
    size_t s = rand() % (MAX_BLOCK_IN_POOL_SIZE - 6 /* 8 */) + 8;

    char* t = m_env.FAllocate(m_env, s);

    m_env.FDeallocate(m_env, t);
  }

  FDestoryAllocator(m_env);

  printf("mf_count : %d\r\n", mf_count);
  system("pause");
}

#ifdef FC_MEM_DEFAULT
// 测试内部默认分配器环境
void Test2() {

  M_ENV m_env = { 0 };

  if (!FC_SUCCESS(FInitAllocator(m_env))) {
    printf("init error\r\n");
    return;
  }

  for (size_t i = 0; i < 10000; i++) {

    // -8 将全部分配到池内
    size_t s = rand() % (MAX_BLOCK_IN_POOL_SIZE - 8) + 8;

    char* t = m_env.FAllocate(m_env, s);

    m_env.FDeallocate(m_env, t);
  }

  FDestoryAllocator(m_env);
  system("pause");
}
#endif

int main() {
  srand((unsigned int)(time(NULL)));
  Test1();

#ifdef FC_MEM_DEFAULT
  Test2();
#endif

  return 0;
}