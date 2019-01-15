#ifndef buffer_h
#define buffer_h

#include <string>

class Buffer {
public:
	Buffer();
	Buffer(const char*);
	~Buffer();

	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	Buffer& operator=(Buffer&& b) noexcept {
		io_.swap(b.io_);
		return *this;
	}

	Buffer(Buffer&& b) {
		io_.swap(b.io_);
	}

public:
	size_t read (char* o, size_t l);
	void   write(const char* i, size_t l);
	void   write(Buffer& b);

	size_t find (const char* p, size_t l);

	size_t len()  noexcept { return io_.length(); }
	size_t size() noexcept { return io_.size();   }

private:
	std::string io_;
};

#endif