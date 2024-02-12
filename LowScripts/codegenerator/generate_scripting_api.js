const fs = require('fs');
const os = require('os');
const exec = require('child_process').execSync;
const YAML = require('yaml');
var CRC32 = require("crc-32");

const {
    read_file,
    collect_types,
    write,
    line,
    empty,
    format,
    save_file,
    find_begin_marker_start,
    find_begin_marker_end,
    find_end_marker_start,
    get_marker_begin,
    get_marker_end,
    find_end_marker_end,
    g_Directory,
} = require('./lib.js')

function hash_name(p_String) {
    var n = CRC32.str(p_String);
    var uint32 = n >>> 0;

    return uint32;
}

function internal_get(p_Prop) {
    let l_Type = p_Prop.cs_type;

    if (p_Prop.handle) {
	return `Low.Internal.Handle.GetHandleValue<${l_Type}>`;
    }

    if (p_Prop.plain_type.endsWith('Util::Name')) {
	return `Low.Internal.HandleHelper.GetNameValue`;
    }
    
    if (l_Type === 'ulong') {
	return `Low.Internal.HandleHelper.GetUlongValue`;
    }
    if (l_Type === 'float') {
	return `Low.Internal.HandleHelper.GetFloatValue`;
    }
    if (l_Type === 'Low.Vector3') {
	return `Low.Internal.HandleHelper.GetVector3Value`;
    }
    if (l_Type === 'Low.Vector2') {
	return `Low.Internal.HandleHelper.GetVector2Value`;
    }
    if (l_Type === 'Low.Vector4') {
	return `Low.Internal.HandleHelper.GetVector4Value`;
    }
    if (l_Type === 'Low.Quaternion') {
	return `Low.Internal.HandleHelper.GetQuaternionValue`;
    }
    if (l_Type === 'Low.Quaternion') {
	return `Low.Internal.HandleHelper.GetQuaternionValue`;
    }
    
    return `Low.Internal.HandleHelper.GetIntValue`;
}

function internal_set(p_Prop) {
    let l_Type = p_Prop.cs_type;

    if (p_Prop.handle) {
	return `Low.Internal.Handle.SetHandleValue<${l_Type}>`;
    }

    if (p_Prop.plain_type.endsWith('Util::Name')) {
	return `Low.Internal.HandleHelper.SetNameValue`;
    }

    if (l_Type === 'ulong') {
	return `Low.Internal.HandleHelper.SetUlongValue`;
    }
    if (l_Type === 'float') {
	return `Low.Internal.HandleHelper.SetFloatValue`;
    }
    if (l_Type === 'Low.Vector3') {
	return `Low.Internal.HandleHelper.SetVector3Value`;
    }
    if (l_Type === 'Low.Vector2') {
	return `Low.Internal.HandleHelper.SetVector2Value`;
    }
    if (l_Type === 'Low.Vector4') {
	return `Low.Internal.HandleHelper.SetVector4Value`;
    }
    if (l_Type === 'Low.Quaternion') {
	return `Low.Internal.HandleHelper.SetQuaternionValue`;
    }
    
    return `Low.Internal.HandleHelper.SetIntValue`;
}

function generate_scripting_api(p_Type) {
    let t = '';

    const l_TypeString = `${p_Type.namespace_string}::${p_Type.name}`

    t += line(`static void register_${p_Type.module.toLowerCase()}_${p_Type.name.toLowerCase()}(){`)
    t += line(`using namespace Low;`)
    t += line(`using namespace Low::Core;`)
    if (!['Low::Core'].includes(p_Type.namespace_string)) {
	t += line(`using namespace ${p_Type.namespace_string};`)
    }
    t += empty()

    t += line(`Cflat::Namespace *l_Namespace = Scripting::get_environment()->requestNamespace("${p_Type.namespace_string}");`)
    t += empty()

    /*
    t += line(`CflatRegisterStruct(l_Namespace, ${p_Type.name});`)
    t += line(`CflatStructAddBaseType(Scripting::get_environment(), ${l_TypeString}, Low::Util::Handle);`)
    */
    t += line(`Cflat::Struct *type = Low::Core::Scripting::g_CflatStructs["${l_TypeString}"];`);
    t += empty()

    t += line('{');
    t += line(`Cflat::Namespace *l_UtilNamespace = Scripting::get_environment()->requestNamespace("Low::Util");`)
    t += empty()
    t += line(`CflatRegisterSTLVectorCustom(l_UtilNamespace, Low::Util::List, ${l_TypeString});`)
    t += line('}');
    t += empty()


    t += line(`CflatStructAddMethodReturn(l_Namespace, ${p_Type.name}, bool, is_alive);`)

    t += line(`CflatStructAddStaticMethodReturn(l_Namespace, ${p_Type.name}, uint32_t, get_capacity);`)
    t += line(`CflatStructAddStaticMethodReturnParams1(l_Namespace, ${p_Type.name}, ${l_TypeString}, find_by_index, uint32_t);`)
    if (!p_Type.component && !p_Type.ui_component) {
	t += line(`CflatStructAddStaticMethodReturnParams1(l_Namespace, ${p_Type.name}, ${l_TypeString}, find_by_name, Low::Util::Name);`)
    }
    t += empty()

    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	if (!i_Prop.expose_scripting) {
	    continue;
	}
	if (!i_Prop.static) {
	    if (!i_Prop.no_getter && !i_Prop.private_getter) {
		t += line(`CflatStructAddMethodReturn(l_Namespace, ${p_Type.name}, ${i_Prop.accessor_type}, ${i_Prop.getter_name});`);
	    }

	    if (!i_Prop.no_setter && !i_Prop.private_setter) {
		t += line(`CflatStructAddMethodVoidParams1(l_Namespace, ${p_Type.name}, void, ${i_Prop.setter_name}, ${i_Prop.accessor_type});`);
	    }
	}
	t += empty()
    }

    if (p_Type.functions) {
	for (let [i_FuncName, i_Func] of Object.entries(p_Type.functions)) {
	    if (!i_Func.expose_scripting) {
		continue;
	    }
	    if (i_Func.parameters && i_Func.parameters.length > 5) {
		continue;
	    }
	    
	    if (i_Func.static) {
	    }
	    else {
		t += write(`CflatStructAddMethod${i_Func.return_type == 'void' ? 'Void' : 'Return'}`);
		if (i_Func.parameters) {
		    t += write(`Params${i_Func.parameters.length}`);
		}
		t += write('(');
		t += write(`l_Namespace, ${p_Type.name}, ${i_Func.return_type}, ${i_FuncName}`);
		if (i_Func.parameters) {
		    for (let i_Param of i_Func.parameters) {
			t += write(`, ${i_Param.type}`);
		    }
		}
		t += write(');\n');
	    }
	}
    }

    t += line(`}`)
    t += empty()

    return t
}

function main() {
    const l_Types = collect_types();

    const l_FilePath = `../../LowCore/src/LowCoreCflatScripting.cpp`

    const l_OriginalContent = read_file(l_FilePath);

    const l_IndexStart = l_OriginalContent.indexOf(`// REGISTER_CFLAT_BEGIN`);
    const l_IndexEnd = l_OriginalContent.indexOf(`// REGISTER_CFLAT_END`);

    const l_PreContent = l_OriginalContent.substring(0, l_IndexStart);
    const l_PostContent = l_OriginalContent.substring(l_IndexEnd + `// REGISTER_CFLAT_END`.length);

    let l_IncludeCode = '';
    let l_RegisterCode = '';
    let l_EntryMethodCode = line(`static void register_types() {`)
    l_EntryMethodCode += line(`preregister_types();`)
    l_EntryMethodCode += empty()

    let l_PreRegisterCode = line(`static void preregister_types() {`)
    l_PreRegisterCode += line(`using namespace Low::Core;`)
    l_PreRegisterCode += empty()

    for (const i_Type of l_Types) {
	if (i_Type.scripting_expose) {
	    l_IncludeCode += line(`#include "${i_Type.header_file_name}"`);

	    l_RegisterCode += generate_scripting_api(i_Type);
	    l_EntryMethodCode += line(`register_${i_Type.module.toLowerCase()}_${i_Type.name.toLowerCase()}();`)
	    l_PreRegisterCode += line(`{`);
	    l_PreRegisterCode += line(`using namespace ${i_Type.namespace_string};`);
	    l_PreRegisterCode += line(`Cflat::Namespace* l_Namespace = Scripting::get_environment()->requestNamespace("${i_Type.namespace_string}");`);
	    l_PreRegisterCode += empty();
	    l_PreRegisterCode += line(`CflatRegisterStruct(l_Namespace, ${i_Type.name});`);
	    l_PreRegisterCode += line(`CflatStructAddBaseType(Scripting::get_environment(), ${i_Type.namespace_string}::${i_Type.name}, Low::Util::Handle);`);
	    l_PreRegisterCode += empty();
	    l_PreRegisterCode += line(`Scripting::g_CflatStructs["${i_Type.namespace_string}::${i_Type.name}"] = type;`);
	    l_PreRegisterCode += line(`}`);
	    l_PreRegisterCode += empty();

	}
    }

    l_EntryMethodCode += line('}');
    l_PreRegisterCode += line('}');

    let l_FinalContent = l_PreContent + `// REGISTER_CFLAT_BEGIN\n`
    l_FinalContent += l_IncludeCode
    l_FinalContent += empty()
    l_FinalContent += l_RegisterCode
    l_FinalContent += l_PreRegisterCode
    l_FinalContent += l_EntryMethodCode
    l_FinalContent += `// REGISTER_CFLAT_END` + l_PostContent

    const l_Formatted = format(l_FilePath, l_FinalContent)
    save_file(l_FilePath, l_Formatted)

}

main();
