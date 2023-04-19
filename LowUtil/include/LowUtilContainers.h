#pragma once

#include <EASTL/vector.h>
#include <EASTL/set.h>
#include <EASTL/array.h>
#include <EASTL/map.h>
#include <EASTL/optional.h>
#include <EASTL/stack.h>
#include <EASTL/queue.h>

#include <EASTL/string.h>

#include "LowUtilMemory.h"

namespace Low {
  namespace Util {
    template <typename T>
    using List = eastl::vector<T, Memory::MallocAllocatorProxy<T>>;

    template <typename T, int S> using Array = eastl::array<T, S>;

    template <typename K, typename V>
    using Map =
        eastl::map<K, V, eastl::less<K>, Memory::MallocAllocatorProxy<V>>;
    template <typename K, typename V>
    using MultiMap =
        eastl::multimap<K, V, eastl::less<K>, Memory::MallocAllocatorProxy<V>>;

    template <typename T>
    using Set = eastl::set<T, eastl::less<T>, Memory::MallocAllocatorProxy<T>>;

    template <typename T> using Stack = eastl::stack<T>;
    template <typename T> using Queue = eastl::queue<T>;
    template <typename T> using Deque = eastl::deque<T>;

    template <typename T> using Optional = eastl::optional<T>;

    typedef eastl::string String;

  } // namespace Util
} // namespace Low
