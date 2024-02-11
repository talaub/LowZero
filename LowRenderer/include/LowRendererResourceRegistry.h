#pragma once

#include "LowRendererApi.h"

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

    struct LOW_RENDERER_API ResourceRegistry
    {
      void initialize(Util::List<ResourceConfig> &p_Configs,
                      Interface::Context p_Context,
                      RenderFlow p_RenderFlow);
      void cleanup();

      void update_dimensions(RenderFlow p_RenderFlow);

      Resource::Buffer get_buffer_resource(Util::Name p_Name);
      Resource::Image get_image_resource(Util::Name p_Name);

      bool has_resource(Util::Name p_Name) const;

      void register_buffer_resource(Util::Name p_Name,
                                    Resource::Buffer p_Buffer);

    private:
      Util::Map<Util::Name, ResourceInfo> m_Resources;
      Interface::Context m_Context;
      Util::List<ResourceConfig> m_Configs;

      Util::Handle get_resource(Util::Name p_Name);
    };
  } // namespace Renderer
} // namespace Low
