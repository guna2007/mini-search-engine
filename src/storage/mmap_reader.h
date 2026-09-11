#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// memory-mapped file reader - lets the os handle paging for large index files
// on mac/linux uses mmap, avoids loading the entire file into user space
struct mmap_reader {
    mmap_reader() = default;
    ~mmap_reader();

    mmap_reader(const mmap_reader&) = delete;
    mmap_reader& operator=(const mmap_reader&) = delete;
    mmap_reader(mmap_reader&& other) noexcept;
    mmap_reader& operator=(mmap_reader&& other) noexcept;

    [[nodiscard]] bool open(const std::string& path);
    void close();

    [[nodiscard]] const uint8_t* data() const { return data_; }
    [[nodiscard]] size_t size() const { return size_; }
    [[nodiscard]] bool is_open() const { return data_ != nullptr; }

private:
    uint8_t* data_ = nullptr;
    size_t size_ = 0;
    int fd_ = -1;
};
