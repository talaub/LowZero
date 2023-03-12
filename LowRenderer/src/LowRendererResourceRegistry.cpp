
#include "LowRendererResourceRegistry.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowMath.h"

namespace Low {
  namespace Renderer {
    static uint64_t create_buffer_resource(Interface::Context p_Context,
                                           Util::Yaml::Node &p_Node,
                                           Util::Name p_Name)
    {
      LOW_ASSERT((bool)p_Node["buffer_info"],
                 "Missing bufferinfo for buffer resource");

      LOW_ASSERT((bool)p_Node["buffer_info"]["data_type"],
                 "Missing data_type for bufferinfo in resource");

      Util::String l_TypeString = Util::String(
          p_Node["buffer_info"]["data_type"].as<std::string>().c_str());

      Backend::BufferCreateParams l_Params;
      l_Params.context = &p_Context.get_context();
      l_Params.data = 0;

      if (l_TypeString == "UVector2") {
        l_Params.bufferSize = sizeof(Math::UVector2);
      } else if (l_TypeString == "Vector2") {
        l_Params.bufferSize = sizeof(Math::Vector2);
      } else {
        LOW_ASSERT(false, "Unknown resource type");
      }

      if (p_Node["buffer_info"]["array_size"]) {
        l_Params.bufferSize *= p_Node["buffer_info"]["array_size"].as<int>();
      }

      l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER |
                            LOW_RENDERER_BUFFER_USAGE_RESOURCE_CONSTANT;

      Resource::Buffer l_Resource = Resource::Buffer::make(p_Name, l_Params);

      return l_Resource.get_id();
    }

    static uint64_t create_resource(Interface::Context p_Context,
                                    Util::Yaml::Node &p_Node, Util::Name p_Name)
    {
      Util::String l_TypeString =
          Util::String(p_Node["type"].as<std::string>().c_str());

      if (l_TypeString == "buffer") {
        return create_buffer_resource(p_Context, p_Node, p_Name);
      }

      LOW_ASSERT(false, "Unknown resource type");
      return 0ull;
    }

    void ResourceRegistry::initialize(Interface::Context p_Context,
                                      Util::Yaml::Node &p_Node)
    {
      m_Context = p_Context;

      for (auto it = p_Node.begin(); it != p_Node.end(); ++it) {
        Util::Name i_ResourceName =
            LOW_NAME(it->first.as<std::string>().c_str());

        m_Resources[i_ResourceName] = ResourceInfo();
        m_Resources[i_ResourceName].handleId =
            create_resource(p_Context, it->second, i_ResourceName);
      }
    }

    void ResourceRegistry::cleanup()
    {
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
  } // namespace Renderer
} // namespace Low
