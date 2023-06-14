#include "LowCoreMonoUtils.h"

#include "LowUtilAssert.h"

#include <fstream>
#include <stdlib.h>

namespace Low {
  namespace Core {
    namespace Mono {
      Util::Map<Util::Name, Util::Map<Util::Name, MonoClass *>> g_Classes;
      Context g_Context;
      Util::Map<uint16_t, MonoClass *> g_TypeClasses;
      Util::Map<uint16_t, MonoClassField *> g_TypeClassIdFields;

      static bool check_error(MonoError &error)
      {
        bool l_HasError = !mono_error_ok(&error);
        if (l_HasError) {
          unsigned short errorCode = mono_error_get_error_code(&error);
          const char *errorMessage = mono_error_get_message(&error);
          printf("Mono Error!\n");
          printf("\tError Code: %hu\n", errorCode);
          printf("\tError Message: %s\n", errorMessage);
          mono_error_cleanup(&error);
        }
        return l_HasError;
      }

      Util::Name mono_string_to_name(MonoString *p_MonoString)
      {
        return LOW_NAME(from_mono_string(p_MonoString).c_str());
      }

      MonoString *create_mono_string(Util::Name p_Name)
      {
        return create_mono_string(p_Name.c_str());
      }

      Util::String from_mono_string(MonoString *p_MonoString)
      {
        if (p_MonoString == nullptr || mono_string_length(p_MonoString) == 0)
          return "";

        MonoError error;
        char *utf8 = mono_string_to_utf8_checked(p_MonoString, &error);
        if (check_error(error))
          return "";
        Util::String result(utf8);
        mono_free(utf8);
        return result;
      }

      MonoString *create_mono_string(char *p_Content)
      {
        return mono_string_new(g_Context.domain, p_Content);
      }

      MonoClass *get_low_class(Util::Name p_Namespace, Util::Name p_ClassName)
      {
        if (g_Classes.find(p_Namespace) != g_Classes.end() &&
            g_Classes[p_Namespace].find(p_ClassName) !=
                g_Classes[p_Namespace].end()) {
          return g_Classes[p_Namespace][p_ClassName];
        }

        MonoClass *klass = mono_class_from_name(
            g_Context.low_image, p_Namespace.c_str(), p_ClassName.c_str());

        LOW_ASSERT(klass, "Could not find mono class");

        g_Classes[p_Namespace][p_ClassName] = klass;

        return klass;
      }

      static char *read_bytes(const Util::String &filepath, uint32_t *outSize)
      {
        std::ifstream stream(filepath.c_str(),
                             std::ios::binary | std::ios::ate);

        if (!stream) {
          // Failed to open the file
          return nullptr;
        }

        std::streampos end = stream.tellg();
        stream.seekg(0, std::ios::beg);
        uint32_t size = end - stream.tellg();

        if (size == 0) {
          // File is empty
          return nullptr;
        }

        char *buffer = new char[size];
        stream.read((char *)buffer, size);
        stream.close();

        *outSize = size;
        return buffer;
      }

      MonoAssembly *load_assembly(const Util::String &assemblyPath)
      {
        uint32_t fileSize = 0;
        char *fileData = read_bytes(assemblyPath, &fileSize);

        // NOTE: We can't use this image for anything other than loading the
        // assembly because this image doesn't have a reference to the assembly
        MonoImageOpenStatus status;
        MonoImage *image =
            mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

        if (status != MONO_IMAGE_OK) {
          const char *errorMessage = mono_image_strerror(status);
          // Log some error message using the errorMessage data
          return nullptr;
        }

        MonoAssembly *assembly = mono_assembly_load_from_full(
            image, assemblyPath.c_str(), &status, 0);
        mono_image_close(image);

        // Don't forget to free the file data
        delete[] fileData;

        return assembly;
      }

      static void fill_context(Context &p_Context)
      {
        p_Context.math.vec3 =
            mono_class_from_name(p_Context.low_image, "Low", "Vector3");

        p_Context.math.vec3X =
            mono_class_get_field_from_name(p_Context.math.vec3, "_x");
        p_Context.math.vec3Y =
            mono_class_get_field_from_name(p_Context.math.vec3, "_y");
        p_Context.math.vec3Z =
            mono_class_get_field_from_name(p_Context.math.vec3, "_z");

        p_Context.math.quat =
            mono_class_from_name(p_Context.low_image, "Low", "Quaternion");

        p_Context.math.quatX =
            mono_class_get_field_from_name(p_Context.math.quat, "_x");
        p_Context.math.quatY =
            mono_class_get_field_from_name(p_Context.math.quat, "_y");
        p_Context.math.quatZ =
            mono_class_get_field_from_name(p_Context.math.quat, "_z");
        p_Context.math.quatW =
            mono_class_get_field_from_name(p_Context.math.quat, "_w");
      }

      void set_context(Context &p_Context)
      {
        fill_context(p_Context);
        g_Context = p_Context;
      }

      Context &get_context()
      {
        return g_Context;
      }

      void register_type_class(uint16_t p_TypeId, MonoClass *p_Class)
      {
        g_TypeClasses[p_TypeId] = p_Class;
        MonoClassField *idField =
            mono_class_get_field_from_name(p_Class, "_id");

        g_TypeClassIdFields[p_TypeId] = idField;
      }

      MonoClass *get_type_class(uint16_t p_TypeId)
      {
        return g_TypeClasses[p_TypeId];
      }

      MonoClassField *get_type_class_id_field(uint16_t p_TypeId)
      {
        return g_TypeClassIdFields[p_TypeId];
      }

      void get_vector3(MonoObject *p_Object, Math::Vector3 &p_Vec3)
      {
        mono_field_get_value(p_Object, get_context().math.vec3X, &p_Vec3.x);
        mono_field_get_value(p_Object, get_context().math.vec3Y, &p_Vec3.y);
        mono_field_get_value(p_Object, get_context().math.vec3Z, &p_Vec3.z);
      }

      MonoObject *create_vector3(Math::Vector3 p_Vec)
      {
        MonoObject *classInstance =
            mono_object_new(g_Context.domain, g_Context.math.vec3);

        LOW_ASSERT(classInstance, "Could not create mono object");

        // Call the parameterless (default) constructor
        mono_runtime_object_init(classInstance);
        mono_field_set_value(classInstance, g_Context.math.vec3X, &p_Vec.x);
        mono_field_set_value(classInstance, g_Context.math.vec3Y, &p_Vec.y);
        mono_field_set_value(classInstance, g_Context.math.vec3Z, &p_Vec.z);

        return classInstance;
      }

      void get_quaternion(MonoObject *p_Object, Math::Quaternion &p_Quat)
      {
        mono_field_get_value(p_Object, get_context().math.quatX, &p_Quat.x);
        mono_field_get_value(p_Object, get_context().math.quatY, &p_Quat.y);
        mono_field_get_value(p_Object, get_context().math.quatZ, &p_Quat.z);
        mono_field_get_value(p_Object, get_context().math.quatW, &p_Quat.w);
      }

      MonoObject *create_quaternion(Math::Quaternion p_Quat)
      {
        MonoObject *classInstance =
            mono_object_new(g_Context.domain, g_Context.math.quat);

        LOW_ASSERT(classInstance, "Could not create mono object");

        // Call the parameterless (default) constructor
        mono_runtime_object_init(classInstance);
        mono_field_set_value(classInstance, g_Context.math.quatX, &p_Quat.x);
        mono_field_set_value(classInstance, g_Context.math.quatY, &p_Quat.y);
        mono_field_set_value(classInstance, g_Context.math.quatZ, &p_Quat.z);
        mono_field_set_value(classInstance, g_Context.math.quatW, &p_Quat.w);

        return classInstance;
      }
    } // namespace Mono
  }   // namespace Core
} // namespace Low
