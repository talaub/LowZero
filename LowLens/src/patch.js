'use strict';

const fs = require('fs');
const path = require('path');

function patch_source_header(p_FilePath, p_GenHeaderName) {
  const l_Content = fs.readFileSync(p_FilePath, 'utf8');
  const l_IncludeLine = `#include "${p_GenHeaderName}"`;

  if (l_Content.includes(l_IncludeLine)) return false;

  const l_Patched = l_Content.trimEnd() + '\n\n' + l_IncludeLine + '\n';
  fs.writeFileSync(p_FilePath, l_Patched);
  console.log(`LowLens: patched ${path.basename(p_FilePath)} -> added ${l_IncludeLine}`);
  return true;
}

module.exports = { patch_source_header };
