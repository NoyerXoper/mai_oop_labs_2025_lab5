#pragma once

#include <memory_resource>
#include <vector>

namespace memory {
class DynamicResource : public std::pmr::memory_resource {
public:
    DynamicResource();
    virtual ~DynamicResource() noexcept final;

private:
    struct MemoryData {
        void* data;
        std::size_t bytes;
        std::size_t aligment;
    };
    virtual void* do_allocate(std::size_t bytes, std::size_t aligment) final;
    virtual void do_deallocate(void* p, std::size_t bytes,
                               std::size_t aligment) final;
    virtual bool
    do_is_equal(const std::pmr::memory_resource& other) const noexcept final;

    std::vector<MemoryData> allocated_;
    std::vector<MemoryData> deallocated_;
};
}  // namespace memory

#include "dynamic_resource.ipp"
