'use strict';

const fs = require('fs');
const path = require('path');
const YAML = require('yaml');

// TypeKind values stored in the DB - mirrors RTTI::PropertyType
const TypeKind = {
  UNKNOWN:    'unknown',
  HANDLE:     'handle',
  ENUM:       'enum',
  STRUCT:     'struct',
  FLOAT:      'float',
  INT:        'int',
  UINT8:      'uint8',
  UINT16:     'uint16',
  UINT32:     'uint32',
  UINT64:     'uint64',
  BOOL:       'bool',
  NAME:       'name',
  STRING:     'string',
  VECTOR2:    'vector2',
  VECTOR3:    'vector3',
  VECTOR4:    'vector4',
  QUATERNION: 'quaternion',
};

// CRC32 matching Low::Util::Name::to_hash (polynomial 0xEDB88320)
function name_hash(p_Str) {
  let l_Crc = 0xFFFFFFFF;
  for (let i = 0; i < p_Str.length; i++) {
    l_Crc ^= p_Str.charCodeAt(i);
    for (let j = 0; j < 8; j++) {
      const l_Mask = -(l_Crc & 1);
      l_Crc = (l_Crc >>> 1) ^ (0xEDB88320 & l_Mask);
    }
  }
  return (~l_Crc) >>> 0;
}

// Each entry: { kind, module, namespace, name }
let g_KnownTypes = {};

function make_key(p_Module, p_Name) {
  return `${p_Module}:${p_Name}`;
}

function register_type(p_Kind, p_Module, p_Namespace, p_Name, p_BindName, p_BindNamespace) {
  const l_Key = make_key(p_Module, p_Name);
  g_KnownTypes[l_Key] = {
    kind: p_Kind,
    module: p_Module,
    namespace: p_Namespace,
    name: p_Name,
    bind_name: p_BindName || p_Name,
    bind_namespace: p_BindNamespace || '',
  };
}

// Load an upstream lensdb.yaml file
function load_db(p_FilePath) {
  if (!fs.existsSync(p_FilePath)) return;
  const l_Content = YAML.parse(fs.readFileSync(p_FilePath, 'utf8'));
  if (!l_Content || !l_Content.types) return;
  for (const [i_Key, i_Entry] of Object.entries(l_Content.types)) {
    g_KnownTypes[i_Key] = i_Entry;
  }
}

function write_file_if_changed(p_FilePath, p_Content) {
  if (fs.existsSync(p_FilePath)) {
    const l_Current = fs.readFileSync(p_FilePath, 'utf8');
    if (l_Current === p_Content) {
      return false;
    }
  }

  fs.writeFileSync(p_FilePath, p_Content);
  return true;
}

// Write this module's discovered types to a lensdb.yaml
function write_db(p_FilePath, p_ModuleName, p_Enums, p_Structs) {
  const l_Types = {};

  for (const i_Enum of p_Enums) {
    const l_Key = make_key(p_ModuleName, i_Enum.name);
    l_Types[l_Key] = {
      kind: TypeKind.ENUM,
      module: p_ModuleName,
      namespace: i_Enum.namespace,
      name: i_Enum.name,
      bind_name: i_Enum.bind_name || i_Enum.name,
      bind_namespace: i_Enum.bind_namespace || '',
    };
  }

  for (const i_Struct of p_Structs) {
    const l_Key = make_key(p_ModuleName, i_Struct.name);
    l_Types[l_Key] = {
      kind: TypeKind.STRUCT,
      module: p_ModuleName,
      namespace: i_Struct.namespace,
      name: i_Struct.name,
      bind_name: i_Struct.bind_name || i_Struct.name,
      bind_namespace: i_Struct.bind_namespace || '',
    };
  }

  return write_file_if_changed(p_FilePath, YAML.stringify({ types: l_Types }));
}

// Ingest YAML type configs from lib.js (handle types and YAML enums)
// p_IsProject: true for misteda-style project paths, false for LowEngine paths
function load_yaml_configs(p_EnginePath, p_IsProject) {
  const l_LibPath = path.resolve(__dirname, '../../LowScripts/codegenerator/lib.js');
  const lib = require(l_LibPath);

  const l_Types = p_IsProject
    ? lib.collect_types_for_project(p_EnginePath, false)
    : lib.collect_types_for_low(p_EnginePath, false);

  const l_Enums = p_IsProject
    ? lib.collect_enums_for_project(p_EnginePath, false)
    : lib.collect_enums_for_low(p_EnginePath, false);

  for (const i_Type of l_Types) {
    const l_Key = make_key(i_Type.module, i_Type.name);
    g_KnownTypes[l_Key] = {
      kind: TypeKind.HANDLE,
      module: i_Type.module,
      namespace: i_Type.namespace_string,
      name: i_Type.name,
      bind_name: i_Type.scripting_name || i_Type.name,
      bind_namespace: i_Type.scripting_namespace || '',
    };
  }

  for (const i_Enum of l_Enums) {
    const l_Key = make_key(i_Enum.module, i_Enum.name);
    g_KnownTypes[l_Key] = {
      kind: TypeKind.ENUM,
      module: i_Enum.module,
      namespace: i_Enum.namespace_string,
      name: i_Enum.name,
      bind_name: i_Enum.scripting_name || i_Enum.name,
      bind_namespace: i_Enum.scripting_namespace || '',
    };
  }
}

// Hardcoded primitive/math type map — checked before DB lookup
const g_HardcodedTypes = [
  { kinds: ['float'],                                                   kind: TypeKind.FLOAT      },
  { kinds: ['bool'],                                                    kind: TypeKind.BOOL       },
  { kinds: ['int', 'int32_t'],                                          kind: TypeKind.INT        },
  { kinds: ['uint8_t', 'u8'],                                           kind: TypeKind.UINT8      },
  { kinds: ['uint16_t', 'u16'],                                         kind: TypeKind.UINT16     },
  { kinds: ['uint32_t', 'u32', 'unsigned int'],                         kind: TypeKind.UINT32     },
  { kinds: ['uint64_t', 'u64'],                                         kind: TypeKind.UINT64     },
  { kinds: ['Low::Util::Name', 'Util::Name', 'Name'],                   kind: TypeKind.NAME       },
  { kinds: ['Low::Util::String', 'Util::String', 'String'],             kind: TypeKind.STRING     },
  { kinds: ['Low::Util::Handle', 'Util::Handle'],                       kind: TypeKind.HANDLE     },
  { kinds: ['Low::Math::Vector2', 'Math::Vector2'],                     kind: TypeKind.VECTOR2    },
  { kinds: ['Low::Math::Vector3', 'Math::Vector3'],                     kind: TypeKind.VECTOR3    },
  { kinds: ['Low::Math::Vector4', 'Math::Vector4'],                     kind: TypeKind.VECTOR4    },
  { kinds: ['Low::Math::Quaternion', 'Math::Quaternion'],               kind: TypeKind.QUATERNION },
];

// Resolve a C++ type string to a { kind, entry } — entry is the DB record for
// complex types (handle/enum/struct), null for primitives/math.
function resolve_type(p_TypeStr) {
  const l_Clean = p_TypeStr.replace(/\s*[*&]+\s*$/, '').trim();

  for (const i_Entry of g_HardcodedTypes) {
    if (i_Entry.kinds.includes(l_Clean)) {
      return { kind: i_Entry.kind, entry: null };
    }
  }

  // Try matching against known types by name suffix
  for (const [i_Key, i_Entry] of Object.entries(g_KnownTypes)) {
    const l_FullNs = i_Entry.namespace
      ? `${i_Entry.namespace}::${i_Entry.name}`
      : i_Entry.name;
    if (l_Clean === l_FullNs || l_Clean === i_Entry.name) {
      return { kind: i_Entry.kind, entry: i_Entry };
    }
  }

  return { kind: TypeKind.UNKNOWN, entry: null };
}

// Returns the AngelScript type name string for a C++ type string
function get_as_type_string(p_TypeStr) {
  const l_Clean = p_TypeStr.replace(/\s*[*&]+\s*$/, '').trim();

  const l_Hardcoded = {
    'float': 'float', 'bool': 'bool',
    'int': 'int', 'int32_t': 'int',
    'uint8_t': 'u8', 'u8': 'u8',
    'uint16_t': 'u16', 'u16': 'u16',
    'uint32_t': 'u32', 'u32': 'u32',
    'uint64_t': 'u64', 'u64': 'u64',
    'Low::Util::Name': 'Name', 'Util::Name': 'Name', 'Name': 'Name',
    'Low::Util::String': 'string', 'Util::String': 'string',
    'Low::Util::Handle': 'Handle', 'Util::Handle': 'Handle',
    'Low::Math::Vector2': 'Vector2', 'Math::Vector2': 'Vector2',
    'Low::Math::Vector3': 'Vector3', 'Math::Vector3': 'Vector3',
    'Low::Math::Vector4': 'Vector4', 'Math::Vector4': 'Vector4',
    'Low::Math::Quaternion': 'Quaternion', 'Math::Quaternion': 'Quaternion',
  };
  if (l_Hardcoded[l_Clean]) return l_Hardcoded[l_Clean];

  // Look up in known types for bind name
  for (const i_Entry of Object.values(g_KnownTypes)) {
    const l_FullNs = i_Entry.namespace
      ? `${i_Entry.namespace}::${i_Entry.name}`
      : i_Entry.name;
    if (l_Clean === l_FullNs || l_Clean === i_Entry.name) {
      const l_BindNs = i_Entry.bind_namespace || '';
      const l_BindName = i_Entry.bind_name || i_Entry.name;
      return l_BindNs ? `${l_BindNs}::${l_BindName}` : l_BindName;
    }
  }

  return '';
}

module.exports = { TypeKind, load_db, write_db, load_yaml_configs, resolve_type, register_type, get_as_type_string, name_hash };
