const fs = require('fs');
const os = require('os');
const exec = require('child_process').execSync;
const YAML = require('yaml');

const g_Directory = `${__dirname}\\..\\..\\misteda\\data\\_internal\\type_configs`; 
let g_TypeIdMap = {};
const g_AllTypeIds = [];

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

function format(p_FilePath, p_Content) {
    const l_TmpPath = `${p_FilePath}.tmp`;

    fs.writeFileSync(l_TmpPath, p_Content);
    const l_Formatted = exec(`clang-format ${l_TmpPath}`).toString();
    fs.unlinkSync(l_TmpPath);

    return l_Formatted;
}


function get_unused_type_id(){
    let id = 1;

    while (g_AllTypeIds.includes(id)) {
	id++;
    }

    return id;
}

function read_file (p_FilePath) {
    return fs.readFileSync(p_FilePath, {encoding:'utf8', flag:'r'});
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
	i_Type.prefix = l_Config.prefix ? l_Config.prefix : l_Config.module
	i_Type.api_file = `${i_Type.prefix}Api.h`;
	if (l_Config.api_file) {
	    i_Type.api_file = l_Config.api_file;
	}
	i_Type.namespace = l_Config.namespace;
	if (l_Config.scripting_namespace) { 
	    i_Type.scripting_namespace = l_Config.scripting_namespace;
	}
	else {
	    i_Type.scripting_namespace = l_Config.namespace;
	}

	i_Type.scripting_namespace_string = '';
	for (const i_Entry of i_Type.scripting_namespace) {
	    i_Type.scripting_namespace_string += `.${i_Entry}` ;
	}
	i_Type.scripting_namespace_string = i_Type.scripting_namespace_string.substring(1);
	
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
		skip_deserialization: true,
		expose_scripting: true,
		scripting_hide_setter: true
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
		'type': 'Low::Util::Name',
		'expose_scripting': true
	    }
	    if (i_Type.name_editable) {
		i_Type.properties.name.editor_editable = true;
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
	    i_Prop.cs_type = get_cs_type(i_Prop);
	    i_Prop.cs_name = get_cs_field_name(i_PropName);
	    if (i_Prop.soa_type.includes(',')) {
		i_Prop.soa_type = `SINGLE_ARG(${i_Prop.soa_type})`;
	    }
	    i_Prop.accessor_type = get_accessor_type(i_Prop.type, i_Prop.handle);
	    if (i_Prop.no_ref) {
		i_Prop.accessor_type = i_Prop.plain_type;
	    }
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
	i_Type.header_file_name = `${i_Type.prefix}${i_Type.name}.h`
	i_Type.header_file_path = `${i_Type.module_path}\\include\\${i_Type.prefix}${i_Type.name}.h`;
	i_Type.source_file_path = `${i_Type.module_path}\\src\\${i_Type.prefix}${i_Type.name}.cpp`;
	i_Type.scripting_api_file_path = `${__dirname}\\..\\..\\LowScriptingApi\\${i_Type.prefix}${i_Type.name}.cs`;
	if (i_Type.module == 'MistedaPlugin') {
	    i_Type.scripting_api_file_path = `${__dirname}\\..\\..\\LowScriptingApi\\${i_Type.prefix}${i_Type.name}.cs`;
	}
	i_Type.dll_macro = l_Config.dll_macro;

	l_Types.push(i_Type);
    }

    return l_Types;
}

function get_plain_type(p_Type) {
    return p_Type;
}

function get_cs_type(p_Prop) {
    let l_Type = p_Prop.type;

    if (p_Prop.handle) {
	l_Type = l_Type.replace(/::/g, '.');
	let l_TypeName = l_Type.split('.');
	l_TypeName = l_TypeName[l_TypeName.length - 1];
	if (l_Type.includes('Core.Component.')
	    || l_Type.includes('Component.')) {
	    return `Low.${l_TypeName}`;
	}
	if (l_TypeName == 'Entity') {
	    return 'Low.Entity';
	}

	return l_Type;
    }

    if (l_Type.endsWith('Util::Name')) {
	return 'string';
    }
    
    if (l_Type === 'uint64_t') {
	return 'ulong';
    }
    if (l_Type.endsWith('Math::ColorRGB')) {
	return 'Low.Vector3';
    }
    if (l_Type.endsWith('Math::Vector3')) {
	return 'Low.Vector3';
    }
    if (l_Type.endsWith('Math::Vector4')) {
	return 'Low.Vector4';
    }
    if (l_Type.endsWith('Math::Vector2')) {
	return 'Low.Vector2';
    }
    if (l_Type.endsWith('Math::Color')) {
	return 'Low.Vector4';
    }
    if (l_Type.endsWith('Math::Quaternion')) {
	return 'Low.Quaternion';
    }
    return get_plain_type(l_Type);
}

function get_cs_field_name(p_Name) {
    let s = '';
    let cap = false;

    for (let i = 0; i < p_Name.length; ++i) {
	if (p_Name[i] === '_') {
	    cap = true;
	    continue;
	}

	if (cap) {
	    s += p_Name[i].toUpperCase();
	} else {
	    s += p_Name[i];
	}

	cap = false;
    }

    return s;
}

function get_cs_method_name(p_Name) {
    let s = '';
    let cap = true;

    for (let i = 0; i < p_Name.length; ++i) {
	if (p_Name[i] === '_') {
	    cap = true;
	    continue;
	}

	if (cap) {
	    s += p_Name[i].toUpperCase();
	} else {
	    s += p_Name[i];
	}

	cap = false;
    }

    return s;
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

function capitalize_first_letter(p_String) {
  return p_String.charAt(0).toUpperCase() + p_String.slice(1);
}

function get_accessor_type(p_Type, p_Handle) {
    const l_PlainType = get_plain_type(p_Type);
    let l_AccessorType = l_PlainType + ' ';

    if (is_reference_type(p_Type) && !p_Handle) {
	l_AccessorType += '&';
    }

    return l_AccessorType;
}

function removeItemOnce(arr, value) {
  var index = arr.indexOf(value);
  if (index > -1) {
    arr.splice(index, 1);
  }
  return arr;
}

function collect_types() {
    const l_TypeIdContent = read_file(`${g_Directory}/typeids.yaml`);
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

    return l_Types;
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

module.exports = {
    read_file,
    collect_types,
    get_plain_type,
    get_accessor_type,
    is_reference_type,
    capitalize_first_letter,
    removeItemOnce,
    format,
    line,
    write,
    include,
    empty,
    save_file,
    get_marker_begin,
    get_marker_end,
    find_begin_marker_start,
    find_begin_marker_end,
    find_end_marker_start,
    find_end_marker_end,
};
