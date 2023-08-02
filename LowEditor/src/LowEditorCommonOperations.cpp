#include "LowEditorCommonOperations.h"

#include "LowEditorMainWindow.h"

#include "LowUtilLogger.h"

namespace Low {
  namespace Editor {
    namespace CommonOperations {
      PropertyEditOperation::PropertyEditOperation(Util::Handle p_Handle,
                                                   Util::Name p_PropertyName,
                                                   Util::Variant p_OldValue,
                                                   Util::Variant p_NewValue)
          : m_Handle(p_Handle), m_PropertyName(p_PropertyName),
            m_OldValue(p_OldValue), m_NewValue(p_NewValue)
      {
      }

      void PropertyEditOperation::execute(ChangeList &p_ChangeList,
                                          Transaction &p_Transaction)
      {
        Util::Handle l_OldHandle = m_Handle;
        m_Handle = p_ChangeList.get_mapping(m_Handle);

        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(m_Handle.get_type());

        if (l_TypeInfo.is_alive(m_Handle)) {
          if (m_NewValue.m_Type == Util::VariantType::Handle) {
            m_NewValue.set_handle(p_ChangeList.get_mapping(m_NewValue));
          }
          l_TypeInfo.properties[m_PropertyName].set(m_Handle,
                                                    &m_NewValue.m_Bool);
        }
      }

      Operation *PropertyEditOperation::invert(ChangeList &p_ChangeList,
                                               Transaction &p_Transaction)
      {
        return new PropertyEditOperation(m_Handle, m_PropertyName, m_NewValue,
                                         m_OldValue);
      }

      HandleCreateOperation::HandleCreateOperation(Util::Handle p_Handle)
          : m_Handle(p_Handle)
      {
        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(p_Handle.get_type());

        if (p_Handle.get_type() == Core::Entity::TYPE_ID) {
          Core::Entity l_Entity = p_Handle.get_id();
          l_Entity.serialize_hierarchy(m_SerializedHandle, true);
        } else {
          l_TypeInfo.serialize(p_Handle, m_SerializedHandle);
        }
      }

      HandleCreateOperation::HandleCreateOperation(
          Util::Handle p_Handle, Util::Yaml::Node &p_SerializedHandle)
          : m_Handle(p_Handle), m_SerializedHandle(p_SerializedHandle)
      {
      }

      static void create_mappings_from_entity_yaml(ChangeList &p_ChangeList,
                                                   Util::Yaml::Node &p_Node)
      {
        p_ChangeList.set_mapping(p_Node["handle"].as<uint64_t>(),
                                 p_Node["_handle"].as<uint64_t>());
        p_Node["handle"] = p_Node["_handle"].as<uint64_t>();

        if (p_Node["components"]) {
          for (uint32_t i = 0; i < p_Node["components"].size(); ++i) {
            p_ChangeList.set_mapping(
                p_Node["components"][i]["handle"].as<uint64_t>(),
                p_Node["components"][i]["_handle"].as<uint64_t>());

            p_Node["components"][i]["handle"] =
                p_Node["components"][i]["_handle"].as<uint64_t>();
          }
        }

        if (p_Node["children"]) {
          for (uint32_t i = 0; i < p_Node["children"].size(); ++i) {
            create_mappings_from_entity_yaml(p_ChangeList,
                                             p_Node["children"][i]);
          }
        }
      }

      void HandleCreateOperation::execute(ChangeList &p_ChangeList,
                                          Transaction &p_Transaction)
      {
        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(m_Handle.get_type());

        Util::Handle l_NewHandle = 0;

        if (m_Handle.get_type() == Core::Entity::TYPE_ID) {
          l_NewHandle =
              Core::Entity::deserialize_hierarchy(m_SerializedHandle, 0);

          create_mappings_from_entity_yaml(p_ChangeList, m_SerializedHandle);
          p_ChangeList.set_mapping(m_Handle, l_NewHandle);
        } else {
          l_TypeInfo.deserialize(m_SerializedHandle, 0);
          p_ChangeList.set_mapping(m_Handle, l_NewHandle);
        }

        m_Handle = l_NewHandle;

        set_selected_handle(m_Handle);
      }

      Operation *HandleCreateOperation::invert(ChangeList &p_ChangeList,
                                               Transaction &p_Transaction)
      {
        m_Handle = p_ChangeList.get_mapping(m_Handle);
        return new HandleDestroyOperation(m_Handle);
      }

      HandleDestroyOperation::HandleDestroyOperation(Util::Handle p_Handle)
          : m_Handle(p_Handle)
      {
        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(p_Handle.get_type());

        if (p_Handle.get_type() == Core::Entity::TYPE_ID) {
          Core::Entity l_Entity = p_Handle.get_id();
          l_Entity.serialize_hierarchy(m_SerializedHandle, true);
        } else {
          l_TypeInfo.serialize(p_Handle, m_SerializedHandle);
        }
      }

      void HandleDestroyOperation::execute(ChangeList &p_ChangeList,
                                           Transaction &p_Transaction)
      {
        m_Handle = p_ChangeList.get_mapping(m_Handle);

        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(m_Handle.get_type());

        l_TypeInfo.destroy(m_Handle);
      }

      Operation *HandleDestroyOperation::invert(ChangeList &p_ChangeList,
                                                Transaction &p_Transaction)
      {
        return new HandleCreateOperation(m_Handle, m_SerializedHandle);
      }
    } // namespace CommonOperations
  }   // namespace Editor
} // namespace Low
