#include <ntddk.h>

#include "fcalloctor.h"

void unload(PDRIVER_OBJECT obj) {
	obj;
}

void* _cdecl alloc_s(size_t s){
	return ExAllocatePoolWithTag(PagedPool, s, 'fcp');
}

void _cdecl free_s(void* p) {
	ExFreePoolWithTag(p, 'fcp');
}

void* _cdecl realloc_s(void* p, size_t ns) {
	size_t os = GET_SIZE_BY_POINTER(p);

	void* np = ExAllocatePoolWithTag(PagedPool, ns, 'fcp');
	if (NULL == np)
		return np;

	// 选短的那个
	RtlCopyMemory(np, p, ns <= os ? ns : os);

	ExFreePoolWithTag(p, 'fcp');
	return np;
}

void BSOD() {
	// do nothing
	return;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT obj, PUNICODE_STRING reg_path) {
	reg_path;

	obj->DriverUnload = unload;

	M_ENV m_env = { sizeof(m_env) };

	m_env.ins.malloc_fn  = alloc_s;
	m_env.ins.free_fn    = free_s;
	m_env.ins.realloc_fn = realloc_s;
	m_env.ins.BSOD       = BSOD;

	if (!FC_SUCCESS(FInitAllocator(m_env))) {
		return STATUS_UNSUCCESSFUL;
	}

	for (size_t i = 0; i < 10000; i++) {

		char* t = m_env.FAllocate(m_env, 100);

		m_env.FDeallocate(m_env, t);
	}

	FDestoryAllocator(m_env);

	return STATUS_SUCCESS;
}