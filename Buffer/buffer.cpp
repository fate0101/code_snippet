#include "buffer.h"
#include <assert.h>

Buffer::Buffer() { }

Buffer::Buffer(const char* sz) : io_(sz) { }

Buffer::~Buffer() { }

size_t Buffer::read(char* o, size_t l) {
	assert(o != nullptr && l > 0);

	auto iol = io_.length();
	if (iol == 0)
		return 0;

	iol = iol > l ? l : iol;

	auto t = io_.substr(0,l);
	assert(t.length() != 0);

	io_.erase(0, l);
	memcpy(o, t.data(), t.size());

	return iol;
}

void Buffer::write(Buffer& b) {
	io_ += b.io_;
}

void Buffer::write(const char* i, size_t l) {
	io_.append(i, l);
}

size_t Buffer::find(const char* p, size_t l) {
	return io_.find(p, l);
}