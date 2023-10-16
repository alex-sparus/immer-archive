//
//  main.cpp
//  immer-test
//
//  Created by Alex Shabalin on 11/10/2023.
//

#include <iostream>

#include <spdlog/spdlog.h>

#include <bnz/immer_map.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

#include <immer/algorithm.hpp>
#include <immer/map.hpp>
#include <immer/vector.hpp>

namespace {
template <typename T>
std::string to_json(const T& serializable)
{
    auto os = std::ostringstream{};
    {
        auto ar = cereal::JSONOutputArchive{os};
        ar(serializable);
    }
    return os.str();
}
} // namespace

namespace imr {

template <typename T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
using vector = immer::vector<T, MemoryPolicy, B, 1>;

}

namespace {

template <typename... Ts> // (7)
struct overload : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

using node_id = std::uint64_t;
static_assert(sizeof(void*) == sizeof(node_id));

template <class T>
struct leaf_node
{
    const T* begin;
    const T* end;

    template <class Archive>
    void serialize(Archive& ar)
    {
        std::vector<T> dump{begin, end};
        ar(cereal::make_nvp("values", dump));
    }
};

struct inner_node
{
    immer::vector<node_id> children;

    template <class Archive>
    void serialize(Archive& ar)
    {
        std::vector<node_id> dump{children.begin(), children.end()};
        ar(cereal::make_nvp("children", dump));
    }
};

struct vector
{
    node_id root;
    node_id tail;
    std::size_t size;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(root), CEREAL_NVP(tail), CEREAL_NVP(size));
    }
};

template <class T>
struct archive
{
    immer::map<node_id, leaf_node<T>> leaves;
    immer::map<node_id, inner_node> inners;
    immer::map<node_id, vector> vectors;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(leaves), CEREAL_NVP(inners), CEREAL_NVP(vectors));
    }
};

struct is_regular_pos_func
{
    template <class... Rest>
    constexpr bool
    operator()(const immer::detail::rbts::regular_sub_pos<Rest...>&)
    {
        return true;
    }

    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::full_pos<Rest...>&)
    {
        return true;
    }

    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::regular_pos<Rest...>&)
    {
        return true;
    }
};

struct is_leaf_pos_func
{
    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::leaf_sub_pos<Rest...>&)
    {
        return true;
    }

    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::full_leaf_pos<Rest...>&)
    {
        return true;
    }

    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::leaf_pos<Rest...>&)
    {
        return true;
    }
};

template <class T>
constexpr bool is_regular_pos = std::is_invocable_v<is_regular_pos_func, T>;

template <class T>
constexpr bool is_leaf_pos = std::is_invocable_v<is_leaf_pos_func, T>;

struct visitor_helper
{
    template <class F, class... T>
    static void visit_regular(immer::detail::rbts::regular_sub_pos<T...>& pos,
                              F&& fn)
    {
        // pos.count() shows the number of children nodes for this node
        // SPDLOG_DEBUG("visit_regular, count = {}", pos.count());
        fn(pos);
    }

    template <class F, class... T>
    static void visit_regular(immer::detail::rbts::full_pos<T...>& pos, F&& fn)
    {
        fn(pos);
    }

    template <class F, class... T>
    static void visit_regular(immer::detail::rbts::regular_pos<T...>& pos,
                              F&& fn)
    {
        fn(pos);
    }

    template <class F, class... T>
    static void visit_leaf(immer::detail::rbts::leaf_sub_pos<T...>& pos, F&& fn)
    {
        // pos.count() shows the number of actual elements in this leaf node
        // SPDLOG_DEBUG("visit_leaf, count = {}", pos.count());
        fn(pos);
    }

    template <class F, class... T>
    static void visit_leaf(immer::detail::rbts::full_leaf_pos<T...>& pos,
                           F&& fn)
    {
        fn(pos);
    }

    template <class F, class... T>
    static void visit_leaf(immer::detail::rbts::leaf_pos<T...>& pos, F&& fn)
    {
        fn(pos);
    }
};

template <class T>
class ZZ;

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
node_id get_leaf_id(immer::detail::rbts::node<T, MemoryPolicy, B, BL>& node)
{
    T* first = node.leaf();
    return reinterpret_cast<node_id>(static_cast<void*>(first));
}

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
node_id get_inner_id(immer::detail::rbts::node<T, MemoryPolicy, B, BL>& node)
{
    immer::detail::rbts::node<T, MemoryPolicy, B, BL>* inner = *node.inner();
    return reinterpret_cast<node_id>(static_cast<void*>(inner));
}

struct save_visitor
{
    using T = int;
    archive<T> ar;

    template <class Pos>
    bool regular(Pos& pos)
    {
        using node_t = typename std::decay_t<decltype(pos)>::node_t;
        auto id      = get_inner_id(*pos.node());
        if (ar.inners.count(id)) {
            // SPDLOG_DEBUG("already seen inner node {}", id);
            const bool have_seen = true;
            return have_seen;
        }

        // SPDLOG_DEBUG("inner node {}", id);

        auto node_info = inner_node{};
        pos.each(visitor_helper{}, [&node_info, this](auto& child_pos) mutable {
            using ChildPos = decltype(child_pos);
            if constexpr (is_regular_pos<ChildPos>) {
                node_info.children =
                    std::move(node_info.children)
                        .push_back(get_inner_id(*child_pos.node()));
                regular(child_pos);
            } else if constexpr (is_leaf_pos<ChildPos>) {
                node_info.children =
                    std::move(node_info.children)
                        .push_back(get_leaf_id(*child_pos.node()));
                leaf(child_pos);
            } else {
                ZZ<ChildPos> q;
                // static_assert(false);
            }
        });
        ar.inners = std::move(ar.inners).set(id, node_info);

        const bool have_seen = false;
        return have_seen;
    }

    template <class Pos>
    bool leaf(Pos& pos)
    {
        using node_t = typename std::decay_t<decltype(pos)>::node_t;
        T* first     = pos.node()->leaf();
        auto id      = get_leaf_id(*pos.node());
        if (ar.leaves.count(id)) {
            // SPDLOG_DEBUG("already seen leaf node {}", id);
            const bool have_seen = true;
            return have_seen;
        }

        // SPDLOG_DEBUG("leaf node {}", id);

        ar.leaves            = std::move(ar.leaves).set(id,
                                             leaf_node<T>{
                                                            .begin = first,
                                                            .end   = first + pos.count(),
                                             });
        const bool have_seen = false;
        return have_seen;
    }
};

template <class Tree, class Archive>
auto traverse_nodes(const Tree& rbtree, Archive ar)
{
    using immer::detail::rbts::branches;

    using node_t      = typename Tree::node_t;
    constexpr auto BL = node_t::bits_leaf;

    SPDLOG_DEBUG("max children in the inner node, branches<BL> = {}",
                 branches<BL>);

    save_visitor save{
        .ar = std::move(ar),
    };
    auto fn = [&save](auto& pos) {
        using Pos = decltype(pos);
        if constexpr (is_regular_pos<Pos>) {
            save.regular(pos);
        } else if constexpr (is_leaf_pos<Pos>) {
            save.leaf(pos);
        } else {
            ZZ<Pos> q;
            // static_assert(false);
        }
    };

    if (rbtree.size > branches<BL>) {
        make_regular_sub_pos(rbtree.root, rbtree.shift, rbtree.tail_offset())
            .visit(visitor_helper{}, fn);
    }

    make_leaf_sub_pos(rbtree.tail, rbtree.tail_size())
        .visit(visitor_helper{}, fn);

    return std::move(save.ar);
}

using example_vector = imr::vector<int>;

archive<int> save_vector(const example_vector& vec, archive<int> archive)
{
    // vec.impl().for_each_chunk([](auto* b, auto* e) {
    //     SPDLOG_DEBUG("chunk size {}", (e - b));
    //     for (auto* p = b; p != e; ++p) {
    //         // SPDLOG_DEBUG("save_vector {}", *p);
    //     }
    // });

    const auto& impl = vec.impl();
    SPDLOG_DEBUG("traverse_nodes begin");
    archive = traverse_nodes(impl, std::move(archive));
    SPDLOG_DEBUG("traverse_nodes end");

    const auto root_id = get_inner_id(*impl.root);
    assert(archive.inners.count(root_id));
    SPDLOG_DEBUG("root_id = {}", root_id);

    const auto tail_id = get_leaf_id(*impl.tail);
    assert(archive.leaves.count(tail_id));
    SPDLOG_DEBUG("tail_id = {}", tail_id);

    auto vector_info = vector{
        .root = root_id,
        .tail = tail_id,
        .size = impl.size,
    };
    const auto vector_id =
        reinterpret_cast<node_id>(static_cast<const void*>(&impl));
    archive.vectors = std::move(archive.vectors).set(vector_id, vector_info);

    return archive;
}

} // namespace

int main(int argc, const char* argv[])
{
    spdlog::set_level(spdlog::level::debug);

    //    auto vec = example_vector{1};
    //    auto v2  = vec.push_back(2);
    //    save_vector(v2);

    const auto gen = [](auto init, int count) {
        for (int i = 0; i < count; ++i) {
            init = std::move(init).push_back(i);
        }
        return init;
    };

    const auto v65  = gen(example_vector{}, 67);
    const auto v66  = v65.push_back(1337);
    const auto v67  = v66.push_back(1338);
    const auto v68  = v67.push_back(1339);
    const auto v900 = gen(v68, 9999);

    auto ar = save_vector(v65, {});
    ar      = save_vector(v66, ar);
    ar      = save_vector(v67, ar);
    ar      = save_vector(v68, ar);
    ar      = save_vector(v900, ar);

    SPDLOG_DEBUG("archive = {}", to_json(ar));

    // auto big_branches = gen(immer::vector<std::uint64_t>{}, 67);
    // traverse_nodes(big_branches.impl());

    return 0;
}
