#pragma once

#include <memory_resource>
#include <memory>
#include <iterator>

namespace queue {
// Once again, I don't want to use smart pointers here:
// To make it simple, I decided to implement as forward list, not chunk based.
// Also, there is 3 options:
// unique_ptr: I need to store end, but Queue doesn't own the end directly (the last owns it)
// shared_ptr is not efficient for simple class like Queue on dynamic structures
// raw pointers: easy to operate here, no need to specify the deleter for neither unique ptr nor shared ptr.
// Also are quite efficient.
template <class T, class Allocator = std::allocator<T>>
class Queue {
private:
    struct Node {
        T val;
        Node* next = nullptr;
    };

    template <bool isConst>
    class IteratorTemplate {
        friend Queue;
    private:
        using NodePtrType = std::conditional_t<isConst, const Node*, Node*>; 
        NodePtrType cur_;
    public:
        IteratorTemplate(Node* node);
        IteratorTemplate();
        using value_type = std::conditional_t<isConst, const T, T>;
        using reference_type = value_type&;
        using pointer = value_type*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;
        

        IteratorTemplate<isConst>& operator++() noexcept;
        IteratorTemplate<isConst> operator++(int) noexcept;

        reference_type operator*() const noexcept;

        pointer operator->() const noexcept;

        template <bool isOtherConst>
        inline bool operator==(const IteratorTemplate<isOtherConst>& other) const noexcept {
            return cur_ == other.cur_;
        }
        template <bool isOtherConst>
        inline bool operator!=(const IteratorTemplate<isOtherConst>& other) const noexcept {
            return cur_ != other.cur_;
        }

        inline operator IteratorTemplate<true>() const noexcept;
    };

public:

    using Iterator = IteratorTemplate<false>;
    using ConstIterator = IteratorTemplate<true>;

    Queue(const Allocator& alloc = Allocator());
    Queue(const std::initializer_list<T>& il, const Allocator& alloc = Allocator());

    template <std::forward_iterator Iter>
    Queue(Iter start, Iter end, const Allocator& alloc = Allocator());

    Queue(const Queue& other);
    Queue(Queue&& other) noexcept;

    Queue& operator=(const Queue& other);
    Queue& operator=(Queue&& other) noexcept;

    std::size_t Size() const noexcept;

    void Clear();

    void PushBack(const T& obj);
    void PushBack(T&& obj);

    T& Front() noexcept;
    const T& Front() const noexcept;

    void PopFront();

    inline Iterator begin() noexcept;
    inline Iterator end() noexcept;

    inline ConstIterator begin() const noexcept;
    inline ConstIterator end() const noexcept;

    inline ConstIterator cbegin() const noexcept;
    inline ConstIterator cend() const noexcept;

    ~Queue() noexcept;

private:
    using NodeAlloc = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using AllocTraits = std::allocator_traits<NodeAlloc>;
    NodeAlloc allocator_;
    Node* head_;
    Node* end_;
    std::size_t size_;
};

namespace pmr {
template <class T>
using Queue = ::queue::Queue<T, std::pmr::polymorphic_allocator<T>>;
}
}

#include <queue.ipp>
