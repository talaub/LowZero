
#include "LowRendererResourceRegistry.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowMath.h"

#include "LowRendererRenderFlow.h"
#include "LowRendererImage.h"

namespace Low {
  namespace Renderer {
    static uint64_t create_buffer_resource(Interface::Context p_Context,
                                           RenderFlow p_RenderFlow,
                                           ResourceConfig &p_Config,
                                           Util::Name p_Name)
    {
      Backend::BufferCreateParams l_Params;
      l_Params.context = &p_Context.get_context();
      l_Params.data = 0;

      l_Params.bufferSize = p_Config.buffer.size;

      l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER |
                            LOW_RENDERER_BUFFER_USAGE_RESOURCE_CONSTANT |
                            p_Config.buffer.usageFlags;

      Resource::Buffer l_Resource = Resource::Buffer::make(p_Name, l_Params);

      return l_Resource.get_id();
    }

    static void fill_image_resource_params(
        Interface::Context p_Context, RenderFlow p_RenderFlow,
        ResourceConfig &p_Config, Backend::ImageResourceCreateParams &p_Params)
    {
      p_Params.context = &p_Context.get_context();
      p_Params.createImage = true;
      p_Params.imageData = 0;
      p_Params.imageDataSize = 0;
      p_Params.depth = p_Config.image.depth;
      p_Params.format = p_Config.image.format;
      p_Params.sampleFilter = p_Config.image.sampleFilter;
      p_Params.writable = true;

      if (p_Config.image.dimensions.type ==
          ImageResourceDimensionType::ABSOLUTE) {
        p_Params.dimensions = p_Config.image.dimensions.absolute;
      } else if (p_Config.image.dimensions.type ==
                 ImageResourceDimensionType::RELATIVE) {
        if (p_Config.image.dimensions.relative.target ==
            ImageResourceDimensionRelativeOptions::CONTEXT) {
          p_Params.dimensions = p_Context.get_dimensions();
        } else if (p_Config.image.dimensions.relative.target ==
                   ImageResourceDimensionRelativeOptions::RENDERFLOW) {
          p_Params.dimensions = p_RenderFlow.get_dimensions();
        } else {
          LOW_ASSERT(false, "Unknown relative dimension option");
        }

        Math::Vector2 l_Dimensions = p_Params.dimensions;
        l_Dimensions *= p_Config.image.dimensions.relative.multiplier;
        p_Params.dimensions = l_Dimensions;
      } else {
        LOW_ASSERT(false, "Unknown dimension type");
      }
    }

    static uint64_t create_image_resource(Interface::Context p_Context,
                                          RenderFlow p_RenderFlow,
                                          ResourceConfig &p_Config,
                                          Util::Name p_Name)
    {
      Backend::ImageResourceCreateParams l_Params;
      fill_image_resource_params(p_Context, p_RenderFlow, p_Config, l_Params);

      return Resource::Image::make(p_Name, l_Params).get_id();
    }

    static uint64_t create_resource(Interface::Context p_Context,
                                    RenderFlow p_RenderFlow,
                                    ResourceConfig &p_Config, Util::Name p_Name)
    {
      LOW_LOG_DEBUG << "Creating resource " << p_Name << LOW_LOG_END;
      if (p_Config.type == ResourceType::BUFFER) {
        return create_buffer_resource(p_Context, p_RenderFlow, p_Config,
                                      p_Name);
      } else if (p_Config.type == ResourceType::IMAGE) {
        return create_image_resource(p_Context, p_RenderFlow, p_Config, p_Name);
      }

      LOW_ASSERT(false, "Unknown resource type");
      return 0ull;
    }

    void ResourceRegistry::initialize(Util::List<ResourceConfig> &p_Configs,
                                      Interface::Context p_Context,
                                      RenderFlow p_RenderFlow)
    {
      m_Context = p_Context;
      m_Configs = p_Configs;

      for (ResourceConfig &i_Config : p_Configs) {
        m_Resources[i_Config.name] = {
            create_resource(p_Context, p_RenderFlow, i_Config, i_Config.name)};
      }
    }

    void ResourceRegistry::cleanup()
    {
    }

    void ResourceRegistry::update_dimensions(RenderFlow p_RenderFlow)
    {
      for (uint32_t i = 0u; i < m_Configs.size(); ++i) {
        ResourceConfig &i_Config = m_Configs[i];

        if (i_Config.type == ResourceType::IMAGE) {
          Backend::ImageResourceCreateParams l_Params;
          fill_image_resource_params(m_Context, p_RenderFlow, i_Config,
                                     l_Params);
          get_image_resource(i_Config.name).reinitialize(l_Params);
        }
      }
    }

    Util::Handle ResourceRegistry::get_resource(Util::Name p_Name)
    {
      LOW_ASSERT(m_Resources.find(p_Name) != m_Resources.end(),
                 "Could not find resource in registry");

      return m_Resources[p_Name].handleId;
    }

    Resource::Buffer ResourceRegistry::get_buffer_resource(Util::Name p_Name)
    {
      Util::Handle l_Handle = get_resource(p_Name);

      LOW_ASSERT(l_Handle.get_type() == Resource::Buffer::TYPE_ID,
                 "Cannot fetch resource as buffer");

      return l_Handle.get_id();
    }

    Resource::Image ResourceRegistry::get_image_resource(Util::Name p_Name)
    {
      Util::Handle l_Handle = get_resource(p_Name);

      LOW_ASSERT(l_Handle.get_type() == Resource::Image::TYPE_ID,
                 "Cannot fetch resource as image");

      return l_Handle.get_id();
    }
  } // namespace Renderer
} // namespace Low
