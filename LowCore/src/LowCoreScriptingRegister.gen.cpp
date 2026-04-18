#include <angelscript.h>

#include "LowCoreRegion.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreCamera.h"
#include "LowCoreScriptModule.h"
#include "LowCoreScriptAsset.h"
#include "LowCoreUiWidgetAsset.h"
#include "LowCoreUiWidgetInstance.h"
#include "LowCoreUiView.h"
#include "LowCoreUiElement.h"
#include "LowCoreUiDisplay.h"
#include "LowCoreUiText.h"

// --------------------------
static void
LowCore_Region_default_construct(Low::Core::Region *p_Memory)
{
  new (p_Memory) Low::Core::Region;
}
static void
LowCore_Region_copy_construct(const Low::Core::Region &p_Other,
                              Low::Core::Region *p_Memory)
{
  new (p_Memory) Low::Core::Region(p_Other);
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
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:REGION:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:REGION:HELPERS

static void expose_LowCore_Region(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Region", sizeof(Low::Core::Region),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0, "Failed to expose Low::Core::Region type.");

  r = p_Engine->RegisterObjectBehaviour(
      "Region", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Region_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose default constructor of Low::Core::Region.");

  r = p_Engine->RegisterObjectBehaviour(
      "Region", asBEHAVE_CONSTRUCT, "void f(const Region& in)",
      asFUNCTION(LowCore_Region_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose copy constructor of Low::Core::Region.");

  r = p_Engine->RegisterObjectBehaviour(
      "Region", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Region_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0,
             "Failed to expose destructor of Low::Core::Region.");
  r = p_Engine->RegisterObjectMethod(
      "Region", "bool get_alive() const property",
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
static void
LowCore_Entity_copy_construct(const Low::Core::Entity &p_Other,
                              Low::Core::Entity *p_Memory)
{
  new (p_Memory) Low::Core::Entity(p_Other);
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
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:ENTITY:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:ENTITY:HELPERS

static void expose_LowCore_Entity(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Entity", sizeof(Low::Core::Entity),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0, "Failed to expose Low::Core::Entity type.");

  r = p_Engine->RegisterObjectBehaviour(
      "Entity", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Entity_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose default constructor of Low::Core::Entity.");

  r = p_Engine->RegisterObjectBehaviour(
      "Entity", asBEHAVE_CONSTRUCT, "void f(const Entity& in)",
      asFUNCTION(LowCore_Entity_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose copy constructor of Low::Core::Entity.");

  r = p_Engine->RegisterObjectBehaviour(
      "Entity", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Entity_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0,
             "Failed to expose destructor of Low::Core::Entity.");
  r = p_Engine->RegisterObjectMethod(
      "Entity", "bool get_alive() const property",
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

  r = p_Engine->SetDefaultNamespace("Entity");
  LOW_ASSERT(r >= 0,
             "Failed to set namespace for Low::Core::Entity.");
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
static void LowCore_Transform_copy_construct(
    const Low::Core::Component::Transform &p_Other,
    Low::Core::Component::Transform *p_Memory)
{
  new (p_Memory) Low::Core::Component::Transform(p_Other);
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
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:TRANSFORM:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:TRANSFORM:HELPERS

static void expose_LowCore_Transform(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Transform", sizeof(Low::Core::Component::Transform),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose Low::Core::Component::Transform type.");

  r = p_Engine->RegisterObjectBehaviour(
      "Transform", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Transform_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectBehaviour(
      "Transform", asBEHAVE_CONSTRUCT, "void f(const Transform& in)",
      asFUNCTION(LowCore_Transform_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::Component::Transform.");

  r = p_Engine->RegisterObjectBehaviour(
      "Transform", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Transform_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose destructor of "
                     "Low::Core::Component::Transform.");
  r = p_Engine->RegisterObjectMethod(
      "Transform", "bool get_alive() const property",
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
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:TRANSFORM:EXPOSE
  // LOW_CODEGEN::END::CUSTOM:LOWCORE:TRANSFORM:EXPOSE
}

// --------------------------
static void LowCore_Camera_default_construct(
    Low::Core::Component::Camera *p_Memory)
{
  new (p_Memory) Low::Core::Component::Camera;
}
static void LowCore_Camera_copy_construct(
    const Low::Core::Component::Camera &p_Other,
    Low::Core::Component::Camera *p_Memory)
{
  new (p_Memory) Low::Core::Component::Camera(p_Other);
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
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:CAMERA:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:CAMERA:HELPERS

static void expose_LowCore_Camera(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Camera", sizeof(Low::Core::Component::Camera),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::Component::Camera type.");

  r = p_Engine->RegisterObjectBehaviour(
      "Camera", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Camera_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::Component::Camera.");

  r = p_Engine->RegisterObjectBehaviour(
      "Camera", asBEHAVE_CONSTRUCT, "void f(const Camera& in)",
      asFUNCTION(LowCore_Camera_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::Component::Camera.");

  r = p_Engine->RegisterObjectBehaviour(
      "Camera", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Camera_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destructor of Low::Core::Component::Camera.");
  r = p_Engine->RegisterObjectMethod(
      "Camera", "bool get_alive() const property",
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
static void LowCore_Module_copy_construct(
    const Low::Core::Scripting::Module &p_Other,
    Low::Core::Scripting::Module *p_Memory)
{
  new (p_Memory) Low::Core::Scripting::Module(p_Other);
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
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:MODULE:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:MODULE:HELPERS

static void expose_LowCore_Module(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Module", sizeof(Low::Core::Scripting::Module),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::Scripting::Module type.");

  r = p_Engine->RegisterObjectBehaviour(
      "Module", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Module_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::Scripting::Module.");

  r = p_Engine->RegisterObjectBehaviour(
      "Module", asBEHAVE_CONSTRUCT, "void f(const Module& in)",
      asFUNCTION(LowCore_Module_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::Scripting::Module.");

  r = p_Engine->RegisterObjectBehaviour(
      "Module", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Module_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destructor of Low::Core::Scripting::Module.");
  r = p_Engine->RegisterObjectMethod(
      "Module", "bool get_alive() const property",
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
static void LowCore_Asset_copy_construct(
    const Low::Core::Scripting::Asset &p_Other,
    Low::Core::Scripting::Asset *p_Memory)
{
  new (p_Memory) Low::Core::Scripting::Asset(p_Other);
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
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:ASSET:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:ASSET:HELPERS

static void expose_LowCore_Asset(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Asset", sizeof(Low::Core::Scripting::Asset),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::Scripting::Asset type.");

  r = p_Engine->RegisterObjectBehaviour(
      "Asset", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Asset_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::Scripting::Asset.");

  r = p_Engine->RegisterObjectBehaviour(
      "Asset", asBEHAVE_CONSTRUCT, "void f(const Asset& in)",
      asFUNCTION(LowCore_Asset_copy_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::Scripting::Asset.");

  r = p_Engine->RegisterObjectBehaviour(
      "Asset", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Asset_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destructor of Low::Core::Scripting::Asset.");
  r = p_Engine->RegisterObjectMethod(
      "Asset", "bool get_alive() const property",
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
static void LowCore_WidgetAsset_copy_construct(
    const Low::Core::UI::WidgetAsset &p_Other,
    Low::Core::UI::WidgetAsset *p_Memory)
{
  new (p_Memory) Low::Core::UI::WidgetAsset(p_Other);
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
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:WIDGETASSET:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:WIDGETASSET:HELPERS

static void expose_LowCore_WidgetAsset(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "WidgetAsset", sizeof(Low::Core::UI::WidgetAsset),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::UI::WidgetAsset type.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetAsset", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_WidgetAsset_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::UI::WidgetAsset.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetAsset", asBEHAVE_CONSTRUCT,
      "void f(const WidgetAsset& in)",
      asFUNCTION(LowCore_WidgetAsset_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::UI::WidgetAsset.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetAsset", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_WidgetAsset_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destructor of Low::Core::UI::WidgetAsset.");
  r = p_Engine->RegisterObjectMethod(
      "WidgetAsset", "bool get_alive() const property",
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
static void LowCore_WidgetInstance_copy_construct(
    const Low::Core::UI::WidgetInstance &p_Other,
    Low::Core::UI::WidgetInstance *p_Memory)
{
  new (p_Memory) Low::Core::UI::WidgetInstance(p_Other);
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
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:WIDGETINSTANCE:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:WIDGETINSTANCE:HELPERS

static void expose_LowCore_WidgetInstance(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "WidgetInstance", sizeof(Low::Core::UI::WidgetInstance),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::UI::WidgetInstance type.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetInstance", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_WidgetInstance_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::UI::WidgetInstance.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetInstance", asBEHAVE_CONSTRUCT,
      "void f(const WidgetInstance& in)",
      asFUNCTION(LowCore_WidgetInstance_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::UI::WidgetInstance.");

  r = p_Engine->RegisterObjectBehaviour(
      "WidgetInstance", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_WidgetInstance_destruct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose destructor of "
                     "Low::Core::UI::WidgetInstance.");
  r = p_Engine->RegisterObjectMethod(
      "WidgetInstance", "bool get_alive() const property",
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
static void
LowCore_View_copy_construct(const Low::Core::UI::View &p_Other,
                            Low::Core::UI::View *p_Memory)
{
  new (p_Memory) Low::Core::UI::View(p_Other);
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
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:VIEW:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:VIEW:HELPERS

static void expose_LowCore_View(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "View", sizeof(Low::Core::UI::View),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0, "Failed to expose Low::Core::UI::View type.");

  r = p_Engine->RegisterObjectBehaviour(
      "View", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_View_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose default constructor of Low::Core::UI::View.");

  r = p_Engine->RegisterObjectBehaviour(
      "View", asBEHAVE_CONSTRUCT, "void f(const View& in)",
      asFUNCTION(LowCore_View_copy_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose copy constructor of Low::Core::UI::View.");

  r = p_Engine->RegisterObjectBehaviour(
      "View", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_View_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0,
             "Failed to expose destructor of Low::Core::UI::View.");
  r = p_Engine->RegisterObjectMethod(
      "View", "bool get_alive() const property",
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

  r = p_Engine->SetDefaultNamespace("View");
  LOW_ASSERT(r >= 0,
             "Failed to set namespace for Low::Core::UI::View.");
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
LowCore_Element_copy_construct(const Low::Core::UI::Element &p_Other,
                               Low::Core::UI::Element *p_Memory)
{
  new (p_Memory) Low::Core::UI::Element(p_Other);
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
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:ELEMENT:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:ELEMENT:HELPERS

static void expose_LowCore_Element(asIScriptEngine *p_Engine)
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

  r = p_Engine->RegisterObjectBehaviour(
      "Element", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Element_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::UI::Element.");

  r = p_Engine->RegisterObjectBehaviour(
      "Element", asBEHAVE_CONSTRUCT, "void f(const Element& in)",
      asFUNCTION(LowCore_Element_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose copy constructor of Low::Core::UI::Element.");

  r = p_Engine->RegisterObjectBehaviour(
      "Element", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Element_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose destructor of Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "bool get_alive() const property",
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
      "Element", "Name get_name() const property",
      asMETHOD(Low::Core::UI::Element, get_name), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for name of "
                     "Low::Core::UI::Element.");
  r = p_Engine->RegisterObjectMethod(
      "Element", "void set_name(Name) property",
      asMETHOD(Low::Core::UI::Element, set_name), asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property setter for name of "
                     "Low::Core::UI::Element.");

  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(
      r >= 0,
      "Failed to reset namespace after Low::Core::UI::Element.");

  r = p_Engine->SetDefaultNamespace("UI::Element");
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
static void LowCore_Display_copy_construct(
    const Low::Core::UI::Component::Display &p_Other,
    Low::Core::UI::Component::Display *p_Memory)
{
  new (p_Memory) Low::Core::UI::Component::Display(p_Other);
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
// LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:DISPLAY:HELPERS
// LOW_CODEGEN::END::CUSTOM:LOWCORE:DISPLAY:HELPERS

static void expose_LowCore_Display(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Display", sizeof(Low::Core::UI::Component::Display),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(
      r >= 0,
      "Failed to expose Low::Core::UI::Component::Display type.");

  r = p_Engine->RegisterObjectBehaviour(
      "Display", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Display_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectBehaviour(
      "Display", asBEHAVE_CONSTRUCT, "void f(const Display& in)",
      asFUNCTION(LowCore_Display_copy_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::UI::Component::Display.");

  r = p_Engine->RegisterObjectBehaviour(
      "Display", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Display_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose destructor of "
                     "Low::Core::UI::Component::Display.");
  r = p_Engine->RegisterObjectMethod(
      "Display", "bool get_alive() const property",
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
      "Display", "u64 get_parent() const property",
      asMETHOD(Low::Core::UI::Component::Display, get_parent),
      asCALL_THISCALL);
  LOW_ASSERT(r >= 0, "Failed to expose property getter for parent of "
                     "Low::Core::UI::Component::Display.");
  r = p_Engine->RegisterObjectMethod(
      "Display", "void set_parent(u64) property",
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
static void LowCore_Text_copy_construct(
    const Low::Core::UI::Component::Text &p_Other,
    Low::Core::UI::Component::Text *p_Memory)
{
  new (p_Memory) Low::Core::UI::Component::Text(p_Other);
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

static void expose_LowCore_Text(asIScriptEngine *p_Engine)
{
  int r = 0;
  r = p_Engine->RegisterObjectType(
      "Text", sizeof(Low::Core::UI::Component::Text),
      asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
  LOW_ASSERT(r >= 0,
             "Failed to expose Low::Core::UI::Component::Text type.");

  r = p_Engine->RegisterObjectBehaviour(
      "Text", asBEHAVE_CONSTRUCT, "void f()",
      asFUNCTION(LowCore_Text_default_construct),
      asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose default constructor of "
                     "Low::Core::UI::Component::Text.");

  r = p_Engine->RegisterObjectBehaviour(
      "Text", asBEHAVE_CONSTRUCT, "void f(const Text& in)",
      asFUNCTION(LowCore_Text_copy_construct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose copy constructor of "
                     "Low::Core::UI::Component::Text.");

  r = p_Engine->RegisterObjectBehaviour(
      "Text", asBEHAVE_DESTRUCT, "void f()",
      asFUNCTION(LowCore_Text_destruct), asCALL_CDECL_OBJLAST);
  LOW_ASSERT(r >= 0, "Failed to expose destructor of "
                     "Low::Core::UI::Component::Text.");
  r = p_Engine->RegisterObjectMethod(
      "Text", "bool get_alive() const property",
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
  r = p_Engine->SetDefaultNamespace("");
  LOW_ASSERT(r >= 0, "Failed to reset default namespace.");
  // LOW_CODEGEN:BEGIN:CUSTOM:LOWCORE:TEXT:EXPOSE
  // LOW_CODEGEN::END::CUSTOM:LOWCORE:TEXT:EXPOSE
}

namespace Low::Core {
  void expose_types(asIScriptEngine *p_Engine)
  {
    expose_LowCore_Region(p_Engine);
    expose_LowCore_Entity(p_Engine);
    expose_LowCore_Transform(p_Engine);
    expose_LowCore_Camera(p_Engine);
    expose_LowCore_Module(p_Engine);
    expose_LowCore_Asset(p_Engine);
    expose_LowCore_WidgetAsset(p_Engine);
    expose_LowCore_WidgetInstance(p_Engine);
    expose_LowCore_View(p_Engine);
    expose_LowCore_Element(p_Engine);
    expose_LowCore_Display(p_Engine);
    expose_LowCore_Text(p_Engine);
  }
} // namespace Low::Core