#pragma once

#include <immer/vector.hpp>

#include <spdlog/spdlog.h>

namespace immer_archive {

template <typename Node>
class node_ptr
{
public:
    node_ptr()
        : ptr{nullptr}
        , deleter{}
    {
    }

    node_ptr(Node* ptr_, std::function<void(Node* ptr)> deleter_)
        : ptr{ptr_}
        , deleter{std::move(deleter_)}
    {
        SPDLOG_TRACE("ctor {} with ptr {}", (void*) this, (void*) ptr);
        // Assuming the node has just been created and not calling inc() on
        // it.
    }

    node_ptr(const node_ptr& other)
        : ptr{other.ptr}
        , deleter{other.deleter}
    {
        SPDLOG_TRACE("copy ctor {} from {}", (void*) this, (void*) &other);
        if (ptr) {
            ptr->inc();
        }
    }

    node_ptr(node_ptr&& other)
        : node_ptr{}
    {
        SPDLOG_TRACE("move ctor {} from {}", (void*) this, (void*) &other);
        swap(*this, other);
    }

    node_ptr& operator=(const node_ptr& other)
    {
        SPDLOG_TRACE("copy assign {} = {}", (void*) this, (void*) &other);
        auto temp = other;
        swap(*this, temp);
        return *this;
    }

    node_ptr& operator=(node_ptr&& other)
    {
        SPDLOG_TRACE("move assign {} = {}", (void*) this, (void*) &other);
        auto temp = node_ptr{std::move(other)};
        swap(*this, temp);
        return *this;
    }

    ~node_ptr()
    {
        SPDLOG_TRACE("dtor {}", (void*) this);
        if (ptr && ptr->dec()) {
            SPDLOG_TRACE("calling deleter for {}", (void*) ptr);
            deleter(ptr);
        }
    }

    explicit operator bool() const { return ptr; }

    Node* release() &&
    {
        auto result = ptr;
        ptr         = nullptr;
        return result;
    }

    Node* get() { return ptr; }

    friend void swap(node_ptr& x, node_ptr& y)
    {
        using std::swap;
        swap(x.ptr, y.ptr);
        swap(x.deleter, y.deleter);
    }

private:
    Node* ptr;
    std::function<void(Node* ptr)> deleter;
};

template <typename Node>
struct inner_node_ptr
{
    node_ptr<Node> node;
    immer::vector<node_ptr<Node>> children;
};

} // namespace immer_archive
