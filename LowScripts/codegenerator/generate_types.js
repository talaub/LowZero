const fs = require("fs");
const os = require("os");
const exec = require("child_process").execSync;
const YAML = require("yaml");

const {
  read_file,
  get_plain_type,
  get_accessor_type,
  is_reference_type,
  format,
  write,
  line,
  include,
  empty,
  get_marker_begin,
  get_marker_end,
  find_begin_marker_start,
  find_begin_marker_end,
  find_end_marker_start,
  find_end_marker_end,
  save_file,
  collect_enums_for_low,
  collect_enums_for_project,
  collect_types_for_project,
  collect_types_for_low,
  is_string_type,
  is_math_type,
  is_name_type,
  is_container_type,
} = require("./lib.js");

function get_deserializer_method_for_math_type(p_Type) {
  if (!is_math_type(p_Type)) {
    console.log("--------- ERROR NOT A MATH TYPE");
  }

  const l_Parts = p_Type.split(":");
  let l_Type = l_Parts[l_Parts.length - 1];

  if (p_Type.endsWith("Color")) {
    l_Type = "Vector4";
  }
  if (p_Type.endsWith("ColorRGB")) {
    l_Type = "Vector3";
  }

  return `Low::Util::Serialization::deserialize_${l_Type.toLowerCase()}`;
}

function get_property_type(p_Type) {
  if (p_Type.endsWith("Math::ColorRGB")) {
    return "COLORRGB";
  }
  if (p_Type.endsWith("Math::Shape")) {
    return "SHAPE";
  }
  if (p_Type.endsWith("Math::Vector2")) {
    return "VECTOR2";
  }
  if (p_Type.endsWith("Math::Vector3")) {
    return "VECTOR3";
  }
  if (p_Type.endsWith("Math::Quaternion")) {
    return "QUATERNION";
  }
  if (p_Type.endsWith("Util::Handle")) {
    return "HANDLE";
  }
  if (p_Type.endsWith("Util::Name")) {
    return "NAME";
  }
  if (["bool"].includes(p_Type)) {
    return "BOOL";
  }
  if (["float"].includes(p_Type)) {
    return "FLOAT";
  }
  if (["uint16_t", "u16"].includes(p_Type)) {
    return "UINT16";
  }
  if (["uint32_t", "u32"].includes(p_Type)) {
    return "UINT32";
  }
  if (["uint64_t", "u64"].includes(p_Type)) {
    return "UINT64";
  }
  if (["int"].includes(p_Type)) {
    return "INT";
  }
  if (["void"].includes(p_Type)) {
    return "VOID";
  }
  if (p_Type.endsWith("Util::String")) {
    return "STRING";
  }
  if (p_Type.endsWith("Util::UniqueId")) {
    return "UINT64";
  }
  return "UNKNOWN";
}

function write_header_imports(p_Imports) { }

const g_EnumType = "uint8_t";

function generate_enum_header(p_Enum) {
  let t = "";
  let n = 0;

  let l_OldCode = "";
  if (fs.existsSync(p_Enum.header_file_path)) {
    l_OldCode = read_file(p_Enum.header_file_path);
  }

  const l_EnumString = `${p_Enum.namespace_string}::${p_Enum.name}`;

  t += line("#pragma once");
  t += empty();
  t += include(`${p_Enum.api_file}`);
  t += empty();
  t += include(`LowMath.h`);
  t += include(`LowUtilName.h`);
  t += empty();

  for (let i_Namespace of p_Enum.namespace) {
    t += line(`namespace ${i_Namespace} {`, n++);
  }

  t += line(`enum class ${p_Enum.name}: u8`, n);
  t += line("{", n++);

  for (let i_Option of p_Enum.options) {
    t += line(`${i_Option.name.toUpperCase()},`, n);
  }

  t += line("};", --n);
  t += empty();

  t += line(`namespace ${p_Enum.name}EnumHelper {`);
  t += line(`void ${p_Enum.dll_macro} initialize();`);
  t += line(`void ${p_Enum.dll_macro} cleanup();`);
  t += empty();

  t += line(
    `Low::Util::Name ${p_Enum.dll_macro} entry_name(${l_EnumString} p_Value);`,
  );

  t += line(
    `Low::Util::Name ${p_Enum.dll_macro} _entry_name(${g_EnumType} p_Value);`,
  );

  t += empty();

  t += line(
    `${l_EnumString} ${p_Enum.dll_macro} entry_value(Low::Util::Name p_Name);`,
  );

  t += line(
    `${g_EnumType} ${p_Enum.dll_macro} _entry_value(Low::Util::Name p_Name);`,
  );

  t += empty();
  t += line(`u16 ${p_Enum.dll_macro} get_enum_id();`);
  t += empty();
  t += line(`u8 ${p_Enum.dll_macro} get_entry_count();`);

  t += line("}");

  for (let i_Namespace of p_Enum.namespace) {
    t += line("}");
  }

  const l_Formatted = format(p_Enum.header_file_path, t);

  if (l_Formatted !== l_OldCode) {
    save_file(p_Enum.header_file_path, l_Formatted);
    return true;
  }
  return false;
}

function generate_enum_source(p_Enum) {
  let t = "";
  let n = 0;

  let l_OldCode = "";
  if (fs.existsSync(p_Enum.source_file_path)) {
    l_OldCode = read_file(p_Enum.source_file_path);
  }

  const l_EnumString = `${p_Enum.namespace_string}::${p_Enum.name}`;

  t += include(`${p_Enum.header_file_name}`);
  t += empty();
  t += include(`LowUtil.h`);
  t += include(`LowUtilAssert.h`);
  t += include(`LowUtilHandle.h`);
  t += empty();

  for (let i_Namespace of p_Enum.namespace) {
    t += line(`namespace ${i_Namespace} {`, n++);
  }

  t += line(`namespace ${p_Enum.name}EnumHelper {`);
  t += line(`void initialize() {`);
  t += line(`Low::Util::RTTI::EnumInfo l_EnumInfo;`);
  t += line(`l_EnumInfo.name = N(${p_Enum.name});`);
  t += line(`l_EnumInfo.enumId = ${p_Enum.enumId};`);
  t += line(`l_EnumInfo.entry_name = &_entry_name;`);
  t += line(`l_EnumInfo.entry_value = &_entry_value;`);
  t += empty();
  let l_OptionIndex = 0;
  for (let i_Option of p_Enum.options) {
    t += line("{");
    t += line("Low::Util::RTTI::EnumEntryInfo l_Entry;");
    t += line(`l_Entry.name = N(${i_Option.name});`);
    t += line(`l_Entry.value = ${l_OptionIndex};`);
    t += empty();
    t += line(`l_EnumInfo.entries.push_back(l_Entry);`);
    t += line("}");
    l_OptionIndex++;
  }
  t += empty();
  t += line(`Low::Util::register_enum_info(${p_Enum.enumId}, l_EnumInfo);`);
  t += line(`}`);
  t += empty();
  t += line(`void cleanup() {`);
  t += line(`}`);
  t += empty();

  t += line(`Low::Util::Name entry_name(${l_EnumString} p_Value) {`);
  for (let i_Option of p_Enum.options) {
    t += line(`if (p_Value == ${p_Enum.name}::${i_Option.uppercase}) {`);
    t += line(`return N(${i_Option.name});`);
    t += line("}");
  }
  t += empty();
  t += line(
    `LOW_ASSERT(false, "Could not find entry in enum ${p_Enum.name}.");`,
  );
  t += line("return N(EMPTY);");
  t += line("}");
  t += empty();

  t += line(`Low::Util::Name _entry_name(${g_EnumType} p_Value) {`);
  t += line(`${l_EnumString} l_Enum = static_cast<${l_EnumString}>(p_Value);`);
  t += line(`return entry_name(l_Enum);`);
  t += line("}");
  t += empty();

  t += line(`${l_EnumString} entry_value(Low::Util::Name p_Name) {`);
  for (let i_Option of p_Enum.options) {
    t += line(`if (p_Name == N(${i_Option.name})) {`);
    t += line(`return ${l_EnumString}::${i_Option.uppercase};`);
    t += line("}");
  }
  t += empty();
  t += line(
    `LOW_ASSERT(false, "Could not find entry in enum ${p_Enum.name}.");`,
  );
  t += line(`return static_cast<${l_EnumString}>(0);`);
  t += line("}");
  t += empty();

  t += line(`${g_EnumType} _entry_value(Low::Util::Name p_Name) {`);
  t += line(`return static_cast<${g_EnumType}>(entry_value(p_Name));`);
  t += line("}");
  t += empty();

  t += line(`u16 get_enum_id() {`);
  t += line(`return ${p_Enum.enumId};`);
  t += line("}");
  t += empty();

  t += line(`u8 get_entry_count() {`);
  t += line(`return ${p_Enum.options.length};`);
  t += line("}");

  t += line("}");

  for (let i_Namespace of p_Enum.namespace) {
    t += line("}");
  }

  const l_Formatted = format(p_Enum.source_file_path, t);

  if (l_Formatted !== l_OldCode) {
    save_file(p_Enum.source_file_path, l_Formatted);
    return true;
  }
  return false;
}

function generate_header(p_Type) {
  let t = "";
  let n = 0;

  let privatelines = [];

  let l_OldCode = "";
  if (fs.existsSync(p_Type.header_file_path)) {
    l_OldCode = read_file(p_Type.header_file_path);
  }

  t += line("#pragma once");
  t += empty();
  t += include(`${p_Type.api_file}`);
  t += empty();
  t += include("LowUtilHandle.h");
  t += include("LowUtilName.h");
  t += include("LowUtilContainers.h");
  t += include("LowUtilYaml.h");
  t += empty();
  if (p_Type.component) {
    t += include("LowCoreEntity.h");
    t += empty();
  } else if (p_Type.ui_component) {
    t += include("LowCoreUiElement.h");
    t += empty();
  }

  if (p_Type.header_imports) {
    for (const i_Import of p_Type.header_imports) {
      t += include(i_Import);
    }
    t += empty();
  }

  if (true) {
    const l_MarkerName = `CUSTOM:HEADER_CODE`;

    const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
    const l_CustomEndMarker = get_marker_end(l_MarkerName);

    const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

    let l_CustomCode = "";

    if (l_BeginMarkerIndex >= 0) {
      const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

      l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_CustomBeginMarker);
    t += l_CustomCode;
    t += line(l_CustomEndMarker);
    t += empty();
  }

  for (let i_Namespace of p_Type.namespace) {
    t += line(`namespace ${i_Namespace} {`, n++);
  }

  if (true) {
    const l_MarkerName = `CUSTOM:NAMESPACE_CODE`;

    const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
    const l_CustomEndMarker = get_marker_end(l_MarkerName);

    const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

    let l_CustomCode = "";

    if (l_BeginMarkerIndex >= 0) {
      const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

      l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_CustomBeginMarker);
    t += l_CustomCode;
    t += line(l_CustomEndMarker);
    t += empty();
  }


  t += line(
    `struct ${p_Type.dll_macro} ${p_Type.name}: public Low::Util::Handle`,
    n,
  );
  t += line("{", n++);
  t += line('public:');
  t += line(`struct Data`, n);
  t += line("{", n++);
  t += line('public:');

  for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
    if (!i_Prop.static && !i_Prop.no_data) {
      t += line(`${i_Prop.plain_type} ${i_PropName};`, n);
    }
  }

  t += empty();

  t += line(`static size_t get_size()`, n);
  t += line("{", n++);
  t += line(`return sizeof(Data);`, n);
  t += line("}", --n);

  t += line("};", --n);
  t += empty();
  t += empty();
  t += line("public:", --n);
  n++;
  t += line("static Low::Util::SharedMutex ms_LivingMutex;", n);
  t += line("static Low::Util::UniqueLock<Low::Util::SharedMutex> ms_PagesLock;");
  t += line("static Low::Util::SharedMutex ms_PagesMutex;", n);
  t += line("static Low::Util::List<Low::Util::Instances::Page*> ms_Pages;", n);
  t += empty();
  t += line(`static Low::Util::List<${p_Type.name}> ms_LivingInstances;`, n);
  t += empty();
  t += line("const static uint16_t TYPE_ID;", n);

  t += empty();
  t += line(`${p_Type.name}();`);
  t += line(`${p_Type.name}(uint64_t p_Id);`);
  t += line(`${p_Type.name}(${p_Type.name} &p_Copy);`);
  t += empty();

  if (p_Type.private_make) {
    t += line("private:");
  }
  if (p_Type.component) {
    t += line(`static ${p_Type.name} make(Low::Core::Entity p_Entity);`);
    t += line(`static Low::Util::Handle _make(Low::Util::Handle p_Entity);`);
    if (p_Type.unique_id) {
      t += line(
        `static ${p_Type.name} make(Low::Core::Entity p_Entity, Low::Util::UniqueId p_UniqueId);`,
      );
    }
  } else if (p_Type.ui_component) {
    t += line(`static ${p_Type.name} make(Low::Core::UI::Element p_Element);`);
    t += line(`static Low::Util::Handle _make(Low::Util::Handle p_Element);`);
    if (p_Type.unique_id) {
      t += line(
        `static ${p_Type.name} make(Low::Core::UI::Element p_Element, Low::Util::UniqueId p_UniqueId);`,
      );
    }
  } else {
    t += line(`static ${p_Type.name} make(Low::Util::Name p_Name);`);
    t += line(`static Low::Util::Handle _make(Low::Util::Name p_Name);`);
    if (p_Type.unique_id) {
      t += line(
        `static ${p_Type.name} make(Low::Util::Name p_Name, Low::Util::UniqueId p_UniqueId);`,
      );
    }
  }
  if (p_Type.private_make) {
    t += line("public:");
  }
  t += line(
    `explicit ${p_Type.name}(const ${p_Type.name}& p_Copy): Low::Util::Handle(p_Copy.m_Id) {`,
  );
  t += line(`}`);
  t += empty();
  t += line(`void destroy();`);
  t += empty();
  t += line(`static void initialize();`);
  t += line(`static void cleanup();`);

  t += empty();
  t += line("static uint32_t living_count() {");
  t += line(`Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(ms_LivingMutex);`);
  t += line("return static_cast<uint32_t>(ms_LivingInstances.size());");
  t += line("}");
  t += line(`static ${p_Type.name} *living_instances() {`);
  t += line(`Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(ms_LivingMutex);`);
  t += line("return ms_LivingInstances.data();");
  t += line("}");
  t += empty();
  t += line(`static ${p_Type.name} create_handle_by_index(u32 p_Index);`);
  t += empty();

  t += line(`static ${p_Type.name} find_by_index(uint32_t p_Index);`);
  t += line(`static Low::Util::Handle _find_by_index(uint32_t p_Index);`);
  t += empty();

  t += line(`bool is_alive() const;`, n);
  t += empty();
  t += line(
    `u64 observe(Low::Util::Name p_Observable, Low::Util::Handle p_Observer) const;`,
    n,
  );
  t += line(
    `u64 observe(Low::Util::Name p_Observable, Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)> p_Observer) const;`,
    n,
  );
  t += line(
    `void notify(Low::Util::Handle p_Observed, Low::Util::Name p_Observable);`,
    n,
  );
  t += line(
    `void broadcast_observable(Low::Util::Name p_Observable) const;`,
    n,
  );
  t += empty();
  t += line(
    `static void _notify(Low::Util::Handle p_Observer, Low::Util::Handle p_Observed, Low::Util::Name p_Observable);`,
    n,
  );
  t += empty();
  if (p_Type.reference_counted) {
    t += line(`void reference(const u64 p_Id);`);
    t += line(`void dereference(const u64 p_Id);`);
    t += line(`u32 references() const;`);
    t += empty();
  }
  t += line(`static uint32_t get_capacity();`);
  t += empty();
  t += line(`void serialize(Low::Util::Yaml::Node& p_Node) const;`, n);
  t += empty();
  if (p_Type.component) {
    t += line(`${p_Type.name} duplicate(Low::Core::Entity p_Entity) const;`, n);
    t += line(
      `static ${p_Type.name} duplicate(${p_Type.name} p_Handle, Low::Core::Entity p_Entity);`,
    );
    t += line(
      `static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle, Low::Util::Handle p_Entity);`,
    );
  } else if (p_Type.ui_component) {
    t += line(
      `${p_Type.name} duplicate(Low::Core::UI::Element p_Entity) const;`,
      n,
    );
    t += line(
      `static ${p_Type.name} duplicate(${p_Type.name} p_Handle, Low::Core::UI::Element p_Element);`,
    );
    t += line(
      `static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle, Low::Util::Handle p_Element);`,
    );
  } else {
    t += line(`${p_Type.name} duplicate(Low::Util::Name p_Name) const;`, n);
    t += line(
      `static ${p_Type.name} duplicate(${p_Type.name} p_Handle, Low::Util::Name p_Name);`,
    );
    t += line(
      `static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle, Low::Util::Name p_Name);`,
    );
  }
  t += empty();
  if (!p_Type.component && !p_Type.ui_component) {
    t += line(`static ${p_Type.name} find_by_name(Low::Util::Name p_Name);`, n);
    t += line(
      `static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);`,
      n,
    );
    t += empty();
  }

  t += line(
    `static void serialize(Low::Util::Handle p_Handle, Low::Util::Yaml::Node& p_Node);`,
    n,
  );
  t += line(
    `static Low::Util::Handle deserialize(Low::Util::Yaml::Node& p_Node, Low::Util::Handle p_Creator);`,
    n,
  );
  t += line("static bool is_alive(Low::Util::Handle p_Handle) {");
  t += line(`${p_Type.name} l_Handle = p_Handle.get_id();`)
  t += line(`return l_Handle.is_alive();`)
  t += line("}");
  t += empty();
  t += line("static void destroy(Low::Util::Handle p_Handle) {");
  t += line("_LOW_ASSERT(is_alive(p_Handle));");
  t += line(`${p_Type.name} l_${p_Type.name} = p_Handle.get_id();`);
  t += line(`l_${p_Type.name}.destroy();`);
  t += line("}");
  t += empty();

  for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
    if (i_Prop.static) {
      continue;
    }
    if (!i_Prop.no_getter && !i_Prop.no_data) {
      const l = `${i_Prop.accessor_type} ${i_Prop.getter_name}() ${i_Prop.getter_no_const ? "" : "const"};`;
      if (i_Prop.private_getter) {
        privatelines.push(l);
      } else {
        t += line(l, n);
      }
    }
    if (!i_Prop.no_setter && !i_Prop.no_data) {
      const l_SetterLines = [
        `void ${i_Prop.setter_name}(${i_Prop.accessor_type} p_Value);`,
      ];

      // Type specific special setters
      if (is_string_type(i_Prop.plain_type)) {
        l_SetterLines.push(`void ${i_Prop.setter_name}(const char* p_Value);`);
      } else if (i_Prop.plain_type.endsWith("UVector2")) {
        l_SetterLines.push(`void ${i_Prop.setter_name}(u32 p_X, u32 p_Y);`);
        l_SetterLines.push(`void ${i_Prop.setter_name}_x(u32 p_Value);`);
        l_SetterLines.push(`void ${i_Prop.setter_name}_y(u32 p_Value);`);
      } else if (i_Prop.plain_type.endsWith("Vector2")) {
        l_SetterLines.push(`void ${i_Prop.setter_name}(float p_X, float p_Y);`);
        l_SetterLines.push(`void ${i_Prop.setter_name}_x(float p_Value);`);
        l_SetterLines.push(`void ${i_Prop.setter_name}_y(float p_Value);`);
      } else if (i_Prop.plain_type.endsWith("Math::Vector3")
        || i_Prop.plain_type.endsWith("Math::ColorRGB")) {
        l_SetterLines.push(
          `void ${i_Prop.setter_name}(float p_X, float p_Y, float p_Z);`,
        );
        l_SetterLines.push(`void ${i_Prop.setter_name}_x(float p_Value);`);
        l_SetterLines.push(`void ${i_Prop.setter_name}_y(float p_Value);`);
        l_SetterLines.push(`void ${i_Prop.setter_name}_z(float p_Value);`);
      }

      if (i_Prop.plain_type == "bool") {
        l_SetterLines.push(`void toggle_${i_PropName}();`);
      }

      if (i_Prop.private_setter) {
        l_SetterLines.forEach((l) => privatelines.push(l));
      } else {
        l_SetterLines.forEach((l) => (t += line(l, n)));
      }
    }

    if (i_Prop.is_dirty_flag) {
      t += line(`void mark_${i_PropName}();`);
    }
    t += empty();
  }

  if (p_Type.functions) {
    for (let [i_FuncName, i_Func] of Object.entries(p_Type.functions)) {
      let func_line = "";
      if (i_Func.description) {
        func_line += line('/*!')
        func_line += line(i_Func.description)
        func_line += line('*/')
      }
      if (i_Func.static) {
        func_line += write("static ");
      }
      func_line += write(`${i_Func.accessor_type} ${i_Func.name}(`);
      if (i_Func.parameters) {
        for (let i = 0; i < i_Func.parameters.length; ++i) {
          if (i > 0) {
            func_line += write(", ");
          }
          const i_Param = i_Func.parameters[i];
          if (i_Param["const"]) {
            func_line += write("const ");
          }
          func_line += write(`${i_Param.accessor_type} ${i_Param.name}`);
        }
      }
      func_line += write(")");
      if (i_Func.constant) {
        func_line += write(" const");
      }
      func_line += line(";");

      if (i_Func.private) {
        privatelines.push(func_line);
      } else {
        t += write(func_line);
      }
    }
  }

  t += line(`static bool get_page_for_index(const u32 p_Index, u32& p_PageIndex, u32& p_SlotIndex);`);

  t += line("private:");
  t += line(`static u32 ms_Capacity;`);
  t += line(`static u32 ms_PageSize;`);
  t += line(`static u32 create_instance(u32& p_PageIndex, u32&p_SlotIndex, Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock);`);
  t += line(`static u32 create_page();`);
  if (privatelines.length) {
    for (const l of privatelines) {
      t += line(l);
    }
  }

  if (true) {
    t += empty();
    const l_MarkerName = `CUSTOM:STRUCT_END_CODE`;

    const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
    const l_CustomEndMarker = get_marker_end(l_MarkerName);

    const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

    let l_CustomCode = "";

    if (l_BeginMarkerIndex >= 0) {
      const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

      l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_CustomBeginMarker);
    t += l_CustomCode;
    t += line(l_CustomEndMarker);
    t += empty();
  }

  t += line("};", --n);

  if (true) {
    t += empty();
    const l_MarkerName = `CUSTOM:NAMESPACE_AFTER_STRUCT_CODE`;

    const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
    const l_CustomEndMarker = get_marker_end(l_MarkerName);

    const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

    let l_CustomCode = "";

    if (l_BeginMarkerIndex >= 0) {
      const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

      l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_CustomBeginMarker);
    t += l_CustomCode;
    t += line(l_CustomEndMarker);
    t += empty();
  }

  for (let i_Namespace of p_Type.namespace) {
    t += line("}", --n);
  }

  const l_Formatted = format(p_Type.header_file_path, t);

  if (l_Formatted !== l_OldCode) {
    save_file(p_Type.header_file_path, l_Formatted);
    return true;
  }
  return false;
}

function generate_source(p_Type) {
  let t = "";
  let n = 0;

  const l_GetReferences = `(TYPE_SOA(${p_Type.name}, references, Low::Util::Set<u64>))`;

  let l_OldCode = "";
  if (fs.existsSync(p_Type.source_file_path)) {
    l_OldCode = read_file(p_Type.source_file_path);
  }

  t += include(p_Type.header_file_name, n);
  t += empty();
  t += line("#include<algorithm>", n);
  t += empty();
  t += include("LowUtil.h", n);
  t += include("LowUtilAssert.h", n);
  t += include("LowUtilLogger.h", n);
  t += include("LowUtilProfiler.h", n);
  t += include("LowUtilConfig.h", n);
  t += include("LowUtilHashing.h", n);
  t += include("LowUtilSerialization.h", n);
  t += include("LowUtilObserverManager.h", n);
  t += empty();
  if (p_Type.component) {
    t += include(`LowCorePrefabInstance.h`);
  }
  if (p_Type.source_imports) {
    for (const i_Include of p_Type.source_imports) {
      t += include(i_Include);
    }
    t += empty();
  }

  if (true) {
    const l_MarkerName = `CUSTOM:SOURCE_CODE`;

    const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
    const l_CustomEndMarker = get_marker_end(l_MarkerName);

    const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

    let l_CustomCode = "";

    if (l_BeginMarkerIndex >= 0) {
      const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

      l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_CustomBeginMarker);
    t += l_CustomCode;
    t += line(l_CustomEndMarker);
    t += empty();
  }

  for (let i_Namespace of p_Type.namespace) {
    t += line(`namespace ${i_Namespace} {`, n++);
  }

  if (true) {
    const l_MarkerName = `CUSTOM:NAMESPACE_CODE`;

    const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
    const l_CustomEndMarker = get_marker_end(l_MarkerName);

    const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

    let l_CustomCode = "";

    if (l_BeginMarkerIndex >= 0) {
      const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

      l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_CustomBeginMarker);
    t += l_CustomCode;
    t += line(l_CustomEndMarker);
    t += empty();
  }

  t += line(`const uint16_t ${p_Type.name}::TYPE_ID = ${p_Type.typeId};`, n);
  t += line(`uint32_t ${p_Type.name}::ms_Capacity = 0u;`, n);
  t += line(`uint32_t ${p_Type.name}::ms_PageSize = 0u;`, n);
  t += line(`Low::Util::SharedMutex ${p_Type.name}::ms_LivingMutex;`, n);
  t += line(`Low::Util::SharedMutex ${p_Type.name}::ms_PagesMutex;`, n);
  t += line(`Low::Util::UniqueLock<Low::Util::SharedMutex> ${p_Type.name}::ms_PagesLock(${p_Type.name}::ms_PagesMutex, std::defer_lock);`, n);
  t += line(
    `Low::Util::List<${p_Type.name}> ${p_Type.name}::ms_LivingInstances;`,
    n,
  );
  t += line(
    `Low::Util::List<Low::Util::Instances::Page*> ${p_Type.name}::ms_Pages;`,
    n,
  );
  t += empty();

  t += line(`${p_Type.name}::${p_Type.name}(): Low::Util::Handle(0ull){`);
  t += line("}");
  t += line(
    `${p_Type.name}::${p_Type.name}(uint64_t p_Id): Low::Util::Handle(p_Id){`,
  );
  t += line("}");
  t += line(
    `${p_Type.name}::${p_Type.name}(${p_Type.name} &p_Copy): Low::Util::Handle(p_Copy.m_Id){`,
  );
  t += line("}");

  t += empty();
  if (p_Type.component) {
    t += line(
      `Low::Util::Handle ${p_Type.name}::_make(Low::Util::Handle p_Entity) {`,
    );
    t += line(`Low::Core::Entity l_Entity = p_Entity.get_id();`);
    t += line(
      `LOW_ASSERT(l_Entity.is_alive(), "Cannot create component for dead entity");`,
    );
    t += line(`return make(l_Entity).get_id();`);
    t += line(`}`);
  } else if (p_Type.ui_component) {
    t += line(
      `Low::Util::Handle ${p_Type.name}::_make(Low::Util::Handle p_Element) {`,
    );
    t += line(`Low::Core::UI::Element l_Element = p_Element.get_id();`);
    t += line(
      `LOW_ASSERT(l_Element.is_alive(), "Cannot create component for dead element");`,
    );
    t += line(`return make(l_Element).get_id();`);
    t += line(`}`);
  } else {
    t += line(
      `Low::Util::Handle ${p_Type.name}::_make(Low::Util::Name p_Name) {`,
    );
    t += line(`return make(p_Name).get_id();`);
    t += line(`}`);
  }
  t += empty();
  if (p_Type.component) {
    t += line(
      `${p_Type.name} ${p_Type.name}::make(Low::Core::Entity p_Entity){`,
    );
    if (p_Type.unique_id) {
      t += line(`return make(p_Entity, 0ull);`);
      t += line("}");
      t += empty();
      t += line(
        `${p_Type.name} ${p_Type.name}::make(Low::Core::Entity p_Entity, Low::Util::UniqueId p_UniqueId){`,
      );
    }
  } else if (p_Type.ui_component) {
    t += line(
      `${p_Type.name} ${p_Type.name}::make(Low::Core::UI::Element p_Element){`,
    );
    if (p_Type.unique_id) {
      t += line(`return make(p_Element, 0ull);`);
      t += line("}");
      t += empty();
      t += line(
        `${p_Type.name} ${p_Type.name}::make(Low::Core::UI::Element p_Element, Low::Util::UniqueId p_UniqueId){`,
      );
    }
  } else {
    t += line(`${p_Type.name} ${p_Type.name}::make(Low::Util::Name p_Name){`);
    if (p_Type.unique_id) {
      t += line(`return make(p_Name, 0ull);`);
      t += line("}");
      t += empty();
      t += line(
        `${p_Type.name} ${p_Type.name}::make(Low::Util::Name p_Name, Low::Util::UniqueId p_UniqueId){`,
      );
    }
  }
  t += line(`u32 l_PageIndex = 0;`);
  t += line(`u32 l_SlotIndex = 0;`);
  t += line(`Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;`)
  t += line(`uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex, l_PageLock);`);
  t += empty();
  t += line(`${p_Type.name} l_Handle;`);
  t += line(`l_Handle.m_Data.m_Index = l_Index;`);
  t += line(`l_Handle.m_Data.m_Generation = ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;`);
  t += line(`l_Handle.m_Data.m_Type = ${p_Type.name}::TYPE_ID;`);
  t += empty();
  t += line(`l_PageLock.unlock();`)
  t += empty();
  t += line(`Low::Util::HandleLock<${p_Type.name}> l_HandleLock(l_Handle);`)
  t += empty();
  for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
    if (i_Prop.no_data) {
      continue;
    }
    if (["bool", "boolean"].includes(i_Prop.type)) {
      t += line(
        `ACCESSOR_TYPE_SOA(l_Handle, ${p_Type.name}, ${i_PropName}, ${i_Prop.soa_type}) = false;`,
      );
    } else if (["float", "double"].includes(i_Prop.type)) {
      t += line(
        `ACCESSOR_TYPE_SOA(l_Handle, ${p_Type.name}, ${i_PropName}, ${i_Prop.soa_type}) = 0.0f;`,
      );
    } else if (["Name", "Low::Util::Name"].includes(i_Prop.type)) {
      t += line(
        `ACCESSOR_TYPE_SOA(l_Handle, ${p_Type.name}, ${i_PropName}, ${i_Prop.soa_type}) = Low::Util::Name(0u);`,
      );
    } else if (is_reference_type(i_Prop.type)) {
      t += line(
        `new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ${p_Type.name}, ${i_PropName}, ${i_Prop.soa_type})) ${i_Prop.plain_type}();`,
      );
    }
  }
  t += empty();
  if (p_Type.component) {
    t += line("l_Handle.set_entity(p_Entity);");
    t += line("p_Entity.add_component(l_Handle);");
    t += empty();
  } else if (p_Type.ui_component) {
    t += line("l_Handle.set_element(p_Element);");
    t += line("p_Element.add_component(l_Handle);");
    t += empty();
  } else {
    t += line("l_Handle.set_name(p_Name);");
    t += empty();
  }
  t += line('{');
  t += line(`Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(ms_LivingMutex);`);
  t += line(`ms_LivingInstances.push_back(l_Handle);`);
  t += line('}');
  if (p_Type.unique_id) {
    t += empty();
    t += line(`if (p_UniqueId > 0ull) {`);
    t += line(`l_Handle.set_unique_id(p_UniqueId);`);
    t += line(`} else {`);
    t += line(
      `l_Handle.set_unique_id(Low::Util::generate_unique_id(l_Handle.get_id()));`,
    );
    t += line(`}`);
    t += line(
      `Low::Util::register_unique_id(l_Handle.get_unique_id(),l_Handle.get_id());`,
    );
  }
  t += empty();
  const l_MakeMarkerName = `CUSTOM:MAKE`;

  const l_MakeBeginMarker = get_marker_begin(l_MakeMarkerName);
  const l_MakeEndMarker = get_marker_end(l_MakeMarkerName);

  const l_MakeBeginMarkerIndex = find_begin_marker_end(
    l_OldCode,
    l_MakeMarkerName,
  );

  let l_MakeCustomCode = "";

  if (l_MakeBeginMarkerIndex >= 0) {
    const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MakeMarkerName);

    l_MakeCustomCode = l_OldCode.substring(
      l_MakeBeginMarkerIndex,
      l_EndMarkerIndex,
    );
  }
  t += line(l_MakeBeginMarker);
  t += l_MakeCustomCode;
  t += line(l_MakeEndMarker);
  t += empty();
  t += line("return l_Handle;");

  t += line("}");

  t += empty();
  t += line(`void ${p_Type.name}::destroy(){`);
  t += line('LOW_ASSERT(is_alive(), "Cannot destroy dead object");');
  t += empty();
  const l_MarkerName = `CUSTOM:DESTROY`;

  const l_DestroyBeginMarker = get_marker_begin(l_MarkerName);
  const l_DestroyEndMarker = get_marker_end(l_MarkerName);

  const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

  let l_CustomCode = "";

  if (l_BeginMarkerIndex >= 0) {
    const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

    l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
  }

  t += line('{');
  t += line(`Low::Util::HandleLock<${p_Type.name}> l_Lock(get_id());`);
  t += line(l_DestroyBeginMarker);
  t += l_CustomCode;
  t += line(l_DestroyEndMarker);
  t += line('}');
  t += empty();
  t += line(`broadcast_observable(OBSERVABLE_DESTROY);`);
  t += empty();
  if (p_Type.unique_id) {
    t += line(`Low::Util::remove_unique_id(get_unique_id());`);
    t += empty();
  }

  t += line(`u32 l_PageIndex = 0;`)
  t += line(`u32 l_SlotIndex = 0;`)
  t += line(`_LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));`)
  t += line(`Low::Util::Instances::Page* l_Page = ms_Pages[l_PageIndex];`)
  t += empty();
  t += line(`Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(l_Page->mutex);`)
  t += line("l_Page->slots[l_SlotIndex].m_Occupied = false;");
  t += line("l_Page->slots[l_SlotIndex].m_Generation++;");
  t += empty();

  /*
  t += line(`const ${p_Type.name} *l_Instances = living_instances();`);
  t += line(`bool l_LivingInstanceFound = false;`);
  t += line(`for (uint32_t i = 0u; i < living_count(); ++i) {`);
  t += line(`if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {`);
  t += line(`ms_LivingInstances.erase(ms_LivingInstances.begin() + i);`);
  t += line(`l_LivingInstanceFound = true;`);
  t += line(`break;`);
  t += line("}");
  t += line("}");
  */
  t += line(`ms_PagesLock.lock();`);

  t += line(`Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(ms_LivingMutex);`);
  t += line(
    `for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();) {`,
  );
  t += line(`if (it->get_id() == get_id()) {`);
  t += line(`it = ms_LivingInstances.erase(it);`);
  t += line("} else {");
  t += line("it++;");
  t += line("}");
  t += line("}");
  //t += line(`_LOW_ASSERT(l_LivingInstanceFound);`);
  t += line(`ms_PagesLock.unlock();`);
  t += line(`l_LivingLock.unlock();`);
  t += line("}");
  t += empty();
  t += line(`void ${p_Type.name}::initialize() {`);
  t += line(`LOCK_PAGES_WRITE(l_PagesLock);`);
  if (true) {
    const l_MarkerName = `CUSTOM:PREINITIALIZE`;

    const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
    const l_CustomEndMarker = get_marker_end(l_MarkerName);

    const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

    let l_CustomCode = "";

    if (l_BeginMarkerIndex >= 0) {
      const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

      l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_CustomBeginMarker);
    t += l_CustomCode;
    t += line(l_CustomEndMarker);
    t += empty();
  }
  t += line(
    `ms_Capacity = Low::Util::Config::get_capacity(N(${p_Type.module}), N(${p_Type.name}));`,
  );
  t += empty();
  if (!p_Type.dynamic_increase) {
    t += line(`ms_PageSize = ms_Capacity;`);
    t += line(`Low::Util::Instances::Page* l_Page = new Low::Util::Instances::Page;`)
    t += line(`Low::Util::Instances::initialize_page(l_Page, ${p_Type.name}::Data::get_size(), ms_PageSize);`)
    t += line(`ms_Pages.push_back(l_Page);`);
  }
  else {
    t += line(
      `ms_PageSize = Low::Math::Util::clamp(Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);`)
    t += line(`{`)
    t += line(`u32 l_Capacity = 0u;`)
    t += line(`while (l_Capacity < ms_Capacity){`);
    t += line(`Low::Util::Instances::Page* i_Page = new Low::Util::Instances::Page;`)
    t += line(`Low::Util::Instances::initialize_page(i_Page, ${p_Type.name}::Data::get_size(), ms_PageSize);`)
    t += line(`ms_Pages.push_back(i_Page);`);
    t += line(`l_Capacity += ms_PageSize;`);
    t += line(`}`)
    t += line(`ms_Capacity = l_Capacity;`);
    t += line(`}`)
  }
  /*
  t += line(`initialize_buffer(`);
  t += line(
    `&ms_Buffer, ${p_Type.name}Data::get_size(), get_capacity(), &ms_Slots`,
  );
  t += line(`);`);
  */
  t += line(`LOCK_UNLOCK(l_PagesLock);`);
  t += empty();
  t += line(`Low::Util::RTTI::TypeInfo l_TypeInfo;`);
  t += line(`l_TypeInfo.name = N(${p_Type.name});`);
  t += line(`l_TypeInfo.typeId = TYPE_ID;`);
  t += line(`l_TypeInfo.get_capacity = &get_capacity;`);
  t += line(`l_TypeInfo.is_alive = &${p_Type.name}::is_alive;`);
  t += line(`l_TypeInfo.destroy = &${p_Type.name}::destroy;`);
  t += line(`l_TypeInfo.serialize = &${p_Type.name}::serialize;`);
  t += line(`l_TypeInfo.deserialize = &${p_Type.name}::deserialize;`);
  t += line(`l_TypeInfo.find_by_index = &${p_Type.name}::_find_by_index;`);
  t += line(`l_TypeInfo.notify = &${p_Type.name}::_notify;`);
  if (p_Type.component) {
    t += line(`l_TypeInfo.make_default = nullptr;`);
    t += line(`l_TypeInfo.make_component = &${p_Type.name}::_make;`);
    t += line(`l_TypeInfo.duplicate_default = nullptr;`);
    t += line(`l_TypeInfo.duplicate_component = &${p_Type.name}::_duplicate;`);
  } else if (p_Type.ui_component) {
    t += line(`l_TypeInfo.make_default = nullptr;`);
    t += line(`l_TypeInfo.make_component = &${p_Type.name}::_make;`);
    t += line(`l_TypeInfo.duplicate_default = nullptr;`);
    t += line(`l_TypeInfo.duplicate_component = &${p_Type.name}::_duplicate;`);
  } else {
    t += line(`l_TypeInfo.find_by_name = &${p_Type.name}::_find_by_name;`);
    t += line(`l_TypeInfo.make_component = nullptr;`);
    t += line(`l_TypeInfo.make_default = &${p_Type.name}::_make;`);
    t += line(`l_TypeInfo.duplicate_default = &${p_Type.name}::_duplicate;`);
    t += line(`l_TypeInfo.duplicate_component = nullptr;`);
  }
  t += line(
    `l_TypeInfo.get_living_instances = reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(&${p_Type.name}::living_instances);`,
  );
  t += line(`l_TypeInfo.get_living_count = &${p_Type.name}::living_count;`);
  if (p_Type.component) {
    t += line(`l_TypeInfo.component = true;`);
    t += line(`l_TypeInfo.uiComponent = false;`);
  } else if (p_Type.ui_component) {
    t += line(`l_TypeInfo.component = false;`);
    t += line(`l_TypeInfo.uiComponent = true;`);
  } else {
    t += line(`l_TypeInfo.component = false;`);
    t += line(`l_TypeInfo.uiComponent = false;`);
  }
  for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
    if (i_Prop.no_data) {
      continue;
    }
    t += line(`{`);
    t += line(`// Property: ${i_PropName}`);
    t += line(`Low::Util::RTTI::PropertyInfo l_PropertyInfo;`);
    t += line(`l_PropertyInfo.name = N(${i_PropName});`);
    t += line(
      `l_PropertyInfo.editorProperty = ${i_Prop.editor_editable ? "true" : "false"};`,
    );
    t += line(
      `l_PropertyInfo.dataOffset = offsetof(${p_Type.name}::Data, ${i_PropName});`,
    );
    if (i_Prop.handle) {
      t += line(`l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;`);
      t += line(`l_PropertyInfo.handleType = ${i_Prop.plain_type}::TYPE_ID;`);
    } else if (i_Prop.enum) {
      t += line(`l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;`);
      t += line(
        `l_PropertyInfo.handleType = ${i_Prop.plain_type}EnumHelper::get_enum_id();`,
      );
    } else {
      t += line(
        `l_PropertyInfo.type = Low::Util::RTTI::PropertyType::${get_property_type(i_Prop.plain_type)};`,
      );
      t += line(`l_PropertyInfo.handleType = 0;`);
    }
    t += line(
      `l_PropertyInfo.get_return = [](Low::Util::Handle p_Handle) -> void const* {`,
    );
    if (!i_Prop.no_getter && !i_Prop.private_getter) {
      t += line(`${p_Type.name} l_Handle = p_Handle.get_id();`);
      t += line(`Low::Util::HandleLock<${p_Type.name}> l_HandleLock(l_Handle);`);
      t += line(`l_Handle.${i_Prop.getter_name}();`);
      t +=
        line(`return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ${p_Type.name}, ${i_PropName},
                                            ${i_Prop.soa_type});`);
      /*
       */
    } else {
      t += line(`return nullptr;`);
    }
    t += line(`};`);
    t += line(
      `l_PropertyInfo.set = [](Low::Util::Handle p_Handle, const void* p_Data) -> void {`,
    );
    if (!i_Prop.no_setter && !i_Prop.private_setter) {
      t += line(`${p_Type.name} l_Handle = p_Handle.get_id();`);
      t += line(
        `l_Handle.${i_Prop.setter_name}(*(${i_Prop.plain_type}*)p_Data);`,
      );
    }
    t += line(`};`);
    t += line(
      `l_PropertyInfo.get = [](Low::Util::Handle p_Handle, void* p_Data) {`,
    );
    if (!i_Prop.no_getter && !i_Prop.private_getter) {
      t += line(`${p_Type.name} l_Handle = p_Handle.get_id();`);
      t += line(`Low::Util::HandleLock<${p_Type.name}> l_HandleLock(l_Handle);`);
      t += line(
        `*((${i_Prop.plain_type}*) p_Data) = l_Handle.${i_Prop.getter_name}();`,
      );
      //t += line(`memcpy(p_Data, &l_Data, sizeof(${i_Prop.plain_type}));`);
      /*
       */
    }
    t += line(`};`);
    t += line(`l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;`);
    t += line(`// End property: ${i_PropName}`);
    t += line(`}`);
  }

  if (p_Type.virtual_properties) {
    for (let [i_VPropName, i_VProp] of Object.entries(
      p_Type.virtual_properties,
    )) {
      t += line(`{`);
      t += line(`// Virtual property: ${i_VPropName}`);
      t += line(`Low::Util::RTTI::VirtualPropertyInfo l_PropertyInfo;`);
      t += line(`l_PropertyInfo.name = N(${i_VPropName});`);
      t += line(
        `l_PropertyInfo.editorProperty = ${i_VProp.editor_editable ? "true" : "false"};`,
      );
      if (i_VProp.handle) {
        t += line(
          `l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;`,
        );
        t += line(
          `l_PropertyInfo.handleType = ${i_VProp.plain_type}::TYPE_ID;`,
        );
      } else if (i_VProp.enum) {
        t += line(`l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;`);
        t += line(
          `l_PropertyInfo.handleType = ${i_VProp.plain_type}EnumHelper::get_enum_id();`,
        );
      } else {
        t += line(
          `l_PropertyInfo.type = Low::Util::RTTI::PropertyType::${get_property_type(i_VProp.plain_type)};`,
        );
        t += line(`l_PropertyInfo.handleType = 0;`);
      }
      t += line(
        `l_PropertyInfo.get = [](Low::Util::Handle p_Handle, void* p_Data) {`,
      );
      if (!i_VProp.no_getter && !i_VProp.private_getter) {
        t += line(`${p_Type.name} l_Handle = p_Handle.get_id();`);
        t += line(`Low::Util::HandleLock<${p_Type.name}> l_HandleLock(l_Handle);`);
        t += line(
          `${i_VProp.plain_type} l_Data = l_Handle.${i_VProp.getter_name}();`,
        );
        t += line(`memcpy(p_Data, &l_Data, sizeof(${i_VProp.plain_type}));`);
        /*
         */
      } else {
        t += line(`return nullptr;`);
      }
      t += line(`};`);
      t += line(
        `l_PropertyInfo.set = [](Low::Util::Handle p_Handle, const void* p_Data) -> void {`,
      );
      if (!i_VProp.no_setter && !i_VProp.private_setter) {
        t += line(`${p_Type.name} l_Handle = p_Handle.get_id();`);
        t += line(
          `l_Handle.${i_VProp.setter_name}(*(${i_VProp.plain_type}*)p_Data);`,
        );
      }
      t += line(`};`);
      t += line(
        `l_TypeInfo.virtualProperties[l_PropertyInfo.name] = l_PropertyInfo;`,
      );
      t += line(`// End virtual property: ${i_VPropName}`);
      t += line(`}`);
    }
  }

  if (p_Type.functions) {
    for (let [i_FuncName, i_Func] of Object.entries(p_Type.functions)) {
      t += line("{");
      t += line(`// Function: ${i_FuncName}`);
      t += line(`Low::Util::RTTI::FunctionInfo l_FunctionInfo;`);
      t += line(`l_FunctionInfo.name = N(${i_FuncName});`);
      if (i_Func.return_handle) {
        t += line(
          `l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;`,
        );
        t += line(
          `l_FunctionInfo.handleType = ${i_Func.return_type}::TYPE_ID;`,
        );
      } else if (i_Func.return_enum) {
        t += line(`l_FunctionInfo.type = Low::Util::RTTI::PropertyType::ENUM;`);
        t += line(
          `l_FunctionInfo.handleType = ${i_Func.return_type}EnumHelper::get_enum_id();`,
        );
      } else {
        t += line(
          `l_FunctionInfo.type = Low::Util::RTTI::PropertyType::${get_property_type(i_Func.return_type)};`,
        );
        t += line(`l_FunctionInfo.handleType = 0;`);
      }
      if (i_Func.parameters) {
        for (const i_Param of i_Func.parameters) {
          t += line(`{`);
          t += line(`Low::Util::RTTI::ParameterInfo l_ParameterInfo;`);
          t += line(`l_ParameterInfo.name = N(${i_Param.name});`);
          if (i_Param.handle) {
            t += line(
              `l_ParameterInfo.type = Low::Util::RTTI::PropertyType::HANDLE;`,
            );
            if (i_Param.type.endsWith("Util::Handle")) {
              t += line(`l_ParameterInfo.handleType = 0;`);
            } else {
              t += line(
                `l_ParameterInfo.handleType = ${i_Param.type}::TYPE_ID;`,
              );
            }
          } else if (i_Param.enum) {
            t += line(
              `l_ParameterInfo.type = Low::Util::RTTI::PropertyType::ENUM;`,
            );
            t += line(
              `l_ParameterInfo.handleType = ${i_Param.type}EnumHelper::get_enum_id();`,
            );
          } else {
            t += line(
              `l_ParameterInfo.type = Low::Util::RTTI::PropertyType::${get_property_type(i_Param.type)};`,
            );
            t += line(`l_ParameterInfo.handleType = 0;`);
          }
          t += line(`l_FunctionInfo.parameters.push_back(l_ParameterInfo);`);
          t += line(`}`);
        }
      }
      t += line(`l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;`);
      t += line(`// End function: ${i_FuncName}`);
      t += line("}");
    }
  }
  t += line(`Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);`);
  t += line("}");
  t += empty();
  t += line(`void ${p_Type.name}::cleanup() {`);
  t += line(
    `Low::Util::List<${p_Type.name}> l_Instances = ms_LivingInstances;`,
  );
  t += line(`for (uint32_t i = 0u; i < l_Instances.size(); ++i) {`);
  t += line(`l_Instances[i].destroy();`);
  t += line("}");
  t += line(`ms_PagesLock.lock();`);
  t += line(`for (auto it = ms_Pages.begin(); it != ms_Pages.end();){`)
  t += line(`Low::Util::Instances::Page* i_Page = *it;`)
  t += line(`free(i_Page->buffer);`);
  t += line(`free(i_Page->slots);`);
  t += line(`free(i_Page->lockWords);`)
  t += line(`delete i_Page;`);
  t += line(`it = ms_Pages.erase(it);`);
  t += line(`}`);
  t += empty();
  t += line(`ms_Capacity = 0;`);
  t += empty();
  t += line(`ms_PagesLock.unlock();`);
  t += line("}");
  t += empty();

  t += line(
    `Low::Util::Handle ${p_Type.name}::_find_by_index(uint32_t p_Index) {`,
  );
  t += line(`return find_by_index(p_Index).get_id();`);
  t += line("}");
  t += empty();

  t += line(`${p_Type.name} ${p_Type.name}::find_by_index(uint32_t p_Index) {`);
  t += line(`LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");`);
  t += empty();
  t += line(`${p_Type.name} l_Handle;`);
  t += line(`l_Handle.m_Data.m_Index = p_Index;`);
  t += line(`l_Handle.m_Data.m_Type = ${p_Type.name}::TYPE_ID;`);
  t += empty();

  t += line(`u32 l_PageIndex = 0;`)
  t += line(`u32 l_SlotIndex = 0;`)
  t += line(`if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)){l_Handle.m_Data.m_Generation = 0;}`)
  t += line(`Low::Util::Instances::Page* l_Page = ms_Pages[l_PageIndex];`)
  t += line(`Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(l_Page->mutex);`);
  t += line(`l_Handle.m_Data.m_Generation = l_Page->slots[l_SlotIndex].m_Generation;`);
  t += empty();
  t += line("return l_Handle;");
  t += line("}");
  t += empty();

  t += line(
    `${p_Type.name} ${p_Type.name}::create_handle_by_index(u32 p_Index) {`,
  );
  t += line(`if(p_Index < get_capacity()){`);
  t += line(`return find_by_index(p_Index);`);
  t += line("}");
  t += empty();
  t += line(`${p_Type.name} l_Handle;`);
  t += line(`l_Handle.m_Data.m_Index = p_Index;`);
  t += line(`l_Handle.m_Data.m_Generation = 0;`);
  t += line(`l_Handle.m_Data.m_Type = ${p_Type.name}::TYPE_ID;`);
  t += empty();
  t += line("return l_Handle;");
  t += line("}");
  t += empty();

  t += line(`bool ${p_Type.name}::is_alive() const {`);
  t += line(`if (m_Data.m_Type != ${p_Type.name}::TYPE_ID) {return false;}`)
  t += line(`u32 l_PageIndex = 0;`)
  t += line(`u32 l_SlotIndex = 0;`)
  t += line(`if (!get_page_for_index(get_index(), l_PageIndex, l_SlotIndex)){return false;}`)
  t += line(`Low::Util::Instances::Page* l_Page = ms_Pages[l_PageIndex];`)
  t += line(`Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(l_Page->mutex);`);
  t += line(
    `return m_Data.m_Type == ${p_Type.name}::TYPE_ID && l_Page->slots[l_SlotIndex].m_Occupied && l_Page->slots[l_SlotIndex].m_Generation == m_Data.m_Generation;`,
  );
  t += line(`}`);

  t += empty();
  t += line(`uint32_t ${p_Type.name}::get_capacity(){`);
  t += line("return ms_Capacity;");
  t += line("}");
  t += empty();
  if (!p_Type.component && !p_Type.ui_component) {
    t += line(
      `Low::Util::Handle ${p_Type.name}::_find_by_name(Low::Util::Name p_Name) {`,
      n,
    );

    t += line("return find_by_name(p_Name).get_id();");
    t += line("}");
    t += empty();

    t += line(
      `${p_Type.name} ${p_Type.name}::find_by_name(Low::Util::Name p_Name) {`,
      n,
    );

    if (true) {
      t += empty();
      const l_MarkerName = `CUSTOM:FIND_BY_NAME`;

      const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
      const l_CustomEndMarker = get_marker_end(l_MarkerName);

      const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

      let l_CustomCode = "";

      if (l_BeginMarkerIndex >= 0) {
        const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

        l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
      }
      t += line(l_CustomBeginMarker);
      t += l_CustomCode;
      t += line(l_CustomEndMarker);
      t += empty();
    }

    t += line(`Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(ms_LivingMutex);`);
    t += line(
      `for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end(); ++it) {`,
    );
    t += line(`if (it->get_name() == p_Name) {`);
    t += line(`return *it;`);
    t += line("}");
    t += line("}");
    t += line("return Low::Util::Handle::DEAD;");
    t += line("}");
  }
  t += empty();
  const l_DuplicateMarkerName = `CUSTOM:DUPLICATE`;

  const l_DuplicateBeginMarker = get_marker_begin(l_DuplicateMarkerName);
  const l_DuplicateEndMarker = get_marker_end(l_DuplicateMarkerName);

  const l_DuplicateBeginMarkerIndex = find_begin_marker_end(
    l_OldCode,
    l_DuplicateMarkerName,
  );

  let l_DuplicateCustomCode = "";

  if (l_DuplicateBeginMarkerIndex >= 0) {
    const l_DuplicateEndMarkerIndex = find_end_marker_start(
      l_OldCode,
      l_DuplicateMarkerName,
    );

    l_DuplicateCustomCode = l_OldCode.substring(
      l_DuplicateBeginMarkerIndex,
      l_DuplicateEndMarkerIndex,
    );
  }
  if (p_Type.component) {
    t += line(
      `${p_Type.name} ${p_Type.name}::duplicate(Low::Core::Entity p_Entity) const {`,
    );
  } else if (p_Type.ui_component) {
    t += line(
      `${p_Type.name} ${p_Type.name}::duplicate(Low::Core::UI::Element p_Element) const {`,
    );
  } else {
    t += line(
      `${p_Type.name} ${p_Type.name}::duplicate(Low::Util::Name p_Name) const {`,
    );
  }
  t += line(`_LOW_ASSERT(is_alive());`);
  if (!p_Type.no_auto_duplicate) {
    t += empty();
    if (p_Type.component) {
      t += line(`${p_Type.name} l_Handle = make(p_Entity);`);
    } else if (p_Type.ui_component) {
      t += line(`${p_Type.name} l_Handle = make(p_Element);`);
    } else {
      t += line(`${p_Type.name} l_Handle = make(p_Name);`);
    }
    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
      if (i_Prop.skip_duplication || i_Prop.no_data) {
        continue;
      }
      if (i_Prop.no_setter || i_Prop.no_getter) {
        continue;
      }
      if (i_Prop.handle) {
        t += line(`if (${i_Prop.getter_name}().is_alive()) {`);
        t += line(`l_Handle.${i_Prop.setter_name}(${i_Prop.getter_name}());`);
        t += line(`}`);
      } else {
        t += line(`l_Handle.${i_Prop.setter_name}(${i_Prop.getter_name}());`);
      }
    }
  }

  if (p_Type.no_auto_duplicate) {
    if (l_DuplicateCustomCode === "") {
      l_DuplicateCustomCode += line(
        `LOW_ASSERT_WARN(false, "Not implemented");`,
      );
      l_DuplicateCustomCode += line(`return 0;`);
    }
  }

  t += empty();
  t += line(l_DuplicateBeginMarker);
  t += l_DuplicateCustomCode;
  t += line(l_DuplicateEndMarker);
  if (!p_Type.no_auto_duplicate) {
    t += empty();
    t += line("return l_Handle;");
  }
  t += line("}");
  t += empty();

  if (p_Type.component) {
    t += line(
      `${p_Type.name} ${p_Type.name}::duplicate(${p_Type.name} p_Handle, Low::Core::Entity p_Entity) {`,
    );
    t += line(`return p_Handle.duplicate(p_Entity);`);
    t += line("}");
  } else if (p_Type.ui_component) {
    t += line(
      `${p_Type.name} ${p_Type.name}::duplicate(${p_Type.name} p_Handle, Low::Core::UI::Element p_Element) {`,
    );
    t += line(`return p_Handle.duplicate(p_Element);`);
    t += line("}");
  } else {
    t += line(
      `${p_Type.name} ${p_Type.name}::duplicate(${p_Type.name} p_Handle, Low::Util::Name p_Name) {`,
    );
    t += line(`return p_Handle.duplicate(p_Name);`);
    t += line("}");
  }
  t += empty();

  if (p_Type.component) {
    t += line(
      `Low::Util::Handle ${p_Type.name}::_duplicate(Low::Util::Handle p_Handle, Low::Util::Handle p_Entity) {`,
    );
    t += line(`${p_Type.name} l_${p_Type.name} = p_Handle.get_id();`);
    t += line(`Low::Core::Entity l_Entity = p_Entity.get_id();`);
    t += line(`return l_${p_Type.name}.duplicate(l_Entity);`);
    t += line("}");
  } else if (p_Type.ui_component) {
    t += line(
      `Low::Util::Handle ${p_Type.name}::_duplicate(Low::Util::Handle p_Handle, Low::Util::Handle p_Element) {`,
    );
    t += line(`${p_Type.name} l_${p_Type.name} = p_Handle.get_id();`);
    t += line(`Low::Core::UI::Element l_Element = p_Element.get_id();`);
    t += line(`return l_${p_Type.name}.duplicate(l_Element);`);
    t += line("}");
  } else {
    t += line(
      `Low::Util::Handle ${p_Type.name}::_duplicate(Low::Util::Handle p_Handle, Low::Util::Name p_Name) {`,
    );
    t += line(`${p_Type.name} l_${p_Type.name} = p_Handle.get_id();`);
    t += line(`return l_${p_Type.name}.duplicate(p_Name);`);
    t += line("}");
  }
  t += empty();

  const l_SerializerMarkerName = `CUSTOM:SERIALIZER`;

  const l_SerializerBeginMarker = get_marker_begin(l_SerializerMarkerName);
  const l_SerializerEndMarker = get_marker_end(l_SerializerMarkerName);

  const l_SerializerBeginMarkerIndex = find_begin_marker_end(
    l_OldCode,
    l_SerializerMarkerName,
  );

  let l_SerializerCustomCode = "";

  if (l_SerializerBeginMarkerIndex >= 0) {
    const l_SerializerEndMarkerIndex = find_end_marker_start(
      l_OldCode,
      l_SerializerMarkerName,
    );

    l_SerializerCustomCode = l_OldCode.substring(
      l_SerializerBeginMarkerIndex,
      l_SerializerEndMarkerIndex,
    );
  }
  t += line(
    `void ${p_Type.name}::serialize(Low::Util::Yaml::Node& p_Node) const {`,
    n,
  );
  t += line(`_LOW_ASSERT(is_alive());`);
  if (!p_Type.no_auto_serialize) {
    t += empty();
    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
      if (i_Prop.skip_serialization) {
        continue;
      }

      if (i_PropName == "unique_id" && p_Type.unique_id) {
        t += line(`p_Node["_unique_id"] = Low::Util::hash_to_string(${i_Prop.getter_name}()).c_str();`)

        continue;
      }

      if (i_Prop.plain_type.endsWith("Variant")) {
        t += line(
          `Low::Util::Serialization::serialize_variant(p_Node["${i_PropName}"], ${i_Prop.getter_name}());`,
        );
      } else if (
        is_name_type(i_Prop.plain_type) ||
        is_string_type(i_Prop.plain_type)
      ) {
        t += line(`p_Node["${i_PropName}"] = ${i_Prop.getter_name}().c_str();`);
      } else if (
        ["int", "uint32_t", "uint8_t", "uint16_t", "uint64_t"].includes(
          i_Prop.plain_type,
        ) ||
        i_Prop.plain_type.endsWith("Util::UniqueId")
      ) {
        if (i_Prop.hash) {
          t += line(`p_Node["${i_PropName}"] = Low::Util::hash_to_string(${i_Prop.getter_name}()).c_str();`)
        } else {
          t += line(`p_Node["${i_PropName}"] = ${i_Prop.getter_name}();`);
        }
      } else if (["float", "double"].includes(i_Prop.plain_type)) {
        t += line(`p_Node["${i_PropName}"] = ${i_Prop.getter_name}();`);
      } else if (["bool"].includes(i_Prop.plain_type)) {
        t += line(`p_Node["${i_PropName}"] = ${i_Prop.getter_name}();`);
      } else if (is_math_type(i_Prop.plain_type)) {
        t += line(
          `Low::Util::Serialization::serialize(p_Node["${i_PropName}"], ${i_Prop.getter_name}());`,
        );
      } else if (i_Prop.handle) {
        t += line(`if (${i_Prop.getter_name}().is_alive()) {`);
        t += line(
          `${i_Prop.getter_name}().serialize(p_Node["${i_PropName}"]);`,
        );
        t += line(`}`);
      } else if (i_Prop.enum) {
        t += line(
          `Low::Util::Serialization::serialize_enum(p_Node["${i_PropName}"], ${i_Prop.plain_type}EnumHelper::get_enum_id(), static_cast<${g_EnumType}>(${i_Prop.getter_name}()));`,
        );
      }
    }
  }

  t += empty();
  t += line(l_SerializerBeginMarker);
  t += l_SerializerCustomCode;
  t += line(l_SerializerEndMarker);
  t += line("}");
  t += empty();
  t += line(
    `void ${p_Type.name}::serialize(Low::Util::Handle p_Handle, Low::Util::Yaml::Node& p_Node) {`,
    n,
  );
  t += line(`${p_Type.name} l_${p_Type.name} = p_Handle.get_id();`);
  t += line(`l_${p_Type.name}.serialize(p_Node);`);
  t += line("}");
  t += empty();

  const l_DeserializerMarkerName = `CUSTOM:DESERIALIZER`;

  const l_DeserializerBeginMarker = get_marker_begin(l_DeserializerMarkerName);
  const l_DeserializerEndMarker = get_marker_end(l_DeserializerMarkerName);

  const l_DeserializerBeginMarkerIndex = find_begin_marker_end(
    l_OldCode,
    l_DeserializerMarkerName,
  );

  let l_DeserializerCustomCode = "";

  if (l_DeserializerBeginMarkerIndex >= 0) {
    const l_DeserializerEndMarkerIndex = find_end_marker_start(
      l_OldCode,
      l_DeserializerMarkerName,
    );

    l_DeserializerCustomCode = l_OldCode.substring(
      l_DeserializerBeginMarkerIndex,
      l_DeserializerEndMarkerIndex,
    );
  }
  t += line(
    `Low::Util::Handle ${p_Type.name}::deserialize(Low::Util::Yaml::Node& p_Node, Low::Util::Handle p_Creator) {`,
    n,
  );
  if (!p_Type.no_auto_deserialize) {
    if (p_Type.unique_id) {
      t += line(`Low::Util::UniqueId l_HandleUniqueId = 0ull;`);
      t += line(`if (p_Node["unique_id"]) {`);
      t += line(`l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();`);
      t += line("}");
      t += line(`else if (p_Node["_unique_id"]) {`);
      t += line(`l_HandleUniqueId = Low::Util::string_to_hash(LOW_YAML_AS_STRING(p_Node["_unique_id"]));`);
      t += line("}");
      t += empty();
    }
    if (p_Type.component) {
      if (p_Type.unique_id) {
        t += line(
          `${p_Type.name} l_Handle = ${p_Type.name}::make(p_Creator.get_id(), l_HandleUniqueId);`,
        );
      }
      else {
        t += line(
          `${p_Type.name} l_Handle = ${p_Type.name}::make(p_Creator.get_id());`,
        );
      }
    } else if (p_Type.ui_component) {
      if (p_Type.unique_id) {
        t += line(
          `${p_Type.name} l_Handle = ${p_Type.name}::make(p_Creator.get_id(), l_HandleUniqueId);`,
        );
      }
      else {
        t += line(
          `${p_Type.name} l_Handle = ${p_Type.name}::make(p_Creator.get_id());`,
        );
      }
    } else {
      if (p_Type.unique_id) {
        t += line(
          `${p_Type.name} l_Handle = ${p_Type.name}::make(N(${p_Type.name}), l_HandleUniqueId);`,
        );
      }
      else {
        t += line(
          `${p_Type.name} l_Handle = ${p_Type.name}::make(N(${p_Type.name}));`,
        );
      }
    }
    t += empty();


    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
      if (i_Prop.skip_deserialization) {
        continue;
      }

      t += line(`if (p_Node["${i_PropName}"]) {`);

      if (i_Prop.plain_type.endsWith("Variant")) {
        t += line(
          `l_Handle.${i_Prop.setter_name}(Low::Util::Serialization::deserialize_variant(p_Node["${i_PropName}"]));`,
        );
      } else if (is_name_type(i_Prop.plain_type)) {
        t += line(
          `l_Handle.${i_Prop.setter_name}(LOW_YAML_AS_NAME(p_Node["${i_PropName}"]));`,
        );
      } else if (is_string_type(i_Prop.plain_type)) {
        t += line(
          `l_Handle.${i_Prop.setter_name}(LOW_YAML_AS_STRING(p_Node["${i_PropName}"]));`,
        );
      } else if (is_math_type(i_Prop.plain_type)) {
        t += line(
          `l_Handle.${i_Prop.setter_name}(${get_deserializer_method_for_math_type(i_Prop.plain_type)}(p_Node["${i_PropName}"]));`,
        );
      } else if (
        [
          "bool",
          "float",
          "int",
          "double",
          "uint64_t",
          "uint32_t",
          "uint8_t",
          "uint16_t",
        ].includes(i_Prop.plain_type) ||
        i_Prop.plain_type.endsWith("Util::UniqueId")
      ) {
        if (i_Prop.hash) {
          t += line(
            `l_Handle.${i_Prop.setter_name}(Low::Util::string_to_hash(LOW_YAML_AS_STRING(p_Node["${i_PropName}"])));`,
          );
        }
        else {
          t += line(
            `l_Handle.${i_Prop.setter_name}(p_Node["${i_PropName}"].as<${i_Prop.plain_type}>());`,
          );
        }
      } else if (i_Prop.handle) {
        t += line(
          `l_Handle.${i_Prop.setter_name}(${i_Prop.plain_type}::deserialize(p_Node["${i_PropName}"], l_Handle.get_id()).get_id());`,
        );
      } else if (i_Prop.enum) {
        t += line(
          `l_Handle.${i_Prop.setter_name}(static_cast<${i_Prop.plain_type}>(Low::Util::Serialization::deserialize_enum(p_Node["${i_PropName}"])));`,
        );
      }
      t += line(`}`);
    }
  }

  t += empty();
  t += line(l_DeserializerBeginMarker);
  t += l_DeserializerCustomCode;
  t += line(l_DeserializerEndMarker);
  if (!p_Type.no_auto_deserialize) {
    t += empty();
    t += line(`return l_Handle;`);
  }
  t += line("}");
  t += empty();

  t += line(
    `void ${p_Type.name}::broadcast_observable(Low::Util::Name p_Observable) const {`,
    n,
  );

  t += line(`Low::Util::ObserverKey l_Key;`);
  t += line(`l_Key.handleId = get_id();`);
  t += line(`l_Key.observableName = p_Observable.m_Index;`);
  t += empty();
  t += line(`Low::Util::notify(l_Key);`);
  t += line("}");
  t += empty();

  t += line(
    `u64 ${p_Type.name}::observe(Low::Util::Name p_Observable, Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)> p_Observer) const {`,
    n,
  );
  t += line(`Low::Util::ObserverKey l_Key;`);
  t += line(`l_Key.handleId = get_id();`);
  t += line(`l_Key.observableName = p_Observable.m_Index;`);
  t += empty();
  t += line(`return Low::Util::observe(l_Key, p_Observer);`);

  t += line("}");
  t += empty();

  t += line(
    `u64 ${p_Type.name}::observe(Low::Util::Name p_Observable, Low::Util::Handle p_Observer) const {`,
    n,
  );

  t += line(`Low::Util::ObserverKey l_Key;`);
  t += line(`l_Key.handleId = get_id();`);
  t += line(`l_Key.observableName = p_Observable.m_Index;`);
  t += empty();
  t += line(`return Low::Util::observe(l_Key, p_Observer);`);

  t += line("}");
  t += empty();

  const l_NotifyMarkerName = `CUSTOM:NOTIFY`;

  const l_NotifyBeginMarker = get_marker_begin(l_NotifyMarkerName);
  const l_NotifyEndMarker = get_marker_end(l_NotifyMarkerName);

  const l_NotifyBeginMarkerIndex = find_begin_marker_end(
    l_OldCode,
    l_NotifyMarkerName,
  );

  let l_NotifyCustomCode = "";

  if (l_NotifyBeginMarkerIndex >= 0) {
    const l_NotifyEndMarkerIndex = find_end_marker_start(
      l_OldCode,
      l_NotifyMarkerName,
    );

    l_NotifyCustomCode = l_OldCode.substring(
      l_NotifyBeginMarkerIndex,
      l_NotifyEndMarkerIndex,
    );
  }
  t += line(
    `void ${p_Type.name}::notify(Low::Util::Handle p_Observed, Low::Util::Name p_Observable) {`,
  );
  t += line(l_NotifyBeginMarker);
  t += l_NotifyCustomCode;
  t += line(l_NotifyEndMarker);
  t += line("}");

  t += empty();

  t += line(
    `void ${p_Type.name}::_notify(Low::Util::Handle p_Observer, Low::Util::Handle p_Observed, Low::Util::Name p_Observable) {`,
  );
  t += line(`${p_Type.name} l_${p_Type.name} = p_Observer.get_id();`);
  t += line(`l_${p_Type.name}.notify(p_Observed, p_Observable);`);
  t += line("}");
  t += empty();

  if (p_Type.reference_counted) {
    t += line(`void ${p_Type.name}::reference(const u64 p_Id) {`);
    t += line(`_LOW_ASSERT(is_alive());`);
    t += empty();
    t += line(`Low::Util::HandleLock<${p_Type.name}> l_HandleLock(get_id());`);
    t += line(`const u32 l_OldReferences = ${l_GetReferences}.size();`);
    t += empty();
    t += line(`${l_GetReferences}.insert(p_Id);`);
    t += empty();
    t += line(`const u32 l_References = ${l_GetReferences}.size();`);
    t += empty();
    t += line(`if (l_OldReferences != l_References) {`);
    if (true) {
      const l_MarkerName = `CUSTOM:NEW_REFERENCE`;

      const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
      const l_CustomEndMarker = get_marker_end(l_MarkerName);

      const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

      let l_CustomCode = "";

      if (l_BeginMarkerIndex >= 0) {
        const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

        l_CustomCode = l_OldCode.substring(
          l_BeginMarkerIndex,
          l_EndMarkerIndex,
        );
      }
      t += line(l_CustomBeginMarker);
      t += l_CustomCode;
      t += line(l_CustomEndMarker);
    }
    t += line("}");
    t += line(`}`);
    t += empty();

    t += line(`void ${p_Type.name}::dereference(const u64 p_Id) {`);
    t += line(`_LOW_ASSERT(is_alive());`);
    t += empty();
    t += line(`Low::Util::HandleLock<${p_Type.name}> l_HandleLock(get_id());`);
    t += line(`const u32 l_OldReferences = ${l_GetReferences}.size();`);
    t += empty();
    t += line(`${l_GetReferences}.erase(p_Id);`);
    t += empty();
    t += line(`const u32 l_References = ${l_GetReferences}.size();`);
    t += empty();
    t += line(`if (l_OldReferences != l_References) {`);
    if (true) {
      const l_MarkerName = `CUSTOM:REFERENCE_REMOVED`;

      const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
      const l_CustomEndMarker = get_marker_end(l_MarkerName);

      const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

      let l_CustomCode = "";

      if (l_BeginMarkerIndex >= 0) {
        const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

        l_CustomCode = l_OldCode.substring(
          l_BeginMarkerIndex,
          l_EndMarkerIndex,
        );
      }
      t += line(l_CustomBeginMarker);
      t += l_CustomCode;
      t += line(l_CustomEndMarker);
    }
    t += line("}");
    t += line(`}`);
    t += empty();

    t += line(`u32 ${p_Type.name}::references() const {`);
    t += line("return get_references().size();");
    t += line(`}`);
    t += empty();
  }

  for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
    if (!i_Prop.no_getter && !i_Prop.no_data) {
      t += line(
        `${i_Prop.accessor_type} ${p_Type.name}::${i_Prop.getter_name}() ${i_Prop.getter_no_const ? "" : "const"}`,
        n,
      );
      t += line("{", n++);
      t += line("_LOW_ASSERT(is_alive());");
      t += line(`Low::Util::HandleLock<${p_Type.name}> l_Lock(get_id());`);
      const l_MarkerName = `CUSTOM:GETTER_${i_PropName}`;

      const i_GetterBeginMarker = get_marker_begin(l_MarkerName);
      const i_GetterEndMarker = get_marker_end(l_MarkerName);

      const i_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

      let i_CustomCode = "";

      if (i_BeginMarkerIndex >= 0) {
        const i_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

        i_CustomCode = l_OldCode.substring(
          i_BeginMarkerIndex,
          i_EndMarkerIndex,
        );
      }
      t += empty();
      t += line(i_GetterBeginMarker);
      t += i_CustomCode;
      t += line(i_GetterEndMarker);
      t += empty();
      t += line(
        `return TYPE_SOA(${p_Type.name}, ${i_Prop.name}, ${i_Prop.soa_type});`,
        n,
      );
      t += line("}", --n);
    }
    if (!i_Prop.no_setter && !i_Prop.no_data) {
      const l_MarkerName = `CUSTOM:SETTER_${i_PropName}`;

      const i_SetterBeginMarker = get_marker_begin(l_MarkerName);
      const i_SetterEndMarker = get_marker_end(l_MarkerName);

      const i_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

      let i_CustomCode = "";

      if (i_BeginMarkerIndex >= 0) {
        const i_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

        i_CustomCode = l_OldCode.substring(
          i_BeginMarkerIndex,
          i_EndMarkerIndex,
        );
      }

      // Type specific special setters
      if (is_string_type(i_Prop.plain_type)) {
        t += line(
          `void ${p_Type.name}::${i_Prop.setter_name}(const char* p_Value){`,
          n,
        );
        t += line(`Low::Util::String l_Val(p_Value);`);
        t += line(`${i_Prop.setter_name}(l_Val);`);
        t += line("}");
        t += empty();
      } else if (i_Prop.plain_type.endsWith("UVector2")) {
        t += line(
          `void ${p_Type.name}::${i_Prop.setter_name}(u32 p_X, u32 p_Y){`,
          n,
        );
        t += line(`Low::Math::UVector2 l_Val(p_X, p_Y);`);
        t += line(`${i_Prop.setter_name}(l_Val);`);
        t += line("}");
        t += empty();

        const l_Coefficients = ["x", "y"];

        for (const it of l_Coefficients) {
          t += line(
            `void ${p_Type.name}::${i_Prop.setter_name}_${it}(u32 p_Value){`,
            n,
          );
          t += line(`Low::Math::UVector2 l_Value = ${i_Prop.getter_name}();`);
          t += line(`l_Value.${it} = p_Value;`);
          t += line(`${i_Prop.setter_name}(l_Value);`);
          t += line("}");
          t += empty();
        }
      } else if (i_Prop.plain_type.endsWith("Vector2")) {
        t += line(
          `void ${p_Type.name}::${i_Prop.setter_name}(float p_X, float p_Y){`,
          n,
        );
        t += line(`Low::Math::Vector2 l_Val(p_X, p_Y);`);
        t += line(`${i_Prop.setter_name}(l_Val);`);
        t += line("}");
        t += empty();

        const l_Coefficients = ["x", "y"];

        for (const it of l_Coefficients) {
          t += line(
            `void ${p_Type.name}::${i_Prop.setter_name}_${it}(float p_Value){`,
            n,
          );
          t += line(`Low::Math::Vector2 l_Value = ${i_Prop.getter_name}();`);
          t += line(`l_Value.${it} = p_Value;`);
          t += line(`${i_Prop.setter_name}(l_Value);`);
          t += line("}");
          t += empty();
        }
      } else if (i_Prop.plain_type.endsWith("Math::Vector3")
        || i_Prop.plain_type.endsWith("Math::ColorRGB")) {
        t += line(
          `void ${p_Type.name}::${i_Prop.setter_name}(float p_X, float p_Y, float p_Z){`,
          n,
        );
        t += line(`Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);`);
        t += line(`${i_Prop.setter_name}(p_Val);`);
        t += line("}");
        t += empty();

        const l_Coefficients = ["x", "y", "z"];

        for (const it of l_Coefficients) {
          t += line(
            `void ${p_Type.name}::${i_Prop.setter_name}_${it}(float p_Value){`,
            n,
          );
          t += line(`Low::Math::Vector3 l_Value = ${i_Prop.getter_name}();`);
          t += line(`l_Value.${it} = p_Value;`);
          t += line(`${i_Prop.setter_name}(l_Value);`);
          t += line("}");
          t += empty();
        }
      } else if (i_Prop.plain_type == "bool") {
        t += line(`void ${p_Type.name}::toggle_${i_PropName}(){`, n);
        t += line(`${i_Prop.setter_name}(!${i_Prop.getter_name}());`);
        t += line("}");
        t += empty();
      }

      t += line(
        `void ${p_Type.name}::${i_Prop.setter_name}(${i_Prop.accessor_type} p_Value)`,
        n,
      );
      t += line("{", n++);
      t += line("_LOW_ASSERT(is_alive());");
      t += line(`Low::Util::HandleLock<${p_Type.name}> l_Lock(get_id());`);
      t += empty();
      if (true) {
        const l_MarkerName = `CUSTOM:PRESETTER_${i_PropName}`;

        const i_SetterBeginMarker = get_marker_begin(l_MarkerName);
        const i_SetterEndMarker = get_marker_end(l_MarkerName);

        const i_BeginMarkerIndex = find_begin_marker_end(
          l_OldCode,
          l_MarkerName,
        );

        let i_CustomCode = "";

        if (i_BeginMarkerIndex >= 0) {
          const i_EndMarkerIndex = find_end_marker_start(
            l_OldCode,
            l_MarkerName,
          );

          i_CustomCode = l_OldCode.substring(
            i_BeginMarkerIndex,
            i_EndMarkerIndex,
          );
        }
        t += line(i_SetterBeginMarker);
        t += i_CustomCode;
        t += line(i_SetterEndMarker);
        t += empty();
      }

      let i_GeneratedWriteLock = false;

      if (i_Prop.dirty_flag) {
        if (!i_Prop.dirty_flag_no_check) {
          t += line(`if (${i_Prop.getter_name}() != p_Value) {`);
        }
        t += line("// Set dirty flags");

        for (var i_Flag of i_Prop.dirty_flag) {
          //t += line(`TYPE_SOA(${p_Type.name}, ${i_Flag}, bool) = true;`, n);
          t += line(`mark_${i_Flag}();`, n);
        }
        t += empty();
      }

      t += line("// Set new value");
      t += line(
        `TYPE_SOA(${p_Type.name}, ${i_Prop.name}, ${i_Prop.soa_type}) = p_Value;`,
        n,
      );
      if (i_Prop.editor_editable) {
        if (p_Type.component && p_Type.name !== "PrefabInstance") {
          t += line("{");
          t += line(`Low::Core::Entity l_Entity = get_entity();`);
          t += line(
            `if (l_Entity.has_component(Low::Core::Component::PrefabInstance::TYPE_ID)) {`,
          );
          t += line(
            `Low::Core::Component::PrefabInstance l_Instance = l_Entity.get_component(Low::Core::Component::PrefabInstance::TYPE_ID);`,
          );
          t += line(`Low::Core::Prefab l_Prefab = l_Instance.get_prefab();`);
          t += line(`if (l_Prefab.is_alive()) {`);
          t += line(
            `l_Instance.override(TYPE_ID, N(${i_Prop.name}), !l_Prefab.compare_property(*this, N(${i_Prop.name}))); `,
          );
          t += line("}");
          t += line("}");
          t += line("}");
        }
      }

      t += empty();
      t += line(i_SetterBeginMarker);
      t += i_CustomCode;
      t += line(i_SetterEndMarker);
      t += empty();
      t += line(`broadcast_observable(N(${i_PropName}));`);
      if (i_Prop.dirty_flag && !i_Prop.dirty_flag_no_check) {
        t += line("}");
      }
      t += line("}", --n);
    }
    t += empty();

    if (i_Prop.is_dirty_flag) {
      t += line(`void ${p_Type.name}::mark_${i_PropName}(){`, n);
      if (!i_Prop.no_data) {
        t += line(`if (!${i_Prop.getter_name}()){`);
        t += line(
          `TYPE_SOA(${p_Type.name}, ${i_Prop.name}, ${i_Prop.soa_type}) = true;`,
          n,
        );
      }

      if (true) {
        const l_MarkerName = `CUSTOM:MARK_${i_PropName}`;

        const i_SetterBeginMarker = get_marker_begin(l_MarkerName);
        const i_SetterEndMarker = get_marker_end(l_MarkerName);

        const i_BeginMarkerIndex = find_begin_marker_end(
          l_OldCode,
          l_MarkerName,
        );

        let i_CustomCode = "";

        if (i_BeginMarkerIndex >= 0) {
          const i_EndMarkerIndex = find_end_marker_start(
            l_OldCode,
            l_MarkerName,
          );

          i_CustomCode = l_OldCode.substring(
            i_BeginMarkerIndex,
            i_EndMarkerIndex,
          );
        }
        t += line(i_SetterBeginMarker);
        t += i_CustomCode;
        t += line(i_SetterEndMarker);
        t += empty();
      }
      if (!i_Prop.no_data) {
        t += line("}");
      }
      t += line("}");
      t += empty();
    }
  }

  if (p_Type.functions) {
    for (let [i_FuncName, i_Func] of Object.entries(p_Type.functions)) {
      const l_MarkerName = `CUSTOM:FUNCTION_${i_FuncName}`;

      const i_FunctionBeginMarker = get_marker_begin(l_MarkerName);
      const i_FunctionEndMarker = get_marker_end(l_MarkerName);

      const i_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

      let i_CustomCode = "";

      if (i_BeginMarkerIndex >= 0) {
        const i_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

        i_CustomCode = l_OldCode.substring(
          i_BeginMarkerIndex,
          i_EndMarkerIndex,
        );
      }
      t += write(`${i_Func.accessor_type} ${p_Type.name}::${i_Func.name} (`);
      if (i_Func.parameters) {
        for (let i = 0; i < i_Func.parameters.length; ++i) {
          if (i > 0) {
            t += write(", ");
          }
          const i_Param = i_Func.parameters[i];
          if (i_Param["const"]) {
            t += write("const ");
          }
          t += write(`${i_Param.accessor_type} ${i_Param.name}`);
        }
      }
      t += write(")");
      if (i_Func.constant) {
        t += write(" const");
      }
      t += line("{");
      if (!i_Func.static) {
        t += line(`Low::Util::HandleLock<${p_Type.name}> l_Lock(get_id());`);
      }
      t += line(i_FunctionBeginMarker);
      t += i_CustomCode;
      t += line(i_FunctionEndMarker);
      t += line("}");
      t += empty();
    }
  }

  t += line(`uint32_t ${p_Type.name}::create_instance(u32& p_PageIndex, u32&p_SlotIndex, Low::Util::UniqueLock<Low::Util::Mutex>& p_PageLock){`);
  t += line(`LOCK_PAGES_WRITE(l_PagesLock);`)
  t += line(`u32 l_Index = 0;`);
  t += line(`u32 l_PageIndex = 0;`)
  t += line(`u32 l_SlotIndex = 0;`)
  t += line(`bool l_FoundIndex = false;`);
  t += line(
    `Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;`);
  t += empty();
  t += line(`for (;!l_FoundIndex && l_PageIndex<ms_Pages.size();++l_PageIndex){`);
  t += line(
    `Low::Util::UniqueLock<Low::Util::Mutex> i_PageLock(ms_Pages[l_PageIndex]->mutex);`);
  t += line(
    `for (l_SlotIndex=0; l_SlotIndex < ms_Pages[l_PageIndex]->size; ++l_SlotIndex) {`);
  t += line(`if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied){`);
  t += line(`l_FoundIndex = true;`);
  t += line(`l_PageLock = std::move(i_PageLock);`)
  t += line(`break;`);
  t += line("}");
  t += line(`l_Index++;`);
  t += line("}");
  t += line(`if (l_FoundIndex) {break;}`)
  t += line("}");
  if (p_Type.dynamic_increase) {
    t += line(`if (!l_FoundIndex) {`);
    t += line(`l_SlotIndex = 0;`);
    t += line(`l_PageIndex = create_page();`);
    t += line(`Low::Util::UniqueLock<Low::Util::Mutex> l_NewLock(ms_Pages[l_PageIndex]->mutex);`);
    t += line(`l_PageLock = std::move(l_NewLock);`);
    t += line("}");
  } else {
    t += line(
      `LOW_ASSERT(l_FoundIndex, "Budget blown for type ${p_Type.name}");`,
    );
  }
  t += line(`ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;`);
  t += line(`p_PageIndex = l_PageIndex;`);
  t += line(`p_SlotIndex = l_SlotIndex;`);
  t += line(`p_PageLock = std::move(l_PageLock);`);
  t += line(`LOCK_UNLOCK(l_PagesLock);`);
  t += line("return l_Index;");
  t += line("}");
  t += empty();

  t += line(`u32 ${p_Type.name}::create_page(){`);
  t += line(`const u32 l_Capacity = get_capacity();`);
  t += line(`LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX, "Could not increase capacity for ${p_Type.name}.");`);
  t += empty();
  t += line(`Low::Util::Instances::Page *l_Page = new Low::Util::Instances::Page;`);
  t += line(`Low::Util::Instances::initialize_page(l_Page, ${p_Type.name}::Data::get_size(), ms_PageSize);`);
  t += line(`ms_Pages.push_back(l_Page);`)
  t += empty();
  t += line(`ms_Capacity = l_Capacity + l_Page->size;`)
  t += line(`return ms_Pages.size()-1;`);
  t += line("}");
  t += empty();
  t += line(`bool ${p_Type.name}::get_page_for_index(const u32 p_Index, u32& p_PageIndex, u32& p_SlotIndex){`);
  t += line(`if (p_Index >= get_capacity()){`);
  t += line(`p_PageIndex = LOW_UINT32_MAX;`)
  t += line(`p_SlotIndex = LOW_UINT32_MAX;`)
  t += line(`return false;`);
  t += line("}");
  t += line(`p_PageIndex = p_Index / ms_PageSize;`);
  t += line(`if (p_PageIndex > (ms_Pages.size() - 1)){return false;}`)
  t += line(`p_SlotIndex = p_Index - (ms_PageSize * p_PageIndex);`);
  t += line(`return true;`);
  t += line("}");

  if (true) {
    t += empty();
    const l_MarkerName = `CUSTOM:NAMESPACE_AFTER_TYPE_CODE`;

    const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
    const l_CustomEndMarker = get_marker_end(l_MarkerName);

    const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

    let l_CustomCode = "";

    if (l_BeginMarkerIndex >= 0) {
      const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

      l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_CustomBeginMarker);
    t += l_CustomCode;
    t += line(l_CustomEndMarker);
    t += empty();
  }

  for (let i_Namespace of p_Type.namespace) {
    t += line("}", --n);
  }

  const l_Formatted = format(p_Type.source_file_path, t);

  if (l_Formatted !== l_OldCode) {
    save_file(p_Type.source_file_path, l_Formatted);

    return true;
  }

  return false;
}

function main() {
  let l_Path = `${__dirname}/../../`;
  let l_Project = false;
  if (process.argv.length > 2) {
    l_Path = process.argv[2];
    l_Project = true;
  }
  if (!l_Path.endsWith("/") && !l_Path.endsWith("\\")) {
    l_Path += "/";
  }

  let l_GeneratedSomething = false;

  let l_Types = 0;
  if (l_Project) {
    l_Types = collect_types_for_project(l_Path);
  } else {
    l_Types = collect_types_for_low(l_Path);
  }

  for (const i_Type of l_Types) {
    const changed_header = generate_header(i_Type);
    const changed_source = generate_source(i_Type);

    if (changed_header || changed_source) {
      let change_string = `${changed_header ? "HEADER" : ""}`;
      if (changed_header && changed_source) {
        change_string += ", ";
      }
      if (changed_source) {
        change_string += "SOURCE";
      }
      l_GeneratedSomething = true;
      console.log(`⚙️ ${i_Type.name} -> ${change_string}`);
    }
  }

  let l_Enums = 0;
  if (l_Project) {
    l_Enums = collect_enums_for_project(l_Path);
  } else {
    l_Enums = collect_enums_for_low(l_Path);
  }

  for (const i_Enum of l_Enums) {
    const changed_header = generate_enum_header(i_Enum);
    const changed_source = generate_enum_source(i_Enum);

    if (changed_header || changed_source) {
      let change_string = `${changed_header ? "HEADER" : ""}`;
      if (changed_header && changed_source) {
        change_string += ", ";
      }
      if (changed_source) {
        change_string += "SOURCE";
      }
      l_GeneratedSomething = true;
      console.log(`⚙️ ${i_Enum.name} -> ${change_string}`);
    }
  }

  if (l_GeneratedSomething) {
    console.log(`✔️ Finished generating type code`);
  } else {
    console.log(`👍 All type code files are up-to-date`);
  }
}

main();
