#include <cassert>
#include <memory>

#include "queue.hpp"

namespace queue {
template <class T, class Allocator>
template <bool isConst>
Queue<T, Allocator>::IteratorTemplate<isConst>::IteratorTemplate(Node* node)
    : cur_(node) {}

template <class T, class Allocator>
template <bool isConst>
Queue<T, Allocator>::IteratorTemplate<isConst>::IteratorTemplate()
    : IteratorTemplate(nullptr) {}

template <class T, class Alloc>
template <bool isConst>
Queue<T, Alloc>::IteratorTemplate<isConst>&
Queue<T, Alloc>::IteratorTemplate<isConst>::operator++() noexcept {
    cur_ = cur_->next;
    return *this;
}

template <class T, class Alloc>
template <bool isConst>
Queue<T, Alloc>::IteratorTemplate<isConst>
Queue<T, Alloc>::IteratorTemplate<isConst>::operator++(int) noexcept {
    IteratorTemplate<isConst> temp = *this;
    ++*this;
    return temp;
}

template <class T, class Alloc>
template <bool isConst>
Queue<T, Alloc>::IteratorTemplate<isConst>::reference_type
Queue<T, Alloc>::IteratorTemplate<isConst>::operator*() const noexcept {
    return cur_->val;
}

template <class T, class Alloc>
template <bool isConst>
Queue<T, Alloc>::IteratorTemplate<isConst>::pointer
Queue<T, Alloc>::IteratorTemplate<isConst>::operator->() const noexcept {
    return std::addressof(cur_->val);
}

template <class T, class Alloc>
template <bool isConst>
Queue<T, Alloc>::IteratorTemplate<isConst>::operator IteratorTemplate<true>()
    const noexcept {
    return IteratorTemplate<true>(cur_);
}

template <class T, class Alloc>
Queue<T, Alloc>::Queue(const Alloc& alloc)
    : allocator_(alloc)
    , head_(nullptr)
    , end_(nullptr)
    , size_(0) {}

template <class T, class Alloc>
Queue<T, Alloc>::Queue(const std::initializer_list<T>& list, const Alloc& alloc)
    : Queue(list.begin(), list.end(), alloc) {}

template <class T, class Alloc>
template <std::forward_iterator Iter>
Queue<T, Alloc>::Queue(Iter start, Iter end, const Alloc& alloc): allocator_(alloc), head_(nullptr), end_(nullptr), size_(0) {
    while (start != end) {
        PushBack(*start);
        ++start;
    }
}

template <class T, class Alloc>
Queue<T, Alloc>::Queue(const Queue& other)
    : Queue(other.begin(), other.end(),
            AllocTraits::
                select_on_container_copy_construction(other.allocator_)) {}

template <class T, class Alloc>
Queue<T, Alloc>::Queue(Queue&& other) noexcept
    : allocator_(std::move(other.allocator_))
    , head_(other.head_)
    , end_(other.end_)
    , size_(other.size_) {
    other.head_ = nullptr;
    other.end_ = nullptr;
    other.size_ = 0;
}

template <class T, class Alloc>
Queue<T, Alloc>& Queue<T, Alloc>::operator=(const Queue& other) {
    Clear();
    if constexpr (AllocTraits::
                      propagate_on_container_copy_assignment::value) {
        allocator_ = other.allocator_;
    }
    for (auto& obj : other) {
        PushBack(obj);
    }
    return *this;
}

template <class T, class Alloc>
Queue<T, Alloc>& Queue<T, Alloc>::operator=(Queue&& other) noexcept {
    Clear();
    if (allocator_ == other.allocator_) {
        head_ = other.head_;
        end_ = other.end_;
        size_ = other.size_;
        other.head_ = nullptr;
        other.end_ = nullptr;
        other.size_ = 0;
        return *this;
    }
    if constexpr (AllocTraits::
                      propagate_on_container_move_assignment::value) {
        allocator_ = other.allocator_;
        head_ = other.head_;
        end_ = other.end_;
        size_ = other.size_;
        other.head_ = nullptr;
        other.end_ = nullptr;
        other.size_ = 0;
        return *this;
    } else {
        for (auto& obj : other) {
            PushBack(std::move(obj));
        }
        return *this;
    }
}
template <class T, class Alloc>
std::size_t Queue<T, Alloc>::Size() const noexcept {
    return size_;
}

template <class T, class Alloc>
void Queue<T, Alloc>::Clear() {
    while (head_) {
        Node* temp = head_->next;
        AllocTraits::destroy(allocator_, head_);
        AllocTraits::deallocate(allocator_, head_, 1);
        head_ = temp;
    }
    end_ = nullptr;
    size_ = 0;
}

template <class T, class Alloc>
void Queue<T, Alloc>::PushBack(const T& obj) {
    if (end_) {
        end_->next = AllocTraits::allocate(allocator_, 1);
        AllocTraits::construct(allocator_, end_->next, obj, nullptr);
        end_ = end_->next;
        ++size_;
    } else {
        head_ = AllocTraits::allocate(allocator_, 1);
        AllocTraits::construct(allocator_, head_, obj, nullptr);
        end_ = head_;
        ++size_;
    }
}

template <class T, class Alloc>
void Queue<T, Alloc>::PushBack(T&& obj) {
    if (end_) {
        end_->next = AllocTraits::allocate(allocator_, 1);
        AllocTraits::construct(allocator_, end_->next, std::move(obj), nullptr);
        end_ = end_->next;
        ++size_;
    } else {
        head_ = AllocTraits::allocate(allocator_, 1);
        AllocTraits::construct(allocator_, head_, std::move(obj), nullptr);
        end_ = head_;
        ++size_;
    }
}

template <class T, class Alloc>
T& Queue<T, Alloc>::Front() noexcept {
    return head_->val;
}

template <class T, class Alloc>
const T& Queue<T, Alloc>::Front() const noexcept {
    return head_->val;
}

template <class T, class Alloc>
void Queue<T, Alloc>::PopFront() {
    assert(size_ > 0);
    Node* temp = head_->next;
    AllocTraits::destroy(allocator_, head_);
    AllocTraits::deallocate(allocator_, head_, 1);
    head_ = temp;
    --size_;
}

template <class T, class Alloc>
Queue<T, Alloc>::Iterator Queue<T, Alloc>::begin() noexcept {
    return Iterator(head_);
}

template <class T, class Alloc>
Queue<T, Alloc>::Iterator Queue<T, Alloc>::end() noexcept {
    return Iterator();
}

template <class T, class Alloc>
Queue<T, Alloc>::ConstIterator Queue<T, Alloc>::begin() const noexcept {
    return ConstIterator(head_);
}

template <class T, class Alloc>
Queue<T, Alloc>::ConstIterator Queue<T, Alloc>::end() const noexcept {
    return ConstIterator();
}

template <class T, class Alloc>
Queue<T, Alloc>::ConstIterator Queue<T, Alloc>::cbegin() const noexcept {
    return begin();
}

template <class T, class Alloc>
Queue<T, Alloc>::ConstIterator Queue<T, Alloc>::cend() const noexcept {
    return end();
}

template <class T, class Alloc>
Queue<T, Alloc>::~Queue() noexcept {
    Clear();
}

}  // namespace queue