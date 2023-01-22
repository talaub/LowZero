#pragma once

#include <EASTL/vector.h>
#include <EASTL/set.h>
#include <EASTL/array.h>
#include <EASTL/map.h>
#include <EASTL/optional.h>

namespace Low {
  namespace Util {
    template <typename T> using List = eastl::vector<T>;

    template <typename T, int S> using Array = eastl::array<T, S>;

    template <typename K, typename V> using Map = eastl::map<K, V>;
    template <typename K, typename V> using MultiMap = eastl::multimap<K, V>;

    template <typename T> using Set = eastl::set<T>;

    template <typename T> using Optional = eastl::optional<T>;

  } // namespace Util
} // namespace Low
