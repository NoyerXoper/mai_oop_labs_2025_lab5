#include <list>
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "dynamic_resource.hpp"
#include "queue.hpp"

struct NonTrivialDestructor {
    static int destructor_count;
    static int copy_count;
    static int move_count;
    int* value;

    NonTrivialDestructor(int v = 0)
        : value(new int(v)) {}

    ~NonTrivialDestructor() {
        ++destructor_count;
        delete value;
    }

    NonTrivialDestructor(const NonTrivialDestructor& other)
        : value(new int(*other.value)) {
        ++copy_count;
    }

    NonTrivialDestructor& operator=(const NonTrivialDestructor& other) {
        if (this != &other) {
            delete value;
            value = new int(*other.value);
            ++copy_count;
        }
        return *this;
    }

    NonTrivialDestructor(NonTrivialDestructor&& other) noexcept
        : value(other.value) {
        other.value = nullptr;
        ++move_count;
    }

    NonTrivialDestructor& operator=(NonTrivialDestructor&& other) noexcept {
        if (this != &other) {
            delete value;
            value = other.value;
            other.value = nullptr;
            ++move_count;
        }
        return *this;
    }

    bool operator==(const NonTrivialDestructor& other) const {
        if (value == nullptr && other.value == nullptr)
            return true;
        if (value == nullptr || other.value == nullptr)
            return false;
        return *value == *other.value;
    }

    int getValue() const { return value ? *value : -1; }
};

int NonTrivialDestructor::destructor_count = 0;
int NonTrivialDestructor::copy_count = 0;
int NonTrivialDestructor::move_count = 0;

void ResetNonTrivialCounters() {
    NonTrivialDestructor::destructor_count = 0;
    NonTrivialDestructor::copy_count = 0;
    NonTrivialDestructor::move_count = 0;
}

template <typename T>
class QueueStdAllocatorTest : public ::testing::Test {
protected:
    queue::Queue<T> queue;

    static auto GetValue(const T& val) {
        if constexpr (std::is_same_v<T, NonTrivialDestructor>) {
            return val.getValue();
        } else {
            return val;
        }
    }
};

using TestTypes = ::testing::Types<int, NonTrivialDestructor>;
TYPED_TEST_SUITE(QueueStdAllocatorTest, TestTypes);

TYPED_TEST(QueueStdAllocatorTest, DefaultConstructor) {
    EXPECT_EQ(this->queue.Size(), 0);
    EXPECT_TRUE(this->queue.begin() == this->queue.end());
}

TYPED_TEST(QueueStdAllocatorTest, InitializerListConstructor) {
    queue::Queue<TypeParam> q{TypeParam(1), TypeParam(2), TypeParam(3)};
    EXPECT_EQ(q.Size(), 3);

    auto it = q.begin();
    if constexpr (std::is_same_v<TypeParam, int>) {
        std::cout << "Created" << *it << std::endl;
    }
    EXPECT_EQ(this->GetValue(*it), 1);
    ++it;
    EXPECT_EQ(this->GetValue(*it), 2);
}

TYPED_TEST(QueueStdAllocatorTest, PushBackAndFront) {
    this->queue.PushBack(TypeParam(10));
    EXPECT_EQ(this->GetValue(this->queue.Front()), 10);
    EXPECT_EQ(this->queue.Size(), 1);

    this->queue.PushBack(TypeParam(20));
    EXPECT_EQ(this->GetValue(this->queue.Front()), 10);
    EXPECT_EQ(this->queue.Size(), 2);
}

TYPED_TEST(QueueStdAllocatorTest, PopFront) {
    this->queue.PushBack(TypeParam(1));
    this->queue.PushBack(TypeParam(2));

    this->queue.PopFront();
    EXPECT_EQ(this->GetValue(this->queue.Front()), 2);
    EXPECT_EQ(this->queue.Size(), 1);

    this->queue.PopFront();
    EXPECT_TRUE(this->queue.begin() == this->queue.end());
}

TYPED_TEST(QueueStdAllocatorTest, Clear) {
    this->queue.PushBack(TypeParam(1));
    this->queue.PushBack(TypeParam(2));

    this->queue.Clear();
    EXPECT_EQ(this->queue.Size(), 0);
    EXPECT_TRUE(this->queue.begin() == this->queue.end());
}

TYPED_TEST(QueueStdAllocatorTest, IteratorTraversal) {
    this->queue.PushBack(TypeParam(1));
    this->queue.PushBack(TypeParam(2));
    this->queue.PushBack(TypeParam(3));

    int expected = 1;
    for (auto it = this->queue.begin(); it != this->queue.end(); ++it) {
        EXPECT_EQ(this->GetValue(*it), expected++);
    }
}

TYPED_TEST(QueueStdAllocatorTest, ConstIterator) {
    this->queue.PushBack(TypeParam(42));
    const auto& const_queue = this->queue;

    EXPECT_EQ(this->GetValue(*const_queue.begin()), 42);
    EXPECT_TRUE(const_queue.cbegin() == const_queue.begin());
}

TYPED_TEST(QueueStdAllocatorTest, CopyConstructor) {
    this->queue.PushBack(TypeParam(1));
    this->queue.PushBack(TypeParam(2));

    queue::Queue<TypeParam> copy = this->queue;
    EXPECT_EQ(copy.Size(), 2);
    EXPECT_EQ(this->GetValue(copy.Front()), 1);

    copy.PopFront();
    EXPECT_EQ(this->queue.Size(), 2);
}

TYPED_TEST(QueueStdAllocatorTest, MoveConstructor) {
    this->queue.PushBack(TypeParam(1));
    queue::Queue<TypeParam> moved = std::move(this->queue);

    EXPECT_EQ(moved.Size(), 1);
    EXPECT_EQ(this->GetValue(moved.Front()), 1);
    EXPECT_EQ(this->queue.Size(), 0);
}

TYPED_TEST(QueueStdAllocatorTest, CopyAssignment) {
    this->queue.PushBack(TypeParam(1));
    this->queue.PushBack(TypeParam(2));

    queue::Queue<TypeParam> copy;
    copy = this->queue;
    EXPECT_EQ(copy.Size(), 2);
    EXPECT_EQ(this->GetValue(copy.Front()), 1);
}

TYPED_TEST(QueueStdAllocatorTest, MoveAssignment) {
    this->queue.PushBack(TypeParam(1));
    this->queue.PushBack(TypeParam(2));

    queue::Queue<TypeParam> moved;
    moved = std::move(this->queue);
    EXPECT_EQ(moved.Size(), 2);
    EXPECT_EQ(this->GetValue(moved.Front()), 1);
    EXPECT_EQ(this->queue.Size(), 0);
}

class QueuePMRTest : public ::testing::Test {
protected:
    void SetUp() override {
        resource = std::make_unique<memory::DynamicResource>();
        ResetNonTrivialCounters();
    }

    void TearDown() override { resource.reset(); }

    std::unique_ptr<memory::DynamicResource> resource;
};

TEST_F(QueuePMRTest, PMRAllocatorUsesResource) {
    queue::pmr::Queue<int> pmr_queue(resource.get());
    pmr_queue.PushBack(1);
    pmr_queue.PushBack(2);

    EXPECT_EQ(pmr_queue.Size(), 2);
    EXPECT_EQ(pmr_queue.Front(), 1);

    pmr_queue.PopFront();
    EXPECT_EQ(pmr_queue.Front(), 2);
}

TEST_F(QueuePMRTest, NonTrivialDestructorWithPMR) {
    ResetNonTrivialCounters();
    {
        queue::pmr::Queue<NonTrivialDestructor> pmr_queue(resource.get());
        pmr_queue.PushBack(NonTrivialDestructor(1));
        pmr_queue.PushBack(NonTrivialDestructor(2));
    }
    EXPECT_GT(NonTrivialDestructor::destructor_count, 0);
}

TEST(QueueIntTest, BasicOperations) {
    queue::Queue<int> q;

    q.PushBack(10);
    EXPECT_EQ(q.Front(), 10);
    EXPECT_EQ(q.Size(), 1);

    q.PushBack(20);
    EXPECT_EQ(q.Front(), 10);
    EXPECT_EQ(q.Size(), 2);

    q.PopFront();
    EXPECT_EQ(q.Front(), 20);
    EXPECT_EQ(q.Size(), 1);
}

TEST(QueueIntTest, InitializerList) {
    queue::Queue<int> q{1, 2, 3, 4, 5};
    EXPECT_EQ(q.Size(), 5);

    int expected = 1;
    for (auto it = q.begin(); it != q.end(); ++it) {
        EXPECT_EQ(*it, expected++);
    }
}

TEST(QueueIntTest, IteratorOperations) {
    queue::Queue<int> q{1, 2, 3};
    auto it = q.begin();

    auto it2 = it++;
    EXPECT_EQ(*it2, 1);
    EXPECT_EQ(*it, 2);

    auto& it_ref = ++it;
    EXPECT_EQ(*it_ref, 3);

    EXPECT_EQ(*it, 3);
}

TEST(QueueEdgeCases, EmptyQueueOperations) {
    queue::Queue<int> q;
    EXPECT_NO_THROW(q.Clear());
}

TEST(QueueEdgeCases, LargeNumberOfElements) {
    queue::Queue<int> q;
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        q.PushBack(i);
    }
    EXPECT_EQ(q.Size(), count);

    for (int i = 0; i < count; ++i) {
        EXPECT_EQ(q.Front(), i);
        q.PopFront();
    }
    EXPECT_TRUE(q.begin() == q.end());
}

TEST(QueueIterator, ForwardIteratorRequirements) {
    struct TestStruct {
        int x;
        int getX() const { return x; }
    };

    queue::Queue<TestStruct> qs{TestStruct{42}};
    EXPECT_EQ(qs.begin()->x, 42);
    EXPECT_EQ(qs.begin()->getX(), 42);
}

TEST(NonTrivialDestructorTest, DestructorCalled) {
    ResetNonTrivialCounters();
    {
        queue::Queue<NonTrivialDestructor> q;
        q.PushBack(NonTrivialDestructor(1));
        q.PushBack(NonTrivialDestructor(2));
    }
    EXPECT_GE(NonTrivialDestructor::destructor_count, 2);
}

TEST(QueueRangeConstructor, FromVector) {
    std::vector<int> vec{1, 2, 3, 4, 5};
    queue::Queue<int> q(vec.begin(), vec.end());

    EXPECT_EQ(q.Size(), 5);
    int expected = 1;
    for (auto it = q.begin(); it != q.end(); ++it) {
        EXPECT_EQ(*it, expected++);
    }
}

TEST(QueueAllocator, StatefulAllocator) {
    std::allocator<int> alloc;
    queue::Queue<int, std::allocator<int>> q(alloc);
    q.PushBack(1);
    q.PushBack(2);

    EXPECT_EQ(q.Size(), 2);
    EXPECT_EQ(q.Front(), 1);
}

TEST(QueueMoveSemantics, PushBackRvalue) {
    ResetNonTrivialCounters();
    queue::Queue<NonTrivialDestructor> q;
    NonTrivialDestructor obj(42);
    int initial_destructor_count = NonTrivialDestructor::destructor_count;

    q.PushBack(std::move(obj));
    EXPECT_EQ(NonTrivialDestructor::destructor_count, initial_destructor_count);
    EXPECT_EQ(obj.getValue(), -1);  // obj.value is now nullptr after move
}

TEST(QueueConstCorrectness, ConstFront) {
    queue::Queue<int> q;
    q.PushBack(42);
    const queue::Queue<int>& const_q = q;

    EXPECT_EQ(const_q.Front(), 42);
    EXPECT_EQ(*const_q.begin(), 42);
}

TEST(QueueIterator, ConstConversion) {
    queue::Queue<int> q{1, 2, 3};
    queue::Queue<int>::Iterator it = q.begin();
    queue::Queue<int>::ConstIterator cit = it;

    EXPECT_EQ(*cit, 1);
    EXPECT_TRUE(cit == it);
}

TEST(QueueNonTrivialDestructor, CopyAndMove) {
    ResetNonTrivialCounters();
    {
        queue::Queue<NonTrivialDestructor> q1;
        q1.PushBack(NonTrivialDestructor(1));
        q1.PushBack(NonTrivialDestructor(2));

        queue::Queue<NonTrivialDestructor> q2 = q1;
        EXPECT_EQ(q2.Size(), 2);
        EXPECT_EQ(q2.Front().getValue(), 1);

        queue::Queue<NonTrivialDestructor> q3 = std::move(q2);
        EXPECT_EQ(q3.Size(), 2);
        EXPECT_EQ(q3.Front().getValue(), 1);
        EXPECT_EQ(q2.Size(), 0);
    }
    EXPECT_GT(NonTrivialDestructor::destructor_count, 0);
}

TEST(NonTrivialDestructorTest, EqualityOperator) {
    NonTrivialDestructor a(5);
    NonTrivialDestructor b(5);
    NonTrivialDestructor c(10);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);

    NonTrivialDestructor d(std::move(a));
    NonTrivialDestructor e;
    EXPECT_FALSE(a == e);
}

TEST(NonTrivialDestructorTest, MemoryManagement) {
    ResetNonTrivialCounters();
    {
        queue::Queue<NonTrivialDestructor> q;
        q.PushBack(NonTrivialDestructor(100));
        q.PushBack(NonTrivialDestructor(200));

        EXPECT_EQ(q.Front().getValue(), 100);
        q.PopFront();
        EXPECT_EQ(q.Front().getValue(), 200);
    }
    EXPECT_GE(NonTrivialDestructor::destructor_count, 4);
}

TEST(NonTrivialDestructorTest, DeepCopy) {
    ResetNonTrivialCounters();
    NonTrivialDestructor original(42);
    NonTrivialDestructor copy = original;

    EXPECT_TRUE(original == copy);
    EXPECT_NE(original.value, copy.value);
    EXPECT_EQ(*original.value, *copy.value);

    *copy.value = 100;
    EXPECT_EQ(original.getValue(), 42);
    EXPECT_EQ(copy.getValue(), 100);
}