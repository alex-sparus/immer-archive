#pragma once

#include <immer/vector.hpp>

#include <spdlog/spdlog.h>

namespace immer_archive {

template <typename Node>
class node_ptr
{
public:
    node_ptr(Node* ptr_, std::function<void(Node* ptr)> deleter_)
        : ptr{ptr_}
        , deleter{std::move(deleter_)}
    {
        SPDLOG_TRACE("ctor {} with ptr {}", (void*) this, (void*) ptr);
        // Assuming the node has just been created and not calling inc() on
        // it.
        ptr->inc();
    }

    node_ptr(std::nullptr_t)
        : ptr{nullptr}
    {
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
        : ptr{std::move(other).release()}
        , deleter{other.deleter}
    {
        SPDLOG_TRACE("move ctor {} from {}", (void*) this, (void*) &other);
    }

    node_ptr& operator=(node_ptr&& other)
    {
        SPDLOG_TRACE("move assign {} = {}", (void*) this, (void*) &other);
        auto temp = node_ptr{std::move(other)};
        using std::swap;
        swap(ptr, temp.ptr);
        swap(deleter, temp.deleter);
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

    operator bool() const { return ptr; }

    Node* release() &&
    {
        auto result = ptr;
        ptr         = nullptr;
        return result;
    }

    Node* get() { return ptr; }

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
