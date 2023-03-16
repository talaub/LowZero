#pragma once

#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererBuffer.h"
#include "LowRendererFrontendConfig.h"
#include "LowRendererContext.h"

namespace Low {
  namespace Renderer {
    struct RenderFlow;

    struct ResourceInfo
    {
      uint64_t handleId;
    };

    struct ResourceRegistry
    {
      void initialize(Util::List<ResourceConfig> &p_Configs,
                      Interface::Context p_Context, RenderFlow p_RenderFlow);
      void cleanup();

      Resource::Buffer get_buffer_resource(Util::Name p_Name);
      Resource::Image get_image_resource(Util::Name p_Name);

    private:
      Interface::Context m_Context;
      Util::Map<Util::Name, ResourceInfo> m_Resources;

      Util::Handle get_resource(Util::Name p_Name);
    };
  } // namespace Renderer
} // namespace Low
