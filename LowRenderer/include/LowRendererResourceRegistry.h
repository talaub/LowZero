#pragma once

#include "LowUtilYaml.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererContext.h"
#include "LowRendererBuffer.h"

namespace Low {
  namespace Renderer {
    struct ResourceInfo
    {
      uint64_t handleId;
    };

    struct ResourceRegistry
    {
      void initialize(Interface::Context p_Context, Util::Yaml::Node &p_Node);
      void cleanup();

      Resource::Buffer get_buffer_resource(Util::Name p_Name);

    private:
      Interface::Context m_Context;
      Util::Map<Util::Name, ResourceInfo> m_Resources;

      Util::Handle get_resource(Util::Name p_Name);
    };
  } // namespace Renderer
} // namespace Low
