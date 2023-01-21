#pragma once

#include <LowUtilCompatibility.h>

#include <EASTL/vector.h>
#include <EASTL/array.h>
#include <EASTL/map.h>

namespace Low {
  namespace Util {
    template <typename T> using List = eastl::vector<T>;

    template <typename T, int S> using Array = eastl::array<T, S>;

    template <typename K, typename V> using Map = eastl::map<K, V>;
  } // namespace Util
} // namespace Low
