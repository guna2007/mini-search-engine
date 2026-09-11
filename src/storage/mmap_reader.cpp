#include "storage/mmap_reader.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

mmap_reader::~mmap_reader() {
    close();
}

mmap_reader::mmap_reader(mmap_reader&& other) noexcept
    : data_(other.data_), size_(other.size_), fd_(other.fd_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.fd_ = -1;
}

mmap_reader& mmap_reader::operator=(mmap_reader&& other) noexcept {
    if (this != &other) {
        close();
        data_ = other.data_;
        size_ = other.size_;
        fd_ = other.fd_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.fd_ = -1;
    }
    return *this;
}

bool mmap_reader::open(const std::string& path) {
    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        std::cerr << "error: could not open " << path << " for mmap\n";
        return false;
    }

    struct stat st;
    if (fstat(fd_, &st) < 0) {
        std::cerr << "error: could not stat " << path << "\n";
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    size_ = static_cast<size_t>(st.st_size);
    if (size_ == 0) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    void* ptr = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (ptr == MAP_FAILED) {
        std::cerr << "error: mmap failed for " << path << "\n";
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // advise the kernel we'll be reading sequentially
    madvise(ptr, size_, MADV_SEQUENTIAL);

    data_ = static_cast<uint8_t*>(ptr);
    return true;
}

void mmap_reader::close() {
    if (data_) {
        munmap(data_, size_);
        data_ = nullptr;
        size_ = 0;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}
