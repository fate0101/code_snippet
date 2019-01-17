#ifndef fcobject_h
#define fcobject_h

enum CSTR{
  IN_STACK = 0x0001,
  IN_HEAP  = 0x0002,

  IN_MULTI_THREAD   = 0x0010,
  IN_SINGLE_THREAD  = 0x0020,
};

#endif