# aud_audio_plugin

Der formatneutrale Kern der Plugin-Shells der Audanika Audio Engine. Eine
Plugin-Shell verpackt die Engine in ein Plugin-Format; alles, was nicht vom
Format abhängt, liegt hier, damit jede Shell den Graphen einer App gleich
spielt.

## Was das Paket enthält

- **C++-Kern (`src/`, Namespace `aud_plugin`).** Die Engine über dem
  Headless Host von `aud_audio_graph` mit ihrem Control-Thread, dem
  lock-freien Parameter-Ring, Presets, den Wartezeiten beim
  Offline-Rendering und den Diagnosen des Graphen mit dem Verbrauch des
  Sample-Caches im Log (`aud_plugin_engine`); `plugin.json`, das
  Graph-Dokument, die Presets und der Zustand einer Instanz
  (`aud_plugin_settings`, siehe [doc/plugin-config.md](doc/plugin-config.md));
  das MIDI eines Hosts als UMP-Events (`aud_plugin_midi`) und sein Transport
  als der des Graphen (`aud_plugin_transport`); die Orte des Contents rund um
  das Bundle eines Plugins und das Log des Systems (`aud_plugin_bundle`); die
  Node-Pakete eines Builds (`aud_plugin_packages`). macOS, iOS und Windows.
- **Build (`scripts/plugin-build.js`).** Ein Node-Modul, das das Build-Tool
  einer Shell über die Package-Config lädt: die Quellen des Kerns, die
  Rezepte, die die Node-Pakete `aud_dsp_sampler`, `aud_dsp_effects` und
  `aud_audio_script` aus ihren Quellen kompilieren (build-001), die
  generierte Registrierung der Pakete, Flags und ein Compile-Cache, der von
  Quellen, Flags und Headern abhängt.
- **Dart (`lib/`).** Die Nachrichten des Shell-Protokolls zwischen einem
  Plugin und seinem Editor (`AudShellMessage`) und ihre Rahmung
  (`AudShellCodec`).

Der Build einer Shell definiert `AUD_PLUGIN_LOG_SUBSYSTEM` (ihre Bundle-ID),
`AUD_PLUGIN_LOG_ENV` (die Variable, die die Log-Datei eines Tests nennt) und,
für den RealtimeSanitizer, `AUD_PLUGIN_RTSAN` (`src/aud_plugin_config.hpp`).

## Bauen und testen

```bash
dart pub get
node scripts/test-native.js
node --test "test/scripts/**/*.spec.js"
dart test
```

`test-native.js` baut und startet die nativen Unit-Tests von `test/native`
auf macOS und Windows; `node --test` startet die von
`scripts/plugin-build.js`, `dart test` die des Shell-Protokolls.
