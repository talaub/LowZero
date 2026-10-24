const {
  is_plugin,
  build_database_for,
  write_file_if_changed,
} = require("./lib.js");

const YAML = require("yaml");
const fs = require("fs");

function build(p_Path, p_OutPath) {
  const db = build_database_for(p_Path);

  write_file_if_changed(p_OutPath, JSON.stringify(db));
}

// TODO: Validation checks
const l_Args = process.argv.slice(2);

build(l_Args[0], l_Args[1]);
