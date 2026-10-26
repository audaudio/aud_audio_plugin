# aud_audio_plugin

The format-agnostic core of the plugin shells of the Audanika Audio Engine.
A plugin shell wraps the engine in a plugin format; everything that does not
depend on the format lives here, so that every shell plays an app's graph the
same way.

## What the package holds

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

## Build and test

```bash
dart pub get
node scripts/test-native.js
node --test "test/scripts/**/*.spec.js"
dart test
```

`test-native.js` builds and runs the native unit tests of `test/native` on
macOS and Windows; `node --test` runs those of `scripts/plugin-build.js`,
`dart test` those of the shell protocol.
