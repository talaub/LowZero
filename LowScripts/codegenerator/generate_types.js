const fs = require('fs');
const os = require('os');
const exec = require('child_process').execSync;
const YAML = require('yaml');

const g_Directory = `${__dirname}\\..\\..\\data\\_internal\\type_configs`; 
const g_TypeInitializerCppPath = `${__dirname}\\..\\..\\LowUtil\\src\\LowUtilTypeInitializer.cpp`; 
let g_TypeId = 1;

function read_file(p_FilePath) {
    return fs.readFileSync(p_FilePath, {encoding:'utf8', flag:'r'});
}

function save_file(p_FilePath, p_Content) {
    fs.writeFileSync(p_FilePath, p_Content);

    const l_Formatted = exec(`clang-format ${p_FilePath}`);

    fs.writeFileSync(p_FilePath, l_Formatted);
}

function get_marker_begin(p_Name) {
    return `// LOW_CODEGEN:BEGIN:${p_Name}`;
}

function get_marker_end(p_Name) {
    return `// LOW_CODEGEN:END:${p_Name}`;
}

function find_begin_marker_start(p_Text, p_Name) {
    const l_Marker = get_marker_begin(p_Name);

    return p_Text.indexOf(l_Marker);
}

function find_begin_marker_end(p_Text, p_Name) {
    const l_Marker = get_marker_begin(p_Name);

    return p_Text.indexOf(l_Marker) + l_Marker.length;
}

function find_end_marker_start(p_Text, p_Name) {
    const l_Marker = get_marker_end(p_Name);

    return p_Text.indexOf(l_Marker);
}

function find_end_marker_end(p_Text, p_Name) {
    const l_Marker = get_marker_end(p_Name);

    return p_Text.indexOf(l_Marker) + l_Marker.length;
}

function is_reference_type(t) {
    return !([
	'void',
        'int',
        'float',
        'Name',
        'Low::Util::Name',
        'bool',
        'uint8_t',
        'uint16_t',
        'uint32_t',
        'uint64_t',
        'int8_t',
        'int16_t',
        'int32_t',
        'int64_t',
    ].includes(t)) && !t.includes('*');
}

function get_plain_type(p_Type) {
    return p_Type;
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

    t += line('#pragma once');
    t += empty();
    t += include('LowUtilApi.h');
    t += empty();
    t += include('LowUtilHandle.h');
    t += include('LowUtilName.h');
    t += include('LowUtilContainers.h');
    t += empty();

    for (let i_Namespace of p_Type.namespace) {
	t += line(`namespace ${i_Namespace} {`, n++);
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
    t += line('friend void Low::Util::Instances::initialize();');
    t += empty();

    t += line('private:', --n);
    n++;

    t += line('static uint8_t *ms_Buffer;', n);
    t += line('static Low::Util::Instances::Slot *ms_Slots;', n);
    t += empty();
    t += line(`static Low::Util::List<${p_Type.name}> ms_LivingInstances;`, n);
    t += empty();
    t += line(`static void initialize_buffer();`, n);
    t += empty();

    t += line('public:', --n);
    n++;
    t += line('const static uint16_t TYPE_ID;', n);

    t += empty();
    t += line(`${p_Type.name}();`);
    t += line(`${p_Type.name}(uint64_t p_Id);`);
    t += line(`${p_Type.name}(${p_Type.name} &p_Copy);`);
    t += empty();

    t += line(`static ${p_Type.name} make(Low::Util::Name p_Name);`);
    t += line(`void destroy();`);
    
    t += empty();
    t += line('static uint32_t living_count() {');
    t += line('return static_cast<uint32_t>(ms_LivingInstances.size());');
    t += line('}');
    t += line(`static ${p_Type.name} *living_instances() {`);
    t += line('return ms_LivingInstances.data();');
    t += line('}');
    t += empty();

    t += line(`bool is_alive() const;`, n);
    
    t += empty();
    t += line(`static uint32_t get_capacity();`);
    t += empty();

    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	if (!i_Prop.no_getter) {
	    t += line(`${i_Prop.accessor_type}${i_Prop.getter_name}() const;`, n);
	}
	if (!i_Prop.no_setter) {
	    t += line(`void ${i_Prop.setter_name}(${i_Prop.accessor_type}p_Value);`, n);
	}
	t += empty();
    }
    
    t += line('};', --n);

    for (let i_Namespace of p_Type.namespace) {
	t += line('}', --n);
    }

    save_file(p_Type.header_file_path, t);
}

function generate_source(p_Type) {
    let t = '';
    let n = 0;

    t += include(p_Type.header_file_name, n);
    t += empty();
    t += include("LowUtilAssert.h", n);
    t += include("LowUtilLogger.h", n);
    t += include("LowUtilConfig.h", n);
    t += empty();

    for (let i_Namespace of p_Type.namespace) {
	t += line(`namespace ${i_Namespace} {`, n++);
    }

    t += line(`const uint16_t ${p_Type.name}::TYPE_ID = ${g_TypeId++};`, n);
    t += line(`uint8_t *${p_Type.name}::ms_Buffer = 0;`, n);
    t += line(`Low::Util::Instances::Slot *${p_Type.name}::ms_Slots = 0;`, n);
    t += line(`Low::Util::List<${p_Type.name}> ${p_Type.name}::ms_LivingInstances = Low::Util::List<${p_Type.name}>();`, n);
    t += empty();
    t += line(`void ${p_Type.name}::initialize_buffer()`, n);
    t += line('{', n++);
    t += line(`LOW_LOG_DEBUG("Initializing buffer");`, n);
    t += line('}', --n);
    t += empty();

    t += line(`${p_Type.name}::${p_Type.name}(): Low::Util::Handle(0ull){`);
    t += line('}');
    t += line(`${p_Type.name}::${p_Type.name}(uint64_t p_Id): Low::Util::Handle(p_Id){`);
    t += line('}');
    t += line(`${p_Type.name}::${p_Type.name}(${p_Type.name} &p_Copy): Low::Util::Handle(p_Copy.m_Id){`);
    t += line('}');
    
    t += empty();
    t += line(`${p_Type.name} ${p_Type.name}::make(Low::Util::Name p_Name){`);
    t += line(`uint32_t l_Index = Low::Util::Instances::create_instance(ms_Buffer, ms_Slots, get_capacity());`);
    t += empty();
    t += line(`${p_Type.name} l_Handle;`);
    t += line(`l_Handle.m_Data.m_Index = l_Index;`);
    t += line(`l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;`);
    t += line(`l_Handle.m_Data.m_Type = ${p_Type.name}::TYPE_ID;`);
    t += empty();
    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	if (['bool', 'boolean'].includes(i_Prop.type)) {
	    t += line(`ACCESSOR_TYPE_SOA(l_Handle, ${p_Type.name}, ${i_PropName}, ${i_Prop.plain_type}) = false;`);
	}
	if (['Name', 'Low::Util::Name'].includes(i_Prop.type)) {
	    t += line(`ACCESSOR_TYPE_SOA(l_Handle, ${p_Type.name}, ${i_PropName}, ${i_Prop.plain_type}) = Low::Util::Name(0u);`);
	}
    }
    t += empty();
    t += line('l_Handle.set_name(p_Name);');
    t += empty();
    t += line('return l_Handle;');

    t += line('}');

    t += empty();
    t += line(`void ${p_Type.name}::destroy(){`);
    t += line('LOW_ASSERT(is_alive(), "Cannot destroy dead object");');
    t += empty();
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

    t += line(`bool ${p_Type.name}::is_alive() const {`);
    t += line(`return m_Data.m_Type == ${p_Type.name}::TYPE_ID && check_alive(ms_Slots, ${p_Type.name}::get_capacity());`);
    t += line(`}`);
    
    t += empty();
    t += line(`uint32_t ${p_Type.name}::get_capacity(){`);
    t += line('static uint32_t l_Capacity = 0u;');
    t += line('if (l_Capacity == 0u) {');
    t += line(`l_Capacity = Low::Util::Config::get_capacity(N(${p_Type.name}));`);
    t += line('}');
    t += line('return l_Capacity;');
    t += line('}');
    t += empty();

    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	if (!i_Prop.no_getter) {
	    t += line(`${i_Prop.accessor_type}${p_Type.name}::${i_Prop.getter_name}() const`, n);
	    t += line('{', n++);
	    t += line('_LOW_ASSERT(is_alive());');
	    t += line(`return TYPE_SOA(${p_Type.name}, ${i_Prop.name}, ${i_Prop.plain_type});`, n);
	    t += line('}', --n);
	}
	if (!i_Prop.no_setter) {
	    t += line(`void ${p_Type.name}::${i_Prop.setter_name}(${i_Prop.accessor_type}p_Value)`, n);
	    t += line('{', n++);
	    t += line('_LOW_ASSERT(is_alive());');
	    t += line(`TYPE_SOA(${p_Type.name}, ${i_Prop.name}, ${i_Prop.plain_type}) = p_Value;`, n);
	    t += line('}', --n);
	}
	t += empty();
    }
    


    for (let i_Namespace of p_Type.namespace) {
	t += line('}', --n);
    }

    save_file(p_Type.source_file_path, t);
}

function process_file(p_FileName) {
    const l_FileContent = read_file(`${g_Directory}\\${p_FileName}`);

    const l_Config = YAML.parse(l_FileContent);

    const l_Types = [];

    for (let [i_TypeName, i_Type] of Object.entries(l_Config.types)) {
	i_Type.name = i_TypeName;
	i_Type.module = l_Config.module;
	i_Type.namespace = l_Config.namespace;

	i_Type.namespace_string = '';

	if (!i_Type.component) {
	    i_Type.properties['name'] = {
		'type': 'Low::Util::Name'
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

	    i_Prop.plain_type = get_plain_type(i_Prop.type);
	    i_Prop.accessor_type = i_Prop.plain_type + ' ';

	    if (is_reference_type(i_Prop.type) && !i_Prop.accessor) {
		i_Prop.accessor_type += '&';
	    }
	}

	i_Type.module_path = `${__dirname}\\..\\..\\${i_Type.module}`;
	i_Type.header_file_name = `${i_Type.module}${i_Type.name}.h`
	i_Type.header_file_path = `${i_Type.module_path}\\include\\${i_Type.module}${i_Type.name}.h`;
	i_Type.source_file_path = `${i_Type.module_path}\\src\\${i_Type.module}${i_Type.name}.cpp`;
	i_Type.dll_macro = l_Config.dll_macro;

	generate_header(i_Type);
	generate_source(i_Type);

	l_Types.push(i_Type);
    }

    return l_Types;
}

function generate_type_initializer(p_Types) {
    let t = '';

    t += include('LowUtilHandle.h');
    t += empty();
    t += include('LowUtilLogger.h');
    t += include('LowUtilAssert.h');
    t += empty();

    for (let i_Type of p_Types) {
	t += include(`${i_Type.module}${i_Type.name}.h`);
    }
    t += empty();

    t += line('namespace Low{');
    t += line('namespace Util{');
    t += line('namespace Instances{');

    t += line('void initialize() {');

    for (let i_Type of p_Types) {
	t += line(`initialize_buffer(&${i_Type.namespace_string}::${i_Type.name}::ms_Buffer, ${i_Type.namespace_string}::${i_Type.name}Data::get_size(), ${i_Type.namespace_string}::${i_Type.name}::get_capacity(), &${i_Type.namespace_string}::${i_Type.name}::ms_Slots);`);
	t += empty();
    }

    t += line('LOW_LOG_DEBUG("Type buffers initialized");');
    
    t += line('}');
    
    t += line('}');
    t += line('}');
    t += line('}');

    save_file(g_TypeInitializerCppPath, t);
}

function main () {
    const l_FileList = fs.readdirSync(g_Directory);

    const l_Types = [];

    for (let i_FileName of l_FileList) {
	if (i_FileName.endsWith('.types.yaml')) {
	    l_Types.push(...process_file(i_FileName));
	}
    }

    generate_type_initializer(l_Types);
}

main();