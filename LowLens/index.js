'use strict';

const Parser = require('tree-sitter');
const Cpp = require('tree-sitter-cpp');
const fs = require('fs');
const path = require('path');
const { parse_file } = require('./src/parse');
const { generate_header_file, generate_module_file } = require('./src/generate');
const { patch_source_header } = require('./src/patch');
const db = require('./src/db');

const g_Parser = new Parser();
g_Parser.setLanguage(Cpp);

function process_file(p_FilePath) {
  const l_Src = fs.readFileSync(p_FilePath, 'utf8');
  const l_Tree = g_Parser.parse(l_Src);
  return parse_file(l_Src, l_Tree);
}

function process_directory(p_DirPath, p_Extensions = ['.h', '.hpp', '.cpp']) {
  const l_Results = [];
  for (const i_Entry of fs.readdirSync(p_DirPath, { withFileTypes: true })) {
    const l_Full = path.join(p_DirPath, i_Entry.name);
    if (i_Entry.isDirectory()) {
      l_Results.push(...process_directory(l_Full, p_Extensions));
    } else if (p_Extensions.includes(path.extname(i_Entry.name))) {
      const { functions: l_Fns, enums: l_Enums, structs: l_Structs } = process_file(l_Full);
      if (l_Fns.length > 0 || l_Enums.length > 0 || l_Structs.length > 0) {
        l_Results.push({ file: l_Full, functions: l_Fns, enums: l_Enums, structs: l_Structs });
      }
    }
  }
  return l_Results;
}

// Parse CLI args — supports:
//   --upstream-db <path>    (repeatable) upstream lensdb.yaml files to read
//   --yaml-configs <path>   (repeatable) engine/project root paths to read YAML type configs from
//   --yaml-project          flag: treat the preceding --yaml-configs path as a project (not Low)
function parse_args(p_Args) {
  const l_Result = {
    positional: [],
    upstream_dbs: [],
    yaml_configs: [],
  };

  let l_I = 0;
  while (l_I < p_Args.length) {
    const l_Arg = p_Args[l_I];
    if (l_Arg === '--upstream-db') {
      l_Result.upstream_dbs.push(p_Args[++l_I]);
    } else if (l_Arg === '--yaml-configs') {
      const l_ConfigPath = p_Args[++l_I];
      const l_IsProject = p_Args[l_I + 1] === '--yaml-project';
      if (l_IsProject) l_I++;
      l_Result.yaml_configs.push({ path: l_ConfigPath, is_project: l_IsProject });
    } else {
      l_Result.positional.push(l_Arg);
    }
    l_I++;
  }

  return l_Result;
}

module.exports = { process_file, process_directory };

// CLI:
//   node index.js <input_dir> <output_dir> <module_name> [--upstream-db <path>]... [--yaml-configs <path> [--yaml-project]]...
//   node index.js <file>
if (require.main === module) {
  const l_Parsed = parse_args(process.argv.slice(2));
  const l_Positional = l_Parsed.positional;

  if (l_Positional.length === 3) {
    const [l_InputDir, l_OutputDir, l_ModuleName] = l_Positional;
    const l_GenDir = path.join(l_OutputDir, 'gen');

    fs.mkdirSync(l_OutputDir, { recursive: true });
    fs.mkdirSync(l_GenDir, { recursive: true });

    // Load upstream lensdb files
    for (const i_DbPath of l_Parsed.upstream_dbs) {
      db.load_db(i_DbPath);
    }

    // Load YAML type configs (without writing deprecated id files)
    for (const i_Config of l_Parsed.yaml_configs) {
      db.load_yaml_configs(i_Config.path, i_Config.is_project);
    }

    const l_FileResults = process_directory(l_InputDir);

    const l_AllFunctions = [];
    const l_AllEnums = [];
    const l_AllStructs = [];
    const l_Headers = [];

    // First pass: collect everything so within-module type resolution works
    for (const i_Result of l_FileResults) {
      l_AllFunctions.push(...i_Result.functions);
      l_AllEnums.push(...i_Result.enums);
      l_AllStructs.push(...i_Result.structs);
    }

    // Register this module's own discovered types into the DB so field
    // type resolution works for same-module cross-references
    for (const i_Enum of l_AllEnums) {
      db.register_type(db.TypeKind.ENUM, l_ModuleName, i_Enum.namespace, i_Enum.name);
    }
    for (const i_Struct of l_AllStructs) {
      db.register_type(db.TypeKind.STRUCT, l_ModuleName, i_Struct.namespace, i_Struct.name);
    }

    // Second pass: generate per-header files
    for (const i_Result of l_FileResults) {
      const l_BaseName = path.basename(i_Result.file);
      l_Headers.push(l_BaseName);

      if (i_Result.enums.length > 0 || i_Result.structs.length > 0) {
        const l_GenHeaderName = `${path.basename(l_BaseName, path.extname(l_BaseName))}.gen.h`;
        const l_GenHeaderPath = path.join(l_GenDir, l_GenHeaderName);
        fs.writeFileSync(l_GenHeaderPath, generate_header_file(i_Result.enums, i_Result.structs));
        console.log(`LowLens: wrote ${l_GenHeaderPath}`);
        patch_source_header(i_Result.file, l_GenHeaderName);
      }

    }

    // Write module .gen.cpp
    const l_CppPath = path.join(l_OutputDir, `${l_ModuleName}.gen.cpp`);
    fs.writeFileSync(l_CppPath, generate_module_file(l_ModuleName, l_AllFunctions, l_AllEnums, l_AllStructs, l_Headers));
    console.log(`LowLens: wrote ${l_CppPath} (${l_AllFunctions.length} function(s), ${l_AllEnums.length} enum(s), ${l_AllStructs.length} struct(s))`);

    // Write this module's lensdb for downstream consumers
    const l_DbPath = path.join(l_OutputDir, `${l_ModuleName}.lensdb.yaml`);
    db.write_db(l_DbPath, l_ModuleName, l_AllEnums, l_AllStructs);

  } else if (l_Positional.length === 1) {
    const l_Result = process_file(l_Positional[0]);
    console.log(JSON.stringify(l_Result, null, 2));

  } else {
    console.error('Usage:');
    console.error('  node index.js <input_dir> <output_dir> <module_name> [--upstream-db <path>]... [--yaml-configs <path> [--yaml-project]]...');
    console.error('  node index.js <file>');
    process.exit(1);
  }
}
