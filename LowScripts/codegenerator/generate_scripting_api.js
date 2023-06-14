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

    let l_OldCode = '';
    if (fs.existsSync(p_Type.scripting_api_file_path)) {
	l_OldCode = read_file(p_Type.scripting_api_file_path);
    }

    t += line('using System.Runtime.CompilerServices;');
    t += line('using System.Runtime.InteropServices;');
    t += empty();

    t += line(`namespace ${p_Type.scripting_namespace_string} {`);

    t += line(`public unsafe partial class ${p_Type.name}: Low.Internal.Handle {`);
    t += empty();

    t += line(`public ${p_Type.name}(): this(0) {`);
    t += line(`}`);
    t += empty();
    t += line(`public ${p_Type.name}(ulong p_Id): base(p_Id) {`);
    t += line(`}`);
    t += empty();

    t += line(`public static ushort type;`);
    t += empty();

    t += line(`public static uint livingInstancesCount {`);
    t += line('get {');
    t += line('return Low.Internal.HandleHelper.GetLivingInstancesCount(type);');
    t += line(`}`);
    t += line(`}`);
    t += empty();
    t += line(`public static ${p_Type.name} GetByIndex(uint p_Index) {`);
    t += line(`ulong id = Low.Internal.HandleHelper.GetLivingInstance(type, p_Index);`);
    t += line(`return new ${p_Type.name}(id);`);
    t += line(`}`);

    for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
	if (!i_Prop.expose_scripting) {
	    continue;
	}
	if ((i_Prop.private_getter && i_Prop.private_setter)
	    || (i_Prop.no_getter && i_Prop.no_setter)) {
	    continue;
	}

	t += line(`public ${i_Prop.cs_type} ${i_Prop.cs_name} {`);
	if (!i_Prop.private_getter && !i_Prop.no_getter && !i_Prop.scripting_hide_getter) {
	    t += line(`get {`);
	    t += line(`return ${internal_get(i_Prop)}(id, ${hash_name(i_PropName)});`);
	    t += line('}');
	}
	if (!i_Prop.private_setter && !i_Prop.no_setter && !i_Prop.scripting_hide_setter) {
	    t += line(`set {`);
	    t += line(`${internal_set(i_Prop)}(id, ${hash_name(i_PropName)}, value);`);
	    t += line('}');
	}
	t += line('}');
    }

    if (true) {
	t += empty();
	const l_MarkerName = `CUSTOM:TYPE_CODE`;

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

    t += line(`}`);

    t += line('}');

    const l_Formatted = format(`${p_Type.name}.cs`, t);

    /*
    console.log(`Type: ${p_Type.name}`);
    console.log(l_Formatted);
    console.log('-----------------------');
    */

    if (l_Formatted !== l_OldCode) {
	save_file(p_Type.scripting_api_file_path, l_Formatted);
	return true
    }
    return false
}

function main() {
    const l_Types = collect_types();

    for (const i_Type of l_Types) {
	if (i_Type.scripting_expose) {
	    generate_scripting_api(i_Type);
	}
    }
}

main();
