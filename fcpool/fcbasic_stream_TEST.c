#include "fcbasic_stream.h"
#include "fcallocator.h"

#define ALLOCATOR(x)       ((PM_ENV)x)
#define GET_TS_ALLCATOR(x) ALLOCATOR((x)->ins.allocator)

#define GET_BLOCK_SIZE()  (sizeof(struct __ins_stream_block) - 1)
#define GET_BLOCK_R_SIZE(x) ((x).block->block_szie - ((x).pos - (x).block->head))
#define GET_BLOCK_L_SIZE(x) ((x).pos - (x).block->head)

/*
  note.
    将多块buffer串接，但是能表现一个平整块给外部，同时支持一定的数据回滚
    另外有效回滚可控，同时不浪费过多空间

   以下代码均未测试
*/

// 还没想好怎么修正
static void iins_fix_block(stream_ptr st) {
  // if (0 == GET_BLOCK_R_SIZE(st->ins.view.vstart)) {
  //   // 切换到下一级
  //   // iins_switch_to_next_block(&st->ins.view.vstart);
  // }
}

static char* iins_switch_to_front_block(struct __ins_position* pos) {
  struct __ins_stream_block* nsb;
  nsb = pos->block->front;
  if (NULL == nsb) return NULL;

  pos->block = nsb;
  pos->pos   = nsb->head + nsb->block_szie - 1;
  return pos->pos;
}

static char* iins_switch_to_next_block(struct __ins_position* pos) {
  struct __ins_stream_block* nsb;
  nsb = pos->block->next;
  if (NULL == nsb) return NULL;

  pos->block = nsb;
  pos->pos   = nsb->head;
  return pos->pos;
}

static size_t ins_Read(stream_ptr st, char* p, size_t s) {
  size_t rs;
  size_t ls;
  size_t pos = 0;

  // 读小的
  ls = rs = s > st->ins.readable_size ? st->ins.readable_size : s;

  // 若不存在可读的
  if (0 == rs)
    return rs;

  for(size_t ts = 0;;) {
    ts = GET_BLOCK_R_SIZE(st->ins.view.vstart);
    if (0 == ts) {
      iins_switch_to_next_block(&st->ins.view.vstart);
      ts = GET_BLOCK_R_SIZE(st->ins.view.vstart);
      ts = ts > st->ins.readable_size ? st->ins.readable_size : ts;
    }

    if (ls > ts) {
      // 读完当前 ts
      memcpy(p + pos, st->ins.view.vstart.pos, ts);
      st->ins.view.vstart.pos += ts;
      st->ins.readable_size -= ts;
    }
    else {
      memcpy(p + pos, st->ins.view.vstart.pos, ls);
      st->ins.view.vstart.pos += ls;
      st->ins.readable_size -= ls;
      break;
    }
    ls  -= ts;
    pos += ts;
  }
  iins_fix_block(st);
}

static size_t ins_Peek(stream_ptr st, char* p, size_t s) {
  size_t rs;
  size_t ls;
  size_t pos = 0;

  // 保存环境
  struct __ins_position vstart = st->ins.view.vstart;
  size_t readable_size = st->ins.readable_size;

  // 读小的
  ls = rs = s > readable_size ? readable_size : s;

  // 若不存在可读的
  if (0 == rs)
    return rs;

  for(size_t ts = 0;;) {
    ts = GET_BLOCK_R_SIZE(vstart);
    if (0 == ts) {
      iins_switch_to_next_block(&vstart);
      ts = GET_BLOCK_R_SIZE(vstart);
      ts = ts > readable_size ? readable_size : ts;
    }

    if (ls > ts) {
      // 读完当前 ts
      memcpy(p + pos, vstart.pos, ts);
      vstart.pos += ts;
      readable_size -= ts;
    }
    else {
      memcpy(p + pos, vstart.pos, ls);
      vstart.pos += ls;
      readable_size -= ls;
      break;
    }
    ls  -= ts;
    pos += ts;
  }
}

static struct __ins_stream_block* iins_append_block(stream_ptr st, size_t s) {
  struct __ins_stream_block* nsb;
  size_t ns;

  // 该粒度策略 与 _back_size 冲突
  ns = st->ins.heap_size * 2;
  ns = ns > s ? ns : s;
  nsb = GET_TS_ALLCATOR(st)->Allocate(GET_TS_ALLCATOR(st), ns + GET_BLOCK_SIZE());
  nsb->next  = NULL;
  nsb->front = st->ins.end;
  nsb->block_szie = ns;
  st->ins.end->next = nsb;
  st->ins.end = nsb;

  st->ins.heap_size += ns;

  return nsb;
}

static size_t ins_Write(stream_ptr st, char* p, size_t s) {
  struct __ins_stream_block* nsb = NULL;
  size_t rs = s;
  // 判断是否存在数据
  if (NULL == st->ins.head || 0 == st->ins.heap_size) {
    nsb = iins_append_block(st, s);
    st->ins.head = nsb;
  }
  else {
    rs = GET_BLOCK_R_SIZE(st->ins.view.vtail);
    if (rs < s) {
      nsb = iins_append_block(st, s);
      memcpy(nsb->head, p + rs, s - rs);
    }
  }

  if (0 != rs)
    memcpy(st->ins.view.vtail.pos, p, rs);

  // 更新 tail
  if (NULL == nsb) {
    st->ins.view.vtail.pos += s;
  }
  else {
    st->ins.view.vtail.block = nsb;
    st->ins.view.vtail.pos = nsb->head + rs;
  }

  st->ins.readable_size += s;

  // 目前还未发现失败的情况
  return s;
}

// 采用惰性切换原则
static char ins_getc(stream_ptr st) {
  struct __ins_stream_block* nsb = NULL;
  size_t rs;
  char ch;

  if(0 == st->ins.readable_size) return FC_EOF;

  // 判断当前 pos 是否指向当前 block 末尾
  if (1 >= (rs = GET_BLOCK_R_SIZE(st->ins.view.vstart))) {
    
    ch = st->ins.view.vstart.pos[0];

    // 切换到下一级
    if (NULL == iins_switch_to_next_block(&st->ins.view.vstart))
      return FC_EOF;
  }
  else {
    ch = st->ins.view.vstart.pos[0];
    st->ins.view.vstart.pos++;
  }
  st->ins.readable_size--;

  // 修正水线
  iins_fix_block(st);

  return ch;
}

static char ins_ungetc(stream_ptr st) {
  struct __ins_stream_block* nsb = NULL;
  size_t rs;
  char ch;

  if(0 == st->ins.readable_size) return FC_EOF;

  // 判断当前 pos 是否指向当前 block 首部
  if (st->ins.view.vstart.pos == st->ins.view.vstart.block->head) {
    // 切换到上一级
    if (NULL == iins_switch_to_front_block(&st->ins.view.vstart))
      return FC_EOF;
  }
  ch = st->ins.view.vstart.pos[0];
  st->ins.view.vstart.pos--;
  st->ins.readable_size++;

  return ch;
}


stream_ptr N_ConstructorStream(void* allocator, unsigned short flag) {
#ifdef FC_ASSERT
  assert(NULL != allocator);
#endif  // FC_ASSERT
  stream_ptr st = ALLOCATOR(allocator)->Allocate(allocator, sizeof(struct __stream));
  memset(st, 0, sizeof(struct __stream));

#ifdef MULTI_THREAD
  if (flag & IN_MULTI_THREAD) {
    /*
      暂留
    */
  }
  else {
#endif  // MULTI_THREAD

    st->Read   = ins_Read;
    st->Peek   = ins_Peek;
    st->Write  = ins_Write;
    st->getc   = ins_getc;
    st->ungetc = ins_ungetc;

#ifdef MULTI_THREAD
  }
#endif  // MULTI_THREAD

  st->ins.allocator = allocator;
  GET_TS_ALLCATOR(st)->Incref(allocator);

  return st;
}

void DeconstructionStream(stream_ptr st) {

  F_COPY_MTL_AND_LCOK(ts, mtl);

  struct __ins_stream_block* nsb, *tsb;
  tsb = nsb = st->ins.head;
  while(nsb != NULL){
    tsb = nsb;
    nsb = tsb->next;
    GET_TS_ALLCATOR(st)->Deallocate(GET_TS_ALLCATOR(st), tsb);
  }

  GET_TS_ALLCATOR(st)->Decref(GET_TS_ALLCATOR(st));
  GET_TS_ALLCATOR(st)->Deallocate(GET_TS_ALLCATOR(st), st);

  F_COPY_MTL_AND_UNLCOK(mtl);
}