#pragma once

#include <stdexcept>

#include <fmt/format.h>

namespace immer_archive {

class archive_exception : public std::invalid_argument
{
public:
    using invalid_argument::invalid_argument;
};

class archive_has_cycles : public archive_exception
{
public:
    explicit archive_has_cycles(std::uint64_t node_id)
        : archive_exception{
              fmt::format("Cycle detected with node ID {}", node_id)}
    {
    }
};

class invalid_node_id : public archive_exception
{
public:
    explicit invalid_node_id(std::uint64_t node_id)
        : archive_exception{fmt::format("Node ID {} is not found", node_id)}
    {
    }
};

class invalid_container_id : public archive_exception
{
public:
    explicit invalid_container_id(std::uint64_t container_id)
        : archive_exception{
              fmt::format("Container ID {} is not found", container_id)}
    {
    }
};

class invalid_children_count : public archive_exception
{
public:
    explicit invalid_children_count(std::uint64_t node_id)
        : archive_exception{
              fmt::format("Node ID {} has more children than allowed", node_id)}
    {
    }
};

} // namespace immer_archive
