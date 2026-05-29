#include <angelscript.h>

#include "LowCoreRegion.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreAnimator.h"
#include "LowCoreCamera.h"
#include "LowCoreScriptModule.h"
#include "LowCoreScriptAsset.h"
#include "LowCoreUiWidgetAsset.h"
#include "LowCoreUiWidgetInstance.h"
#include "LowCoreUiView.h"
#include "LowCoreUiElement.h"
#include "LowCoreUiDisplay.h"
#include "LowCoreUiText.h"
#include "LowCoreAnimationClip.h"
#include "LowRendererAnimationClip.h"
#include "LowRendererSkeleton.h"

// --------------------------
static void
LowCore_Region_default_construct(Low::Core::Region *p_Memory)
{
  new (p_Memory) Low::Core::Region;
}
static void LowCore_Region_id_construct(u64 p_Id,
                                        Low::Core::Region *p_Memory)
{
  new (p_Memory) Low::Core::Region(p_Id);
}
static void
LowCore_Region_copy_construct(const Low::Core::Region &p_Other,
                              Low::Core::Region *p_Memory)
{
  new (p_Memory) Low::Core::Region(p_Other);
}
static Low::Core::Region &
LowCore_Region_assign(const Low::Core::Region &p_Other,
                      Low::Core::Region *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void LowCore_Region_destruct(Low::Core::Region *p_Memory)
{
  using namespace Low::Core;
  p_Memory->~Region();
}
static Low::Core::Region
LowCore_Region_genmake(Low::Util::Name p_Name)
{
  return Low::Core::Region::make(p_Name);
}
static Low::Core::Region
LowCore_Region_genfindbyname(Low::Util::Name p_Name)
{
  return Low::Core::Region::find_by_name(p_Name);
}
static u32 LowCore_Region_living_count()
{
  return Low::Core::Region::living_count();
}
static u16 LowCore_Region_type_id()
{
  return Low::Core::Region::type_id();
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:REGION:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:REGION:HELPERS

static void register_LowCore_Region(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Region", sizeof(Low::Core::Region),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0, "Failed to expose Low::Core::Region type.");
}
static void expose_LowCore_Region(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "Region", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Region_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose default constructor of Low::Core::Region.");

  r = p_Engine->RegisterObjectBehaviour(
      "Region", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_Region_id_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0,
             "Failed to expose id constructor of Low::Core::Region.");

  r = p_Engine->RegisterObjectBehaviour(
      "Region", asBEHAVE_CONSTRUCT, "void f(const Region& in)",
      asFUNCTION(LowCore_Region_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose copy constructor of Low::Core::Region.");

  r = p_Engine->RegisterObjectMethod(
      "Region", "Region& opAssign(const Region& in)",
      asFUNCTION(LowCore_Region_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose assignment operator of Low::Core::Region.");

  r = p_Engine->RegisterObjectBehaviour(
      "Region", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Region_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0,
             "Failed to expose destructor of Low::Core::Region.");
  r = p_Engine->RegisterObjectMethod(
      "Region", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::Region, is_alive, () const, bool),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose is_alive getter for Low::Core::Region.");
  r = p_Engine->RegisterObjectMethod(
      "Region", "void destroy()",
      asMETHODPR(Low::Core::Region, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose destroy for Low::Core::Region.");

  r = p_Engine->RegisterObjectMethod(
      "Region", "Name get_name() const property",
      asMETHOD(Low::Core::Region, get_name), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for name of "
                     "Low::Core::Region.");
  r = p_Engine->RegisterObjectMethod(
      "Region", "void set_name(Name) property",
      asMETHOD(Low::Core::Region, set_name), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for name of "
                     "Low::Core::Region.");

  r = p_Engine->SetDefaultNamespace("Region");
  LOW_ASSERT(r >= 0,
             "Failed to set namespace for Low::Core::Region.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property",
      asFUNCTION(LowCore_Region_type_id), asCALL_CDECL);
  LOW_ASSERT(r >= 0,
             "Failed to expose TYPE_ID for Low::Core::Region.");
  r = p_Engine->RegisterGlobalFunction(
      "Region make(Name)", asFUNCTION(LowCore_Region_genmake),
      asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic make function for "
                     "Low::Core::Region.");
  r = p_Engine->RegisterGlobalFunction(
      "Region find_by_name(Name)",
      asFUNCTION(LowCore_Region_genfindbyname), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic find by name function "
                     "for Low::Core::Region.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:REGION:EXPOSE

  // LOW_CODEGEN::END::CUSTOM:LOWCORE:REGION:EXPOSE
}

// --------------------------
static void
LowCore_Entity_default_construct(Low::Core::Entity *p_Memory)
{
  new (p_Memory) Low::Core::Entity;
}
static void LowCore_Entity_id_construct(u64 p_Id,
                                        Low::Core::Entity *p_Memory)
{
  new (p_Memory) Low::Core::Entity(p_Id);
}
static void
LowCore_Entity_copy_construct(const Low::Core::Entity &p_Other,
                              Low::Core::Entity *p_Memory)
{
  new (p_Memory) Low::Core::Entity(p_Other);
}
static Low::Core::Entity &
LowCore_Entity_assign(const Low::Core::Entity &p_Other,
                      Low::Core::Entity *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void LowCore_Entity_destruct(Low::Core::Entity *p_Memory)
{
  using namespace Low::Core;
  p_Memory->~Entity();
}
static Low::Core::Entity
LowCore_Entity_genmake(Low::Util::Name p_Name)
{
  return Low::Core::Entity::make(p_Name);
}
static Low::Core::Entity
LowCore_Entity_genfindbyname(Low::Util::Name p_Name)
{
  return Low::Core::Entity::find_by_name(p_Name);
}
static u32 LowCore_Entity_living_count()
{
  return Low::Core::Entity::living_count();
}
static u16 LowCore_Entity_type_id()
{
  return Low::Core::Entity::type_id();
}
static uint64_t
LowCore_Entity_func_get_component(Low::Core::Entity p_This,
                                  uint16_t p_TypeId)
{
  return p_This.get_component(p_TypeId);
}
static void
LowCore_Entity_func_add_component(Low::Core::Entity p_This,
                                  Low::Util::Handle p_Component)
{
  p_This.add_component(p_Component);
}
static void
LowCore_Entity_func_remove_component(Low::Core::Entity p_This,
                                     uint16_t p_ComponentType)
{
  p_This.remove_component(p_ComponentType);
}
static bool
LowCore_Entity_func_has_component(Low::Core::Entity p_This,
                                  uint16_t p_ComponentType)
{
  return p_This.has_component(p_ComponentType);
}
static Low::Core::Component::Transform
LowCore_Entity_func_get_transform(Low::Core::Entity p_This)
{
  return p_This.get_transform();
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:ENTITY:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:ENTITY:HELPERS

static void register_LowCore_Entity(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Entity", sizeof(Low::Core::Entity),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0, "Failed to expose Low::Core::Entity type.");
}
static void expose_LowCore_Entity(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "Entity", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Entity_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose default constructor of Low::Core::Entity.");

  r = p_Engine->RegisterObjectBehaviour(
      "Entity", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_Entity_id_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0,
             "Failed to expose id constructor of Low::Core::Entity.");

  r = p_Engine->RegisterObjectBehaviour(
      "Entity", asBEHAVE_CONSTRUCT, "void f(const Entity& in)",
      asFUNCTION(LowCore_Entity_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose copy constructor of Low::Core::Entity.");

  r = p_Engine->RegisterObjectMethod(
      "Entity", "Entity& opAssign(const Entity& in)",
      asFUNCTION(LowCore_Entity_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose assignment operator of Low::Core::Entity.");

  r = p_Engine->RegisterObjectBehaviour(
      "Entity", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Entity_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0,
             "Failed to expose destructor of Low::Core::Entity.");
  r = p_Engine->RegisterObjectMethod(
      "Entity", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::Entity, is_alive, () const, bool),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose is_alive getter for Low::Core::Entity.");
  r = p_Engine->RegisterObjectMethod(
      "Entity", "void destroy()",
      asMETHODPR(Low::Core::Entity, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose destroy for Low::Core::Entity.");

  r = p_Engine->RegisterObjectMethod(
      "Entity", "Region get_region() const property",
      asMETHOD(Low::Core::Entity, get_region), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for region of "
                     "Low::Core::Entity.");
  r = p_Engine->RegisterObjectMethod(
      "Entity", "void set_region(Region) property",
      asMETHOD(Low::Core::Entity, set_region), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for region of "
                     "Low::Core::Entity.");

  r = p_Engine->RegisterObjectMethod(
      "Entity", "Name get_name() const property",
      asMETHOD(Low::Core::Entity, get_name), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for name of "
                     "Low::Core::Entity.");
  r = p_Engine->RegisterObjectMethod(
      "Entity", "void set_name(Name) property",
      asMETHOD(Low::Core::Entity, set_name), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for name of "
                     "Low::Core::Entity.");
  r = p_Engine->RegisterObjectMethod(
      "Entity", "u64 get_component(u16) ",
      asFUNCTION(LowCore_Entity_func_get_component),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function get_component of "
                     "Low::Core::Entity.");
  r = p_Engine->RegisterObjectMethod(
      "Entity", "void add_component(Handle) ",
      asFUNCTION(LowCore_Entity_func_add_component),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function add_component of "
                     "Low::Core::Entity.");
  r = p_Engine->RegisterObjectMethod(
      "Entity", "void remove_component(u16) ",
      asFUNCTION(LowCore_Entity_func_remove_component),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function remove_component of "
                     "Low::Core::Entity.");
  r = p_Engine->RegisterObjectMethod(
      "Entity", "bool has_component(u16) ",
      asFUNCTION(LowCore_Entity_func_has_component),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function has_component of "
                     "Low::Core::Entity.");
  r = p_Engine->RegisterObjectMethod(
      "Entity", "Transform get_transform() ",
      asFUNCTION(LowCore_Entity_func_get_transform),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function get_transform of "
                     "Low::Core::Entity.");

  r = p_Engine->SetDefaultNamespace("Entity");
  LOW_ASSERT(r >= 0,
             "Failed to set namespace for Low::Core::Entity.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property",
      asFUNCTION(LowCore_Entity_type_id), asCALL_CDECL);
  LOW_ASSERT(r >= 0,
             "Failed to expose TYPE_ID for Low::Core::Entity.");
  r = p_Engine->RegisterGlobalFunction(
      "Entity make(Name)", asFUNCTION(LowCore_Entity_genmake),
      asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic make function for "
                     "Low::Core::Entity.");
  r = p_Engine->RegisterGlobalFunction(
      "Entity find_by_name(Name)",
      asFUNCTION(LowCore_Entity_genfindbyname), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic find by name function "
                     "for Low::Core::Entity.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:ENTITY:EXPOSE

  // LOW_CODEGEN::END::CUSTOM:LOWCORE:ENTITY:EXPOSE
}

// --------------------------
static void LowCore_Transform_default_construct(
    Low::Core::Component::Transform *p_Memory)
{
  new (p_Memory) Low::Core::Component::Transform;
}
static void LowCore_Transform_id_construct(
    u64 p_Id, Low::Core::Component::Transform *p_Memory)
{
  new (p_Memory) Low::Core::Component::Transform(p_Id);
}
static void LowCore_Transform_copy_construct(
    const Low::Core::Component::Transform &p_Other,
    Low::Core::Component::Transform *p_Memory)
{
  new (p_Memory) Low::Core::Component::Transform(p_Other);
}
static Low::Core::Component::Transform &LowCore_Transform_assign(
    const Low::Core::Component::Transform &p_Other,
    Low::Core::Component::Transform *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void
LowCore_Transform_destruct(Low::Core::Component::Transform *p_Memory)
{
  using namespace Low::Core::Component;
  p_Memory->~Transform();
}
static u32 LowCore_Transform_living_count()
{
  return Low::Core::Component::Transform::living_count();
}
static u16 LowCore_Transform_type_id()
{
  return Low::Core::Component::Transform::type_id();
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:TRANSFORM:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:TRANSFORM:HELPERS

static void register_LowCore_Transform(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Transform", sizeof(Low::Core::Component::Transform),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose Low::Core::Component::Transform type.");
}
static void expose_LowCore_Transform(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "Transform", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Transform_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectBehaviour(
      "Transform", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_Transform_id_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose id constructor of "
                     "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectBehaviour(
      "Transform", asBEHAVE_CONSTRUCT, "void f(const Transform& in)",
      asFUNCTION(LowCore_Transform_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectMethod(
      "Transform", "Transform& opAssign(const Transform& in)",
      asFUNCTION(LowCore_Transform_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectBehaviour(
      "Transform", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Transform_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose destructor of "
                     "Low::Core::Component::Transform.");
  r = p_Engine->RegisterObjectMethod(
      "Transform", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::Component::Transform, is_alive, () const,
                 bool),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for "
                     "Low::Core::Component::Transform.");
  r = p_Engine->RegisterObjectMethod(
      "Transform", "void destroy()",
      asMETHODPR(Low::Core::Component::Transform, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose destroy for "
                     "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectMethod(
      "Transform", "Vector3 get_position() const property",
      asMETHODPR(Low::Core::Component::Transform, position, () const,
                 Low::Math::Vector3),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for position "
                     "of Low::Core::Component::Transform.");
  r = p_Engine->RegisterObjectMethod(
      "Transform", "void set_position(Vector3) property",
      asMETHODPR(Low::Core::Component::Transform, position,
                 (Low::Math::Vector3), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for position "
                     "of Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectMethod(
      "Transform", "Quaternion get_rotation() const property",
      asMETHODPR(Low::Core::Component::Transform, rotation, () const,
                 Low::Math::Quaternion),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for rotation "
                     "of Low::Core::Component::Transform.");
  r = p_Engine->RegisterObjectMethod(
      "Transform", "void set_rotation(Quaternion) property",
      asMETHODPR(Low::Core::Component::Transform, rotation,
                 (Low::Math::Quaternion), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for rotation "
                     "of Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectMethod(
      "Transform", "Vector3 get_scale() const property",
      asMETHODPR(Low::Core::Component::Transform, scale, () const,
                 Low::Math::Vector3),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for scale of "
                     "Low::Core::Component::Transform.");
  r = p_Engine->RegisterObjectMethod(
      "Transform", "void set_scale(Vector3) property",
      asMETHODPR(Low::Core::Component::Transform, scale,
                 (Low::Math::Vector3), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for scale of "
                     "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectMethod(
      "Transform", "u64 get_parent() const property",
      asMETHOD(Low::Core::Component::Transform, get_parent),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for parent of "
                     "Low::Core::Component::Transform.");
  r = p_Engine->RegisterObjectMethod(
      "Transform", "void set_parent(u64) property",
      asMETHOD(Low::Core::Component::Transform, set_parent),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for parent of "
                     "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectMethod(
      "Transform", "Vector3 get_world_position() const property",
      asMETHOD(Low::Core::Component::Transform, get_world_position),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose property getter for world_position of "
             "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectMethod(
      "Transform", "Quaternion get_world_rotation() const property",
      asMETHOD(Low::Core::Component::Transform, get_world_rotation),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose property getter for world_rotation of "
             "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectMethod(
      "Transform", "Vector3 get_world_scale() const property",
      asMETHOD(Low::Core::Component::Transform, get_world_scale),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose property getter for world_scale of "
             "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectMethod(
      "Transform", "Entity get_entity() const property",
      asMETHOD(Low::Core::Component::Transform, get_entity),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for entity of "
                     "Low::Core::Component::Transform.");
  r = p_Engine->RegisterObjectMethod(
      "Transform", "void set_entity(Entity) property",
      asMETHOD(Low::Core::Component::Transform, set_entity),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for entity of "
                     "Low::Core::Component::Transform.");

  r = p_Engine->SetDefaultNamespace("Transform");
  LOW_ASSERT(
      r >= 0,
      "Failed to set namespace for Low::Core::Component::Transform.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property",
      asFUNCTION(LowCore_Transform_type_id), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose TYPE_ID for "
                     "Low::Core::Component::Transform.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:TRANSFORM:EXPOSE

  // LOW_CODEGEN::END::CUSTOM:LOWCORE:TRANSFORM:EXPOSE
}

// --------------------------
static void LowCore_Animator_default_construct(
    Low::Core::Component::Animator *p_Memory)
{
  new (p_Memory) Low::Core::Component::Animator;
}
static void LowCore_Animator_id_construct(
    u64 p_Id, Low::Core::Component::Animator *p_Memory)
{
  new (p_Memory) Low::Core::Component::Animator(p_Id);
}
static void LowCore_Animator_copy_construct(
    const Low::Core::Component::Animator &p_Other,
    Low::Core::Component::Animator *p_Memory)
{
  new (p_Memory) Low::Core::Component::Animator(p_Other);
}
static Low::Core::Component::Animator &
LowCore_Animator_assign(const Low::Core::Component::Animator &p_Other,
                        Low::Core::Component::Animator *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void
LowCore_Animator_destruct(Low::Core::Component::Animator *p_Memory)
{
  using namespace Low::Core::Component;
  p_Memory->~Animator();
}
static u32 LowCore_Animator_living_count()
{
  return Low::Core::Component::Animator::living_count();
}
static u16 LowCore_Animator_type_id()
{
  return Low::Core::Component::Animator::type_id();
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:ANIMATOR:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:ANIMATOR:HELPERS

static void register_LowCore_Animator(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Animator", sizeof(Low::Core::Component::Animator),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::Component::Animator type.");
}
static void expose_LowCore_Animator(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "Animator", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Animator_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::Component::Animator.");

  r = p_Engine->RegisterObjectBehaviour(
      "Animator", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_Animator_id_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose id constructor of "
                     "Low::Core::Component::Animator.");

  r = p_Engine->RegisterObjectBehaviour(
      "Animator", asBEHAVE_CONSTRUCT, "void f(const Animator& in)",
      asFUNCTION(LowCore_Animator_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::Component::Animator.");

  r = p_Engine->RegisterObjectMethod(
      "Animator", "Animator& opAssign(const Animator& in)",
      asFUNCTION(LowCore_Animator_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Core::Component::Animator.");

  r = p_Engine->RegisterObjectBehaviour(
      "Animator", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Animator_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose destructor of "
                     "Low::Core::Component::Animator.");
  r = p_Engine->RegisterObjectMethod(
      "Animator", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::Component::Animator, is_alive, () const,
                 bool),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for "
                     "Low::Core::Component::Animator.");
  r = p_Engine->RegisterObjectMethod(
      "Animator", "void destroy()",
      asMETHODPR(Low::Core::Component::Animator, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destroy for Low::Core::Component::Animator.");

  r = p_Engine->RegisterObjectMethod(
      "Animator", "AnimationClip get_active_clip() const property",
      asMETHOD(Low::Core::Component::Animator, get_active_clip),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose property getter for active_clip of "
             "Low::Core::Component::Animator.");
  r = p_Engine->RegisterObjectMethod(
      "Animator", "void set_active_clip(AnimationClip) property",
      asMETHOD(Low::Core::Component::Animator, set_active_clip),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose property setter for active_clip of "
             "Low::Core::Component::Animator.");

  r = p_Engine->RegisterObjectMethod(
      "Animator", "Entity get_entity() const property",
      asMETHOD(Low::Core::Component::Animator, get_entity),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for entity of "
                     "Low::Core::Component::Animator.");
  r = p_Engine->RegisterObjectMethod(
      "Animator", "void set_entity(Entity) property",
      asMETHOD(Low::Core::Component::Animator, set_entity),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for entity of "
                     "Low::Core::Component::Animator.");

  r = p_Engine->SetDefaultNamespace("Animator");
  LOW_ASSERT(
      r >= 0,
      "Failed to set namespace for Low::Core::Component::Animator.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property",
      asFUNCTION(LowCore_Animator_type_id), asCALL_CDECL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose TYPE_ID for Low::Core::Component::Animator.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:ANIMATOR:EXPOSE
  r = p_Engine->RegisterObjectMethod(
      "Animator", "Skeleton get_skeleton() const property",
      asMETHOD(Low::Core::Component::Animator, get_skeleton),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for skeleton of "
                     "Low::Core::Component::Animator.");
  // LOW_CODEGEN::END::CUSTOM:LOWCORE:ANIMATOR:EXPOSE
}

// --------------------------
static void LowCore_Camera_default_construct(
    Low::Core::Component::Camera *p_Memory)
{
  new (p_Memory) Low::Core::Component::Camera;
}
static void
LowCore_Camera_id_construct(u64 p_Id,
                            Low::Core::Component::Camera *p_Memory)
{
  new (p_Memory) Low::Core::Component::Camera(p_Id);
}
static void LowCore_Camera_copy_construct(
    const Low::Core::Component::Camera &p_Other,
    Low::Core::Component::Camera *p_Memory)
{
  new (p_Memory) Low::Core::Component::Camera(p_Other);
}
static Low::Core::Component::Camera &
LowCore_Camera_assign(const Low::Core::Component::Camera &p_Other,
                      Low::Core::Component::Camera *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void
LowCore_Camera_destruct(Low::Core::Component::Camera *p_Memory)
{
  using namespace Low::Core::Component;
  p_Memory->~Camera();
}
static u32 LowCore_Camera_living_count()
{
  return Low::Core::Component::Camera::living_count();
}
static u16 LowCore_Camera_type_id()
{
  return Low::Core::Component::Camera::type_id();
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:CAMERA:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:CAMERA:HELPERS

static void register_LowCore_Camera(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Camera", sizeof(Low::Core::Component::Camera),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::Component::Camera type.");
}
static void expose_LowCore_Camera(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "Camera", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Camera_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::Component::Camera.");

  r = p_Engine->RegisterObjectBehaviour(
      "Camera", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_Camera_id_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose id constructor of "
                     "Low::Core::Component::Camera.");

  r = p_Engine->RegisterObjectBehaviour(
      "Camera", asBEHAVE_CONSTRUCT, "void f(const Camera& in)",
      asFUNCTION(LowCore_Camera_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::Component::Camera.");

  r = p_Engine->RegisterObjectMethod(
      "Camera", "Camera& opAssign(const Camera& in)",
      asFUNCTION(LowCore_Camera_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Core::Component::Camera.");

  r = p_Engine->RegisterObjectBehaviour(
      "Camera", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Camera_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destructor of Low::Core::Component::Camera.");
  r = p_Engine->RegisterObjectMethod(
      "Camera", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::Component::Camera, is_alive, () const,
                 bool),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for "
                     "Low::Core::Component::Camera.");
  r = p_Engine->RegisterObjectMethod(
      "Camera", "void destroy()",
      asMETHODPR(Low::Core::Component::Camera, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destroy for Low::Core::Component::Camera.");

  r = p_Engine->RegisterObjectMethod(
      "Camera", "bool get_active() const property",
      asMETHOD(Low::Core::Component::Camera, is_active),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for active of "
                     "Low::Core::Component::Camera.");

  r = p_Engine->RegisterObjectMethod(
      "Camera", "float get_fov() const property",
      asMETHOD(Low::Core::Component::Camera, get_fov),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for fov of "
                     "Low::Core::Component::Camera.");
  r = p_Engine->RegisterObjectMethod(
      "Camera", "void set_fov(float) property",
      asMETHOD(Low::Core::Component::Camera, set_fov),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for fov of "
                     "Low::Core::Component::Camera.");

  r = p_Engine->RegisterObjectMethod(
      "Camera", "Entity get_entity() const property",
      asMETHOD(Low::Core::Component::Camera, get_entity),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for entity of "
                     "Low::Core::Component::Camera.");
  r = p_Engine->RegisterObjectMethod(
      "Camera", "void set_entity(Entity) property",
      asMETHOD(Low::Core::Component::Camera, set_entity),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for entity of "
                     "Low::Core::Component::Camera.");

  r = p_Engine->SetDefaultNamespace("Camera");
  LOW_ASSERT(
      r >= 0,
      "Failed to set namespace for Low::Core::Component::Camera.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property",
      asFUNCTION(LowCore_Camera_type_id), asCALL_CDECL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose TYPE_ID for Low::Core::Component::Camera.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:CAMERA:EXPOSE

  // LOW_CODEGEN::END::CUSTOM:LOWCORE:CAMERA:EXPOSE
}

// --------------------------
static void LowCore_Module_default_construct(
    Low::Core::Scripting::Module *p_Memory)
{
  new (p_Memory) Low::Core::Scripting::Module;
}
static void
LowCore_Module_id_construct(u64 p_Id,
                            Low::Core::Scripting::Module *p_Memory)
{
  new (p_Memory) Low::Core::Scripting::Module(p_Id);
}
static void LowCore_Module_copy_construct(
    const Low::Core::Scripting::Module &p_Other,
    Low::Core::Scripting::Module *p_Memory)
{
  new (p_Memory) Low::Core::Scripting::Module(p_Other);
}
static Low::Core::Scripting::Module &
LowCore_Module_assign(const Low::Core::Scripting::Module &p_Other,
                      Low::Core::Scripting::Module *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void
LowCore_Module_destruct(Low::Core::Scripting::Module *p_Memory)
{
  using namespace Low::Core::Scripting;
  p_Memory->~Module();
}
static Low::Core::Scripting::Module
LowCore_Module_genmake(Low::Util::Name p_Name)
{
  return Low::Core::Scripting::Module::make(p_Name);
}
static Low::Core::Scripting::Module
LowCore_Module_genfindbyname(Low::Util::Name p_Name)
{
  return Low::Core::Scripting::Module::find_by_name(p_Name);
}
static u32 LowCore_Module_living_count()
{
  return Low::Core::Scripting::Module::living_count();
}
static u16 LowCore_Module_type_id()
{
  return Low::Core::Scripting::Module::type_id();
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:MODULE:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:MODULE:HELPERS

static void register_LowCore_Module(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Module", sizeof(Low::Core::Scripting::Module),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::Scripting::Module type.");
}
static void expose_LowCore_Module(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "Module", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Module_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::Scripting::Module.");

  r = p_Engine->RegisterObjectBehaviour(
      "Module", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_Module_id_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose id constructor of "
                     "Low::Core::Scripting::Module.");

  r = p_Engine->RegisterObjectBehaviour(
      "Module", asBEHAVE_CONSTRUCT, "void f(const Module& in)",
      asFUNCTION(LowCore_Module_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::Scripting::Module.");

  r = p_Engine->RegisterObjectMethod(
      "Module", "Module& opAssign(const Module& in)",
      asFUNCTION(LowCore_Module_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Core::Scripting::Module.");

  r = p_Engine->RegisterObjectBehaviour(
      "Module", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Module_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destructor of Low::Core::Scripting::Module.");
  r = p_Engine->RegisterObjectMethod(
      "Module", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::Scripting::Module, is_alive, () const,
                 bool),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for "
                     "Low::Core::Scripting::Module.");
  r = p_Engine->RegisterObjectMethod(
      "Module", "void destroy()",
      asMETHODPR(Low::Core::Scripting::Module, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destroy for Low::Core::Scripting::Module.");

  r = p_Engine->RegisterObjectMethod(
      "Module", "Name get_name() const property",
      asMETHOD(Low::Core::Scripting::Module, get_name),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for name of "
                     "Low::Core::Scripting::Module.");
  r = p_Engine->RegisterObjectMethod(
      "Module", "void set_name(Name) property",
      asMETHOD(Low::Core::Scripting::Module, set_name),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for name of "
                     "Low::Core::Scripting::Module.");

  r = p_Engine->SetDefaultNamespace("Module");
  LOW_ASSERT(
      r >= 0,
      "Failed to set namespace for Low::Core::Scripting::Module.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property",
      asFUNCTION(LowCore_Module_type_id), asCALL_CDECL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose TYPE_ID for Low::Core::Scripting::Module.");
  r = p_Engine->RegisterGlobalFunction(
      "Module make(Name)", asFUNCTION(LowCore_Module_genmake),
      asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic make function for "
                     "Low::Core::Scripting::Module.");
  r = p_Engine->RegisterGlobalFunction(
      "Module find_by_name(Name)",
      asFUNCTION(LowCore_Module_genfindbyname), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic find by name function "
                     "for Low::Core::Scripting::Module.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:MODULE:EXPOSE

  // LOW_CODEGEN::END::CUSTOM:LOWCORE:MODULE:EXPOSE
}

// --------------------------
static void
LowCore_Asset_default_construct(Low::Core::Scripting::Asset *p_Memory)
{
  new (p_Memory) Low::Core::Scripting::Asset;
}
static void
LowCore_Asset_id_construct(u64 p_Id,
                           Low::Core::Scripting::Asset *p_Memory)
{
  new (p_Memory) Low::Core::Scripting::Asset(p_Id);
}
static void LowCore_Asset_copy_construct(
    const Low::Core::Scripting::Asset &p_Other,
    Low::Core::Scripting::Asset *p_Memory)
{
  new (p_Memory) Low::Core::Scripting::Asset(p_Other);
}
static Low::Core::Scripting::Asset &
LowCore_Asset_assign(const Low::Core::Scripting::Asset &p_Other,
                     Low::Core::Scripting::Asset *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void
LowCore_Asset_destruct(Low::Core::Scripting::Asset *p_Memory)
{
  using namespace Low::Core::Scripting;
  p_Memory->~Asset();
}
static Low::Core::Scripting::Asset
LowCore_Asset_genmake(Low::Util::Name p_Name)
{
  return Low::Core::Scripting::Asset::make(p_Name);
}
static Low::Core::Scripting::Asset
LowCore_Asset_genfindbyname(Low::Util::Name p_Name)
{
  return Low::Core::Scripting::Asset::find_by_name(p_Name);
}
static u32 LowCore_Asset_living_count()
{
  return Low::Core::Scripting::Asset::living_count();
}
static u16 LowCore_Asset_type_id()
{
  return Low::Core::Scripting::Asset::type_id();
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:ASSET:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:ASSET:HELPERS

static void register_LowCore_Asset(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Asset", sizeof(Low::Core::Scripting::Asset),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::Scripting::Asset type.");
}
static void expose_LowCore_Asset(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "Asset", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Asset_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::Scripting::Asset.");

  r = p_Engine->RegisterObjectBehaviour(
      "Asset", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_Asset_id_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose id constructor of "
                     "Low::Core::Scripting::Asset.");

  r = p_Engine->RegisterObjectBehaviour(
      "Asset", asBEHAVE_CONSTRUCT, "void f(const Asset& in)",
      asFUNCTION(LowCore_Asset_copy_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::Scripting::Asset.");

  r = p_Engine->RegisterObjectMethod(
      "Asset", "Asset& opAssign(const Asset& in)",
      asFUNCTION(LowCore_Asset_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Core::Scripting::Asset.");

  r = p_Engine->RegisterObjectBehaviour(
      "Asset", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Asset_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destructor of Low::Core::Scripting::Asset.");
  r = p_Engine->RegisterObjectMethod(
      "Asset", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::Scripting::Asset, is_alive, () const,
                 bool),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for "
                     "Low::Core::Scripting::Asset.");
  r = p_Engine->RegisterObjectMethod(
      "Asset", "void destroy()",
      asMETHODPR(Low::Core::Scripting::Asset, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destroy for Low::Core::Scripting::Asset.");

  r = p_Engine->RegisterObjectMethod(
      "Asset", "Name get_name() const property",
      asMETHOD(Low::Core::Scripting::Asset, get_name),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for name of "
                     "Low::Core::Scripting::Asset.");
  r = p_Engine->RegisterObjectMethod(
      "Asset", "void set_name(Name) property",
      asMETHOD(Low::Core::Scripting::Asset, set_name),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for name of "
                     "Low::Core::Scripting::Asset.");

  r = p_Engine->SetDefaultNamespace("Asset");
  LOW_ASSERT(
      r >= 0,
      "Failed to set namespace for Low::Core::Scripting::Asset.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property", asFUNCTION(LowCore_Asset_type_id),
      asCALL_CDECL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose TYPE_ID for Low::Core::Scripting::Asset.");
  r = p_Engine->RegisterGlobalFunction(
      "Asset make(Name)", asFUNCTION(LowCore_Asset_genmake),
      asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic make function for "
                     "Low::Core::Scripting::Asset.");
  r = p_Engine->RegisterGlobalFunction(
      "Asset find_by_name(Name)",
      asFUNCTION(LowCore_Asset_genfindbyname), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic find by name function "
                     "for Low::Core::Scripting::Asset.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:ASSET:EXPOSE

  // LOW_CODEGEN::END::CUSTOM:LOWCORE:ASSET:EXPOSE
}

// --------------------------
static void LowCore_WidgetAsset_default_construct(
    Low::Core::UI::WidgetAsset *p_Memory)
{
  new (p_Memory) Low::Core::UI::WidgetAsset;
}
static void
LowCore_WidgetAsset_id_construct(u64 p_Id,
                                 Low::Core::UI::WidgetAsset *p_Memory)
{
  new (p_Memory) Low::Core::UI::WidgetAsset(p_Id);
}
static void LowCore_WidgetAsset_copy_construct(
    const Low::Core::UI::WidgetAsset &p_Other,
    Low::Core::UI::WidgetAsset *p_Memory)
{
  new (p_Memory) Low::Core::UI::WidgetAsset(p_Other);
}
static Low::Core::UI::WidgetAsset &
LowCore_WidgetAsset_assign(const Low::Core::UI::WidgetAsset &p_Other,
                           Low::Core::UI::WidgetAsset *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void
LowCore_WidgetAsset_destruct(Low::Core::UI::WidgetAsset *p_Memory)
{
  using namespace Low::Core::UI;
  p_Memory->~WidgetAsset();
}
static Low::Core::UI::WidgetAsset
LowCore_WidgetAsset_genmake(Low::Util::Name p_Name)
{
  return Low::Core::UI::WidgetAsset::make(p_Name);
}
static Low::Core::UI::WidgetAsset
LowCore_WidgetAsset_genfindbyname(Low::Util::Name p_Name)
{
  return Low::Core::UI::WidgetAsset::find_by_name(p_Name);
}
static u32 LowCore_WidgetAsset_living_count()
{
  return Low::Core::UI::WidgetAsset::living_count();
}
static u16 LowCore_WidgetAsset_type_id()
{
  return Low::Core::UI::WidgetAsset::type_id();
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:WIDGETASSET:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:WIDGETASSET:HELPERS

static void register_LowCore_WidgetAsset(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "WidgetAsset", sizeof(Low::Core::UI::WidgetAsset),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::UI::WidgetAsset type.");
}
static void expose_LowCore_WidgetAsset(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "WidgetAsset", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_WidgetAsset_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::UI::WidgetAsset.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetAsset", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_WidgetAsset_id_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose id constructor of "
                     "Low::Core::UI::WidgetAsset.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetAsset", asBEHAVE_CONSTRUCT,
      "void f(const WidgetAsset& in)",
      asFUNCTION(LowCore_WidgetAsset_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::UI::WidgetAsset.");

  r = p_Engine->RegisterObjectMethod(
      "WidgetAsset", "WidgetAsset& opAssign(const WidgetAsset& in)",
      asFUNCTION(LowCore_WidgetAsset_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Core::UI::WidgetAsset.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetAsset", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_WidgetAsset_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destructor of Low::Core::UI::WidgetAsset.");
  r = p_Engine->RegisterObjectMethod(
      "WidgetAsset", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::UI::WidgetAsset, is_alive, () const,
                 bool),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for "
                     "Low::Core::UI::WidgetAsset.");
  r = p_Engine->RegisterObjectMethod(
      "WidgetAsset", "void destroy()",
      asMETHODPR(Low::Core::UI::WidgetAsset, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destroy for Low::Core::UI::WidgetAsset.");

  r = p_Engine->RegisterObjectMethod(
      "WidgetAsset", "Name get_name() const property",
      asMETHOD(Low::Core::UI::WidgetAsset, get_name),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for name of "
                     "Low::Core::UI::WidgetAsset.");
  r = p_Engine->RegisterObjectMethod(
      "WidgetAsset", "void set_name(Name) property",
      asMETHOD(Low::Core::UI::WidgetAsset, set_name),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for name of "
                     "Low::Core::UI::WidgetAsset.");

  r = p_Engine->SetDefaultNamespace("WidgetAsset");
  LOW_ASSERT(
      r >= 0,
      "Failed to set namespace for Low::Core::UI::WidgetAsset.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property",
      asFUNCTION(LowCore_WidgetAsset_type_id), asCALL_CDECL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose TYPE_ID for Low::Core::UI::WidgetAsset.");
  r = p_Engine->RegisterGlobalFunction(
      "WidgetAsset make(Name)",
      asFUNCTION(LowCore_WidgetAsset_genmake), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic make function for "
                     "Low::Core::UI::WidgetAsset.");
  r = p_Engine->RegisterGlobalFunction(
      "WidgetAsset find_by_name(Name)",
      asFUNCTION(LowCore_WidgetAsset_genfindbyname), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic find by name function "
                     "for Low::Core::UI::WidgetAsset.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:WIDGETASSET:EXPOSE

  // LOW_CODEGEN::END::CUSTOM:LOWCORE:WIDGETASSET:EXPOSE
}

// --------------------------
static void LowCore_WidgetInstance_default_construct(
    Low::Core::UI::WidgetInstance *p_Memory)
{
  new (p_Memory) Low::Core::UI::WidgetInstance;
}
static void LowCore_WidgetInstance_id_construct(
    u64 p_Id, Low::Core::UI::WidgetInstance *p_Memory)
{
  new (p_Memory) Low::Core::UI::WidgetInstance(p_Id);
}
static void LowCore_WidgetInstance_copy_construct(
    const Low::Core::UI::WidgetInstance &p_Other,
    Low::Core::UI::WidgetInstance *p_Memory)
{
  new (p_Memory) Low::Core::UI::WidgetInstance(p_Other);
}
static Low::Core::UI::WidgetInstance &LowCore_WidgetInstance_assign(
    const Low::Core::UI::WidgetInstance &p_Other,
    Low::Core::UI::WidgetInstance *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void LowCore_WidgetInstance_destruct(
    Low::Core::UI::WidgetInstance *p_Memory)
{
  using namespace Low::Core::UI;
  p_Memory->~WidgetInstance();
}
static Low::Core::UI::WidgetInstance
LowCore_WidgetInstance_genmake(Low::Util::Name p_Name)
{
  return Low::Core::UI::WidgetInstance::make(p_Name);
}
static Low::Core::UI::WidgetInstance
LowCore_WidgetInstance_genfindbyname(Low::Util::Name p_Name)
{
  return Low::Core::UI::WidgetInstance::find_by_name(p_Name);
}
static u32 LowCore_WidgetInstance_living_count()
{
  return Low::Core::UI::WidgetInstance::living_count();
}
static u16 LowCore_WidgetInstance_type_id()
{
  return Low::Core::UI::WidgetInstance::type_id();
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:WIDGETINSTANCE:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:WIDGETINSTANCE:HELPERS

static void register_LowCore_WidgetInstance(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "WidgetInstance", sizeof(Low::Core::UI::WidgetInstance),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::UI::WidgetInstance type.");
}
static void expose_LowCore_WidgetInstance(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "WidgetInstance", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_WidgetInstance_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::UI::WidgetInstance.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetInstance", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_WidgetInstance_id_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose id constructor of "
                     "Low::Core::UI::WidgetInstance.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetInstance", asBEHAVE_CONSTRUCT,
      "void f(const WidgetInstance& in)",
      asFUNCTION(LowCore_WidgetInstance_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::UI::WidgetInstance.");

  r = p_Engine->RegisterObjectMethod(
      "WidgetInstance",
      "WidgetInstance& opAssign(const WidgetInstance& in)",
      asFUNCTION(LowCore_WidgetInstance_assign),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Core::UI::WidgetInstance.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetInstance", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_WidgetInstance_destruct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose destructor of "
                     "Low::Core::UI::WidgetInstance.");
  r = p_Engine->RegisterObjectMethod(
      "WidgetInstance", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::UI::WidgetInstance, is_alive, () const,
                 bool),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for "
                     "Low::Core::UI::WidgetInstance.");
  r = p_Engine->RegisterObjectMethod(
      "WidgetInstance", "void destroy()",
      asMETHODPR(Low::Core::UI::WidgetInstance, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destroy for Low::Core::UI::WidgetInstance.");

  r = p_Engine->RegisterObjectMethod(
      "WidgetInstance", "Name get_name() const property",
      asMETHOD(Low::Core::UI::WidgetInstance, get_name),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for name of "
                     "Low::Core::UI::WidgetInstance.");
  r = p_Engine->RegisterObjectMethod(
      "WidgetInstance", "void set_name(Name) property",
      asMETHOD(Low::Core::UI::WidgetInstance, set_name),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for name of "
                     "Low::Core::UI::WidgetInstance.");

  r = p_Engine->SetDefaultNamespace("WidgetInstance");
  LOW_ASSERT(
      r >= 0,
      "Failed to set namespace for Low::Core::UI::WidgetInstance.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property",
      asFUNCTION(LowCore_WidgetInstance_type_id), asCALL_CDECL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose TYPE_ID for Low::Core::UI::WidgetInstance.");
  r = p_Engine->RegisterGlobalFunction(
      "WidgetInstance make(Name)",
      asFUNCTION(LowCore_WidgetInstance_genmake), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic make function for "
                     "Low::Core::UI::WidgetInstance.");
  r = p_Engine->RegisterGlobalFunction(
      "WidgetInstance find_by_name(Name)",
      asFUNCTION(LowCore_WidgetInstance_genfindbyname), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic find by name function "
                     "for Low::Core::UI::WidgetInstance.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:WIDGETINSTANCE:EXPOSE

  // LOW_CODEGEN::END::CUSTOM:LOWCORE:WIDGETINSTANCE:EXPOSE
}

// --------------------------
static void
LowCore_View_default_construct(Low::Core::UI::View *p_Memory)
{
  new (p_Memory) Low::Core::UI::View;
}
static void LowCore_View_id_construct(u64 p_Id,
                                      Low::Core::UI::View *p_Memory)
{
  new (p_Memory) Low::Core::UI::View(p_Id);
}
static void
LowCore_View_copy_construct(const Low::Core::UI::View &p_Other,
                            Low::Core::UI::View *p_Memory)
{
  new (p_Memory) Low::Core::UI::View(p_Other);
}
static Low::Core::UI::View &
LowCore_View_assign(const Low::Core::UI::View &p_Other,
                    Low::Core::UI::View *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void LowCore_View_destruct(Low::Core::UI::View *p_Memory)
{
  using namespace Low::Core::UI;
  p_Memory->~View();
}
static Low::Core::UI::View
LowCore_View_genmake(Low::Util::Name p_Name)
{
  return Low::Core::UI::View::make(p_Name);
}
static Low::Core::UI::View
LowCore_View_genfindbyname(Low::Util::Name p_Name)
{
  return Low::Core::UI::View::find_by_name(p_Name);
}
static u32 LowCore_View_living_count()
{
  return Low::Core::UI::View::living_count();
}
static u16 LowCore_View_type_id()
{
  return Low::Core::UI::View::type_id();
}
static Low::Core::UI::View
LowCore_View_func_spawn_instance(Low::Core::UI::View p_This,
                                 Low::Util::Name p_Name)
{
  return p_This.spawn_instance(p_Name);
}
static Low::Core::UI::Element
LowCore_View_func_find_element_by_name(Low::Core::UI::View p_This,
                                       Low::Util::Name p_Name)
{
  return p_This.find_element_by_name(p_Name);
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:VIEW:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:VIEW:HELPERS

static void register_LowCore_View(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "View", sizeof(Low::Core::UI::View),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0, "Failed to expose Low::Core::UI::View type.");
}
static void expose_LowCore_View(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "View", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_View_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose default constructor of Low::Core::UI::View.");

  r = p_Engine->RegisterObjectBehaviour(
      "View", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_View_id_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose id constructor of Low::Core::UI::View.");

  r = p_Engine->RegisterObjectBehaviour(
      "View", asBEHAVE_CONSTRUCT, "void f(const View& in)",
      asFUNCTION(LowCore_View_copy_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose copy constructor of Low::Core::UI::View.");

  r = p_Engine->RegisterObjectMethod(
      "View", "View& opAssign(const View& in)",
      asFUNCTION(LowCore_View_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose assignment operator of Low::Core::UI::View.");

  r = p_Engine->RegisterObjectBehaviour(
      "View", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_View_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0,
             "Failed to expose destructor of Low::Core::UI::View.");
  r = p_Engine->RegisterObjectMethod(
      "View", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::UI::View, is_alive, () const, bool),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose is_alive getter for Low::Core::UI::View.");
  r = p_Engine->RegisterObjectMethod(
      "View", "void destroy()",
      asMETHODPR(Low::Core::UI::View, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose destroy for Low::Core::UI::View.");

  r = p_Engine->RegisterObjectMethod(
      "View", "Vector2 get_pixel_position() const property",
      asMETHODPR(Low::Core::UI::View, pixel_position, () const,
                 Low::Math::Vector2),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for "
                     "pixel_position of Low::Core::UI::View.");
  r = p_Engine->RegisterObjectMethod(
      "View", "void set_pixel_position(Vector2) property",
      asMETHODPR(Low::Core::UI::View, pixel_position,
                 (Low::Math::Vector2), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for "
                     "pixel_position of Low::Core::UI::View.");

  r = p_Engine->RegisterObjectMethod(
      "View", "float get_rotation() const property",
      asMETHODPR(Low::Core::UI::View, rotation, () const, float),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for rotation "
                     "of Low::Core::UI::View.");
  r = p_Engine->RegisterObjectMethod(
      "View", "void set_rotation(float) property",
      asMETHODPR(Low::Core::UI::View, rotation, (float), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for rotation "
                     "of Low::Core::UI::View.");

  r = p_Engine->RegisterObjectMethod(
      "View", "float get_scale_multiplier() const property",
      asMETHODPR(Low::Core::UI::View, scale_multiplier, () const,
                 float),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for "
                     "scale_multiplier of Low::Core::UI::View.");
  r = p_Engine->RegisterObjectMethod(
      "View", "void set_scale_multiplier(float) property",
      asMETHODPR(Low::Core::UI::View, scale_multiplier, (float),
                 void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for "
                     "scale_multiplier of Low::Core::UI::View.");

  r = p_Engine->RegisterObjectMethod(
      "View", "u32 get_layer_offset() const property",
      asMETHODPR(Low::Core::UI::View, layer_offset, () const,
                 uint32_t),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for "
                     "layer_offset of Low::Core::UI::View.");
  r = p_Engine->RegisterObjectMethod(
      "View", "void set_layer_offset(u32) property",
      asMETHODPR(Low::Core::UI::View, layer_offset, (uint32_t), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for "
                     "layer_offset of Low::Core::UI::View.");

  r = p_Engine->RegisterObjectMethod(
      "View", "Name get_name() const property",
      asMETHOD(Low::Core::UI::View, get_name), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for name of "
                     "Low::Core::UI::View.");
  r = p_Engine->RegisterObjectMethod(
      "View", "void set_name(Name) property",
      asMETHOD(Low::Core::UI::View, set_name), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for name of "
                     "Low::Core::UI::View.");
  r = p_Engine->RegisterObjectMethod(
      "View", "View spawn_instance(Name) ",
      asFUNCTION(LowCore_View_func_spawn_instance),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function spawn_instance of "
                     "Low::Core::UI::View.");
  r = p_Engine->RegisterObjectMethod(
      "View", "UI::Element find_element_by_name(Name) ",
      asFUNCTION(LowCore_View_func_find_element_by_name),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function find_element_by_name "
                     "of Low::Core::UI::View.");

  r = p_Engine->SetDefaultNamespace("View");
  LOW_ASSERT(r >= 0,
             "Failed to set namespace for Low::Core::UI::View.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property", asFUNCTION(LowCore_View_type_id),
      asCALL_CDECL);
  LOW_ASSERT(r >= 0,
             "Failed to expose TYPE_ID for Low::Core::UI::View.");
  r = p_Engine->RegisterGlobalFunction(
      "View make(Name)", asFUNCTION(LowCore_View_genmake),
      asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic make function for "
                     "Low::Core::UI::View.");
  r = p_Engine->RegisterGlobalFunction(
      "View find_by_name(Name)",
      asFUNCTION(LowCore_View_genfindbyname), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic find by name function "
                     "for Low::Core::UI::View.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:VIEW:EXPOSE

  // LOW_CODEGEN::END::CUSTOM:LOWCORE:VIEW:EXPOSE
}

// --------------------------
static void
LowCore_Element_default_construct(Low::Core::UI::Element *p_Memory)
{
  new (p_Memory) Low::Core::UI::Element;
}
static void
LowCore_Element_id_construct(u64 p_Id,
                             Low::Core::UI::Element *p_Memory)
{
  new (p_Memory) Low::Core::UI::Element(p_Id);
}
static void
LowCore_Element_copy_construct(const Low::Core::UI::Element &p_Other,
                               Low::Core::UI::Element *p_Memory)
{
  new (p_Memory) Low::Core::UI::Element(p_Other);
}
static Low::Core::UI::Element &
LowCore_Element_assign(const Low::Core::UI::Element &p_Other,
                       Low::Core::UI::Element *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void LowCore_Element_destruct(Low::Core::UI::Element *p_Memory)
{
  using namespace Low::Core::UI;
  p_Memory->~Element();
}
static Low::Core::UI::Element
LowCore_Element_genfindbyname(Low::Util::Name p_Name)
{
  return Low::Core::UI::Element::find_by_name(p_Name);
}
static u32 LowCore_Element_living_count()
{
  return Low::Core::UI::Element::living_count();
}
static u16 LowCore_Element_type_id()
{
  return Low::Core::UI::Element::type_id();
}
static uint64_t
LowCore_Element_func_get_component(Low::Core::UI::Element p_This,
                                   uint16_t p_TypeId)
{
  return p_This.get_component(p_TypeId);
}
static void
LowCore_Element_func_add_component(Low::Core::UI::Element p_This,
                                   Low::Util::Handle p_Component)
{
  p_This.add_component(p_Component);
}
static void
LowCore_Element_func_remove_component(Low::Core::UI::Element p_This,
                                      uint16_t p_ComponentType)
{
  p_This.remove_component(p_ComponentType);
}
static bool
LowCore_Element_func_has_component(Low::Core::UI::Element p_This,
                                   uint16_t p_ComponentType)
{
  return p_This.has_component(p_ComponentType);
}
static Low::Core::UI::Component::Display
LowCore_Element_func_get_display(Low::Core::UI::Element p_This)
{
  return p_This.get_display();
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:ELEMENT:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:ELEMENT:HELPERS

static void register_LowCore_Element(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->SetDefaultNamespace("UI");
  LOW_ASSERT(
      r >= 0,
      "Failed to set namespace for type Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectType(
      "Element", sizeof(Low::Core::UI::Element),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0, "Failed to expose Low::Core::UI::Element type.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(
      r >= 0,
      "Failed to reset namespace after Low::Core::UI::Element.");
}
static void expose_LowCore_Element(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->SetDefaultNamespace("UI");
  LOW_ASSERT(
      r >= 0,
      "Failed to set namespace for type Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectBehaviour(
      "Element", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Element_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::UI::Element.");

  r = p_Engine->RegisterObjectBehaviour(
      "Element", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_Element_id_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose id constructor of Low::Core::UI::Element.");

  r = p_Engine->RegisterObjectBehaviour(
      "Element", asBEHAVE_CONSTRUCT, "void f(const Element& in)",
      asFUNCTION(LowCore_Element_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose copy constructor of Low::Core::UI::Element.");

  r = p_Engine->RegisterObjectMethod(
      "Element", "Element& opAssign(const Element& in)",
      asFUNCTION(LowCore_Element_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Core::UI::Element.");

  r = p_Engine->RegisterObjectBehaviour(
      "Element", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Element_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destructor of Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::UI::Element, is_alive, () const, bool),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose is_alive getter for Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "void destroy()",
      asMETHODPR(Low::Core::UI::Element, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose destroy for Low::Core::UI::Element.");

  r = p_Engine->RegisterObjectMethod(
      "Element", "View get_view() const property",
      asMETHOD(Low::Core::UI::Element, get_view), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for view of "
                     "Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "void set_view(View) property",
      asMETHOD(Low::Core::UI::Element, set_view), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for view of "
                     "Low::Core::UI::Element.");

  r = p_Engine->RegisterObjectMethod(
      "Element", "bool get_click_passthrough() const property",
      asMETHOD(Low::Core::UI::Element, is_click_passthrough),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for "
                     "click_passthrough of Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "void set_click_passthrough(bool) property",
      asMETHOD(Low::Core::UI::Element, set_click_passthrough),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for "
                     "click_passthrough of Low::Core::UI::Element.");

  r = p_Engine->RegisterObjectMethod(
      "Element", "u64 get_local_id() const property",
      asMETHOD(Low::Core::UI::Element, get_local_id),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for local_id "
                     "of Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "void set_local_id(u64) property",
      asMETHOD(Low::Core::UI::Element, set_local_id),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for local_id "
                     "of Low::Core::UI::Element.");

  r = p_Engine->RegisterObjectMethod(
      "Element", "Name get_name() const property",
      asMETHOD(Low::Core::UI::Element, get_name), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for name of "
                     "Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "void set_name(Name) property",
      asMETHOD(Low::Core::UI::Element, set_name), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for name of "
                     "Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "u64 get_component(u16) ",
      asFUNCTION(LowCore_Element_func_get_component),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function get_component of "
                     "Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "void add_component(Handle) ",
      asFUNCTION(LowCore_Element_func_add_component),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function add_component of "
                     "Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "void remove_component(u16) ",
      asFUNCTION(LowCore_Element_func_remove_component),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function remove_component of "
                     "Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "bool has_component(u16) ",
      asFUNCTION(LowCore_Element_func_has_component),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function has_component of "
                     "Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "Display get_display() ",
      asFUNCTION(LowCore_Element_func_get_display),
      asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose function get_display of "
                     "Low::Core::UI::Element.");

  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(
      r >= 0,
      "Failed to reset namespace after Low::Core::UI::Element.");

  r = p_Engine->SetDefaultNamespace("UI::Element");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property",
      asFUNCTION(LowCore_Element_type_id), asCALL_CDECL);
  LOW_ASSERT(r >= 0,
             "Failed to expose TYPE_ID for Low::Core::UI::Element.");
  r = p_Engine->RegisterGlobalFunction(
      "Element find_by_name(Name)",
      asFUNCTION(LowCore_Element_genfindbyname), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic find by name function "
                     "for Low::Core::UI::Element.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:ELEMENT:EXPOSE

  LOW_LOG_DEBUG << "Registered ELEMENT" << LOW_LOG_END;
  // LOW_CODEGEN::END::CUSTOM:LOWCORE:ELEMENT:EXPOSE
}

// --------------------------
static void LowCore_Display_default_construct(
    Low::Core::UI::Component::Display *p_Memory)
{
  new (p_Memory) Low::Core::UI::Component::Display;
}
static void LowCore_Display_id_construct(
    u64 p_Id, Low::Core::UI::Component::Display *p_Memory)
{
  new (p_Memory) Low::Core::UI::Component::Display(p_Id);
}
static void LowCore_Display_copy_construct(
    const Low::Core::UI::Component::Display &p_Other,
    Low::Core::UI::Component::Display *p_Memory)
{
  new (p_Memory) Low::Core::UI::Component::Display(p_Other);
}
static Low::Core::UI::Component::Display &LowCore_Display_assign(
    const Low::Core::UI::Component::Display &p_Other,
    Low::Core::UI::Component::Display *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void
LowCore_Display_destruct(Low::Core::UI::Component::Display *p_Memory)
{
  using namespace Low::Core::UI::Component;
  p_Memory->~Display();
}
static u32 LowCore_Display_living_count()
{
  return Low::Core::UI::Component::Display::living_count();
}
static u16 LowCore_Display_type_id()
{
  return Low::Core::UI::Component::Display::type_id();
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:DISPLAY:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:DISPLAY:HELPERS

static void register_LowCore_Display(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Display", sizeof(Low::Core::UI::Component::Display),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose Low::Core::UI::Component::Display type.");
}
static void expose_LowCore_Display(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "Display", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Display_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectBehaviour(
      "Display", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_Display_id_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose id constructor of "
                     "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectBehaviour(
      "Display", asBEHAVE_CONSTRUCT, "void f(const Display& in)",
      asFUNCTION(LowCore_Display_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectMethod(
      "Display", "Display& opAssign(const Display& in)",
      asFUNCTION(LowCore_Display_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectBehaviour(
      "Display", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Display_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose destructor of "
                     "Low::Core::UI::Component::Display.");
  r = p_Engine->RegisterObjectMethod(
      "Display", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::UI::Component::Display, is_alive,
                 () const, bool),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for "
                     "Low::Core::UI::Component::Display.");
  r = p_Engine->RegisterObjectMethod(
      "Display", "void destroy()",
      asMETHODPR(Low::Core::UI::Component::Display, destroy, (),
                 void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose destroy for "
                     "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectMethod(
      "Display", "Vector2 get_pixel_position() const property",
      asMETHODPR(Low::Core::UI::Component::Display, pixel_position,
                 () const, Low::Math::Vector2),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose property getter for pixel_position of "
             "Low::Core::UI::Component::Display.");
  r = p_Engine->RegisterObjectMethod(
      "Display", "void set_pixel_position(Vector2) property",
      asMETHODPR(Low::Core::UI::Component::Display, pixel_position,
                 (Low::Math::Vector2), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose property setter for pixel_position of "
             "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectMethod(
      "Display", "float get_rotation() const property",
      asMETHODPR(Low::Core::UI::Component::Display, rotation,
                 () const, float),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for rotation "
                     "of Low::Core::UI::Component::Display.");
  r = p_Engine->RegisterObjectMethod(
      "Display", "void set_rotation(float) property",
      asMETHODPR(Low::Core::UI::Component::Display, rotation, (float),
                 void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for rotation "
                     "of Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectMethod(
      "Display", "Vector2 get_pixel_scale() const property",
      asMETHODPR(Low::Core::UI::Component::Display, pixel_scale,
                 () const, Low::Math::Vector2),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose property getter for pixel_scale of "
             "Low::Core::UI::Component::Display.");
  r = p_Engine->RegisterObjectMethod(
      "Display", "void set_pixel_scale(Vector2) property",
      asMETHODPR(Low::Core::UI::Component::Display, pixel_scale,
                 (Low::Math::Vector2), void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose property setter for pixel_scale of "
             "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectMethod(
      "Display", "u32 get_layer() const property",
      asMETHODPR(Low::Core::UI::Component::Display, layer, () const,
                 uint32_t),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for layer of "
                     "Low::Core::UI::Component::Display.");
  r = p_Engine->RegisterObjectMethod(
      "Display", "void set_layer(u32) property",
      asMETHODPR(Low::Core::UI::Component::Display, layer, (uint32_t),
                 void),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for layer of "
                     "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectMethod(
      "Display", "Display get_parent() const property",
      asMETHOD(Low::Core::UI::Component::Display, get_parent),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for parent of "
                     "Low::Core::UI::Component::Display.");
  r = p_Engine->RegisterObjectMethod(
      "Display", "void set_parent(Display) property",
      asMETHOD(Low::Core::UI::Component::Display, set_parent),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for parent of "
                     "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectMethod(
      "Display",
      "Vector2 get_absolute_pixel_position() const property",
      asMETHOD(Low::Core::UI::Component::Display,
               get_absolute_pixel_position),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose property getter for absolute_pixel_position "
      "of Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectMethod(
      "Display", "float get_absolute_rotation() const property",
      asMETHOD(Low::Core::UI::Component::Display,
               get_absolute_rotation),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose property getter for absolute_rotation "
             "of Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectMethod(
      "Display", "Vector2 get_absolute_pixel_scale() const property",
      asMETHOD(Low::Core::UI::Component::Display,
               get_absolute_pixel_scale),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose property getter for absolute_pixel_scale of "
      "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectMethod(
      "Display", "u32 get_absolute_layer() const property",
      asMETHOD(Low::Core::UI::Component::Display, get_absolute_layer),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose property getter for absolute_layer of "
             "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectMethod(
      "Display", "UI::Element get_element() const property",
      asMETHOD(Low::Core::UI::Component::Display, get_element),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for element "
                     "of Low::Core::UI::Component::Display.");
  r = p_Engine->RegisterObjectMethod(
      "Display", "void set_element(UI::Element) property",
      asMETHOD(Low::Core::UI::Component::Display, set_element),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for element "
                     "of Low::Core::UI::Component::Display.");

  r = p_Engine->SetDefaultNamespace("Display");
  LOW_ASSERT(r >= 0, "Failed to set namespace for "
                     "Low::Core::UI::Component::Display.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property",
      asFUNCTION(LowCore_Display_type_id), asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose TYPE_ID for "
                     "Low::Core::UI::Component::Display.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:DISPLAY:EXPOSE

  // LOW_CODEGEN::END::CUSTOM:LOWCORE:DISPLAY:EXPOSE
}

// --------------------------
static void LowCore_Text_default_construct(
    Low::Core::UI::Component::Text *p_Memory)
{
  new (p_Memory) Low::Core::UI::Component::Text;
}
static void
LowCore_Text_id_construct(u64 p_Id,
                          Low::Core::UI::Component::Text *p_Memory)
{
  new (p_Memory) Low::Core::UI::Component::Text(p_Id);
}
static void LowCore_Text_copy_construct(
    const Low::Core::UI::Component::Text &p_Other,
    Low::Core::UI::Component::Text *p_Memory)
{
  new (p_Memory) Low::Core::UI::Component::Text(p_Other);
}
static Low::Core::UI::Component::Text &
LowCore_Text_assign(const Low::Core::UI::Component::Text &p_Other,
                    Low::Core::UI::Component::Text *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void
LowCore_Text_destruct(Low::Core::UI::Component::Text *p_Memory)
{
  using namespace Low::Core::UI::Component;
  p_Memory->~Text();
}
static u32 LowCore_Text_living_count()
{
  return Low::Core::UI::Component::Text::living_count();
}
static u16 LowCore_Text_type_id()
{
  return Low::Core::UI::Component::Text::type_id();
}
static std::string
LowCore_Text_get_text(Low::Core::UI::Component::Text p_This)
{
  Low::Util::String l_Value = p_This.get_text();
  return std::string(l_Value.c_str());
}
static void
LowCore_Text_set_text(Low::Core::UI::Component::Text p_This,
                      const std::string &p_Value)
{
  p_This.set_text(Low::Util::String(p_Value.c_str()));
}
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:TEXT:HELPERS

// LOW_CODEGEN::END::CUSTOM:LOWCORE:TEXT:HELPERS

static void register_LowCore_Text(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Text", sizeof(Low::Core::UI::Component::Text),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::UI::Component::Text type.");
}
static void expose_LowCore_Text(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "Text", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Text_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::UI::Component::Text.");

  r = p_Engine->RegisterObjectBehaviour(
      "Text", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCore_Text_id_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose id constructor of "
                     "Low::Core::UI::Component::Text.");

  r = p_Engine->RegisterObjectBehaviour(
      "Text", asBEHAVE_CONSTRUCT, "void f(const Text& in)",
      asFUNCTION(LowCore_Text_copy_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::UI::Component::Text.");

  r = p_Engine->RegisterObjectMethod(
      "Text", "Text& opAssign(const Text& in)",
      asFUNCTION(LowCore_Text_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Core::UI::Component::Text.");

  r = p_Engine->RegisterObjectBehaviour(
      "Text", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Text_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose destructor of "
                     "Low::Core::UI::Component::Text.");
  r = p_Engine->RegisterObjectMethod(
      "Text", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::UI::Component::Text, is_alive, () const,
                 bool),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for "
                     "Low::Core::UI::Component::Text.");
  r = p_Engine->RegisterObjectMethod(
      "Text", "void destroy()",
      asMETHODPR(Low::Core::UI::Component::Text, destroy, (), void),
      asCALL_THISCALL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destroy for Low::Core::UI::Component::Text.");

  r = p_Engine->RegisterObjectMethod(
      "Text", "string get_text() const property",
      asFUNCTION(LowCore_Text_get_text), asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for text of "
                     "Low::Core::UI::Component::Text.");
  r = p_Engine->RegisterObjectMethod(
      "Text", "void set_text(string) property",
      asFUNCTION(LowCore_Text_set_text), asCALL_CDECL_OBJFIRST);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for text of "
                     "Low::Core::UI::Component::Text.");

  r = p_Engine->RegisterObjectMethod(
      "Text", "Vector4 get_color() const property",
      asMETHOD(Low::Core::UI::Component::Text, get_color),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for color of "
                     "Low::Core::UI::Component::Text.");
  r = p_Engine->RegisterObjectMethod(
      "Text", "void set_color(Vector4) property",
      asMETHOD(Low::Core::UI::Component::Text, set_color),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for color of "
                     "Low::Core::UI::Component::Text.");

  r = p_Engine->RegisterObjectMethod(
      "Text", "float get_size() const property",
      asMETHOD(Low::Core::UI::Component::Text, get_size),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for size of "
                     "Low::Core::UI::Component::Text.");
  r = p_Engine->RegisterObjectMethod(
      "Text", "void set_size(float) property",
      asMETHOD(Low::Core::UI::Component::Text, set_size),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for size of "
                     "Low::Core::UI::Component::Text.");

  r = p_Engine->RegisterObjectMethod(
      "Text", "UI::Element get_element() const property",
      asMETHOD(Low::Core::UI::Component::Text, get_element),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for element "
                     "of Low::Core::UI::Component::Text.");
  r = p_Engine->RegisterObjectMethod(
      "Text", "void set_element(UI::Element) property",
      asMETHOD(Low::Core::UI::Component::Text, set_element),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for element "
                     "of Low::Core::UI::Component::Text.");

  r = p_Engine->SetDefaultNamespace("Text");
  LOW_ASSERT(
      r >= 0,
      "Failed to set namespace for Low::Core::UI::Component::Text.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property", asFUNCTION(LowCore_Text_type_id),
      asCALL_CDECL);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose TYPE_ID for Low::Core::UI::Component::Text.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:TEXT:EXPOSE

  // LOW_CODEGEN::END::CUSTOM:LOWCORE:TEXT:EXPOSE
}

// --------------------------
static void
LowRenderer2_Skeleton_default_construct(Low::Renderer::Skeleton *p_Memory)
{
  new (p_Memory) Low::Renderer::Skeleton;
}
static void LowRenderer2_Skeleton_id_construct(
    u64 p_Id, Low::Renderer::Skeleton *p_Memory)
{
  new (p_Memory) Low::Renderer::Skeleton(p_Id);
}
static void LowRenderer2_Skeleton_copy_construct(
    const Low::Renderer::Skeleton &p_Other,
    Low::Renderer::Skeleton *p_Memory)
{
  new (p_Memory) Low::Renderer::Skeleton(p_Other);
}
static Low::Renderer::Skeleton &
LowRenderer2_Skeleton_assign(const Low::Renderer::Skeleton &p_Other,
                             Low::Renderer::Skeleton *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void
LowRenderer2_Skeleton_destruct(Low::Renderer::Skeleton *p_Memory)
{
  using namespace Low::Renderer;
  p_Memory->~Skeleton();
}

static void register_LowRenderer2_Skeleton(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Skeleton", sizeof(Low::Renderer::Skeleton),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0, "Failed to expose Low::Renderer::Skeleton type.");
}
static void expose_LowRenderer2_Skeleton(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "Skeleton", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowRenderer2_Skeleton_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Renderer::Skeleton.");
  r = p_Engine->RegisterObjectBehaviour(
      "Skeleton", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowRenderer2_Skeleton_id_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose id constructor of "
                     "Low::Renderer::Skeleton.");
  r = p_Engine->RegisterObjectBehaviour(
      "Skeleton", asBEHAVE_CONSTRUCT, "void f(const Skeleton& in)",
      asFUNCTION(LowRenderer2_Skeleton_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Renderer::Skeleton.");
  r = p_Engine->RegisterObjectMethod(
      "Skeleton", "Skeleton& opAssign(const Skeleton& in)",
      asFUNCTION(LowRenderer2_Skeleton_assign),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Renderer::Skeleton.");
  r = p_Engine->RegisterObjectBehaviour(
      "Skeleton", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowRenderer2_Skeleton_destruct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0,
             "Failed to expose destructor of Low::Renderer::Skeleton.");
  r = p_Engine->RegisterObjectMethod(
      "Skeleton", "bool get_is_alive() const property",
      asMETHODPR(Low::Renderer::Skeleton, is_alive, () const, bool),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for "
                     "Low::Renderer::Skeleton.");
}

// --------------------------
static void LowCoreAnimation_Clip_default_construct(
    Low::Core::Animation::Clip *p_Memory)
{
  new (p_Memory) Low::Core::Animation::Clip;
}
static void LowCoreAnimation_Clip_id_construct(
    u64 p_Id, Low::Core::Animation::Clip *p_Memory)
{
  new (p_Memory) Low::Core::Animation::Clip(p_Id);
}
static void LowCoreAnimation_Clip_copy_construct(
    const Low::Core::Animation::Clip &p_Other,
    Low::Core::Animation::Clip *p_Memory)
{
  new (p_Memory) Low::Core::Animation::Clip(p_Other);
}
static Low::Core::Animation::Clip &
LowCoreAnimation_Clip_assign(
    const Low::Core::Animation::Clip &p_Other,
    Low::Core::Animation::Clip *p_Self)
{
  *p_Self = p_Other;
  return *p_Self;
}
static void
LowCoreAnimation_Clip_destruct(Low::Core::Animation::Clip *p_Memory)
{
  using namespace Low::Core::Animation;
  p_Memory->~Clip();
}
static Low::Core::Animation::Clip
LowCoreAnimation_Clip_genfindbyname(Low::Util::Name p_Name)
{
  return Low::Core::Animation::Clip::find_by_name(p_Name);
}
static Low::Core::Animation::Clip
LowCoreAnimation_Clip_find(Low::Renderer::Skeleton p_Skeleton,
                           Low::Util::Name p_Name)
{
  return Low::Core::Animation::Clip::find(p_Skeleton, p_Name);
}
static u16 LowCoreAnimation_Clip_type_id()
{
  return Low::Core::Animation::Clip::type_id();
}

static void
register_LowCoreAnimation_Clip(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "AnimationClip", sizeof(Low::Core::Animation::Clip),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::Animation::Clip type.");
}
static void
expose_LowCoreAnimation_Clip(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectBehaviour(
      "AnimationClip", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCoreAnimation_Clip_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::Animation::Clip.");

  r = p_Engine->RegisterObjectBehaviour(
      "AnimationClip", asBEHAVE_CONSTRUCT, "void f(u64 id)",
      asFUNCTION(LowCoreAnimation_Clip_id_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose id constructor of "
                     "Low::Core::Animation::Clip.");

  r = p_Engine->RegisterObjectBehaviour(
      "AnimationClip", asBEHAVE_CONSTRUCT,
      "void f(const AnimationClip& in)",
      asFUNCTION(LowCoreAnimation_Clip_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::Animation::Clip.");

  r = p_Engine->RegisterObjectMethod(
      "AnimationClip",
      "AnimationClip& opAssign(const AnimationClip& in)",
      asFUNCTION(LowCoreAnimation_Clip_assign), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose assignment operator of "
                     "Low::Core::Animation::Clip.");

  r = p_Engine->RegisterObjectBehaviour(
      "AnimationClip", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCoreAnimation_Clip_destruct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0,
             "Failed to expose destructor of Low::Core::Animation::Clip.");
  r = p_Engine->RegisterObjectMethod(
      "AnimationClip", "bool get_is_alive() const property",
      asMETHODPR(Low::Core::Animation::Clip, is_alive, () const,
                 bool),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for "
                     "Low::Core::Animation::Clip.");
  r = p_Engine->RegisterObjectMethod(
      "AnimationClip", "Name get_name() const property",
      asMETHOD(Low::Core::Animation::Clip, get_name),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for name of "
                     "Low::Core::Animation::Clip.");
  r = p_Engine->RegisterObjectMethod(
      "AnimationClip", "bool is_loaded() const",
      asMETHOD(Low::Core::Animation::Clip, is_loaded),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0,
             "Failed to expose is_loaded for Low::Core::Animation::Clip.");
  r = p_Engine->RegisterObjectMethod(
      "AnimationClip", "float get_duration() const property",
      asMETHOD(Low::Core::Animation::Clip, get_duration),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose duration getter for "
                     "Low::Core::Animation::Clip.");
  r = p_Engine->RegisterObjectMethod(
      "AnimationClip",
      "float get_ticks_per_second() const property",
      asMETHOD(Low::Core::Animation::Clip, get_ticks_per_second),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose ticks_per_second getter for "
                     "Low::Core::Animation::Clip.");

  r = p_Engine->SetDefaultNamespace("AnimationClip");
  LOW_ASSERT(
      r >= 0,
      "Failed to set namespace for Low::Core::Animation::Clip.");
  r = p_Engine->RegisterGlobalFunction(
      "u16 get_TYPE_ID() property",
      asFUNCTION(LowCoreAnimation_Clip_type_id), asCALL_CDECL);
  LOW_ASSERT(r >= 0,
             "Failed to expose TYPE_ID for Low::Core::Animation::Clip.");
  r = p_Engine->RegisterGlobalFunction(
      "AnimationClip find(Skeleton, Name)",
      asFUNCTION(LowCoreAnimation_Clip_find), asCALL_CDECL);
  LOW_ASSERT(r >= 0,
             "Failed to expose find for Low::Core::Animation::Clip.");
  r = p_Engine->RegisterGlobalFunction(
      "AnimationClip find_by_name(Name)",
      asFUNCTION(LowCoreAnimation_Clip_genfindbyname),
      asCALL_CDECL);
  LOW_ASSERT(r >= 0, "Failed to expose generic find by name function "
                     "for Low::Core::Animation::Clip.");
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
}

namespace Low::Core {
  void register_types(asIScriptEngine *p_Engine)
  {
    register_LowCore_Region(p_Engine);
    register_LowCore_Entity(p_Engine);
    register_LowCore_Transform(p_Engine);
    register_LowCore_Animator(p_Engine);
    register_LowCore_Camera(p_Engine);
    register_LowCore_Module(p_Engine);
    register_LowCore_Asset(p_Engine);
    register_LowCore_WidgetAsset(p_Engine);
    register_LowCore_WidgetInstance(p_Engine);
    register_LowCore_View(p_Engine);
    register_LowCore_Element(p_Engine);
    register_LowCore_Display(p_Engine);
    register_LowCore_Text(p_Engine);
    register_LowRenderer2_Skeleton(p_Engine);
    register_LowCoreAnimation_Clip(p_Engine);
  }
} // namespace Low::Core
namespace Low::Core {
  void expose_types(asIScriptEngine *p_Engine)
  {
    expose_LowCore_Region(p_Engine);
    expose_LowCore_Entity(p_Engine);
    expose_LowCore_Transform(p_Engine);
    expose_LowCore_Animator(p_Engine);
    expose_LowCore_Camera(p_Engine);
    expose_LowCore_Module(p_Engine);
    expose_LowCore_Asset(p_Engine);
    expose_LowCore_WidgetAsset(p_Engine);
    expose_LowCore_WidgetInstance(p_Engine);
    expose_LowCore_View(p_Engine);
    expose_LowCore_Element(p_Engine);
    expose_LowCore_Display(p_Engine);
    expose_LowCore_Text(p_Engine);
    expose_LowRenderer2_Skeleton(p_Engine);
    expose_LowCoreAnimation_Clip(p_Engine);
  }
} // namespace Low::Core
