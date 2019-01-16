#include "fcallocator.h"
#include "fcbuffer.h"

#include <stdio.h>

int main() {
  M_ENV m_env = { 0 };
  if (!FC_SUCCESS(FConstructorAllocator(m_env))) {
    printf("init error\r\n");
    return;
  }

  string_ptr ts = F_STRING(&m_env);
  ts->AppendString(ts, "tes123456esc");
  string_ptr ns = N_ConstructorStringWithString(ts);


  printf("%d\r\n", ns->FindBuffer(ns, 8, "es", 2));
  F_DSTRING(ts);

  printf("%s\r\n", ns->GetBuffer_thread_unsafe(ns));
  F_DSTRING(ns);

  FDeconstructionAllocator(m_env);
  system("pause");
}
