#include "LowCoreScriptingSerializer.h"

#include "LowUtilAssert.h"

#include <angelscript.h>
#include <scriptarray/scriptarray.h>
#include <serializer/serializer.h>

namespace Low {
  namespace Core {
    namespace Scripting {
      class ArrayUserType : public CUserType
      {
      public:
        void *AllocateUnitializedMemory(CSerializedValue *p_Value)
        {
          return CScriptArray::Create(p_Value->GetType());
        }

        void Store(CSerializedValue *p_Value, void *p_Ptr)
        {
          CScriptArray *l_Array = static_cast<CScriptArray *>(p_Ptr);

          for (asUINT i = 0; i < l_Array->GetSize(); ++i) {
            p_Value->m_children.push_back(
                new CSerializedValue(p_Value, "", "", l_Array->At(i),
                                     l_Array->GetElementTypeId()));
          }
        }

        void Restore(CSerializedValue *p_Value, void *p_Ptr)
        {
          CScriptArray *l_Array = static_cast<CScriptArray *>(p_Ptr);

          l_Array->Resize(
              static_cast<asUINT>(p_Value->m_children.size()));

          for (asUINT i = 0; i < p_Value->m_children.size(); ++i) {
            p_Value->m_children[i]->Restore(
                l_Array->At(i), l_Array->GetElementTypeId());
          }
        }
      };

      class GenericValueUserType : public CUserType
      {
      public:
        GenericValueUserType(asIScriptEngine *p_Engine)
            : m_Engine(p_Engine)
        {
        }

        void *AllocateUnitializedMemory(CSerializedValue *p_Value)
        {
          return nullptr;
        }

        void Store(CSerializedValue *p_Value, void *p_Ptr)
        {
          asITypeInfo *l_Type = p_Value->GetType();
          LOW_ASSERT(l_Type, "Could not resolve type to serialize.");

          void *l_Copy =
              m_Engine->CreateScriptObjectCopy(p_Ptr, l_Type);
          p_Value->SetUserData(l_Copy);
        }

        void Restore(CSerializedValue *p_Value, void *p_Ptr)
        {
          void *l_Copy = p_Value->GetUserData();
          asITypeInfo *l_Type = p_Value->GetType();

          if (!l_Copy || !l_Type) {
            return;
          }

          m_Engine->AssignScriptObject(p_Ptr, l_Copy, l_Type);
        }

        void CleanupUserData(CSerializedValue *p_Value)
        {
          void *l_Copy = p_Value->GetUserData();
          asITypeInfo *l_Type = p_Value->GetType();

          if (l_Copy && l_Type) {
            m_Engine->ReleaseScriptObject(l_Copy, l_Type);
          }

          p_Value->SetUserData(nullptr);
        }

      private:
        asIScriptEngine *m_Engine;
      };

      void ScriptSerializer::detach_extra_objects()
      {
        m_extraObjects.clear();
      }

      bool
      ScriptSerializer::restore_object(asIScriptObject *p_OldObject,
                                       asIScriptObject *p_NewObject)
      {
        if (!p_OldObject || !p_NewObject) {
          return false;
        }

        CSerializedValue *l_Value = m_root.FindByPtr(p_OldObject);
        if (!l_Value) {
          return false;
        }

        l_Value->Restore(p_NewObject, p_NewObject->GetTypeId());
        return true;
      }

      void register_generic_user_types(CSerializer &p_Serializer,
                                       asIScriptEngine *p_Engine)
      {
        p_Serializer.AddUserType(new ArrayUserType(), "array");

        const asUINT l_Count = p_Engine->GetObjectTypeCount();

        for (asUINT i = 0; i < l_Count; ++i) {
          asITypeInfo *i_Type = p_Engine->GetObjectTypeByIndex(i);

          const asDWORD i_Flags = i_Type->GetFlags();

          if (!(i_Flags & asOBJ_VALUE)) {
            continue;
          }

          if (i_Flags & asOBJ_POD) {
            continue;
          }

          if (i_Flags & (asOBJ_TEMPLATE | asOBJ_TEMPLATE_SUBTYPE)) {
            continue;
          }

          p_Serializer.AddUserType(new GenericValueUserType(p_Engine),
                                   i_Type->GetName());
        }
      }
    } // namespace Scripting
  } // namespace Core
} // namespace Low
