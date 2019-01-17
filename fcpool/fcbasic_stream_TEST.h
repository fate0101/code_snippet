#ifndef fcbasic_stream_h
#define fcbasic_stream_h

#include "config.h"
#include "fclock.h"
#include "fcobject.h"

// 暂定
#define FC_EOF  (-1)

// block 串
struct __ins_stream_block {
  struct __ins_stream_block* front;
  struct __ins_stream_block* next;
  size_t                     block_szie;
  char                       head[1];    // 担心触发警告
};

// 视图构成块
struct __ins_position {
  struct __ins_stream_block* block;
  char* pos;
};

typedef struct __stream {

  // 读取操作会影响 pos
  size_t (*Read)   (struct __stream* st, char* p, size_t s);

  // peek 操作将不会影响 pos
  size_t (*Peek)   (struct __stream* st, char* p, size_t s);

  size_t (*Write)  (struct __stream* st, char* p, size_t s);


  // 如果没有数据则获取失败, 返回 EOF
  char   (*getc)   (struct __stream* st);
  // 如果没有数据则回退失败, 返回 EOF
  char   (*ungetc) (struct __stream* st);

  struct {
    void*  allocator;                              // 分配器

    struct __ins_stream_block*    head;
    struct __ins_stream_block*    end;             // 尾节点
    struct {
      struct __ins_position       vstart;          // 当前指向 
      struct __ins_position       vtail;           // 当前结束位置
    } view;

    size_t readable_size;                          // 可读大小
    size_t heap_size;                              // 当前堆大小
    size_t _back_size;                             // 最大可回退大小

    F_STAT_LOCK_WITHOUT_SEM                        // 快速声明 lock
  } ins;
}stream, *stream_ptr;

// 只允许堆上构造，禁止拷贝
stream_ptr N_ConstructorStream(void* allocator, unsigned short flag);
void       DeconstructionStream(stream_ptr st);

#endif  // fcbasic_stream_h