#include "LowCore.h"

#include "LowCoreEntity.h"
#include "LowCoreTransform.h"

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char *pName, int flags, unsigned debugFlags,
                     const char *file, int line)
{
  return malloc(size);
}

namespace Low {
  namespace Core {
    static void initialize_component_types()
    {
      Component::Transform::initialize();
    }

    static void initialize_base_types()
    {
      Entity::initialize();
    }

    static void initialize_types()
    {
      initialize_base_types();
      initialize_component_types();
    }

    void initialize()
    {
      initialize_types();
    }

    static void cleanup_component_types()
    {
      Component::Transform::cleanup();
    }

    static void cleanup_base_types()
    {
      Entity::cleanup();
    }

    static void cleanup_types()
    {
      cleanup_base_types();
      cleanup_component_types();
    }

    void cleanup()
    {
      cleanup_types();
    }
  } // namespace Core
} // namespace Low
