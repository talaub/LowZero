#pragma once

#include "MtdPluginApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowMath.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowCoreUiElement.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Mtd {
  // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
  // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

  struct MISTEDA_API AbilityData
  {
    Low::Util::String title;
    Low::Util::String description;
    Low::Util::String execute_function_name;
    uint32_t resource_cost;
    Low::Util::Name name;

    static size_t get_size()
    {
      return sizeof(AbilityData);
    }
  };

  struct MISTEDA_API Ability : public Low::Util::Handle
  {
  public:
    static uint8_t *ms_Buffer;
    static Low::Util::Instances::Slot *ms_Slots;

    static Low::Util::List<Ability> ms_LivingInstances;

    const static uint16_t TYPE_ID;

    Ability();
    Ability(uint64_t p_Id);
    Ability(Ability &p_Copy);

    static Ability make(Low::Util::Name p_Name);
    static Low::Util::Handle _make(Low::Util::Name p_Name);
    explicit Ability(const Ability &p_Copy)
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
    static Ability *living_instances()
    {
      return ms_LivingInstances.data();
    }

    static Ability find_by_index(uint32_t p_Index);

    bool is_alive() const;

    static uint32_t get_capacity();

    void serialize(Low::Util::Yaml::Node &p_Node) const;

    Ability duplicate(Low::Util::Name p_Name) const;
    static Ability duplicate(Ability p_Handle,
                             Low::Util::Name p_Name);
    static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                        Low::Util::Name p_Name);

    static Ability find_by_name(Low::Util::Name p_Name);

    static void serialize(Low::Util::Handle p_Handle,
                          Low::Util::Yaml::Node &p_Node);
    static Low::Util::Handle
    deserialize(Low::Util::Yaml::Node &p_Node,
                Low::Util::Handle p_Creator);
    static bool is_alive(Low::Util::Handle p_Handle)
    {
      return p_Handle.get_type() == Ability::TYPE_ID &&
             p_Handle.check_alive(ms_Slots, get_capacity());
    }

    static void destroy(Low::Util::Handle p_Handle)
    {
      _LOW_ASSERT(is_alive(p_Handle));
      Ability l_Ability = p_Handle.get_id();
      l_Ability.destroy();
    }

    Low::Util::String &get_title() const;
    void set_title(Low::Util::String &p_Value);

    Low::Util::String &get_description() const;
    void set_description(Low::Util::String &p_Value);

    Low::Util::String &get_execute_function_name() const;
    void set_execute_function_name(Low::Util::String &p_Value);

    uint32_t get_resource_cost() const;
    void set_resource_cost(uint32_t p_Value);

    Low::Util::Name get_name() const;
    void set_name(Low::Util::Name p_Value);

  private:
    static uint32_t ms_Capacity;
    static uint32_t create_instance();
    static void increase_budget();
  };

  // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
  struct AbilityElement
  {
    Ability ability;
    Low::Core::UI::Element element;
  };
  // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

} // namespace Mtd
