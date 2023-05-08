const fs = require('fs');
const os = require('os');
const exec = require('child_process').execSync;
const YAML = require('yaml');

const g_Directory = `${__dirname}\\..\\..\\misteda\\data\\_internal\\type_configs`; 
const g_TypeInitializerCppPath = `${__dirname}\\..\\..\\LowUtil\\src\\LowUtilTypeInitializer.cpp`; 
let g_TypeId = 1;

let g_TypeIdMap = {};
const g_AllTypeIds = [];

function get_unused_type_id(){
    let id = 1;

    while (g_AllTypeIds.includes(id)) {
	id++;
    }

    return id;
}

function read_file(p_FilePath) {
    return fs.readFileSync(p_FilePath, {encoding:'utf8', flag:'r'});
}

function format(p_FilePath, p_Content) {
    const l_TmpPath = `${p_FilePath}.tmp`;

    fs.writeFileSync(l_TmpPath, p_Content);
    const l_Formatted = exec(`clang-format ${l_TmpPath}`).toString();
    fs.unlinkSync(l_TmpPath);

    return l_Formatted;
}

function get_deserializer_method_for_math_type(p_Type) {
    if (!is_math_type(p_Type)) {
	console.log("--------- ERROR NOT A MATH TYPE");
    }

    const l_Parts = p_Type.split(':');
    let l_Type = l_Parts[l_Parts.length - 1];

    if (p_Type.endsWith('Color')) {
	l_Type = 'Vector4';
    }
    if (p_Type.endsWith('ColorRGB')) {
	l_Type = 'Vector3';
    }

    return `Low::Util::Serialization::deserialize_${l_Type.toLowerCase()}`;
}

function is_math_type(p_Type) {
    const l_Name = [
	'Vector2',
	'Vector3',
	'Vector4',
	'Color',
	'ColorRGB',
	'Quaternion'
    ];

    const l_Prefixes = [
	'Low::Math::',
	'Math::',
	''
    ];

    for (const i_Name of l_Name) {
	for (const i_Prefix of l_Prefixes) {
	    if (p_Type === `${i_Prefix}${i_Name}`) {
		return true;
	    }
	}
    }

    return false;
}

function is_name_type(p_Type) {
    const l_Name = [
	'Name'
    ];

    const l_Prefixes = [
	'Low::Util::',
	'Util::',
	''
    ];

    for (const i_Name of l_Name) {
	for (const i_Prefix of l_Prefixes) {
	    if (p_Type === `${i_Prefix}${i_Name}`) {
		return true;
	    }
	}
    }

    return false;
}

function is_string_type(p_Type) {
    const l_Name = [
	'String'
    ];

    const l_Prefixes = [
	'Low::Util::',
	'Util::',
	''
    ];

    for (const i_Name of l_Name) {
	for (const i_Prefix of l_Prefixes) {
	    if (p_Type === `${i_Prefix}${i_Name}`) {
		return true;
	    }
	}
    }

    return false;
}

function is_container_type(p_Type) {
    const l_Containers = [
	'List',
	'Array',
	'Map',
	'Set',
	'Queue'
    ];

    const l_Prefixes = [
	'Low::Util::',
	'Util::',
	''
    ];

    for (const i_Container of l_Containers) {
	for (const i_Prefix of l_Prefixes) {
	    if (p_Type.startsWith(`${i_Prefix}${i_Container}<`)) {
		return true;
	    }
	}
    }

    return false;
}

function get_property_type(p_Type) {
    if (p_Type.endsWith('Math::Vector2')) {
	return 'VECTOR2';
    }
    if (p_Type.endsWith('Math::Vector3')) {
	return 'VECTOR3';
    }
    if (p_Type.endsWith('Math::Quaternion')) {
	return 'QUATERNION';
    }
    if (p_Type.endsWith('Util::Name')) {
	return 'NAME';
    }
    if (['bool'].includes(p_Type)) {
	return 'BOOL';
    }
    if (['float'].includes(p_Type)) {
	return 'FLOAT';
    }
    if (['uint32_t'].includes(p_Type)) {
	return 'UINT32';
    }
    if (['int'].includes(p_Type)) {
	return 'INT';
    }
    if (p_Type.endsWith('Util::UniqueId')) {
	return 'UINT64';
    }
    return "UNKNOWN";
}

function save_file(p_FilePath, p_Content) {
    fs.writeFileSync(p_FilePath, p_Content);
}

function get_marker_begin(p_Name) {
    return `// LOW_CODEGEN:BEGIN:${p_Name}`;
}

function get_marker_end(p_Name) {
    return `// LOW_CODEGEN::END::${p_Name}`;
}

function find_begin_marker_start(p_Text, p_Name) {
    const l_Marker = get_marker_begin(p_Name);

    return p_Text.indexOf(l_Marker);
}

function find_begin_marker_end(p_Text, p_Name) {
    const l_Marker = get_marker_begin(p_Name);
    const l_Index = p_Text.indexOf(l_Marker);

    if (l_Index < 0) {
	return -1;
    }

    return l_Index + l_Marker.length + 1;
}

function find_end_marker_start(p_Text, p_Name) {
    const l_Marker = get_marker_end(p_Name);

    return p_Text.indexOf(l_Marker);
}

function find_end_marker_end(p_Text, p_Name) {
    const l_Marker = get_marker_end(p_Name);
    const l_Index = p_Text.indexOf(l_Marker);

    if (l_Index < 0) {
	return -1;
    }

    return l_Index + l_Marker.length;
}

function capitalize_first_letter(p_String) {
  return p_String.charAt(0).toUpperCase() + p_String.slice(1);
}

function is_reference_type(t) {
    return !([
	'void',
        'int',
        'float',
        'Name',
        'Low::Util::Name',
        'Util::Name',
        'bool',
        'uint8_t',
        'uint16_t',
        'uint32_t',
        'uint64_t',
        'int8_t',
        'int16_t',
        'int32_t',
        'int64_t',
        'size_t',
	'Util::UniqueId',
	'Low::Util::UniqueId'
    ].includes(t)) && !t.includes('*');
}

function get_plain_type(p_Type) {
    return p_Type;
}

function get_accessor_type(p_Type, p_Handle) {
    const l_PlainType = get_plain_type(p_Type);
    let l_AccessorType = l_PlainType + ' ';

    if (is_reference_type(p_Type) && !p_Handle) {
	l_AccessorType += '&';
    }

    return l_AccessorType;
}

function line(l, n = 0) {
    let t = ''
    for (let i = 0; i < n; i++) {
        t += '  ';
    }
    return t + `${l}\n`;
}

function write(l, n = 0) {
    let t = ''
    for (let i = 0; i < n; i++) {
        t += '  ';
    }
    return t + `${l}`;
}

function include(p) {
    return line(`#include "${p}"`);

}

function empty() {
    return line('');
}

function write_header_imports(p_Imports) {
}

function generate_header(p_Type) {
    let t = '';
    let n = 0;

    let privatelines = []

    let l_OldCode = '';
    if (fs.existsSync(p_Type.header_file_path)) {
	l_OldCode = read_file(p_Type.header_file_path);
    }

    t += line('#pragma once');
    t += empty();
    t += include(`${p_Type.module}Api.h`);
    t += empty();
    t += include('LowUtilHandle.h');
    t += include('LowUtilName.h');
    t += include('LowUtilContainers.h');
    t += include('LowUtilYaml.h');
    t += empty();
    if (p_Type.component) {
	t += include('LowCoreEntity.h');
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

	let l_CustomCode = '';

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

	let l_CustomCode = '';

	if (l_BeginMarkerIndex >= 0) {
	    const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

	    l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
	}
	t += line(l_CustomBeginMarker);
	t += l_CustomCode;
	t += line(l_CustomEndMarker);
	t += empty();
    }

    t += line(`struct ${p_Type.dll_macro} ${p_Type.name}Data`, n);
    t += line('{', n++);

    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	t += line(`${i_Prop.plain_type} ${i_PropName};`, n);
    }

    t += empty();

    t += line(`static size_t get_size()`, n);
    t += line('{', n++);
    t += line(`return sizeof(${p_Type.name}Data);`, n);
    t += line('}', --n);

    t += line('};', --n);
    t += empty();

    t += line(`struct ${p_Type.dll_macro} ${p_Type.name}: public Low::Util::Handle`, n);
    t += line('{', n++);
    t += line('public:', --n);
    n++;
    t += line('static uint8_t *ms_Buffer;', n);
    t += line('static Low::Util::Instances::Slot *ms_Slots;', n);
    t += empty();
    t += line(`static Low::Util::List<${p_Type.name}> ms_LivingInstances;`, n);
    t += empty();
    t += line('const static uint16_t TYPE_ID;', n);

    t += empty();
    t += line(`${p_Type.name}();`);
    t += line(`${p_Type.name}(uint64_t p_Id);`);
    t += line(`${p_Type.name}(${p_Type.name} &p_Copy);`);
    t += empty();

    if (p_Type.private_make) {
	t += line('private:');
    }
    if (p_Type.component) {
	t += line(`static ${p_Type.name} make(Low::Core::Entity p_Entity);`);
    }
    else {
	t += line(`static ${p_Type.name} make(Low::Util::Name p_Name);`);
    }
    if (p_Type.private_make) {
	t += line('public:');
    }
    t += line(`explicit ${p_Type.name}(const ${p_Type.name}& p_Copy): Low::Util::Handle(p_Copy.m_Id) {`);
    t += line(`}`);
    t += empty();
    t += line(`void destroy();`);
    t += empty();
    t += line(`static void initialize();`);
    t += line(`static void cleanup();`);

    t += empty();
    t += line('static uint32_t living_count() {');
    t += line('return static_cast<uint32_t>(ms_LivingInstances.size());');
    t += line('}');
    t += line(`static ${p_Type.name} *living_instances() {`);
    t += line('return ms_LivingInstances.data();');
    t += line('}');
    t += empty();

    t += line(`static ${p_Type.name} find_by_index(uint32_t p_Index);`);
    t += empty();

    t += line(`bool is_alive() const;`, n);
    
    t += empty();
    t += line(`static uint32_t get_capacity();`);
    t += empty();
    t += line(`void serialize(Low::Util::Yaml::Node& p_Node) const;`, n);
    t += empty();
    if (!p_Type.component) {
	t += line(`static ${p_Type.name} find_by_name(Low::Util::Name p_Name);`, n);
	t += empty();
    }

    t += line(`static void serialize(Low::Util::Handle p_Handle, Low::Util::Yaml::Node& p_Node);`, n);
    t += line(`static Low::Util::Handle deserialize(Low::Util::Yaml::Node& p_Node, Low::Util::Handle p_Creator);`, n);
    t += line('static bool is_alive(Low::Util::Handle p_Handle) {');
    t += line('return p_Handle.check_alive(ms_Slots, get_capacity());');
    t += line('}');
    t += empty();
    t += line('static void destroy(Low::Util::Handle p_Handle) {');
    t += line('_LOW_ASSERT(is_alive(p_Handle));');
    t += line(`${p_Type.name} l_${p_Type.name} = p_Handle.get_id();`);
    t += line(`l_${p_Type.name}.destroy();`);
    t += line('}');
    t += empty();

    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	if (!i_Prop.no_getter) {
	    const l = `${i_Prop.accessor_type}${i_Prop.getter_name}() const;`;
	    if (i_Prop.private_getter) {
		privatelines.push(l);
	    }
	    else {
		t += line(l, n);
	    }
	}
	if (!i_Prop.no_setter) {
	    const l = `void ${i_Prop.setter_name}(${i_Prop.accessor_type}p_Value);`;

	    if (i_Prop.private_setter) {
		privatelines.push(l);
	    }
	    else {
		t += line(l, n);
	    }
	}
	t += empty();
    }

    if (p_Type.functions) {
	for (let [i_FuncName, i_Func] of Object.entries(p_Type.functions)) {
	    let func_line= '';
	    if (i_Func.static) {
		func_line += write('static ');
	    }
	    func_line += write(`${i_Func.accessor_type}${i_Func.name}(`);
	    if (i_Func.parameters) {
		for (let i = 0; i < i_Func.parameters.length; ++i) {
		    if (i > 0) {
			func_line += write(', ');
		    }
		    const i_Param = i_Func.parameters[i];
		    func_line += write(`${i_Param.accessor_type}${i_Param.name}`);
		}
	    }
	    func_line += write(')');
	    if (i_Func.constant) {
		func_line += write(' const');
	    }
	    func_line += line(';');

	    if (i_Func.private) {
		privatelines.push(func_line);
	    }
	    else {
		t += write(func_line);
	    }
	}
    }

    t += line('private:');
    t += line(`static uint32_t ms_Capacity;`);
    t += line(`static uint32_t create_instance();`);
    if (p_Type.dynamic_increase) {
	t += line(`static void increase_budget();`);
    }
    if (privatelines.length) {
	for (const l of privatelines) {
	    t += line(l);
	}
    }
    
    t += line('};', --n);

    for (let i_Namespace of p_Type.namespace) {
	t += line('}', --n);
    }

    const l_Formatted = format(p_Type.header_file_path, t);

    if (l_Formatted !== l_OldCode) {
	save_file(p_Type.header_file_path, l_Formatted);
	return true
    }
    return false
}

function generate_source(p_Type) {
    let t = '';
    let n = 0;

    let l_OldCode = '';
    if (fs.existsSync(p_Type.source_file_path)) {
	l_OldCode = read_file(p_Type.source_file_path);
    }

    t += include(p_Type.header_file_name, n);
    t += empty();
    t += line("#include<algorithm>", n);
    t += empty();
    t += include("LowUtilAssert.h", n);
    t += include("LowUtilLogger.h", n);
    t += include("LowUtilProfiler.h", n);
    t += include("LowUtilConfig.h", n);
    t += include("LowUtilSerialization.h", n);
    t += empty();
    if (p_Type.source_imports) {
	for (const i_Include of p_Type.source_imports) {
	    t += include(i_Include);
	}
	t += empty();
    }

    for (let i_Namespace of p_Type.namespace) {
	t += line(`namespace ${i_Namespace} {`, n++);
    }

    t += line(`const uint16_t ${p_Type.name}::TYPE_ID = ${p_Type.typeId};`, n);
    t += line(`uint32_t ${p_Type.name}::ms_Capacity = 0u;`, n);
    t += line(`uint8_t *${p_Type.name}::ms_Buffer = 0;`, n);
    t += line(`Low::Util::Instances::Slot *${p_Type.name}::ms_Slots = 0;`, n);
    t += line(`Low::Util::List<${p_Type.name}> ${p_Type.name}::ms_LivingInstances = Low::Util::List<${p_Type.name}>();`, n);
    t += empty();

    t += line(`${p_Type.name}::${p_Type.name}(): Low::Util::Handle(0ull){`);
    t += line('}');
    t += line(`${p_Type.name}::${p_Type.name}(uint64_t p_Id): Low::Util::Handle(p_Id){`);
    t += line('}');
    t += line(`${p_Type.name}::${p_Type.name}(${p_Type.name} &p_Copy): Low::Util::Handle(p_Copy.m_Id){`);
    t += line('}');
    
    t += empty();
    if (p_Type.component) {
	t += line(`${p_Type.name} ${p_Type.name}::make(Low::Core::Entity p_Entity){`);
    }
    else {
	t += line(`${p_Type.name} ${p_Type.name}::make(Low::Util::Name p_Name){`);
    }
    t += line(`uint32_t l_Index = create_instance();`);
    t += empty();
    t += line(`${p_Type.name} l_Handle;`);
    t += line(`l_Handle.m_Data.m_Index = l_Index;`);
    t += line(`l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;`);
    t += line(`l_Handle.m_Data.m_Type = ${p_Type.name}::TYPE_ID;`);
    t += empty();
    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	if (['bool', 'boolean'].includes(i_Prop.type)) {
	    t += line(`ACCESSOR_TYPE_SOA(l_Handle, ${p_Type.name}, ${i_PropName}, ${i_Prop.soa_type}) = false;`);
	}
	else if (['Name', 'Low::Util::Name'].includes(i_Prop.type)) {
	    t += line(`ACCESSOR_TYPE_SOA(l_Handle, ${p_Type.name}, ${i_PropName}, ${i_Prop.soa_type}) = Low::Util::Name(0u);`);
	}
	else if (is_reference_type(i_Prop.type)) {
	    t += line(`new (&ACCESSOR_TYPE_SOA(l_Handle, ${p_Type.name}, ${i_PropName}, ${i_Prop.soa_type})) ${i_Prop.plain_type}();`);
	}
    }
    t += empty();
    if (p_Type.component) {
	t += line('l_Handle.set_entity(p_Entity);');
	t += line('p_Entity.add_component(l_Handle);');
	t += empty();
    } else
    {
	t += line('l_Handle.set_name(p_Name);');
	t += empty();
    }
    t += line(`ms_LivingInstances.push_back(l_Handle);`);
    if (p_Type.unique_id) {
	t += empty();
	t += line(`l_Handle.set_unique_id(Low::Util::generate_unique_id(l_Handle.get_id()));`);
	t += line(`Low::Util::register_unique_id(l_Handle.get_unique_id(),l_Handle.get_id());`);
    }
    t += empty();
    const l_MakeMarkerName = `CUSTOM:MAKE`;

    const l_MakeBeginMarker = get_marker_begin(l_MakeMarkerName);
    const l_MakeEndMarker = get_marker_end(l_MakeMarkerName);

    const l_MakeBeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MakeMarkerName);

    let l_MakeCustomCode = '';

    if (l_MakeBeginMarkerIndex >= 0) {
	const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MakeMarkerName);

	l_MakeCustomCode = l_OldCode.substring(l_MakeBeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_MakeBeginMarker);
    t += l_MakeCustomCode;
    t += line(l_MakeEndMarker);
    t += empty();
    t += line('return l_Handle;');

    t += line('}');

    t += empty();
    t += line(`void ${p_Type.name}::destroy(){`);
    t += line('LOW_ASSERT(is_alive(), "Cannot destroy dead object");');
    t += empty();
    const l_MarkerName = `CUSTOM:DESTROY`;

    const l_DestroyBeginMarker = get_marker_begin(l_MarkerName);
    const l_DestroyEndMarker = get_marker_end(l_MarkerName);

    const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

    let l_CustomCode = '';

    if (l_BeginMarkerIndex >= 0) {
	const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

	l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_DestroyBeginMarker);
    t += l_CustomCode;
    t += line(l_DestroyEndMarker);
    t += empty();
    if (p_Type.unique_id) {
	t += line(`Low::Util::remove_unique_id(get_unique_id());`);
	t += empty();
    }
    t += line('ms_Slots[this->m_Data.m_Index].m_Occupied = false;');
    t += line('ms_Slots[this->m_Data.m_Index].m_Generation++;');
    t += empty();
    t += line(`const ${p_Type.name} *l_Instances = living_instances();`);
    t += line(`bool l_LivingInstanceFound = false;`);
    t += line(`for (uint32_t i = 0u; i < living_count(); ++i) {`);
    t += line(`if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {`);
    t += line(`ms_LivingInstances.erase(ms_LivingInstances.begin() + i);`);
    t += line(`l_LivingInstanceFound = true;`);
    t += line(`break;`);
    t += line('}');
    t += line('}');
    t += line(`_LOW_ASSERT(l_LivingInstanceFound);`);
    t += line('}');
    t += empty();
    t += line(`void ${p_Type.name}::initialize() {`);
    t += line(`ms_Capacity = Low::Util::Config::get_capacity(N(${p_Type.module}), N(${p_Type.name}));`);
    t += empty();
    t += line(`initialize_buffer(`);
    t += line(`&ms_Buffer, ${p_Type.name}Data::get_size(), get_capacity(), &ms_Slots`);
    t += line(`);`);
    t += empty();
    t += line(`LOW_PROFILE_ALLOC(type_buffer_${p_Type.name});`);
    t += line(`LOW_PROFILE_ALLOC(type_slots_${p_Type.name});`);
    t += empty();
    t += line(`Low::Util::RTTI::TypeInfo l_TypeInfo;`);
    t += line(`l_TypeInfo.name = N(${p_Type.name});`);
    t += line(`l_TypeInfo.get_capacity = &get_capacity;`);
    t += line(`l_TypeInfo.is_alive = &${p_Type.name}::is_alive;`);
    t += line(`l_TypeInfo.destroy = &${p_Type.name}::destroy;`);
    t += line(`l_TypeInfo.serialize = &${p_Type.name}::serialize;`);
    t += line(`l_TypeInfo.deserialize = &${p_Type.name}::deserialize;`);
    t += line(`l_TypeInfo.get_living_instances = reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(&${p_Type.name}::living_instances);`);
    t += line(`l_TypeInfo.get_living_count = &${p_Type.name}::living_count;`);
    if (p_Type.component) {
	t += line(`l_TypeInfo.component = true;`);
    } else {
	t += line(`l_TypeInfo.component = false;`);
    }
    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	t += line(`{`);
	t += line(`Low::Util::RTTI::PropertyInfo l_PropertyInfo;`);
	t += line(`l_PropertyInfo.name = N(${i_PropName});`);
	t += line(`l_PropertyInfo.editorProperty = ${i_Prop.editor_editable ? 'true' : 'false'};`);
	t += line(`l_PropertyInfo.dataOffset = offsetof(${p_Type.name}Data, ${i_PropName});`);
	if (i_Prop.handle) {
	    t += line(`l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;`);
	    t += line(`l_PropertyInfo.handleType = ${i_Prop.plain_type}::TYPE_ID;`);
	} else {
	    t += line(`l_PropertyInfo.type = Low::Util::RTTI::PropertyType::${get_property_type(i_Prop.plain_type)};`);
	}
	t += line(`l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const* {`);
	if (!i_Prop.no_getter && !i_Prop.private_getter) {
	    t += line(`return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ${p_Type.name}, ${i_PropName},
                                            ${i_Prop.soa_type});`);

	}
	else {
	    t += line(`return nullptr;`);
	}
	t += line(`};`);
	t += line(`l_PropertyInfo.set = [](Low::Util::Handle p_Handle, const void* p_Data) -> void {`);
	if (!i_Prop.no_setter && !i_Prop.private_setter) {
	    t += line(`${p_Type.name} l_Handle = p_Handle.get_id();`);
	    t += line(`l_Handle.${i_Prop.setter_name}(*(${i_Prop.plain_type}*)p_Data);`);
	}
	t += line(`};`);
	t += line(`l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;`);
	t += line(`}`);
    }
    t += line(`Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);`);
    t += line('}');
    t += empty();
    t += line(`void ${p_Type.name}::cleanup() {`);
    t += line(`Low::Util::List<${p_Type.name}> l_Instances = ms_LivingInstances;`);
    t += line(`for (uint32_t i = 0u; i < l_Instances.size(); ++i) {`);
    t += line(`l_Instances[i].destroy();`);
    t += line('}');
    t += line('free(ms_Buffer);');
    t += line('free(ms_Slots);');
    t += empty();
    t += line(`LOW_PROFILE_FREE(type_buffer_${p_Type.name});`);
    t += line(`LOW_PROFILE_FREE(type_slots_${p_Type.name});`);
    t += line('}');
    t += empty();

    t += line(`${p_Type.name} ${p_Type.name}::find_by_index(uint32_t p_Index) {`);
    t += line(`LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");`);
    t += empty();
    t += line(`${p_Type.name} l_Handle;`);
    t += line(`l_Handle.m_Data.m_Index = p_Index;`);
    t += line(`l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;`);
    t += line(`l_Handle.m_Data.m_Type = ${p_Type.name}::TYPE_ID;`);
    t += empty();
    t += line('return l_Handle;');
    t += line('}');
    t += empty();

    t += line(`bool ${p_Type.name}::is_alive() const {`);
    t += line(`return m_Data.m_Type == ${p_Type.name}::TYPE_ID && check_alive(ms_Slots, ${p_Type.name}::get_capacity());`);
    t += line(`}`);

    t += empty();
    t += line(`uint32_t ${p_Type.name}::get_capacity(){`);
    t += line('return ms_Capacity;');
    t += line('}');
    t += empty();
    if (!p_Type.component) {
	t += line(`${p_Type.name} ${p_Type.name}::find_by_name(Low::Util::Name p_Name) {`, n);
	t += line(`for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end(); ++it) {`);
	t += line(`if (it->get_name() == p_Name) {`);
	t += line(`return *it;`);
	t += line('}');
	t += line('}');
	t += line('}');
    }
    t += empty();

    const l_SerializerMarkerName = `CUSTOM:SERIALIZER`;

    const l_SerializerBeginMarker = get_marker_begin(l_SerializerMarkerName);
    const l_SerializerEndMarker = get_marker_end(l_SerializerMarkerName);

    const l_SerializerBeginMarkerIndex =
	  find_begin_marker_end(l_OldCode, l_SerializerMarkerName);

    let l_SerializerCustomCode = '';

    if (l_SerializerBeginMarkerIndex >= 0) {
	const l_SerializerEndMarkerIndex =
	      find_end_marker_start(l_OldCode, l_SerializerMarkerName);

	l_SerializerCustomCode =
	    l_OldCode.substring(l_SerializerBeginMarkerIndex,
					  l_SerializerEndMarkerIndex);
    }
    t += line(`void ${p_Type.name}::serialize(Low::Util::Yaml::Node& p_Node) const {`, n);
    t += line(`_LOW_ASSERT(is_alive());`);
    if (!p_Type.no_auto_serialize) {
	t += empty();
	for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	    if (i_Prop.skip_serialization) {
		continue;
	    }
	    if (is_name_type(i_Prop.plain_type) || is_string_type(i_Prop.plain_type)) {
		t += line(`p_Node["${i_PropName}"] = ${i_Prop.getter_name}().c_str();`);
	    }
	    else if (['int', 'uint32_t', 'uint8_t', 'uint16_t', 'uint64_t']
		     .includes(i_Prop.plain_type) || i_Prop.plain_type.endsWith('Util::UniqueId')) {
		t += line(`p_Node["${i_PropName}"] = ${i_Prop.getter_name}();`);
	    }
	    else if (['float', 'double']
		    .includes(i_Prop.plain_type)) {
		t += line(`p_Node["${i_PropName}"] = ${i_Prop.getter_name}();`);
	    }
	    else if (['bool']
		    .includes(i_Prop.plain_type)) {
		t += line(`p_Node["${i_PropName}"] = ${i_Prop.getter_name}();`);
	    }
	    else if (is_math_type(i_Prop.plain_type)) {
		t += line(`Low::Util::Serialization::serialize(p_Node["${i_PropName}"], ${i_Prop.getter_name}());`);
	    }
	    else if (i_Prop.handle) {
		t += line(`${i_Prop.getter_name}().serialize(p_Node["${i_PropName}"]);`);
	    }
	}
    }

    t += empty();
    t += line(l_SerializerBeginMarker);
    t += l_SerializerCustomCode;
    t += line(l_SerializerEndMarker);
    t += line('}');
    t += empty();
    t += line(`void ${p_Type.name}::serialize(Low::Util::Handle p_Handle, Low::Util::Yaml::Node& p_Node) {`, n);
    t += line(`${p_Type.name} l_${p_Type.name} = p_Handle.get_id();`);
    t += line(`l_${p_Type.name}.serialize(p_Node);`);
    t += line('}');
    t += empty();

    const l_DeserializerMarkerName = `CUSTOM:DESERIALIZER`;

    const l_DeserializerBeginMarker = get_marker_begin(l_DeserializerMarkerName);
    const l_DeserializerEndMarker = get_marker_end(l_DeserializerMarkerName);

    const l_DeserializerBeginMarkerIndex =
	  find_begin_marker_end(l_OldCode, l_DeserializerMarkerName);

    let l_DeserializerCustomCode = '';

    if (l_DeserializerBeginMarkerIndex >= 0) {
	const l_DeserializerEndMarkerIndex =
	      find_end_marker_start(l_OldCode, l_DeserializerMarkerName);

	l_DeserializerCustomCode =
	    l_OldCode.substring(l_DeserializerBeginMarkerIndex,
					  l_DeserializerEndMarkerIndex);
    }
    t += line(`Low::Util::Handle ${p_Type.name}::deserialize(Low::Util::Yaml::Node& p_Node, Low::Util::Handle p_Creator) {`, n);
    if (!p_Type.no_auto_deserialize) {
	if (p_Type.component) {
	    t += line(`${p_Type.name} l_Handle = ${p_Type.name}::make(p_Creator.get_id());`);
	} else {
	    t += line(`${p_Type.name} l_Handle = ${p_Type.name}::make(N(${p_Type.name}));`);
	}
	t += empty();

	if (p_Type.unique_id) {
	    t += line(`Low::Util::remove_unique_id(l_Handle.get_unique_id());`);
	    t += line(`l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());`);
	    t += line(`Low::Util::register_unique_id(l_Handle.get_unique_id(), l_Handle.get_id());`);
	    t += empty();
	}

	for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	    if (i_Prop.skip_deserialization) {
		continue;
	    }

	    if (is_name_type(i_Prop.plain_type)) {
		t += line(`l_Handle.${i_Prop.setter_name}(LOW_YAML_AS_NAME(p_Node["${i_PropName}"]));`);
	    }
	    else if (is_string_type(i_Prop.plain_type)) {
		t += line(`l_Handle.${i_Prop.setter_name}(LOW_YAML_AS_STRING(p_Node["${i_PropName}"]));`);
	    }
	    else if (is_math_type(i_Prop.plain_type)) {
		t += line(`l_Handle.${i_Prop.setter_name}(${get_deserializer_method_for_math_type(i_Prop.plain_type)}(p_Node["${i_PropName}"]));`);
	    }
	    else if (['bool', 'float', 'int', 'double', 'uint64_t', 'uint32_t', 'uint8_t', 'uint16_t'].includes(i_Prop.plain_type) || i_Prop.plain_type.endsWith('Util::UniqueId')) {
		t += line(`l_Handle.${i_Prop.setter_name}(p_Node["${i_PropName}"].as<${i_Prop.plain_type}>());`);
	    }
	    else if (i_Prop.handle) {
		t += line(`l_Handle.${i_Prop.setter_name}(${i_Prop.plain_type}::deserialize(p_Node["${i_PropName}"], l_Handle.get_id()).get_id());`);
	    }
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
    t += line('}');
    t += empty();

    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	if (!i_Prop.no_getter) {
	    t += line(`${i_Prop.accessor_type}${p_Type.name}::${i_Prop.getter_name}() const`, n);
	    t += line('{', n++);
	    t += line('_LOW_ASSERT(is_alive());');
	    t += line(`return TYPE_SOA(${p_Type.name}, ${i_Prop.name}, ${i_Prop.soa_type});`, n);
	    t += line('}', --n);
	}
	if (!i_Prop.no_setter) {
	    const l_MarkerName = `CUSTOM:SETTER_${i_PropName}`;

	    const i_SetterBeginMarker = get_marker_begin(l_MarkerName);
	    const i_SetterEndMarker = get_marker_end(l_MarkerName);

	    const i_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

	    let i_CustomCode = '';

	    if (i_BeginMarkerIndex >= 0) {
		const i_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);
		
		i_CustomCode = l_OldCode.substring(i_BeginMarkerIndex, i_EndMarkerIndex);
	    }
	    
	    t += line(`void ${p_Type.name}::${i_Prop.setter_name}(${i_Prop.accessor_type}p_Value)`, n);
	    t += line('{', n++);
	    t += line('_LOW_ASSERT(is_alive());');
	    t += empty();
	    if (true) {
		const l_MarkerName = `CUSTOM:PRESETTER_${i_PropName}`;

		const i_SetterBeginMarker = get_marker_begin(l_MarkerName);
		const i_SetterEndMarker = get_marker_end(l_MarkerName);

		const i_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

		let i_CustomCode = '';

		if (i_BeginMarkerIndex >= 0) {
		    const i_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

		    i_CustomCode = l_OldCode.substring(i_BeginMarkerIndex, i_EndMarkerIndex);
		}
		t += line(i_SetterBeginMarker);
		t += i_CustomCode;
		t += line(i_SetterEndMarker);
		t += empty();

	    }
	    if (i_Prop.dirty_flag) {
		t += line(`if (${i_Prop.getter_name}() != p_Value) {`);
		t += line('// Set dirty flags');
		for (var i_Flag of i_Prop.dirty_flag) {
		    t += line(`TYPE_SOA(${p_Type.name}, ${i_Flag}, bool) = true;`, n);
		}
		t += empty();
	    }
	    t += line('// Set new value');
	    t += line(`TYPE_SOA(${p_Type.name}, ${i_Prop.name}, ${i_Prop.soa_type}) = p_Value;`, n);
	    t += empty();
	    t += line(i_SetterBeginMarker);
	    t += i_CustomCode;
	    t += line(i_SetterEndMarker);
	    if (i_Prop.dirty_flag) {
		t += line('}');
	    }
	    t += line('}', --n);
	}
	t += empty();
    }

    if (p_Type.functions) {
	for (let [i_FuncName, i_Func] of Object.entries(p_Type.functions)) {
	    const l_MarkerName = `CUSTOM:FUNCTION_${i_FuncName}`;

	    const i_FunctionBeginMarker = get_marker_begin(l_MarkerName);
	    const i_FunctionEndMarker = get_marker_end(l_MarkerName);

	    const i_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

	    let i_CustomCode = '';

	    if (i_BeginMarkerIndex >= 0) {
		const i_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);
		
		i_CustomCode = l_OldCode.substring(i_BeginMarkerIndex, i_EndMarkerIndex);
	    }
	    t += write(`${i_Func.accessor_type}${p_Type.name}::${i_Func.name}(`);
	    if (i_Func.parameters) {
		for (let i = 0; i < i_Func.parameters.length; ++i) {
		    if (i > 0) {
			t += write(', ');
		    }
		    const i_Param = i_Func.parameters[i];
		    t += write(`${i_Param.accessor_type}${i_Param.name}`);
		}
	    }
	    t += write(')');
	    if (i_Func.constant) {
		t += write(' const');
	    }
	    t += line('{');
	    t += line(i_FunctionBeginMarker);
	    t += i_CustomCode;
	    t += line(i_FunctionEndMarker);
	    t += line('}');
	    t += empty();
	}
    }

    t += line(`uint32_t ${p_Type.name}::create_instance(){`);
    t += line(`uint32_t l_Index = 0u;`);
    t += empty();
    t += line(`for (;l_Index<get_capacity();++l_Index){`);
    t += line(`if (!ms_Slots[l_Index].m_Occupied){`);
    t += line(`break;`);
    t += line('}');
    t += line('}');
    if (p_Type.dynamic_increase) {
	t += line(`if (l_Index >= get_capacity()) {`);
	t += line(`increase_budget();`);
	t += line('}');
    }
    else {
	t += line(`LOW_ASSERT(l_Index < get_capacity(), "Budget blown for type ${p_Type.name}");`);
    }
    t += line(`ms_Slots[l_Index].m_Occupied = true;`);
    t += line('return l_Index;');
    t += line('}');
    t += empty();

    if (p_Type.dynamic_increase) {
	t += line(`void ${p_Type.name}::increase_budget(){`);
	t += line(`uint32_t l_Capacity = get_capacity();`);
	t += line(`uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);`);
	t += line(`l_CapacityIncrease = std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);`);
	t += empty();
	t += line(`LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");`);
	t += empty();
	t += line(`uint8_t *l_NewBuffer = (uint8_t*) malloc((l_Capacity + l_CapacityIncrease) * sizeof(${p_Type.name}Data));`);
	t += line(`Low::Util::Instances::Slot *l_NewSlots = (Low::Util::Instances::Slot*) malloc((l_Capacity + l_CapacityIncrease) * sizeof(Low::Util::Instances::Slot));`);
	t += empty();
	t += line(`memcpy(l_NewSlots, ms_Slots, l_Capacity * sizeof(Low::Util::Instances::Slot));`);
	for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	    t += line('{');
	    if (is_container_type(i_Prop.plain_type)) {
		t += line(`for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end(); ++it) {`);
		t += line(`auto* i_ValPtr = new (&l_NewBuffer[offsetof(${p_Type.name}Data, ${i_PropName})*(l_Capacity + l_CapacityIncrease) + (it->get_index() * sizeof(${i_Prop.plain_type}))]) ${i_Prop.plain_type}();`);
		t += line(`*i_ValPtr = it->${i_Prop.getter_name}();`);
		t += line('}');
	    } else {
		t += line(`memcpy(&l_NewBuffer[offsetof(${p_Type.name}Data, ${i_PropName})*(l_Capacity + l_CapacityIncrease)], &ms_Buffer[offsetof(${p_Type.name}Data, ${i_PropName})*(l_Capacity)], l_Capacity * sizeof(${i_Prop.plain_type}));`);
	    }
	    t += line('}');
	}
	t += line(`for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease;++i) {`);
	t += line(`l_NewSlots[i].m_Occupied = false;`);
	t += line(`l_NewSlots[i].m_Generation = 0;`);
	t += line('}');
	t += line('free(ms_Buffer);');
	t += line('free(ms_Slots);');
	t += line('ms_Buffer = l_NewBuffer;');
	t += line('ms_Slots = l_NewSlots;');
	t += line(`ms_Capacity = l_Capacity + l_CapacityIncrease;`);
	t += empty();
	t += line(`LOW_LOG_DEBUG << "Auto-increased budget for ${p_Type.name} from " << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;`);
	t += line('}');
    }
    
    for (let i_Namespace of p_Type.namespace) {
	t += line('}', --n);
    }

    const l_Formatted = format(p_Type.source_file_path, t);

    if (l_Formatted !== l_OldCode) {
	save_file(p_Type.source_file_path, l_Formatted);

	return true;
    }

    return false
}

function process_file(p_FileName) {
    const l_FileContent = read_file(`${g_Directory}\\${p_FileName}`);

    const l_Config = YAML.parse(l_FileContent);

    if (!g_TypeIdMap[l_Config.module]) {
	g_TypeIdMap[l_Config.module] = {};
    }

    const l_Types = [];

    for (let [i_TypeName, i_Type] of Object.entries(l_Config.types)) {
	i_Type.name = i_TypeName;
	i_Type.module = l_Config.module;
	i_Type.namespace = l_Config.namespace;
	i_Type.typeId = 0;
	if (g_TypeIdMap[l_Config.module][i_TypeName]) {
	    i_Type.typeId = g_TypeIdMap[l_Config.module][i_TypeName];
	}
	else {
	    i_Type.typeId = get_unused_type_id();
	    g_AllTypeIds.push(i_Type.typeId);
	    g_TypeIdMap[l_Config.module][i_TypeName] = i_Type.typeId;
	}

	if (i_Type.dynamic_increase === undefined) {
	    i_Type.dynamic_increase = true;
	}

	i_Type.namespace_string = '';

	if (i_Type.component) {
	    i_Type.properties['entity'] = {
		type: 'Low::Core::Entity',
		handle: true,
		skip_serialization: true,
		skip_deserialization: true
	    }

	    i_Type.unique_id = true;
	}

	if (i_Type.unique_id) {
	    i_Type.properties['unique_id'] = {
		type: 'Low::Util::UniqueId',
		private_setter: true
	    }
	}

	const l_DirtyFlags = [];
	for (let [i_PropName, i_Prop] of Object.entries(i_Type.properties)) {
	    if (!i_Prop.dirty_flag) {
		continue;
	    }
	    if (Array.isArray(i_Prop.dirty_flag)) {
		for (let i_Flag of i_Prop.dirty_flag) {
		    if (!l_DirtyFlags.includes(i_Flag)) {
			l_DirtyFlags.push(i_Flag);
		    }
		}
	    }
	    else if (!l_DirtyFlags.includes(i_Prop.dirty_flag)) {
		l_DirtyFlags.push(i_Prop.dirty_flag);
	    }
	}


	i_Type.dirty_flags = l_DirtyFlags;

	for (let i_Flag of i_Type.dirty_flags) {
	    i_Type.properties[i_Flag] = {
		type: 'bool',
		skip_serialization: true,
		skip_deserialization: true
	    }
	}

	if (!i_Type.component) {
	    i_Type.properties['name'] = {
		'type': 'Low::Util::Name'
	    }
	    if (i_Type.skip_name_serialization) {
		i_Type.properties['name']['skip_serialization'] = true;
	    }
	    if (i_Type.skip_name_deserialization) {
		i_Type.properties['name']['skip_deserialization'] = true;
	    }
	}

	for (let i = 0; i < i_Type.namespace.length; ++i) {
	    if (i) {
		i_Type.namespace_string += '::';
	    }
	    i_Type.namespace_string += i_Type.namespace[i];
	}

	for (let [i_PropName, i_Prop] of Object.entries(i_Type.properties)) {
	    i_Prop.name = i_PropName;

	    if (!i_Prop.getter_name) {
		i_Prop.getter_name = `get_${i_PropName}`;

		if (['bool', 'boolean'].includes(i_Prop.type)) {
		    i_Prop.getter_name = `is_${i_PropName}`;
		}
	    }
	    if (!i_Prop.setter_name) {
		i_Prop.setter_name = `set_${i_PropName}`;
	    }

	    if (i_Prop.dirty_flag && !Array.isArray(i_Prop.dirty_flag)) {
		i_Prop.dirty_flag = [i_Prop.dirty_flag];
	    }

	    i_Prop.plain_type = get_plain_type(i_Prop.type);
	    i_Prop.soa_type = i_Prop.plain_type;
	    if (i_Prop.soa_type.includes(',')) {
		i_Prop.soa_type = `SINGLE_ARG(${i_Prop.soa_type})`;
	    }
	    i_Prop.accessor_type = get_accessor_type(i_Prop.type, i_Prop.handle);
	}

	if (i_Type.functions) {
	    for (let [i_FuncName, i_Func] of Object.entries(i_Type.functions)) {
		i_Func.accessor_type = get_accessor_type(i_Func.return_type, i_Func.return_handle);
		i_Func.name = i_FuncName;
		if (i_Func.parameters) {
		    for (let i_Param of i_Func.parameters) {
			i_Param.accessor_type = get_accessor_type(i_Param.type, i_Param.handle);
			i_Param.name = `p_${capitalize_first_letter(i_Param.name)}`;
		    }
		}
	    }
	}

	i_Type.module_path = `${__dirname}\\..\\..\\${i_Type.module}`;
	i_Type.header_file_name = `${i_Type.module}${i_Type.name}.h`
	i_Type.header_file_path = `${i_Type.module_path}\\include\\${i_Type.module}${i_Type.name}.h`;
	i_Type.source_file_path = `${i_Type.module_path}\\src\\${i_Type.module}${i_Type.name}.cpp`;
	i_Type.dll_macro = l_Config.dll_macro;

	const changed_header = generate_header(i_Type);
	const changed_source = generate_source(i_Type);

	if (changed_header || changed_source) {
	    let change_string = `${changed_header ? 'HEADER' : ''}`;
	    if (changed_header && changed_source) {
		change_string += ', ';
	    }
	    if (changed_source) {
		change_string += 'SOURCE';
	    }
	    console.log(`${i_Type.name} -> ${change_string}`);
	}

	l_Types.push(i_Type);
    }

    return l_Types;
}

function removeItemOnce(arr, value) {
  var index = arr.indexOf(value);
  if (index > -1) {
    arr.splice(index, 1);
  }
  return arr;
}

function main () {
    const l_TypeIdContent = read_file(`codegenerator/typeids.yaml`);
    g_TypeIdMap = YAML.parse(l_TypeIdContent);
    for (const [key, value] of Object.entries(g_TypeIdMap)) {
	for (const [k, v] of Object.entries(value)) {
	    g_AllTypeIds.push(v);
	}
    }

    const l_FileList = fs.readdirSync(g_Directory);

    const l_Types = [];

    const l_TypeFiles = [];

    const l_Order = [
	'lowrenderer_interface',
	'lowrenderer_resources',
	'lowrenderer',
	'lowcore_base',
	'lowcore_resources'
    ];

    for (let i_FileName of l_FileList) {
	if (i_FileName.endsWith('.types.yaml')) {
	    l_TypeFiles.push(i_FileName);
	}
    }

    for (let i_OrderItem of l_Order) {
	for (let i_FileName of l_TypeFiles) {
	    if (i_FileName.endsWith(`${i_OrderItem}.types.yaml`)) {
		l_Types.push(...process_file(i_FileName));
		removeItemOnce(l_TypeFiles, i_FileName);
		break;
	    }
	}
    }

    for (let i_FileName of l_TypeFiles) {
	l_Types.push(...process_file(i_FileName));
    }

    fs.writeFileSync("codegenerator/typeids.yaml", YAML.stringify(g_TypeIdMap));

    // generate_type_initializer(l_Types);
}

main();
