// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

// The build every plugin shell shares, with clang and without CMake: the
// sources of the shared core (src/), the graph engine and the node packages
// a plugin.json names. The packages come from a package config - the app's,
// so that a plugin plays the versions the app plays -, compiled from their
// sources and the manifests of their vendored code (build-001) by the
// recipes below, which mirror their build hooks. A shell's build tool
// requires this module through the package config, adds its own sources,
// links and packs its bundle.
//
// Objects are cached by the hash of their source, the flags and all headers
// the build may include. On Windows clang++ comes from the LLVM of Visual
// Studio; useVisualStudioLlvm() puts it on the PATH.

'use strict';

const { createHash } = require('node:crypto');
const { spawn, spawnSync } = require('node:child_process');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { fileURLToPath, pathToFileURL } = require('node:url');

const windows = process.platform === 'win32';

// The root of this package and its native sources.
const pluginRoot = path.resolve(__dirname, '..');
const pluginSrc = path.join(pluginRoot, 'src');

// On Windows, clang++, lld-link and llvm-readobj come from the LLVM of
// Visual Studio (component "C++ Clang compiler for Windows").
function useVisualStudioLlvm() {
  const vswhere = path.join(
    process.env['ProgramFiles(x86)'] || 'C:/Program Files (x86)',
    'Microsoft Visual Studio',
    'Installer',
    'vswhere.exe',
  );
  const found = spawnSync(
    vswhere,
    ['-latest', '-products', '*', '-find', 'VC/Tools/Llvm/x64/bin/clang++.exe'],
    { encoding: 'utf8' },
  );
  const clang = found.status === 0 ? found.stdout.split(/\r?\n/)[0].trim() : '';
  if (!clang) throw new Error('Install the C++ Clang compiler of Visual Studio');
  if (!process.env.PATH.split(path.delimiter).includes(path.dirname(clang))) {
    process.env.PATH = [path.dirname(clang), process.env.PATH].join(path.delimiter);
  }
}

// ............................................................................
// The shared core

// The sources of the shared core for `platform`: macos, ios or windows.
function coreSources(platform) {
  return [
    'aud_plugin_bundle.cpp',
    'aud_plugin_engine.cpp',
    'aud_plugin_midi.cpp',
    'aud_plugin_settings.cpp',
    'aud_plugin_sync.cpp',
    'aud_plugin_transport.cpp',
    platform === 'windows' ? 'aud_plugin_bundle_win.cpp' : 'aud_plugin_bundle_apple.mm',
  ].map((file) => path.join(pluginSrc, file));
}

// ............................................................................
// The node packages: what their build hooks compile, the function that
// registers their nodes, and the one that sets the samplers' voice budget.

// Every C++ file directly in `dir`, as the hooks list them.
function cppIn(dir) {
  return fs
    .readdirSync(dir)
    .filter((file) => file.endsWith('.cpp'))
    .sort()
    .map((file) => path.join(dir, file));
}

function readList(file) {
  return fs
    .readFileSync(file, 'utf8')
    .split('\n')
    .map((line) => line.trim())
    .filter(Boolean);
}

function listIn(dir, manifest) {
  return readList(path.join(dir, manifest)).map((entry) => path.join(dir, entry));
}

const recipes = {
  aud_dsp_sampler: (src) => {
    const sfizz = path.join(src, 'third_party', 'sfizz');
    return {
      sources: [...cppIn(src), ...listIn(sfizz, 'SOURCES.txt')],
      includes: [src, ...listIn(sfizz, 'INCLUDES.txt')],
      defines: [
        'GHC_FILESYSTEM_FWD',
        'ENABLE_DSD',
        'ENABLE_THREADS',
        'HAVE_FSEEKO',
        'HAVE___BUILTIN_CLZ',
        'NOMINMAX',
        // M_PI and its kin in the C runtime of Windows.
        ...(windows ? ['_USE_MATH_DEFINES'] : []),
      ],
      // As the hook on Windows: sfizz's AVX objects carry no __AVX__ guard,
      // so all of it is built for AVX; simde needs <cmath> first.
      flags: windows ? ['-mavx', '-include', 'aud_windows_math.h'] : [],
      // CommandLineToArgvW of ghc::filesystem.
      libraries: windows ? ['shell32'] : [],
      register: 'aud_dsp_sampler_register',
      voiceBudget: 'aud_dsp_sampler_set_voice_budget',
      notices: [path.join(src, 'third_party', 'NOTICES.md')],
    };
  },
  aud_dsp_effects: (src) => {
    const third = path.join(src, 'third_party');
    return {
      sources: [
        ...cppIn(src),
        path.join(third, 'dunne_audio_kit', 'AdjustableDelayLine.cpp'),
        path.join(third, 'dunne_audio_kit', 'StereoDelay.cpp'),
        path.join(third, 'freeverb', 'allpass.cpp'),
        path.join(third, 'freeverb', 'comb.cpp'),
        path.join(third, 'freeverb', 'revmodel.cpp'),
      ],
      includes: [src, path.join(third, 'freeverb'), path.join(third, 'dunne_audio_kit')],
      defines: [],
      flags: ['-ffp-contract=off'],
      register: 'aud_dsp_effects_register',
      notices: [path.join(third, 'NOTICES.md')],
    };
  },
  aud_audio_script: (src, packageSrc) => {
    const quickjs = path.join(packageSrc('gg_quickjs'), 'quickjs-ng');
    return {
      sources: [
        ...cppIn(src),
        ...['dtoa.c', 'libregexp.c', 'libunicode.c', 'quickjs.c'].map((file) =>
          path.join(quickjs, file),
        ),
      ],
      includes: [src, quickjs],
      defines: ['_GNU_SOURCE'],
      flags: [],
      // QuickJS-ng as gg_quickjs builds it: unsigned char, no warnings of
      // third-party code, and on Windows the defines of
      // QuickjsNgBuild.defines, which the package's C++ files do not take.
      cFlags: [
        '-funsigned-char',
        '-w',
        ...(windows ? ['-DWIN32_LEAN_AND_MEAN', '-D_WIN32_WINNT=0x0601'] : []),
      ],
      register: 'aud_audio_script_register',
      notices: [
        path.join(src, 'third_party', 'NOTICES.md'),
        path.join(quickjs, 'LICENSE'),
      ],
    };
  },
};

// The recipes of the packages `names`, in their order, with their names;
// throws for a package without a recipe.
function recipesOf(names, packageSrc) {
  return names.map((name) => {
    if (!recipes[name]) throw new Error(`No recipe for package ${name}`);
    return { name, ...recipes[name](packageSrc(name), packageSrc) };
  });
}

// The registration of the packages (aud_plugin_packages.hpp), generated per
// build.
function packagesSource(packageRecipes) {
  const lines = [
    '// Generated by aud_audio_plugin/scripts/plugin-build.js; do not edit.',
    '#include "aud_plugin_packages.hpp"',
    '',
  ];
  for (const recipe of packageRecipes) {
    lines.push(`extern "C" int32_t ${recipe.register}(const AudHostApi* host);`);
    if (recipe.voiceBudget) {
      lines.push(`extern "C" void ${recipe.voiceBudget}(int32_t voices);`);
    }
  }
  lines.push(
    '',
    'namespace aud_plugin {',
    '',
    'bool registerPackages(AudGraph* graph, std::string* error) {',
    '  const AudHostApi* api = aud_graph_host_api(graph);',
  );
  for (const recipe of packageRecipes) {
    lines.push(
      `  if (const int32_t result = ${recipe.register}(api); result != AUD_OK) {`,
      `    if (error != nullptr) *error = "${recipe.name} refused: " + std::to_string(result);`,
      '    return false;',
      '  }',
    );
  }
  lines.push('  return true;', '}', '', 'void setVoiceBudget(int32_t voices) {');
  const budget = packageRecipes.find((recipe) => recipe.voiceBudget);
  lines.push(budget ? `  ${budget.voiceBudget}(voices);` : '  (void)voices;');
  lines.push('}', '', '}  // namespace aud_plugin', '');
  return lines.join('\n');
}

// ............................................................................
// Packages, tools

function run(command, args, cwd = process.cwd()) {
  const result = spawnSync(command, args, { cwd, encoding: 'utf8' });
  if (result.error) throw new Error(`${command} failed: ${result.error.message}`);
  if (result.status !== 0) {
    throw new Error(`${path.basename(command)} failed:\n${result.stdout}${result.stderr}`);
  }
  return result.stdout;
}

// The root directory of a package, resolved through `configPath`.
function packageRootOf(configPath) {
  if (!fs.existsSync(configPath)) {
    throw new Error(`Run dart pub get first: ${configPath} is missing`);
  }
  const config = JSON.parse(fs.readFileSync(configPath, 'utf8'));
  return (name) => {
    const entry = config.packages.find((p) => p.name === name);
    if (!entry) throw new Error(`Package ${name} is not in ${configPath}`);
    return fileURLToPath(new URL(entry.rootUri, pathToFileURL(configPath)));
  };
}

// The `src` directory of a package, resolved through `configPath`.
function packageSrcOf(configPath) {
  const rootOf = packageRootOf(configPath);
  return (name) => path.join(rootOf(name), 'src');
}

function hashOf(parts) {
  const hash = createHash('sha256');
  for (const part of parts) hash.update(part).update('\0');
  return hash.digest('hex').slice(0, 20);
}

// The compiler: Apple's clang or that of Visual Studio, or LLVM from
// Homebrew for RealtimeSanitizer.
function compilerOf(options) {
  if (!options.rtsan) return 'clang++';
  if (windows) throw new Error('RealtimeSanitizer builds run on macOS only');
  for (const candidate of [
    process.env.AUD_RTSAN_CXX,
    '/opt/homebrew/opt/llvm/bin/clang++',
    '/usr/local/opt/llvm/bin/clang++',
  ].filter(Boolean)) {
    if (fs.existsSync(candidate)) return candidate;
  }
  throw new Error('RealtimeSanitizer needs LLVM from Homebrew');
}

// The flags every object of a plugin shares: `target`, optimization, hidden
// symbols and the includes of the graph, the core and the shared core.
// `options.rtsan` marks the realtime paths for RealtimeSanitizer,
// `options.watchdog` builds the graph's watchdog in.
function coreFlags(options, dirs, target) {
  const flags = [
    ...target,
    '-O3',
    '-g',
    '-DNDEBUG=1',
    '-fvisibility=hidden',
    '-fvisibility-inlines-hidden',
    '-I',
    dirs.graph,
    '-I',
    dirs.core,
    '-I',
    pluginSrc,
  ];
  if (windows) {
    flags.push(
      '-DNOMINMAX',
      '-D_CRT_SECURE_NO_WARNINGS',
      // The engine compiled into a plugin exports nothing of its own.
      '-DAUD_EXPORT=',
    );
  }
  if (options.rtsan) {
    // The graph and the core mark their realtime path nonblocking, so the
    // sanitizer checks everything a block calls.
    flags.push(
      '-fsanitize=realtime',
      '-DAUD_GRAPH_RTSAN=1',
      '-DAUD_PLUGIN_RTSAN=1',
      '-Wno-function-effects',
    );
  }
  if (options.watchdog) flags.push('-DAUD_GRAPH_WATCHDOG=1');
  return flags;
}

function languageFlags(file) {
  if (file.endsWith('.mm')) return ['-x', 'objective-c++', '-std=c++17', '-fobjc-arc'];
  if (file.endsWith('.m')) return ['-x', 'objective-c', '-fobjc-arc'];
  if (file.endsWith('.c')) return ['-x', 'c', '-std=c11'];
  return ['-x', 'c++', '-std=c++17'];
}

// One hash over every header the builds may include: a header change
// rebuilds everything, an unchanged tree compiles nothing.
function headersHash(dirs) {
  const files = [];
  const walk = (dir) => {
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
      const full = path.join(dir, entry.name);
      if (entry.isDirectory()) walk(full);
      else if (/\.(h|hpp|hh|inc)$/.test(entry.name)) files.push(full);
    }
  };
  for (const dir of dirs) if (fs.existsSync(dir)) walk(dir);
  return hashOf(files.sort().map((file) => file + fs.statSync(file).mtimeMs + fs.statSync(file).size));
}

// Compiles `files` with `flags` (plus `cFlags` for C files) into `objDir`.
async function compileAll(compiler, flags, files, objDir, headers, cFlags = []) {
  fs.mkdirSync(objDir, { recursive: true });
  const jobs = files.map((file) => {
    const key = hashOf([
      compiler,
      ...flags,
      ...cFlags,
      headers,
      file,
      fs.readFileSync(file, 'utf8'),
    ]);
    const object = path.join(objDir, `${path.basename(file)}-${key}.o`);
    return { file, object };
  });
  const pending = jobs.filter((job) => !fs.existsSync(job.object));
  const limit = Math.max(1, os.cpus().length);
  let next = 0;
  const failures = [];
  async function worker() {
    while (next < pending.length) {
      const job = pending[next++];
      const args = [
        ...flags,
        ...(job.file.endsWith('.c') ? cFlags : []),
        ...languageFlags(job.file),
        '-c',
        job.file,
        '-o',
        `${job.object}.tmp`,
      ];
      const code = await new Promise((resolve) => {
        const child = spawn(compiler, args, { stdio: 'inherit' });
        child.on('close', resolve);
      });
      if (code === 0) fs.renameSync(`${job.object}.tmp`, job.object);
      else failures.push(job.file);
    }
  }
  await Promise.all(Array.from({ length: limit }, worker));
  if (failures.length > 0) {
    throw new Error(`Compilation failed: ${failures.join(', ')}`);
  }
  return jobs.map((job) => job.object);
}

// Removes `target`: a link or junction itself, never what it points to, or
// a tree.
function removePath(target) {
  let stat;
  try {
    stat = fs.lstatSync(target);
  } catch {
    return;
  }
  if (stat.isSymbolicLink()) fs.unlinkSync(target);
  else fs.rmSync(target, { recursive: true, force: true });
}

module.exports = {
  windows,
  pluginRoot,
  pluginSrc,
  useVisualStudioLlvm,
  coreSources,
  cppIn,
  readList,
  recipes,
  recipesOf,
  packagesSource,
  run,
  packageRootOf,
  packageSrcOf,
  hashOf,
  compilerOf,
  coreFlags,
  languageFlags,
  headersHash,
  compileAll,
  removePath,
};
