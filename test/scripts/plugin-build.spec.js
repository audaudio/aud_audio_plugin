// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

// The parts of scripts/plugin-build.js that decide what a shell compiles:
// the core's sources, the node packages' recipes and their registration,
// the hashes of the compile cache and the language flags. Run with
// `node --test "test/scripts/**/*.spec.js"`.

'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { describe, it } = require('node:test');

const build = require('../../scripts/plugin-build.js');

describe('coreSources(platform)', () => {
  it('takes the Apple bundle code on macOS and iOS', () => {
    const names = build.coreSources('macos').map((file) => path.basename(file));
    assert.ok(names.includes('aud_plugin_bundle_apple.mm'));
    assert.ok(!names.includes('aud_plugin_bundle_win.cpp'));
    assert.ok(names.includes('aud_plugin_engine.cpp'));
  });

  it('takes the Windows bundle code on Windows', () => {
    const names = build.coreSources('windows').map((file) => path.basename(file));
    assert.ok(names.includes('aud_plugin_bundle_win.cpp'));
    assert.ok(!names.includes('aud_plugin_bundle_apple.mm'));
  });

  it('names files that exist', () => {
    for (const file of build.coreSources('macos')) assert.ok(fs.existsSync(file), file);
  });
});

describe('readList(file)', () => {
  it('reads the trimmed lines that are not empty', () => {
    const file = path.join(fs.mkdtempSync(path.join(os.tmpdir(), 'plugin-build-')), 'list.txt');
    fs.writeFileSync(file, ' a.cpp \n\nb.cpp\n');
    assert.deepEqual(build.readList(file), ['a.cpp', 'b.cpp']);
    fs.rmSync(path.dirname(file), { recursive: true });
  });
});

describe('recipesOf(names, packageSrc)', () => {
  it('refuses a package without a recipe', () => {
    assert.throws(() => build.recipesOf(['aud_unknown'], () => ''), /No recipe for package aud_unknown/);
  });

  it('lists the sources, includes and registration of a known package', () => {
    // The layout of aud_dsp_sampler: its own sources and sfizz's lists.
    const src = path.join(fs.mkdtempSync(path.join(os.tmpdir(), 'plugin-build-')), 'src');
    const sfizz = path.join(src, 'third_party', 'sfizz');
    fs.mkdirSync(sfizz, { recursive: true });
    fs.writeFileSync(path.join(src, 'b.cpp'), '');
    fs.writeFileSync(path.join(src, 'a.cpp'), '');
    fs.writeFileSync(path.join(src, 'a.hpp'), '');
    fs.writeFileSync(path.join(sfizz, 'SOURCES.txt'), 'src/x.cpp\n');
    fs.writeFileSync(path.join(sfizz, 'INCLUDES.txt'), 'src\n');
    const [sampler] = build.recipesOf(['aud_dsp_sampler'], () => src);
    assert.equal(sampler.name, 'aud_dsp_sampler');
    assert.equal(sampler.register, 'aud_dsp_sampler_register');
    assert.equal(sampler.voiceBudget, 'aud_dsp_sampler_set_voice_budget');
    assert.deepEqual(sampler.sources, [
      path.join(src, 'a.cpp'),
      path.join(src, 'b.cpp'),
      path.join(sfizz, 'src', 'x.cpp'),
    ]);
    assert.deepEqual(sampler.includes, [src, path.join(sfizz, 'src')]);
    fs.rmSync(path.dirname(src), { recursive: true });
  });
});

describe('packagesSource(packageRecipes)', () => {
  it('registers nothing and ignores the voice budget without packages', () => {
    const source = build.packagesSource([]);
    assert.match(source, /bool registerPackages\(AudGraph\* graph, std::string\* error\)/);
    assert.match(source, /\(void\)voices;/);
    assert.match(source, /return true;/);
  });

  it('registers every package and hands the budget to the one that takes it', () => {
    const source = build.packagesSource([
      { name: 'a', register: 'a_register' },
      { name: 'b', register: 'b_register', voiceBudget: 'b_set_voice_budget' },
    ]);
    assert.match(source, /extern "C" int32_t a_register\(const AudHostApi\* host\);/);
    assert.match(source, /if \(const int32_t result = b_register\(api\); result != AUD_OK\)/);
    assert.match(source, /"a refused: "/);
    assert.match(source, / {2}b_set_voice_budget\(voices\);/);
    assert.doesNotMatch(source, /\(void\)voices;/);
  });
});

describe('hashOf(parts)', () => {
  it('is 20 hex digits, the same for the same parts', () => {
    assert.match(build.hashOf(['a', 'b']), /^[0-9a-f]{20}$/);
    assert.equal(build.hashOf(['a', 'b']), build.hashOf(['a', 'b']));
  });

  it('tells the parts apart, not only their text', () => {
    assert.notEqual(build.hashOf(['ab']), build.hashOf(['a', 'b']));
  });
});

describe('languageFlags(file)', () => {
  it('compiles each file as its language', () => {
    assert.deepEqual(build.languageFlags('x.mm').slice(0, 2), ['-x', 'objective-c++']);
    assert.deepEqual(build.languageFlags('x.m').slice(0, 2), ['-x', 'objective-c']);
    assert.deepEqual(build.languageFlags('x.c'), ['-x', 'c', '-std=c11']);
    assert.deepEqual(build.languageFlags('x.cpp'), ['-x', 'c++', '-std=c++17']);
  });
});
