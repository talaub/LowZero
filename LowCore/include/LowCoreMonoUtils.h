#pragma once

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include "LowUtilContainers.h"
#include "LowUtilName.h"

#include "LowMath.h"

namespace Low {
  namespace Core {
    namespace Mono {
      struct MathContext
      {
        MonoClass *vec3;
        MonoClassField *vec3X;
        MonoClassField *vec3Y;
        MonoClassField *vec3Z;

        MonoClass *quat;
        MonoClassField *quatX;
        MonoClassField *quatY;
        MonoClassField *quatZ;
        MonoClassField *quatW;
      };

      struct Context
      {
        MonoDomain *domain;
        MonoAssembly *low_assembly;
        MonoAssembly *game_assembly;
        MonoImage *low_image;

        MathContext math;
      };

      Util::String from_mono_string(MonoString *p_MonoString);
      MonoString *create_mono_string(char *p_Content);

      Util::Name mono_string_to_name(MonoString *p_MonoString);
      MonoString *create_mono_string(Util::Name p_Name);

      void set_context(Context &p_Context);
      Context &get_context();
      MonoAssembly *load_assembly(const Util::String &assemblyPath);
      MonoClass *get_low_class(Util::Name p_Namespace, Util::Name p_ClassName);
      void register_type_class(uint16_t p_TypeId, MonoClass *p_Class);
      MonoClass *get_type_class(uint16_t p_TypeId);
      MonoClassField *get_type_class_id_field(uint16_t p_TypeId);

      void get_vector3(MonoObject *p_Object, Math::Vector3 &p_Vec3);
      MonoObject *create_vector3(Math::Vector3 p_Vec);

      void get_quaternion(MonoObject *p_Object, Math::Quaternion &p_Quat);
      MonoObject *create_quaternion(Math::Quaternion p_Quat);
    } // namespace Mono
  }   // namespace Core
} // namespace Low
