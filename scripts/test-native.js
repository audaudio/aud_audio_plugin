// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

// Builds and runs the native unit tests of test/native with the compile
// settings of scripts/plugin-build.js: every test is linked with the
// sources it covers into an executable under .dart_tool/plugin-build/test,
// which runs its tests and reports the failures. Exits with 1 when a test
// fails or does not build. Runs on macOS and Windows.
//
//   node scripts/test-native.js           every test
//   node scripts/test-native.js engine    the tests whose file name holds it

'use strict';

const fs = require('node:fs');
const path = require('node:path');
const { spawnSync } = require('node:child_process');

const {
  windows,
  pluginRoot,
  pluginSrc,
  useVisualStudioLlvm,
  coreSources,
  packagesSource,
  run,
  packageSrcOf,
  hashOf,
  compilerOf,
  coreFlags,
  headersHash,
  compileAll,
  cppIn,
} = require('./plugin-build.js');

if (windows) useVisualStudioLlvm();

const testDir = path.join(pluginRoot, 'test', 'native');
const buildDir = path.join(pluginRoot, '.dart_tool', 'plugin-build', 'test');

const target = windows
  ? ['--target=x86_64-pc-windows-msvc', '-fms-runtime-lib=static']
  : ['-arch', 'arm64', '-mmacosx-version-min=12.0'];
const frameworks = windows ? [] : ['AppKit', 'CoreFoundation', 'Foundation'];
const systemLibraries = windows ? ['user32', 'shell32', 'advapi32'] : [];

const source = (file) => path.join(pluginSrc, file);
const bundleSources = coreSources(windows ? 'windows' : 'macos').filter((file) =>
  path.basename(file).startsWith('aud_plugin_bundle'),
);

// The tests: the sources each covers, and whether it needs the graph engine.
const tests = [
  { file: 'aud_plugin_bundle_test.cpp', sources: bundleSources },
  { file: 'aud_plugin_midi_test.cpp', sources: [source('aud_plugin_midi.cpp')] },
  {
    file: 'aud_plugin_settings_test.cpp',
    sources: [source('aud_plugin_settings.cpp')],
    graph: true,
  },
  { file: 'aud_plugin_transport_test.cpp', sources: [source('aud_plugin_transport.cpp')] },
  { file: 'aud_plugin_sync_test.cpp', sources: [source('aud_plugin_sync.cpp')] },
  {
    file: 'aud_plugin_engine_test.cpp',
    sources: [
      ...bundleSources,
      source('aud_plugin_engine.cpp'),
      source('aud_plugin_midi.cpp'),
      source('aud_plugin_settings.cpp'),
      source('aud_plugin_sync.cpp'),
    ],
    graph: true,
    packages: true,
  },
];

// Builds the executable of `test` and returns its path.
async function buildTest(test) {
  const options = { rtsan: false, watchdog: false };
  const compiler = compilerOf(options);
  const packageSrc = packageSrcOf(path.join(pluginRoot, '.dart_tool', 'package_config.json'));
  const dirs = { graph: packageSrc('aud_audio_graph'), core: packageSrc('aud_audio_core') };
  const flags = coreFlags(options, dirs, target);
  const headers = headersHash([dirs.graph, dirs.core, pluginSrc, testDir]);
  const files = [
    path.join(testDir, test.file),
    ...test.sources,
    ...(test.graph ? cppIn(dirs.graph) : []),
  ];
  if (test.packages) {
    // A build without node packages: the graph's own nodes only.
    const generated = path.join(buildDir, `packages-${hashOf(['none'])}.cpp`);
    fs.mkdirSync(buildDir, { recursive: true });
    fs.writeFileSync(generated, packagesSource([]));
    files.push(generated);
  }
  const objects = await compileAll(compiler, flags, files, buildDir, headers);
  const binary = path.join(
    buildDir,
    `${path.basename(test.file, '.cpp')}-${hashOf(objects)}${windows ? '.exe' : ''}`,
  );
  if (!fs.existsSync(binary)) {
    run(compiler, [
      ...target,
      '-o',
      binary,
      ...objects,
      ...frameworks.flatMap((framework) => ['-framework', framework]),
      ...(windows ? ['-fuse-ld=lld', ...systemLibraries.map((library) => `-l${library}`)] : []),
    ]);
  }
  return binary;
}

async function main() {
  const filter = process.argv[2] ?? '';
  const selected = tests.filter((test) => test.file.includes(filter));
  if (selected.length === 0) {
    throw new Error(`No native test matches ${filter}`);
  }
  const failed = [];
  for (const test of selected) {
    const name = path.basename(test.file, '.cpp');
    console.log(`${name}:`);
    const binary = await buildTest(test);
    const result = spawnSync(binary, [], { stdio: 'inherit' });
    if (result.status !== 0) failed.push(name);
  }
  if (failed.length > 0) throw new Error(`Failed: ${failed.join(', ')}`);
  console.log('All native tests passed');
}

main().catch((error) => {
  console.error(error.message);
  process.exit(1);
});
