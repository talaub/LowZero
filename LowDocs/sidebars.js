// @ts-check

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  docsSidebar: [
    'intro',
    {
      type: 'category',
      label: 'User Guide',
      collapsed: false,
      items: [
        'user-guide/core-concepts',
        'user-guide/editor-ui',
        'user-guide/assets',
        'user-guide/components',
        'user-guide/flode',
        'user-guide/workflows',
      ],
    },
    {
      type: 'category',
      label: 'Technical Reference',
      collapsed: false,
      items: [
        'technical-reference/architecture-overview',
        'technical-reference/core-runtime',
        'technical-reference/rendering',
        'technical-reference/assets-resources',
        'technical-reference/scripting',
        'technical-reference/flode-internals',
        'technical-reference/editor-internals',
        'technical-reference/build-system',
        'technical-reference/debugging-profiling',
      ],
    },
  ],
};

export default sidebars;
