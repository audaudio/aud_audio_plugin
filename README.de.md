# aud_audio_plugin

[English](README.md) | Deutsch

[![CI](https://github.com/audaudio/aud_audio_plugin/actions/workflows/quick_check.yaml/badge.svg)](https://github.com/audaudio/aud_audio_plugin/actions/workflows/quick_check.yaml)

Formatneutraler Kern der Plugin-Shells der Audanika Audio Engine: Engine
über dem Headless Host, Plugin-Konfiguration, Presets, Zustand, MIDI und
Transport. Läuft auf macOS, iOS und Windows.

## Motivation

Eine Plugin-Shell verpackt die Engine in ein Plugin-Format. Alles, was
nicht vom Format abhängt — die Engine über dem Headless Host, die
Plugin-Konfiguration, Presets und Zustand, das MIDI und der Transport eines
Hosts, die Orte des Contents und der Build der Node-Pakete — müsste sonst
pro Shell neu geschrieben werden. Dieses Paket hält es einmal, damit jede
Shell den Graphen einer App gleich spielt und eine Shell nur noch das
Format beisteuert.

## Installation

Das Paket ist nicht auf pub.dev veröffentlicht; binde es über seine
Git-Tags ein:

```yaml
dependencies:
  aud_audio_plugin:
    git:
      url: git@github.com:audaudio/aud_audio_plugin.git
      tag_pattern: "{{version}}"
    version: ^0.1.0
```

## Verwendung

Der Dart-Teil ist das Shell-Protokoll zwischen einem Plugin und seinem
Editor: die Nachrichten (`AudShellMessage`) und ihre Rahmung
(`AudShellCodec`).

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

Es gibt aus:

```text
70
[AudShellHello({type: hello, instance: 0, token: abc, pid: 4711, version: 3})]
```

## Grenzen

- Kein eigenes Plugin-Format: eine Shell wie ein VST3- oder AUv3-Wrapper
  setzt das Format auf diesen Kern
- Der C++-Kern baut nur auf macOS, iOS und Windows
- Das Dart-Paket enthält nur das Shell-Protokoll, nicht die Engine; die
  Engine erreicht man über den Build des C++-Kerns einer Shell

## Dokumentation

- [Plugin-Konfiguration](doc/plugin-config.md): `plugin.json`, das
  Graph-Dokument, Presets und der Zustand einer Instanz
- [Guides](doc/guides/): Entwicklungs-, Code-, Test- und
  Dokumentationsleitfäden
- [Blog](doc/de/blog/): die Beiträge zu den Änderungen des Pakets

## Funktionsweise

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

## Status

Vor 1.0 — die API und das Shell-Protokoll können sich zwischen
Minor-Versionen ändern.

## Mitwirken

Siehe [doc/guides/develop-guide.md](doc/guides/develop-guide.md). Baue und
teste das Paket mit:

```bash
dart pub get
node scripts/test-native.js
node --test "test/scripts/**/*.spec.js"
dart test
```

`test-native.js` baut und startet die nativen Unit-Tests von `test/native`
auf macOS und Windows; `node --test` startet die von
`scripts/plugin-build.js`, `dart test` die des Shell-Protokolls.

## Lizenz

MIT — siehe [LICENSE](LICENSE).
