const g_FullProjectPath = "P:\\misteda";

const { assert } = require("console");
const fs = require("fs");

function build_project(p_FullProjectPath) {
  console.log(`Building ${p_FullProjectPath}`);

  const l_ProjectModulesPath = `${p_FullProjectPath}\\modules`;

  assert(fs.existsSync(p_FullProjectPath), "Project directory does not exist");
  assert(
    fs.existsSync(l_ProjectModulesPath),
    "Project's modules directory does not exist",
  );

  const l_ProjectModuleDirectories = fs
    .readdirSync(l_ProjectModulesPath, { withFileTypes: true })
    .filter((it) => it.isDirectory())
    .map((it) => it.name);

  for (const i_ModuleDirectory of l_ProjectModuleDirectories) {
    const i_ModulePath = `${l_ProjectModulesPath}\\${i_ModuleDirectory}`;
    assert(
      fs.existsSync(i_ModulePath),
      `Could not find module directory ${i_ModulePath}`,
    );

    const i_ModuleConfigPath = `${i_ModulePath}\\module.yaml`;

    if (!fs.existsSync(i_ModulePath)) {
      continue;
    }

    console.log(`Found module ${i_ModuleDirectory}`);
  }
}

build_project(g_FullProjectPath);
