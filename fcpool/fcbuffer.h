#ifndef fcbuffer_h
#define fcbuffer_h

#include "config.h"
#include "fclock.h"
#include "fcobject.h"

/*
  虽然叫 string ,但是不以 0 结尾
*/
typedef struct __string {
  void        (*AppendBuffer)     (struct __string* ts, char* p, size_t s);
  void        (*AppendString)     (struct __string* ts, char* p); 
  void        (*AppendRune)       (struct __string* ts, char c);
  void        (*Insert)           (struct __string* ts, size_t pos, char* p, size_t s);
  int         (*FindBuffer)       (struct __string* ts, size_t pos, char* p, size_t s);
  int         (*FindRune)         (struct __string* ts, char c);
  size_t      (*Len)              (struct __string* ts);
                                  
  size_t      (*Read)             (struct __string* ts, char* p, size_t s);
  size_t      (*Write)            (struct __string* ts, char* p, size_t s);     // 等同于 AppendString

  // 线程非安全的
  const char* (*GetBuffer_thread_unsafe) (struct __string* ts);
  // 未实现的
  void        (*AppendTS)         (struct __string* dts, struct __string* sts); 

  struct {
    void*          allocator;                                             // 减少头文件负担
    char*          _00oo00_buffer;                                        // 禁止直接操作 buffer

    size_t         *ref_cout_;                                            // 引用计数
    size_t         len;
    unsigned short flag;                                                  // IN_STACK

    F_STAT_LOCK_WITHOUT_SEM                                               // 快速声明 lock
  } ins;
}*string_ptr;

///
// 栈上构造与堆上构造区别不大

/*
  栈上构造, 禁止栈上构造
*/
// string_ptr ConstructorString            (string_ptr ts, void* allocator);
// string_ptr ConstructorStringWithRune    (string_ptr ts, void* allocator, char c);
// string_ptr ConstructorStringWithBuffer  (string_ptr ts, void* allocator, char* p, size_t s);
// string_ptr ConstructorStringWithSize    (string_ptr ts, void* allocator, size_t s);

/*
  堆上构造
*/
string_ptr N_ConstructorString                 (void* allocator, unsigned short flag);
string_ptr N_ConstructorStringWithRune         (void* allocator, unsigned short flag, char c);
string_ptr N_ConstructorStringWithBuffer       (void* allocator, unsigned short flag, char* p, size_t s);
string_ptr N_ConstructorStringWithSize_unsafe  (void* allocator, unsigned short flag, size_t s);

/*
  拷贝构造
*/
// string_ptr ConstructorStringWithString  (string_ptr ts, void* allocator, string_ptr sts);
string_ptr N_ConstructorStringWithString(string_ptr sts);

/*
  析构
*/
void       DeconstructionString(string_ptr ts);


#define F_STRING(x)      N_ConstructorString(x, IN_SINGLE_THREAD)

#ifdef  MULTI_THREAD
#define F_STRING_SAFE(x) N_ConstructorString(x, IN_MULTI_THREAD)
#endif

#define F_DSTRING(x)     DeconstructionString(x)

#endif
