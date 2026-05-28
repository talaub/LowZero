#include <angelscript.h>

#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilString.h"
#include "LowUtilHandle.h"

#include "LowMathVectorUtil.h"

#include "LowCoreEntity.h"
#include "LowCoreInput.h"

#include "LowCore.h"

#include <scriptstdstring/scriptstdstring.h>

namespace Low {
  namespace Core {
    namespace Scripting {
      // BEGIN REGISTER RUNTIME
      static bool runtime_is_playing()
      {
        return Low::Core::get_engine_state() ==
               Low::Util::EngineState::PLAYING;
      }

      static void expose_runtime(asIScriptEngine *p_Engine)
      {
        int r = p_Engine->SetDefaultNamespace("Runtime");
        LOW_ASSERT(r >= 0, "Failed to set namespace");

        r = p_Engine->RegisterEnum("EngineState");
        LOW_ASSERT(r >= 0, "Failed to register Runtime::EngineState");

        r = p_Engine->RegisterEnumValue(
            "EngineState", "Editing",
            static_cast<int>(Low::Util::EngineState::EDITING));
        LOW_ASSERT(
            r >= 0,
            "Failed to register Runtime::EngineState::Editing");

        r = p_Engine->RegisterEnumValue(
            "EngineState", "Playing",
            static_cast<int>(Low::Util::EngineState::PLAYING));
        LOW_ASSERT(
            r >= 0,
            "Failed to register Runtime::EngineState::Playing");

        r = p_Engine->RegisterGlobalFunction(
            "float get_delta_time() property",
            asFUNCTION(Low::Core::get_delta_time), asCALL_CDECL);
        LOW_ASSERT(r >= 0, "Failed to register Runtime::delta_time");

        r = p_Engine->RegisterGlobalFunction(
            "EngineState get_state() property",
            asFUNCTION(Low::Core::get_engine_state), asCALL_CDECL);
        LOW_ASSERT(r >= 0, "Failed to register Runtime::state");

        r = p_Engine->RegisterGlobalFunction(
            "bool get_is_playing() property",
            asFUNCTION(runtime_is_playing), asCALL_CDECL);
        LOW_ASSERT(r >= 0, "Failed to register Runtime::is_playing");

        r = p_Engine->SetDefaultNamespace("");
        LOW_ASSERT(r >= 0, "Failed to reset namespace");
      }
      // END REGISTER RUNTIME
      // ------------------------------------------------------
      // BEGIN REGISTER INPUT
      static bool input_is_key_down(Low::Util::KeyboardButton p_Button)
      {
        return Low::Core::Input::keyboard_button_down(p_Button);
      }

      static void expose_input(asIScriptEngine *p_Engine)
      {
        int r = p_Engine->SetDefaultNamespace("Input");
        LOW_ASSERT(r >= 0, "Failed to set Input namespace");

        r = p_Engine->RegisterEnum("KeyboardButton");
        LOW_ASSERT(r >= 0, "Failed to register Input::KeyboardButton");

#define LOW_AS_REGISTER_KEYBOARD_BUTTON(x)                          \
  r = p_Engine->RegisterEnumValue(                                  \
      "KeyboardButton", #x,                                         \
      static_cast<int>(Low::Util::KeyboardButton::x));              \
  LOW_ASSERT(r >= 0,                                                \
             "Failed to register Input::KeyboardButton::" #x)

        LOW_AS_REGISTER_KEYBOARD_BUTTON(Q);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(W);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(E);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(R);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(T);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(Y);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(U);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(I);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(O);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(P);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(A);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(S);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(D);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(F);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(G);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(H);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(J);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(K);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(L);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(Z);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(X);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(C);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(V);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(B);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(N);
        LOW_AS_REGISTER_KEYBOARD_BUTTON(M);

#undef LOW_AS_REGISTER_KEYBOARD_BUTTON

        r = p_Engine->RegisterGlobalFunction(
            "bool is_key_down(KeyboardButton)",
            asFUNCTION(input_is_key_down), asCALL_CDECL);
        LOW_ASSERT(r >= 0, "Failed to register Input::is_key_down");

        r = p_Engine->SetDefaultNamespace("");
        LOW_ASSERT(r >= 0, "Failed to reset namespace");
      }
      // END REGISTER INPUT
      // ------------------------------------------------------
      // BEGIN REGISTER HANDLE
      static void
      handle_default_construct(Low::Util::Handle *p_Memory)
      {
        new (p_Memory) Low::Util::Handle();
      }

      static void handle_u64_construct(u64 p_Id,
                                       Low::Util::Handle *p_Memory)
      {
        new (p_Memory) Low::Util::Handle(p_Id);
      }

      static void
      handle_copy_construct(const Low::Util::Handle &p_Other,
                            Low::Util::Handle *p_Memory)
      {
        new (p_Memory) Low::Util::Handle(p_Other);
      }

      static void handle_destruct(Low::Util::Handle *p_Memory)
      {
        p_Memory->~Handle();
      }

      static Low::Util::Handle &
      handle_assign(const Low::Util::Handle &p_Other,
                    Low::Util::Handle *p_Self)
      {
        *p_Self = p_Other;
        return *p_Self;
      }

      static bool handle_equals(const Low::Util::Handle &p_A,
                                const Low::Util::Handle &p_B)
      {
        return p_A == p_B;
      }

      static int handle_cmp(const Low::Util::Handle &p_A,
                            const Low::Util::Handle &p_B)
      {
        if (p_A == p_B) {
          return 0;
        }
        return p_A < p_B ? -1 : 1;
      }

      static u64 handle_get_id(const Low::Util::Handle &p_Handle)
      {
        return p_Handle.get_id();
      }

      static u32 handle_get_index(const Low::Util::Handle &p_Handle)
      {
        return p_Handle.get_index();
      }

      static u16
      handle_get_generation(const Low::Util::Handle &p_Handle)
      {
        return p_Handle.get_generation();
      }

      static u16 handle_get_type(const Low::Util::Handle &p_Handle)
      {
        return p_Handle.get_type();
      }

      static bool handle_is_valid(const Low::Util::Handle &p_Handle)
      {
        return p_Handle.get_id() != Low::Util::Handle::DEAD;
      }

      static void expose_handle(asIScriptEngine *p_Engine)
      {
        int r = 0;

        r = p_Engine->RegisterObjectType(
            "Handle", sizeof(Low::Util::Handle),
            asOBJ_VALUE | asGetTypeTraits<Low::Util::Handle>());
        LOW_ASSERT(r >= 0, "Failed to register Handle type");

        r = p_Engine->RegisterObjectBehaviour(
            "Handle", asBEHAVE_CONSTRUCT, "void f()",
            asFUNCTION(handle_default_construct),
            asCALL_CDECL_OBJLAST);
        LOW_ASSERT(r >= 0, "Failed to register Handle default ctor");

        r = p_Engine->RegisterObjectBehaviour(
            "Handle", asBEHAVE_CONSTRUCT, "void f(u64 id)",
            asFUNCTION(handle_u64_construct), asCALL_CDECL_OBJLAST);
        LOW_ASSERT(r >= 0, "Failed to register Handle u64 ctor");

        r = p_Engine->RegisterObjectBehaviour(
            "Handle", asBEHAVE_CONSTRUCT, "void f(const Handle &in)",
            asFUNCTION(handle_copy_construct), asCALL_CDECL_OBJLAST);
        LOW_ASSERT(r >= 0, "Failed to register Handle copy ctor");

        r = p_Engine->RegisterObjectBehaviour(
            "Handle", asBEHAVE_DESTRUCT, "void f()",
            asFUNCTION(handle_destruct), asCALL_CDECL_OBJLAST);
        LOW_ASSERT(r >= 0, "Failed to register Handle dtor");

        r = p_Engine->RegisterObjectMethod(
            "Handle", "Handle &opAssign(const Handle &in)",
            asFUNCTION(handle_assign), asCALL_CDECL_OBJLAST);
        LOW_ASSERT(r >= 0, "Failed to register Handle assignment");

        r = p_Engine->RegisterObjectMethod(
            "Handle", "bool opEquals(const Handle &in) const",
            asFUNCTION(handle_equals), asCALL_CDECL_OBJFIRST);
        LOW_ASSERT(r >= 0, "Failed to register Handle equality");

        r = p_Engine->RegisterObjectMethod(
            "Handle", "int opCmp(const Handle &in) const",
            asFUNCTION(handle_cmp), asCALL_CDECL_OBJFIRST);
        LOW_ASSERT(r >= 0, "Failed to register Handle compare");

        r = p_Engine->RegisterObjectMethod(
            "Handle", "u64 get_id() const property",
            asFUNCTION(handle_get_id), asCALL_CDECL_OBJFIRST);
        LOW_ASSERT(r >= 0, "Failed to register Handle::id");

        r = p_Engine->RegisterObjectMethod(
            "Handle", "u32 get_index() const property",
            asFUNCTION(handle_get_index), asCALL_CDECL_OBJFIRST);
        LOW_ASSERT(r >= 0, "Failed to register Handle::index");

        r = p_Engine->RegisterObjectMethod(
            "Handle", "u16 get_generation() const property",
            asFUNCTION(handle_get_generation), asCALL_CDECL_OBJFIRST);
        LOW_ASSERT(r >= 0, "Failed to register Handle::generation");

        r = p_Engine->RegisterObjectMethod(
            "Handle", "u16 get_type() const property",
            asFUNCTION(handle_get_type), asCALL_CDECL_OBJFIRST);
        LOW_ASSERT(r >= 0, "Failed to register Handle::type");

        r = p_Engine->RegisterObjectMethod(
            "Handle", "bool get_is_valid() const property",
            asFUNCTION(handle_is_valid), asCALL_CDECL_OBJFIRST);
        LOW_ASSERT(r >= 0, "Failed to register Handle::is_valid");
      }

      // END REGISTER HANDLE
      // ------------------------------------------------------
      // BEGIN REGISTER MATH
      static void expose_typedefs(asIScriptEngine *p_Engine)
      {
        int r = 0;

        r = p_Engine->RegisterTypedef("u8", "uint8");
        LOW_ASSERT(r >= 0, "Failed to register typedef u8");

        r = p_Engine->RegisterTypedef("u16", "uint16");
        LOW_ASSERT(r >= 0, "Failed to register typedef u16");

        r = p_Engine->RegisterTypedef("u32", "uint");
        LOW_ASSERT(r >= 0, "Failed to register typedef u32");

        r = p_Engine->RegisterTypedef("u64", "uint64");
        LOW_ASSERT(r >= 0, "Failed to register typedef u64");

        r = p_Engine->RegisterTypedef("i32", "int");
        LOW_ASSERT(r >= 0, "Failed to register typedef i32");

        r = p_Engine->RegisterTypedef("i64", "int64");
        LOW_ASSERT(r >= 0, "Failed to register typedef i64");
      }
      static void
      uvector2_default_construct(Low::Math::UVector2 *p_Memory)
      {
        new (p_Memory) Low::Math::UVector2();
      }
      static void
      uvector2_init_construct(const u32 p_X, const u32 p_Y,
                              Low::Math::UVector2 *p_Memory)
      {
        new (p_Memory) Low::Math::UVector2(p_X, p_Y);
      }

      static void uvector2_scalar_construct(const u32 p_Value,
                                            Low::Math::UVector2 *p_Memory)
      {
        new (p_Memory) Low::Math::UVector2(p_Value);
      }

      static void
      vector2_default_construct(Low::Math::Vector2 *p_Memory)
      {
        new (p_Memory) Low::Math::Vector2();
      }

      static void vector2_init_construct(const float p_X,
                                         const float p_Y,
                                         Low::Math::Vector2 *p_Memory)
      {
        new (p_Memory) Low::Math::Vector2(p_X, p_Y);
      }

      static void vector2_scalar_construct(const float p_Value,
                                           Low::Math::Vector2 *p_Memory)
      {
        new (p_Memory) Low::Math::Vector2(p_Value);
      }

      static void
      vector3_default_construct(Low::Math::Vector3 *p_Memory)
      {
        new (p_Memory) Low::Math::Vector3();
      }

      static void vector3_init_construct(float p_X, float p_Y,
                                         float p_Z,
                                         Low::Math::Vector3 *p_Memory)
      {
        new (p_Memory) Low::Math::Vector3(p_X, p_Y, p_Z);
      }

      static void vector3_scalar_construct(const float p_Value,
                                           Low::Math::Vector3 *p_Memory)
      {
        new (p_Memory) Low::Math::Vector3(p_Value);
      }

      static void
      vector4_default_construct(Low::Math::Vector4 *p_Memory)
      {
        new (p_Memory) Low::Math::Vector4();
      }

      static void vector4_init_construct(float p_X, float p_Y,
                                         float p_Z, float p_W,
                                         Low::Math::Vector4 *p_Memory)
      {
        new (p_Memory) Low::Math::Vector4(p_X, p_Y, p_Z, p_W);
      }

      static void vector4_scalar_construct(const float p_Value,
                                           Low::Math::Vector4 *p_Memory)
      {
        new (p_Memory) Low::Math::Vector4(p_Value);
      }

      static void
      quaternion_default_construct(Low::Math::Quaternion *p_Memory)
      {
        new (p_Memory) Low::Math::Quaternion();
      }

      static void
      quaternion_init_construct(float p_X, float p_Y, float p_Z,
                                float p_W,
                                Low::Math::Quaternion *p_Memory)
      {
        Low::Math::Quaternion l_Q;
        l_Q.x = p_X;
        l_Q.y = p_Y;
        l_Q.z = p_Z;
        l_Q.w = p_W;
        new (p_Memory) Low::Math::Quaternion(l_Q);
      }

      static float script_arg_float(asIScriptGeneric *p_Gen)
      {
        return p_Gen->GetArgFloat(0);
      }

      static u32 script_arg_u32(asIScriptGeneric *p_Gen)
      {
        return static_cast<u32>(p_Gen->GetArgDWord(0));
      }

#define VEC_OPT(x, y, s, get_scalar)                                 \
  static void x##_add_generic(asIScriptGeneric *p_Gen)               \
  {                                                                  \
    auto *l_Self = static_cast<Low::Math::y *>(p_Gen->GetObject());  \
    auto *l_Other =                                                  \
        static_cast<Low::Math::y *>(p_Gen->GetArgAddress(0));        \
    auto *l_Return = static_cast<Low::Math::y *>(                    \
        p_Gen->GetAddressOfReturnLocation());                        \
    *l_Return = (*l_Self) + (*l_Other);                              \
  }                                                                  \
  static Low::Math::y &x##_add_assign(const Low::Math::y &p_Other,   \
                                      Low::Math::y *p_Self)          \
  {                                                                  \
    *p_Self += p_Other;                                              \
    return *p_Self;                                                  \
  }                                                                  \
  static void x##_sub_generic(asIScriptGeneric *p_Gen)               \
  {                                                                  \
    auto *l_Self = static_cast<Low::Math::y *>(p_Gen->GetObject());  \
    auto *l_Other =                                                  \
        static_cast<Low::Math::y *>(p_Gen->GetArgAddress(0));        \
    auto *l_Return = static_cast<Low::Math::y *>(                    \
        p_Gen->GetAddressOfReturnLocation());                        \
    *l_Return = (*l_Self) - (*l_Other);                              \
  }                                                                  \
  static void x##_mul_generic(asIScriptGeneric *p_Gen)               \
  {                                                                  \
    auto *l_Self = static_cast<Low::Math::y *>(p_Gen->GetObject());  \
    const s l_Other = get_scalar(p_Gen);                             \
    auto *l_Return = static_cast<Low::Math::y *>(                    \
        p_Gen->GetAddressOfReturnLocation());                        \
    *l_Return = (*l_Self) * l_Other;                                 \
  }                                                                  \
  static void x##_div_generic(asIScriptGeneric *p_Gen)               \
  {                                                                  \
    auto *l_Self = static_cast<Low::Math::y *>(p_Gen->GetObject());  \
    const s l_Other = get_scalar(p_Gen);                             \
    auto *l_Return = static_cast<Low::Math::y *>(                    \
        p_Gen->GetAddressOfReturnLocation());                        \
    *l_Return = (*l_Self) / l_Other;                                 \
  }                                                                  \
  static void x##_equals_generic(asIScriptGeneric *p_Gen)            \
  {                                                                  \
    auto *l_Self = static_cast<Low::Math::y *>(p_Gen->GetObject());  \
    auto *l_Other =                                                  \
        static_cast<Low::Math::y *>(p_Gen->GetArgAddress(0));        \
    auto *l_Return =                                                 \
        static_cast<bool *>(p_Gen->GetAddressOfReturnLocation());    \
    *l_Return = (*l_Self) == (*l_Other);                             \
  }

      VEC_OPT(vector2, Vector2, float, script_arg_float)
      VEC_OPT(uvector2, UVector2, u32, script_arg_u32)
      VEC_OPT(vector3, Vector3, float, script_arg_float)
      VEC_OPT(vector4, Vector4, float, script_arg_float)

#undef VEC_OPT

      static float vector3_magnitude(const Low::Math::Vector3 &p_Self)
      {
        return Low::Math::VectorUtil::magnitude(p_Self);
      }

      static Low::Math::Vector3
      vector3_normalized(const Low::Math::Vector3 &p_Self)
      {
        return Low::Math::VectorUtil::normalize(p_Self);
      }

      static float
      vector3_distance_to(const Low::Math::Vector3 &p_Self,
                          const Low::Math::Vector3 &p_Other)
      {
        return Low::Math::VectorUtil::distance(p_Self, p_Other);
      }

      static Low::Math::Vector3
      vector3_lerp(const Low::Math::Vector3 &p_From,
                   const Low::Math::Vector3 &p_To,
                   const float p_Delta)
      {
        return Low::Math::VectorUtil::lerp(p_From, p_To, p_Delta);
      }

      static Low::Math::Vector3 vector3_forward()
      {
        return Low::Math::Vector3(0.0f, 0.0f, -1.0f);
      }

      static Low::Math::Vector3 vector3_back()
      {
        return Low::Math::Vector3(0.0f, 0.0f, 1.0f);
      }

      static Low::Math::Vector3 vector3_up()
      {
        return Low::Math::Vector3(0.0f, 1.0f, 0.0f);
      }

      static Low::Math::Vector3 vector3_down()
      {
        return Low::Math::Vector3(0.0f, -1.0f, 0.0f);
      }

      static Low::Math::Vector3 vector3_right()
      {
        return Low::Math::Vector3(1.0f, 0.0f, 0.0f);
      }

      static Low::Math::Vector3 vector3_left()
      {
        return Low::Math::Vector3(-1.0f, 0.0f, 0.0f);
      }

      static Low::Math::Quaternion
      quaternion_from_direction(const Low::Math::Vector3 &p_Direction,
                                const Low::Math::Vector3 &p_Up)
      {
        return Low::Math::VectorUtil::from_direction(p_Direction,
                                                     p_Up);
      }

      static void expose_math(asIScriptEngine *p_Engine)
      {
        expose_typedefs(p_Engine);

        int r = p_Engine->RegisterObjectType(
            "Vector2", sizeof(Low::Math::Vector2),
            asOBJ_VALUE | asOBJ_POD |
                asGetTypeTraits<Low::Math::Vector2>());
        LOW_ASSERT(r >= 0, "Failed to register Vector2");

        r = p_Engine->RegisterObjectType(
            "UVector2", sizeof(Low::Math::UVector2),
            asOBJ_VALUE | asOBJ_POD |
                asGetTypeTraits<Low::Math::UVector2>());
        LOW_ASSERT(r >= 0, "Failed to register UVector2");

        r = p_Engine->RegisterObjectType(
            "Vector3", sizeof(Low::Math::Vector3),
            asOBJ_VALUE | asOBJ_POD |
                asGetTypeTraits<Low::Math::Vector3>());
        LOW_ASSERT(r >= 0, "Failed to register Vector3");

        r = p_Engine->RegisterObjectType(
            "Vector4", sizeof(Low::Math::Vector4),
            asOBJ_VALUE | asOBJ_POD |
                asGetTypeTraits<Low::Math::Vector4>());
        LOW_ASSERT(r >= 0, "Failed to register Vector4");

        r = p_Engine->RegisterObjectType(
            "Quaternion", sizeof(Low::Math::Quaternion),
            asOBJ_VALUE | asOBJ_POD |
                asGetTypeTraits<Low::Math::Quaternion>());
        LOW_ASSERT(r >= 0, "Failed to register Quaternion");

        // UVec2
        {
          r = p_Engine->RegisterObjectProperty(
              "UVector2", "u32 x", asOFFSET(Low::Math::UVector2, x));
          LOW_ASSERT(r >= 0, "Failed to register UVector2 x field");
          r = p_Engine->RegisterObjectProperty(
              "UVector2", "u32 y", asOFFSET(Low::Math::UVector2, y));
          LOW_ASSERT(r >= 0, "Failed to register UVector2 y field");

          r = p_Engine->RegisterObjectBehaviour(
              "UVector2", asBEHAVE_CONSTRUCT, "void f()",
              asFUNCTION(uvector2_default_construct),
              asCALL_CDECL_OBJLAST);
          LOW_ASSERT(
              r >= 0,
              "Failed to register UVector2 default constructor");

          r = p_Engine->RegisterObjectBehaviour(
              "UVector2", asBEHAVE_CONSTRUCT, "void f(u32 x, u32 y)",
              asFUNCTION(uvector2_init_construct),
              asCALL_CDECL_OBJLAST);
          LOW_ASSERT(r >= 0,
                     "Failed to register UVector2 init constructor");
          r = p_Engine->RegisterObjectBehaviour(
              "UVector2", asBEHAVE_CONSTRUCT, "void f(u32 value)",
              asFUNCTION(uvector2_scalar_construct),
              asCALL_CDECL_OBJLAST);
          LOW_ASSERT(r >= 0,
                     "Failed to register UVector2 scalar constructor");

          r = p_Engine->RegisterObjectMethod(
              "UVector2", "UVector2 opAdd(const UVector2 &in) const",
              asFUNCTION(uvector2_add_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register UVector2::opAdd");
          r = p_Engine->RegisterObjectMethod(
              "UVector2",
              "UVector2& opAddAssign(const UVector2 &in)",
              asFUNCTION(uvector2_add_assign), asCALL_CDECL_OBJLAST);
          LOW_ASSERT(r >= 0,
                     "Failed to register UVector2::opAddAssign");
          r = p_Engine->RegisterObjectMethod(
              "UVector2", "UVector2 opSub(const UVector2 &in) const",
              asFUNCTION(uvector2_sub_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register UVector2::opSub");
          r = p_Engine->RegisterObjectMethod(
              "UVector2", "UVector2 opMul(const u32) const",
              asFUNCTION(uvector2_mul_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register UVector2::opMul");
          r = p_Engine->RegisterObjectMethod(
              "UVector2", "UVector2 opDiv(const u32) const",
              asFUNCTION(uvector2_div_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register UVector2::opDiv");
          r = p_Engine->RegisterObjectMethod(
              "UVector2", "bool opEquals(const UVector2& in) const",
              asFUNCTION(uvector2_equals_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register UVector2::opEquals");
        }

        // Vec2
        {
          r = p_Engine->RegisterObjectProperty(
              "Vector2", "float x", asOFFSET(Low::Math::Vector2, x));
          LOW_ASSERT(r >= 0, "Failed to register Vector2 x field");
          r = p_Engine->RegisterObjectProperty(
              "Vector2", "float y", asOFFSET(Low::Math::Vector2, y));
          LOW_ASSERT(r >= 0, "Failed to register Vector2 y field");

          r = p_Engine->RegisterObjectBehaviour(
              "Vector2", asBEHAVE_CONSTRUCT, "void f()",
              asFUNCTION(vector2_default_construct),
              asCALL_CDECL_OBJLAST);
          LOW_ASSERT(
              r >= 0,
              "Failed to register Vector2 default constructor");

          r = p_Engine->RegisterObjectBehaviour(
              "Vector2", asBEHAVE_CONSTRUCT,
              "void f(float x, float y)",
              asFUNCTION(vector2_init_construct),
              asCALL_CDECL_OBJLAST);
          LOW_ASSERT(r >= 0,
                     "Failed to register Vector2 init constructor");
          r = p_Engine->RegisterObjectBehaviour(
              "Vector2", asBEHAVE_CONSTRUCT, "void f(float value)",
              asFUNCTION(vector2_scalar_construct),
              asCALL_CDECL_OBJLAST);
          LOW_ASSERT(r >= 0,
                     "Failed to register Vector2 scalar constructor");

          r = p_Engine->RegisterObjectMethod(
              "Vector2", "Vector2 opAdd(const Vector2 &in) const",
              asFUNCTION(vector2_add_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector2::opAdd");
          r = p_Engine->RegisterObjectMethod(
              "Vector2", "Vector2& opAddAssign(const Vector2 &in)",
              asFUNCTION(vector2_add_assign), asCALL_CDECL_OBJLAST);
          LOW_ASSERT(r >= 0,
                     "Failed to register Vector2::opAddAssign");
          r = p_Engine->RegisterObjectMethod(
              "Vector2", "Vector2 opSub(const Vector2 &in) const",
              asFUNCTION(vector2_sub_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector2::opSub");
          r = p_Engine->RegisterObjectMethod(
              "Vector2", "Vector2 opMul(const float) const",
              asFUNCTION(vector2_mul_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector2::opMul");
          r = p_Engine->RegisterObjectMethod(
              "Vector2", "Vector2 opDiv(const float) const",
              asFUNCTION(vector2_div_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector2::opDiv");
          r = p_Engine->RegisterObjectMethod(
              "Vector2", "bool opEquals(const Vector2& in) const",
              asFUNCTION(vector2_equals_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector2::opEquals");
        }
        // Vec3
        {
          r = p_Engine->RegisterObjectProperty(
              "Vector3", "float x", asOFFSET(Low::Math::Vector3, x));
          r = p_Engine->RegisterObjectProperty(
              "Vector3", "float y", asOFFSET(Low::Math::Vector3, y));
          r = p_Engine->RegisterObjectProperty(
              "Vector3", "float z", asOFFSET(Low::Math::Vector3, z));

          r = p_Engine->RegisterObjectBehaviour(
              "Vector3", asBEHAVE_CONSTRUCT, "void f()",
              asFUNCTION(vector3_default_construct),
              asCALL_CDECL_OBJLAST);

          r = p_Engine->RegisterObjectBehaviour(
              "Vector3", asBEHAVE_CONSTRUCT,
              "void f(float x, float y, float z)",
              asFUNCTION(vector3_init_construct),
              asCALL_CDECL_OBJLAST);
          r = p_Engine->RegisterObjectBehaviour(
              "Vector3", asBEHAVE_CONSTRUCT, "void f(float value)",
              asFUNCTION(vector3_scalar_construct),
              asCALL_CDECL_OBJLAST);
          LOW_ASSERT(r >= 0,
                     "Failed to register Vector3 scalar constructor");

          r = p_Engine->RegisterObjectMethod(
              "Vector3", "Vector3 opAdd(const Vector3 &in) const",
              asFUNCTION(vector3_add_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::opAdd");
          r = p_Engine->RegisterObjectMethod(
              "Vector3", "Vector3& opAddAssign(const Vector3 &in)",
              asFUNCTION(vector3_add_assign), asCALL_CDECL_OBJLAST);
          LOW_ASSERT(r >= 0,
                     "Failed to register Vector3::opAddAssign");
          r = p_Engine->RegisterObjectMethod(
              "Vector3", "Vector3 opSub(const Vector3 &in) const",
              asFUNCTION(vector3_sub_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::opSub");
          r = p_Engine->RegisterObjectMethod(
              "Vector3", "Vector3 opMul(const float) const",
              asFUNCTION(vector3_mul_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::opMul");
          r = p_Engine->RegisterObjectMethod(
              "Vector3", "Vector3 opDiv(const float) const",
              asFUNCTION(vector3_div_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::opDiv");
          r = p_Engine->RegisterObjectMethod(
              "Vector3", "bool opEquals(const Vector3& in) const",
              asFUNCTION(vector3_equals_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::opEquals");

          r = p_Engine->RegisterObjectMethod(
              "Vector3", "float get_magnitude() const property",
              asFUNCTION(vector3_magnitude), asCALL_CDECL_OBJFIRST);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::magnitude");

          r = p_Engine->RegisterObjectMethod(
              "Vector3", "Vector3 get_normalized() const property",
              asFUNCTION(vector3_normalized), asCALL_CDECL_OBJFIRST);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::normalize");

          r = p_Engine->RegisterObjectMethod(
              "Vector3", "float distance_to(const Vector3 &in) const",
              asFUNCTION(vector3_distance_to), asCALL_CDECL_OBJFIRST);
          LOW_ASSERT(r >= 0,
                     "Failed to register Vector3::distance_to");

          r = p_Engine->SetDefaultNamespace("Vector3");
          LOW_ASSERT(r >= 0, "Failed to set Vector3 namespace");

          r = p_Engine->RegisterGlobalFunction(
              "Vector3 lerp(const Vector3 &in, const Vector3 &in, "
              "float)",
              asFUNCTION(vector3_lerp), asCALL_CDECL);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::lerp");

          r = p_Engine->RegisterGlobalFunction(
              "Vector3 get_forward() property",
              asFUNCTION(vector3_forward), asCALL_CDECL);
          LOW_ASSERT(r >= 0,
                     "Failed to register Vector3::forward");

          r = p_Engine->RegisterGlobalFunction(
              "Vector3 get_back() property", asFUNCTION(vector3_back),
              asCALL_CDECL);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::back");

          r = p_Engine->RegisterGlobalFunction(
              "Vector3 get_up() property", asFUNCTION(vector3_up),
              asCALL_CDECL);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::up");

          r = p_Engine->RegisterGlobalFunction(
              "Vector3 get_down() property", asFUNCTION(vector3_down),
              asCALL_CDECL);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::down");

          r = p_Engine->RegisterGlobalFunction(
              "Vector3 get_right() property",
              asFUNCTION(vector3_right), asCALL_CDECL);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::right");

          r = p_Engine->RegisterGlobalFunction(
              "Vector3 get_left() property", asFUNCTION(vector3_left),
              asCALL_CDECL);
          LOW_ASSERT(r >= 0, "Failed to register Vector3::left");

          r = p_Engine->SetDefaultNamespace("");
          LOW_ASSERT(r >= 0, "Failed to reset namespace");
        }
        // Vec4
        {
          r = p_Engine->RegisterObjectProperty(
              "Vector4", "float x", asOFFSET(Low::Math::Vector4, x));
          r = p_Engine->RegisterObjectProperty(
              "Vector4", "float y", asOFFSET(Low::Math::Vector4, y));
          r = p_Engine->RegisterObjectProperty(
              "Vector4", "float z", asOFFSET(Low::Math::Vector4, z));
          r = p_Engine->RegisterObjectProperty(
              "Vector4", "float w", asOFFSET(Low::Math::Vector4, w));

          r = p_Engine->RegisterObjectBehaviour(
              "Vector4", asBEHAVE_CONSTRUCT, "void f()",
              asFUNCTION(vector4_default_construct),
              asCALL_CDECL_OBJLAST);

          r = p_Engine->RegisterObjectBehaviour(
              "Vector4", asBEHAVE_CONSTRUCT,
              "void f(float x, float y, float z, float w)",
              asFUNCTION(vector4_init_construct),
              asCALL_CDECL_OBJLAST);
          r = p_Engine->RegisterObjectBehaviour(
              "Vector4", asBEHAVE_CONSTRUCT, "void f(float value)",
              asFUNCTION(vector4_scalar_construct),
              asCALL_CDECL_OBJLAST);
          LOW_ASSERT(r >= 0,
                     "Failed to register Vector4 scalar constructor");

          r = p_Engine->RegisterObjectMethod(
              "Vector4", "Vector4 opAdd(const Vector4 &in) const",
              asFUNCTION(vector4_add_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector4::opAdd");
          r = p_Engine->RegisterObjectMethod(
              "Vector4", "Vector4& opAddAssign(const Vector4 &in)",
              asFUNCTION(vector4_add_assign), asCALL_CDECL_OBJLAST);
          LOW_ASSERT(r >= 0,
                     "Failed to register Vector4::opAddAssign");
          r = p_Engine->RegisterObjectMethod(
              "Vector4", "Vector4 opSub(const Vector4 &in) const",
              asFUNCTION(vector4_sub_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector4::opSub");
          r = p_Engine->RegisterObjectMethod(
              "Vector4", "Vector4 opMul(const float) const",
              asFUNCTION(vector4_mul_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector4::opMul");
          r = p_Engine->RegisterObjectMethod(
              "Vector4", "Vector4 opDiv(const float) const",
              asFUNCTION(vector4_div_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector4::opDiv");
          r = p_Engine->RegisterObjectMethod(
              "Vector4", "bool opEquals(const Vector4& in) const",
              asFUNCTION(vector4_equals_generic), asCALL_GENERIC);
          LOW_ASSERT(r >= 0, "Failed to register Vector4::opEquals");
        }
        // Quat
        {
          r = p_Engine->RegisterObjectProperty(
              "Quaternion", "float x",
              asOFFSET(Low::Math::Quaternion, x));
          r = p_Engine->RegisterObjectProperty(
              "Quaternion", "float y",
              asOFFSET(Low::Math::Quaternion, y));
          r = p_Engine->RegisterObjectProperty(
              "Quaternion", "float z",
              asOFFSET(Low::Math::Quaternion, z));
          r = p_Engine->RegisterObjectProperty(
              "Quaternion", "float w",
              asOFFSET(Low::Math::Quaternion, w));

          r = p_Engine->RegisterObjectBehaviour(
              "Quaternion", asBEHAVE_CONSTRUCT, "void f()",
              asFUNCTION(quaternion_default_construct),
              asCALL_CDECL_OBJLAST);

          r = p_Engine->RegisterObjectBehaviour(
              "Quaternion", asBEHAVE_CONSTRUCT,
              "void f(float x, float y, float z, float w)",
              asFUNCTION(quaternion_init_construct),
              asCALL_CDECL_OBJLAST);

          r = p_Engine->SetDefaultNamespace("Quaternion");
          LOW_ASSERT(r >= 0, "Failed to set Quaternion namespace");

          r = p_Engine->RegisterGlobalFunction(
              "Quaternion from_direction(const Vector3 &in, "
              "const Vector3 &in)",
              asFUNCTION(quaternion_from_direction), asCALL_CDECL);
          LOW_ASSERT(
              r >= 0,
              "Failed to register Quaternion::from_direction");

          r = p_Engine->SetDefaultNamespace("");
          LOW_ASSERT(r >= 0, "Failed to reset namespace");
        }
      }
      // END REGISTER MATH
      // ------------------------------------------------------
      // BEGIN REGISTER NAME
      static void name_default_construct(Low::Util::Name *p_Memory)
      {
        new (p_Memory) Low::Util::Name();
      }

      static void name_copy_construct(const Low::Util::Name &p_Other,
                                      Low::Util::Name *p_Memory)
      {
        new (p_Memory) Low::Util::Name(p_Other);
      }

      static void
      name_from_string_construct(const std::string &p_String,
                                 Low::Util::Name *p_Memory)
      {
        new (p_Memory) Low::Util::Name(p_String.c_str());
      }

      static void name_destruct(Low::Util::Name *p_Memory)
      {
        p_Memory->~Name();
      }

      static int name_op_cmp(const Low::Util::Name &p_A,
                             const Low::Util::Name &p_B)
      {
        if (p_A == p_B) {
          return 0;
        }
        return p_A < p_B ? -1 : 1;
      }

      static std::string name_to_string(const Low::Util::Name &p_Name)
      {
        return p_Name.c_str() ? p_Name.c_str() : "";
      }

      static Low::Util::Name make_name(const std::string &p_String)
      {
        return Low::Util::Name(p_String.c_str());
      }

      static void expose_name(asIScriptEngine *p_Engine)
      {
        int r = 0;

        r = p_Engine->RegisterObjectType(
            "Name", sizeof(Low::Util::Name),
            asOBJ_VALUE | asGetTypeTraits<Low::Util::Name>());
        LOW_ASSERT(r >= 0, "Failed to register Name type");

        r = p_Engine->RegisterObjectBehaviour(
            "Name", asBEHAVE_CONSTRUCT, "void f()",
            asFUNCTION(name_default_construct), asCALL_CDECL_OBJLAST);
        LOW_ASSERT(r >= 0, "Failed to register Name default ctor");

        r = p_Engine->RegisterObjectBehaviour(
            "Name", asBEHAVE_CONSTRUCT, "void f(const Name &in)",
            asFUNCTION(name_copy_construct), asCALL_CDECL_OBJLAST);
        LOW_ASSERT(r >= 0, "Failed to register Name copy ctor");

        r = p_Engine->RegisterObjectBehaviour(
            "Name", asBEHAVE_CONSTRUCT, "void f(const string &in)",
            asFUNCTION(name_from_string_construct),
            asCALL_CDECL_OBJLAST);
        LOW_ASSERT(r >= 0, "Failed to register Name string ctor");

        r = p_Engine->RegisterObjectBehaviour(
            "Name", asBEHAVE_DESTRUCT, "void f()",
            asFUNCTION(name_destruct), asCALL_CDECL_OBJLAST);
        LOW_ASSERT(r >= 0, "Failed to register Name dtor");

        r = p_Engine->RegisterObjectMethod(
            "Name", "Name &opAssign(const Name &in)",
            asMETHODPR(Low::Util::Name, operator=,
                       (const Low::Util::Name), Low::Util::Name &),
            asCALL_THISCALL);
        LOW_ASSERT(r >= 0, "Failed to register Name assignment");

        r = p_Engine->RegisterObjectMethod(
            "Name", "bool opEquals(const Name &in) const",
            asMETHODPR(Low::Util::Name, operator==,
                       (const Low::Util::Name &) const, bool),
            asCALL_THISCALL);
        LOW_ASSERT(r >= 0, "Failed to register Name equality");

        r = p_Engine->RegisterObjectMethod(
            "Name", "bool opCmp(const Name &in) const",
            asFUNCTION(name_op_cmp), asCALL_CDECL_OBJFIRST);
        LOW_ASSERT(r >= 0, "Failed to register Name compare");

        r = p_Engine->RegisterObjectMethod(
            "Name", "bool is_valid() const",
            asMETHOD(Low::Util::Name, is_valid), asCALL_THISCALL);
        LOW_ASSERT(r >= 0, "Failed to register Name::is_valid");

        r = p_Engine->RegisterObjectMethod(
            "Name", "string to_string() const",
            asFUNCTION(name_to_string), asCALL_CDECL_OBJFIRST);
        LOW_ASSERT(r >= 0, "Failed to register Name::to_string");

        r = p_Engine->RegisterGlobalFunction(
            "Name _N(const string &in)", asFUNCTION(make_name),
            asCALL_CDECL);
        LOW_ASSERT(r >= 0, "Failed to register _N() helper");
      }
      // END REGISTER NAME
      // ------------------------------------------------------
      // BEGIN REGISTER LOGGER
      static Low::Util::String
      script_arg_to_string(asIScriptGeneric *p_Gen, asUINT p_Index,
                           asIScriptEngine *p_Engine)
      {
        int l_TypeId = p_Gen->GetArgTypeId(p_Index);
        void *l_Arg = p_Gen->GetArgAddress(p_Index);

        using namespace Low;
        using namespace Low::Util;

        if (l_TypeId == p_Engine->GetTypeIdByDecl("int")) {
          return LOW_TO_STRING(*static_cast<int *>(l_Arg));
        }
        if (l_TypeId == p_Engine->GetTypeIdByDecl("uint")) {
          return LOW_TO_STRING(*static_cast<uint32_t *>(l_Arg));
        }
        if (l_TypeId == p_Engine->GetTypeIdByDecl("uint64")) {
          return LOW_TO_STRING(*static_cast<uint64_t *>(l_Arg));
        }
        if (l_TypeId == p_Engine->GetTypeIdByDecl("float")) {
          return LOW_TO_STRING(*static_cast<float *>(l_Arg));
        }
        if (l_TypeId == p_Engine->GetTypeIdByDecl("bool")) {
          return *static_cast<bool *>(l_Arg) ? "true" : "false";
        }
        if (l_TypeId == p_Engine->GetTypeIdByDecl("string")) {
          return static_cast<std::string *>(l_Arg)->c_str();
        }

        // Engine types you expose to script:
        if (l_TypeId == p_Engine->GetTypeIdByDecl("Name")) {
          Name l_Name = *static_cast<Name *>(l_Arg);
          return l_Name.c_str();
        }
        if (l_TypeId == p_Engine->GetTypeIdByDecl("Handle")) {
          Handle l_Handle = *static_cast<Handle *>(l_Arg);
          return LOW_TO_STRING(l_Handle.get_id());
        }

        return "<unsupported>";
      }

      static Low::Util::String
      simple_format(const Low::Util::String &p_Fmt,
                    const Low::Util::List<Low::Util::String> &p_Args)
      {
        using namespace Low::Util;

        String l_Out;
        size_t l_SearchStart = 0;
        uint32_t l_ArgIndex = 0;

        while (true) {
          size_t l_Pos = p_Fmt.find("{}", l_SearchStart);
          if (l_Pos == String::npos) {
            l_Out += p_Fmt.substr(l_SearchStart);
            break;
          }

          l_Out += p_Fmt.substr(l_SearchStart, l_Pos - l_SearchStart);

          if (l_ArgIndex < p_Args.size()) {
            l_Out += p_Args[l_ArgIndex++];
          } else {
            l_Out += "{}";
          }

          l_SearchStart = l_Pos + 2;
        }

        return l_Out;
      }

      static void
      script_log_impl(asIScriptGeneric *p_Gen,
                      Low::Util::Log::LogLevel::Enum p_Level)
      {
        using namespace Low;
        using namespace Low::Util;

        auto *l_FmtStd =
            static_cast<std::string *>(p_Gen->GetArgAddress(0));
        String l_Fmt = l_FmtStd->c_str();

        List<String> l_Args;
        for (asUINT i = 1; i < p_Gen->GetArgCount(); ++i) {
          l_Args.push_back(
              script_arg_to_string(p_Gen, i, p_Gen->GetEngine()));
        }

        String l_Message = simple_format(l_Fmt, l_Args);

        Log::begin_log(p_Level, "Script")
            << l_Message << Log::LogLineEnd::LINE_END;
      }

      static void script_log_info(asIScriptGeneric *p_Gen)
      {
        script_log_impl(p_Gen, Low::Util::Log::LogLevel::INFO);
      }

      static void script_log_warn(asIScriptGeneric *p_Gen)
      {
        script_log_impl(p_Gen, Low::Util::Log::LogLevel::WARN);
      }

      static void script_log_error(asIScriptGeneric *p_Gen)
      {
        script_log_impl(p_Gen, Low::Util::Log::LogLevel::ERROR);
      }

      static void script_log_debug(asIScriptGeneric *p_Gen)
      {
        script_log_impl(p_Gen, Low::Util::Log::LogLevel::DEBUG);
      }

      static void expose_logger(asIScriptEngine *p_Engine)
      {
        p_Engine->SetDefaultNamespace("Log");
        int r = p_Engine->RegisterGlobalFunction(
            "void info(const string &in fmt, const ?&in ...)",
            asFUNCTION(script_log_info), asCALL_GENERIC);

        r = p_Engine->RegisterGlobalFunction(
            "void warn(const string &in fmt, const ?&in ...)",
            asFUNCTION(script_log_warn), asCALL_GENERIC);

        r = p_Engine->RegisterGlobalFunction(
            "void error(const string &in fmt, const ?&in ...)",
            asFUNCTION(script_log_error), asCALL_GENERIC);

        r = p_Engine->RegisterGlobalFunction(
            "void debug(const string &in fmt, const ?&in ...)",
            asFUNCTION(script_log_debug), asCALL_GENERIC);
        p_Engine->SetDefaultNamespace("");
      }
      // END REGISTER LOGGER
      // BEGIN REGISTER INTERFACES
      static void
      register_ui_controller_interface(asIScriptEngine *p_Engine)
      {
        int r = 0;

        r = p_Engine->RegisterInterface("UiController");
        LOW_ASSERT(r >= 0,
                   "Failed to register interface UiController");

        r = p_Engine->RegisterInterfaceMethod(
            "UiController", "void on_click(UI::Element p_Element)");
        LOW_ASSERT(r >= 0,
                   "Failed to register UiController::on_click");
      }
      // END REGISTER INTERFACES

      static void register_base_types(asIScriptEngine *p_Engine)
      {
        RegisterStdString(p_Engine);
      }

      void expose(asIScriptEngine *p_Engine)
      {
        register_base_types(p_Engine);
        expose_math(p_Engine);
        expose_name(p_Engine);
        expose_logger(p_Engine);
        expose_handle(p_Engine);
        expose_runtime(p_Engine);
        expose_input(p_Engine);
      }

      void register_interfaces(asIScriptEngine *p_Engine)
      {
        register_ui_controller_interface(p_Engine);
      }
    } // namespace Scripting
  } // namespace Core
} // namespace Low
