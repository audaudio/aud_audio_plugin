# aud_audio_plugin

English | [Deutsch](README.de.md)

[![CI](https://github.com/audaudio/aud_audio_plugin/actions/workflows/quick_check.yaml/badge.svg)](https://github.com/audaudio/aud_audio_plugin/actions/workflows/quick_check.yaml)

Format-agnostic core of the plugin shells of the Audanika Audio Engine:
engine over the headless host, plugin configuration, presets, state, MIDI
and transport. Runs on macOS, iOS and Windows.

## Motivation

A plugin shell wraps the engine in a plugin format. Everything that does
not depend on the format — the engine over the headless host, the plugin
configuration, presets and state, a host's MIDI and transport, the content
locations and the build of the node packages — would otherwise be written
once per shell. This package holds it once, so that every shell plays an
app's graph the same way and a shell only adds the format.

## Installation

The package is not published on pub.dev; add it from its git tags:

```yaml
dependencies:
  aud_audio_plugin:
    git:
      url: git@github.com:audaudio/aud_audio_plugin.git
      tag_pattern: "{{version}}"
    version: ^0.1.0
```

## Usage

The Dart part is the shell protocol between a plugin and its editor: the
messages (`AudShellMessage`) and their framing (`AudShellCodec`).

```dart
import 'dart:typed_data';

import 'package:aud_audio_plugin/aud_audio_plugin.dart';

Future<void> main() async {
  // The editor introduces itself; the message travels as one frame.
  const hello = AudShellHello(token: 'abc', instance: 0, pid: 4711);
  final frame = AudShellCodec.encode(hello);
  print(frame.length);

  // The plugin's end decodes the bytes, in whatever chunks they arrive.
  final chunks = [frame.sublist(0, 7), frame.sublist(7)];
  final messages = await Stream<Uint8List>.fromIterable(
    chunks,
  ).transform(AudShellCodec.decoder()).toList();
  print(messages);
}
```

It prints:

```text
70
[AudShellHello({type: hello, instance: 0, token: abc, pid: 4711, version: 3})]
```

## Limitations

- No plugin format of its own: a shell such as a VST3 or AUv3 wrapper
  adds the format on top of this core
- The C++ core builds on macOS, iOS and Windows only
- The Dart package holds the shell protocol only, not the engine; the
  engine is reached through a shell's build of the C++ core

## Documentation

- [Plugin configuration](doc/plugin-config.md): `plugin.json`, the graph
  document, presets and the state of an instance
- [Guides](doc/guides/): development, code, test and documentation guides
- [Blog](doc/blog/): the posts about the changes of the package

## How it works

- **C++ core (`src/`, namespace `aud_plugin`).** The engine over the headless
  host of `aud_audio_graph` with its control thread, the lock-free parameter
  ring, presets, the waits of offline rendering and the graph's
  diagnostics with the sample cache's use in the log (`aud_plugin_engine`);
  `plugin.json`, the graph document, the presets and the state of an instance
  (`aud_plugin_settings`, see [doc/plugin-config.md](doc/plugin-config.md));
  a host's MIDI as UMP events (`aud_plugin_midi`) and its transport as the
  graph's (`aud_plugin_transport`); the content locations around a plugin's
  bundle and the system's log (`aud_plugin_bundle`); the node packages of a
  build (`aud_plugin_packages`). macOS, iOS and Windows.
- **Build (`scripts/plugin-build.js`).** A Node module a shell's build tool
  requires through the package config: the core's sources, the recipes that
  compile the node packages `aud_dsp_sampler`, `aud_dsp_effects` and
  `aud_audio_script` from their sources (build-001), the generated
  registration of the packages, flags, and a compile cache keyed by sources,
  flags and headers.
- **Dart (`lib/`).** The messages of the shell protocol between a plugin and
  its editor (`AudShellMessage`) and their framing (`AudShellCodec`).

A shell's build defines `AUD_PLUGIN_LOG_SUBSYSTEM` (its bundle id),
`AUD_PLUGIN_LOG_ENV` (the variable that names a test's log file) and, for
RealtimeSanitizer, `AUD_PLUGIN_RTSAN` (`src/aud_plugin_config.hpp`).

## Status

Before 1.0 — the API and the shell protocol may change between minor
versions.

## Contributing

See [doc/guides/develop-guide.md](doc/guides/develop-guide.md). Build and
test the package with:

```bash
dart pub get
node scripts/test-native.js
node --test "test/scripts/**/*.spec.js"
dart test
```

`test-native.js` builds and runs the native unit tests of `test/native` on
macOS and Windows; `node --test` runs those of `scripts/plugin-build.js`,
`dart test` those of the shell protocol.

## License

MIT — see [LICENSE](LICENSE).
