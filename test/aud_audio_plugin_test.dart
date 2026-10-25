// @license
// Copyright (c) audaudio
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

import 'package:aud_audio_plugin/aud_audio_plugin.dart';
import 'package:test/test.dart' show expect, group, test;

void main() {
  group('AudAudioPlugin', () {
    test('greets with the greeting', () {
      expect(const AudAudioPlugin(greeting: 'Hi').greet('Ada'), 'Hi, Ada!');
    });

    test('example() greets with Hello', () {
      expect(AudAudioPlugin.example().greet('Ada'), 'Hello, Ada!');
      expect(AudAudioPlugin.example(greeting: 'Hey').greeting, 'Hey');
    });
  });
}
