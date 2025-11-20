#include <algorithm>

#include "gtest/gtest.h"

#include "dynamic_resource.hpp"

class DynamicResourceTest : public ::testing::Test {
protected:
    void SetUp() override {
        resource = std::make_unique<memory::DynamicResource>();
    }

    void TearDown() override { resource.reset(); }

    std::unique_ptr<memory::DynamicResource> resource;
};

TEST_F(DynamicResourceTest, BasicAllocationDeallocation) {
    void* ptr = resource->allocate(100, 8);
    EXPECT_NE(ptr, nullptr);

    int* int_ptr = static_cast<int*>(ptr);
    *int_ptr = 42;
    EXPECT_EQ(*int_ptr, 42);

    resource->deallocate(ptr, 100, 8);
}

TEST_F(DynamicResourceTest, MemoryReuse) {
    void* ptr1 = resource->allocate(64, 8);
    EXPECT_NE(ptr1, nullptr);

    resource->deallocate(ptr1, 64, 8);

    void* ptr2 = resource->allocate(64, 8);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_EQ(ptr1, ptr2);

    resource->deallocate(ptr2, 64, 8);
}

TEST_F(DynamicResourceTest, Alignment) {
    const std::size_t alignments[] = {1, 2, 4, 8, 16, 32, 64};

    for (std::size_t alignment : alignments) {
        void* ptr = resource->allocate(100, alignment);
        EXPECT_NE(ptr, nullptr);

        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) & (alignment - 1), 0u)
            << "Pointer " << ptr << " is not aligned to " << alignment;

        resource->deallocate(ptr, 100, alignment);
    }
}

TEST_F(DynamicResourceTest, DifferentSizes) {
    void* small_ptr = resource->allocate(16, 8);
    void* medium_ptr = resource->allocate(64, 8);
    void* large_ptr = resource->allocate(256, 8);

    EXPECT_NE(small_ptr, nullptr);
    EXPECT_NE(medium_ptr, nullptr);
    EXPECT_NE(large_ptr, nullptr);

    EXPECT_NE(small_ptr, medium_ptr);
    EXPECT_NE(small_ptr, large_ptr);
    EXPECT_NE(medium_ptr, large_ptr);

    resource->deallocate(small_ptr, 16, 8);
    resource->deallocate(medium_ptr, 64, 8);
    resource->deallocate(large_ptr, 256, 8);
}

TEST_F(DynamicResourceTest, PatternedReuse) {
    std::vector<void*> pointers;

    for (int i = 0; i < 10; ++i) {
        void* ptr = resource->allocate(32 * (i + 1), 8);
        EXPECT_NE(ptr, nullptr);
        pointers.push_back(ptr);
    }

    for (std::size_t i = 0; i < pointers.size(); ++i) {
        resource->deallocate(pointers[i], 32 * (i + 1), 8);
    }

    for (int i = 0; i < 10; ++i) {
        void* ptr = resource->allocate(32 * (i + 1), 8);
        EXPECT_NE(ptr, nullptr);

        auto it = std::find(pointers.begin(), pointers.end(), ptr);
        EXPECT_NE(it, pointers.end())
            << "Memory not reused for allocation " << i;

        resource->deallocate(ptr, 32 * (i + 1), 8);
    }
}

TEST_F(DynamicResourceTest, DeallocationSizeLimit) {
    const std::size_t block_size = 512;
    const int num_blocks = 20;

    std::vector<void*> blocks;

    for (int i = 0; i < num_blocks; ++i) {
        void* ptr = resource->allocate(block_size, 8);
        EXPECT_NE(ptr, nullptr);
        blocks.push_back(ptr);
    }

    for (void* ptr : blocks) {
        resource->deallocate(ptr, block_size, 8);
    }

    void* new_ptr = resource->allocate(block_size, 8);
    EXPECT_NE(new_ptr, nullptr);
    resource->deallocate(new_ptr, block_size, 8);
}

TEST_F(DynamicResourceTest, LargeAllocations) {
    void* large_ptr = resource->allocate(16384, 8);
    EXPECT_NE(large_ptr, nullptr);

    char* char_ptr = static_cast<char*>(large_ptr);
    for (int i = 0; i < 1000; ++i) {
        char_ptr[i] = static_cast<char>(i % 256);
    }

    resource->deallocate(large_ptr, 16384, 8);
}

TEST_F(DynamicResourceTest, IsEqual) {
    memory::DynamicResource resource2;

    EXPECT_TRUE(resource->is_equal(*resource));

    EXPECT_FALSE(resource->is_equal(resource2));

    std::pmr::monotonic_buffer_resource monotonic;
    EXPECT_FALSE(resource->is_equal(monotonic));
}

TEST_F(DynamicResourceTest, MultipleResources) {
    memory::DynamicResource resource1;
    memory::DynamicResource resource2;

    void* ptr1 = resource1.allocate(100, 8);
    void* ptr2 = resource2.allocate(100, 8);

    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_NE(ptr1, ptr2);

    resource1.deallocate(ptr1, 100, 8);
    resource2.deallocate(ptr2, 100, 8);
}

TEST_F(DynamicResourceTest, ZeroSizeAllocation) {
    void* ptr = resource->allocate(0, 8);
    if (ptr != nullptr) {
        resource->deallocate(ptr, 0, 8);
    }
}

TEST_F(DynamicResourceTest, MixedPatterns) {
    struct Allocation {
        void* ptr;
        std::size_t size;
        std::size_t alignment;
    };

    std::vector<Allocation> allocations;

    allocations.push_back({resource->allocate(16, 1), 16, 1});
    allocations.push_back({resource->allocate(32, 2), 32, 2});
    allocations.push_back({resource->allocate(64, 4), 64, 4});
    allocations.push_back({resource->allocate(128, 8), 128, 8});
    allocations.push_back({resource->allocate(256, 16), 256, 16});

    for (const auto& alloc : allocations) {
        EXPECT_NE(alloc.ptr, nullptr);

        EXPECT_EQ(
            reinterpret_cast<uintptr_t>(alloc.ptr) & (alloc.alignment - 1), 0u);
    }

    for (std::size_t i = 0; i < allocations.size(); i += 2) {
        resource->deallocate(allocations[i].ptr, allocations[i].size,
                             allocations[i].alignment);
    }

    std::vector<Allocation> new_allocations;
    new_allocations.push_back({resource->allocate(16, 1), 16, 1});
    new_allocations.push_back({resource->allocate(64, 4), 64, 4});
    new_allocations.push_back({resource->allocate(256, 16), 256, 16});

    for (const auto& alloc : new_allocations) {
        EXPECT_NE(alloc.ptr, nullptr);
        EXPECT_EQ(
            reinterpret_cast<uintptr_t>(alloc.ptr) & (alloc.alignment - 1), 0u);
    }

    for (std::size_t i = 1; i < allocations.size(); i += 2) {
        resource->deallocate(allocations[i].ptr, allocations[i].size,
                             allocations[i].alignment);
    }
    for (const auto& alloc : new_allocations) {
        resource->deallocate(alloc.ptr, alloc.size, alloc.alignment);
    }
}

TEST_F(DynamicResourceTest, MemoryUsability) {
    const std::size_t size = 1024;
    void* ptr = resource->allocate(size, 8);

    int* int_ptr = static_cast<int*>(ptr);
    *int_ptr = 0x12345678;
    EXPECT_EQ(*int_ptr, 0x12345678);

    double* double_ptr = static_cast<double*>(ptr);
    *double_ptr = 3.14159;
    EXPECT_DOUBLE_EQ(*double_ptr, 3.14159);

    char* char_ptr = static_cast<char*>(ptr);
    for (std::size_t i = 0; i < size; ++i) {
        char_ptr[i] = static_cast<char>(i % 256);
    }
    for (std::size_t i = 0; i < size; ++i) {
        EXPECT_EQ(char_ptr[i], static_cast<char>(i % 256));
    }

    resource->deallocate(ptr, size, 8);
}

TEST_F(DynamicResourceTest, ResourceDestructionWithAllocations) {
    void* ptr1 = resource->allocate(100, 8);
    void* ptr2 = resource->allocate(200, 16);

    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    resource.reset();
}

TEST_F(DynamicResourceTest, SequentialAllocationPattern) {
    std::vector<void*> pointers;

    for (int i = 0; i < 100; ++i) {
        void* ptr = resource->allocate(50, 8);
        EXPECT_NE(ptr, nullptr);
        pointers.push_back(ptr);
    }

    for (auto it = pointers.rbegin(); it != pointers.rend(); ++it) {
        resource->deallocate(*it, 50, 8);
    }

    for (int i = 0; i < 100; ++i) {
        void* ptr = resource->allocate(50, 8);
        EXPECT_NE(ptr, nullptr);

        auto found = std::find(pointers.begin(), pointers.end(), ptr);
        EXPECT_NE(found, pointers.end()) << "Memory not reused";

        resource->deallocate(ptr, 50, 8);
    }
}

TEST_F(DynamicResourceTest, BoundaryConditions) {
    void* tiny_ptr = resource->allocate(1, 1);
    EXPECT_NE(tiny_ptr, nullptr);
    resource->deallocate(tiny_ptr, 1, 1);

    void* aligned_ptr = resource->allocate(100, 64);
    EXPECT_NE(aligned_ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(aligned_ptr) & 63, 0u);
    resource->deallocate(aligned_ptr, 100, 64);
}

TEST_F(DynamicResourceTest, DeallocatedMemoryContent) {
    void* ptr = resource->allocate(sizeof(int), alignof(int));
    int* int_ptr = static_cast<int*>(ptr);
    *int_ptr = 0xDEADBEEF;

    resource->deallocate(ptr, sizeof(int), alignof(int));

    void* ptr2 = resource->allocate(sizeof(int), alignof(int));
    EXPECT_EQ(ptr, ptr2);

    int* int_ptr2 = static_cast<int*>(ptr2);
    *int_ptr2 = 0xCAFEBABE;
    EXPECT_EQ(*int_ptr2, 0xCAFEBABE);

    resource->deallocate(ptr2, sizeof(int), alignof(int));
}