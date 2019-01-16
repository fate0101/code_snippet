#ifndef fclock_h
#define fclock_h

#ifdef  MULTI_THREAD
/*
  lock
*/
typedef void (*LOCK_HANDLER)(void* ctx);

struct __mtl{
    LOCK_HANDLER   lock;
    LOCK_HANDLER   unlock;
    void*          ctx;
};

#define FC_LOCK_BEGIN(x)    do{(x)->ins.mtl.lock((x)->ins.mtl.ctx);}while(0)
#define FC_LOCK_END(x)      do{(x)->ins.mtl.unlock((x)->ins.mtl.ctx);}while(0)
#define F_SET_LOCK(x,y,z,c) do{(x)->ins.mtl.lock=y;(x)->ins.mtl.unlock=z;(x)->ins.mtl.ctx=c;}while(0)

#define F_STAT_LOCK_WITHOUT_SEM  struct __mtl mtl;

#define F_COPY_MTL_AND_LCOK(x,y) struct __mtl y=(x)->ins.mtl;do{(y).lock((y).ctx);}while(0)
#define F_COPY_MTL_AND_UNLCOK(x) do{(x).unlock(x.ctx);}while(0)

#define F_GET_CTX(x) ((x)->ins.mtl.ctx))
#else

#define F_STAT_LOCK_WITHOUT_SEM
#define FC_LOCK_BEGIN(x)
#define FC_LOCK_END(x)
#define F_SET_LOCK(x,y,z,c)
#define F_COPY_MTL_AND_LCOK(x,y)
#define F_COPY_MTL_AND_UNLCOK(x)
#define F_GET_CTX(x)

#endif  // FC_MEM_MULTI_THREAD
#endif