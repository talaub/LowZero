#include "LowCoreMonoUtils.h"

#include "LowUtilHandle.h"
#include "LowUtilLogger.h"

namespace Low {
  namespace Core {
    namespace Mono {
      uint32_t handle_get_living_instances_count(uint16_t p_TypeId)
      {
        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(p_TypeId);
        return l_TypeInfo.get_living_count();
      }

      uint64_t handle_get_living_instance(uint16_t p_TypeId, uint32_t p_Index)
      {
        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(p_TypeId);

        Util::Handle l_Handle = l_TypeInfo.get_living_instances()[p_Index];

        /*
              MonoClass *l_Class = get_type_class(p_TypeId);
              MonoObject *classInstance =
                  mono_object_new(get_context().domain, l_Class);

              LOW_ASSERT(classInstance, "Could not create object instance");
              // Call the parameterless (default) constructor
              mono_runtime_object_init(classInstance);

              MonoClassField *idField = get_type_class_id_field(p_TypeId);

              uint64_t l_Id = l_Handle.get_id();

              mono_field_set_value(classInstance, idField, &l_Id);

              return classInstance;
        */

        return l_Handle.get_id();
      }

      const void *handle_get_generic_value(uint64_t p_HandleId,
                                           uint32_t p_PropertyNameHash)
      {
        Util::Name l_PropName(p_PropertyNameHash);
        Util::Handle l_Handle = p_HandleId;
        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(l_Handle.get_type());

        return l_TypeInfo.properties[l_PropName].get(l_Handle);
      }

      void handle_set_generic_value(uint64_t p_HandleId,
                                    uint32_t p_PropertyNameHash, void *p_Value)
      {
        Util::Name l_PropName(p_PropertyNameHash);
        Util::Handle l_Handle = p_HandleId;
        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(l_Handle.get_type());

        l_TypeInfo.properties[l_PropName].set(l_Handle, p_Value);
      }

      uint64_t handle_get_ulong_value(uint64_t p_HandleId,
                                      uint32_t p_PropertyNameHash)
      {
        uint64_t l_Val = *(uint64_t *)handle_get_generic_value(
            p_HandleId, p_PropertyNameHash);

        return l_Val;
      }

      void handle_set_float_value(uint64_t p_HandleId,
                                  uint32_t p_PropertyNameHash, float p_Value)
      {
        handle_set_generic_value(p_HandleId, p_PropertyNameHash, &p_Value);
      }

      void handle_set_ulong_value(uint64_t p_HandleId,
                                  uint32_t p_PropertyNameHash, uint64_t p_Value)
      {
        handle_set_generic_value(p_HandleId, p_PropertyNameHash, &p_Value);
      }

      void handle_set_uint32_value(uint64_t p_HandleId,
                                   uint32_t p_PropertyNameHash,
                                   uint32_t p_Value)
      {
        handle_set_generic_value(p_HandleId, p_PropertyNameHash, &p_Value);
      }

      void handle_set_name_value(uint64_t p_HandleId,
                                 uint32_t p_PropertyNameHash,
                                 MonoString *p_Value)
      {
        Util::Name l_Name = mono_string_to_name(p_Value);
        handle_set_generic_value(p_HandleId, p_PropertyNameHash,
                                 &l_Name.m_Index);
      }

      MonoString *handle_get_name_value(uint64_t p_HandleId,
                                        uint32_t p_PropertyNameHash)
      {
        Util::Name l_Name = *(Util::Name *)handle_get_generic_value(
            p_HandleId, p_PropertyNameHash);

        return create_mono_string(l_Name);
      }

      void handle_set_vector3_value(uint64_t p_HandleId,
                                    uint32_t p_PropertyNameHash,
                                    MonoObject *p_Value)
      {
        Math::Vector3 l_Vec;
        get_vector3(p_Value, l_Vec);
        handle_set_generic_value(p_HandleId, p_PropertyNameHash, &l_Vec);
      }

      MonoObject *handle_get_vector3_value(uint64_t p_HandleId,
                                           uint32_t p_PropertyNameHash)
      {
        Math::Vector3 l_Vec = *(Math::Vector3 *)handle_get_generic_value(
            p_HandleId, p_PropertyNameHash);

        return create_vector3(l_Vec);
      }

      void handle_set_quat_value(uint64_t p_HandleId,
                                 uint32_t p_PropertyNameHash,
                                 MonoObject *p_Value)
      {
        Math::Quaternion l_Quat;
        get_quaternion(p_Value, l_Quat);
        handle_set_generic_value(p_HandleId, p_PropertyNameHash, &l_Quat);
      }

      MonoObject *handle_get_quat_value(uint64_t p_HandleId,
                                        uint32_t p_PropertyNameHash)
      {
        Math::Quaternion l_Quat = *(Math::Quaternion *)handle_get_generic_value(
            p_HandleId, p_PropertyNameHash);

        return create_quaternion(l_Quat);
      }
    } // namespace Mono
  }   // namespace Core
} // namespace Low
