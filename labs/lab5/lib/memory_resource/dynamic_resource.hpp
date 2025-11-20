#pragma once

#include <vector>
#include <memory_resource>

namespace memory {
class DynamicResource: public std::pmr::memory_resource {
public:
    DynamicResource();
    virtual ~DynamicResource() noexcept final;
private:
    struct MemoryData {
        void* data;
        std::size_t bytes;
        std::size_t aligment;
    };
    static constexpr unsigned short MAX_DEALOCATED_SYZE_BYTES = 8192;
    virtual void* do_allocate(std::size_t bytes, std::size_t aligment) final;
    virtual void do_deallocate(void* p, std::size_t bytes, std::size_t aligment) final;
    virtual bool do_is_equal(const std::pmr::memory_resource& other) const noexcept final;

    std::vector<MemoryData> allocated_;
    std::vector<MemoryData> deallocated_;
    unsigned short deallocated_size_ = 0;
};
}

#include "dynamic_resource.ipp"
