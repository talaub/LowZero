#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererBackend.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      struct UniformScopeCreateParams;
      struct UniformScopeBindGraphicsParams;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT UniformScopeData
      {
        Backend::UniformScope scope;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(UniformScopeData);
        }
      };

      struct LOW_EXPORT UniformScope : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<UniformScope> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        UniformScope();
        UniformScope(uint64_t p_Id);
        UniformScope(UniformScope &p_Copy);

      private:
        static UniformScope make(Low::Util::Name p_Name);

      public:
        explicit UniformScope(const UniformScope &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static UniformScope *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Backend::UniformScope &get_scope() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static UniformScope make(Util::Name p_Name,
                                 UniformScopeCreateParams &p_Params);
        static void bind(UniformScopeBindGraphicsParams &p_Params);
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
