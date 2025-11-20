#include "dynamic_resource.hpp"

namespace memory {
DynamicResource::DynamicResource() = default;
void* DynamicResource::do_allocate(std::size_t bytes, std::size_t aligment) {
    for (auto it = deallocated_.begin(); it != deallocated_.end(); ++it) {
        if (it->bytes == bytes && (it->aligment % aligment == 0)) {
            deallocated_size_ -= it->bytes;
            allocated_.push_back(*it);
            deallocated_.erase(it);
            return allocated_.back().data;
        }
    }
    void* data = ::operator new(bytes, std::align_val_t(aligment));
    allocated_.push_back({data, bytes, aligment});
    return data;
}
void DynamicResource::do_deallocate(void* p, std::size_t bytes, std::size_t aligment) {
    for(auto it = allocated_.begin(); it != allocated_.end(); ++it) {
        if (it->data == p) {
            deallocated_size_ += it->bytes;
            deallocated_.push_back(*it);
            allocated_.erase(it);
            return;
        }
    }
}

bool DynamicResource::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
    return this == std::addressof(other);
}

DynamicResource::~DynamicResource() noexcept {
    for (auto& memory_data: allocated_) {
        ::operator delete(memory_data.data, memory_data.bytes, std::align_val_t(memory_data.aligment));
    }
    for (auto& memory_data: deallocated_) {
        ::operator delete(memory_data.data, memory_data.bytes, std::align_val_t(memory_data.aligment));
    }
}
}
