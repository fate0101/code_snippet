#include <ntddk.h>
#include "fcalloctor.h"

#include <stdio.h>
#include <stdarg.h>

static char tb[200] = { 0 };

void _cdecl dbgp(const char* fmt, ...) {

	va_list vArgList;
	va_start(vArgList, fmt);
	_vsnprintf_s(tb, 200, _TRUNCATE, fmt, vArgList);
	va_end(vArgList);

	KdPrint((tb));
}

F_STAT_MALLOC(alloc_s) {
	F_STAT_ARG(n, f);
	return ExAllocatePoolWithTag(PagedPool, s, 'fcp');
}

F_STAT_FREE(free_s) {
	F_STAT_ARG(n, f);
	ExFreePoolWithTag(p, 'fcp');
}

F_STAT_REALLOC(realloc_s) {
	F_STAT_ARG(n, f);
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


void unload(PDRIVER_OBJECT obj) {
	obj;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT obj, PUNICODE_STRING reg_path) {
	reg_path;

	obj->DriverUnload = unload;

	M_ENV m_env = { sizeof(m_env) };

	m_env.ins.malloc_fn  = alloc_s;
	m_env.ins.free_fn    = free_s;
	m_env.ins.realloc_fn = realloc_s;
	m_env.ins.BSOD       = BSOD;
	F_SET_DBG(m_env, dbgp);

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