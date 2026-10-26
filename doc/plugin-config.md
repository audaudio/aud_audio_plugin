# The plugin configuration

What an app hands a plugin shell to make a plugin of its own: one
configuration, one graph document and its presets, in the plugin's folder
`aud_plugin`. The shell is generic; everything that makes the plugin an
instrument of the app lives in these three files. `readSettings()` of
`src/aud_plugin_settings.hpp` reads them; a shell adds the keys of its
format, e.g. its identity, to `plugin.json` and reads them itself.

## plugin.json

```jsonc
{
  "schema": 1,
  // The name of the plugin.
  "name": "Audanika",
  // The node packages compiled into the plugin and registered with its
  // graph, in this order (scripts/plugin-build.js has a recipe for each);
  // the graph's own nodes are always there.
  "packages": ["aud_dsp_sampler", "aud_dsp_effects", "aud_audio_script"],
  // The graph document and the presets, next to this file.
  "document": "document.json",
  "presets": "presets.json",
  // Where the assets of the document are: the first location that exists
  // is the host's base directory (locate() of aud_plugin_bundle.hpp).
  //   containing-app:<path>    inside the .app that holds the plugin's
  //                            resolved bundle, e.g. through a link
  //   containing-folder:<path> inside the folder that holds the resolved
  //                            bundle
  //   app:<bundle id>:<path>   inside the installed app with that id
  //                            (macOS)
  //   bundle:<path>            inside the plugin's own bundle
  "content": [
    "containing-app:Contents/Resources/content/sfz",
    "app:com.audanika:Contents/Resources/content/sfz"
  ],
  // The voice budget of aud_dsp_sampler for the process; 0 lifts it.
  "voiceBudget": 0,
  // Whether the transport runs on at the host's tempo while the host is
  // stopped; true by default.
  "freeRunningTransport": true,
  // The graph parameters the host sees, under their stable ids
  // (aud_host_param_id of node and parameter); the others stay inside.
  "parameters": [
    { "node": "delay", "param": "mix", "name": "Delay Mix" },
    { "node": "limiter", "param": "gain", "name": "Volume" }
  ],
  // Parameters that send a MIDI control change to the graph's event input,
  // e.g. the slider of scripts that read CC 74 of channel 1 (0 here). Their
  // stable id is aud_host_param_id("control", id).
  "controls": [
    { "id": "slider", "name": "Slider", "channel": 0, "cc": 74,
      "default": 0 }
  ],
  // The editor the shell starts, as the shell finds it; null for the
  // host's generic view.
  "editor": "Audanika Editor.app",
  // The app group of the app the plugin ships in, which a sandboxed editor
  // shares with the plugin.
  "appGroup": "TEAMID.com.audanika",
  // The editor's size in points when a host opens it, and the smallest the
  // host may make it.
  "editorSize": { "width": 1024, "height": 640,
                  "minWidth": 768, "minHeight": 480 }
}
```

## presets.json

```jsonc
{
  "schema": 1,
  // The preset a new instance plays.
  "default": "AUD007871_Welcome_Combi",
  "presets": [
    {
      "id": "AUD007871_Welcome_Combi",
      "name": "Welcome Combi",
      // A node preset (aud_node_preset.schema.json of aud_audio_core) per
      // node the preset sets, applied with aud_host_apply_preset; every
      // preset names the same nodes, so that a change replaces all of a
      // preset. Strings may name an asset of the document as asset:<id>.
      "nodes": {
        "channel-0": { "schema": 1, "type": "aud.sampler.sfz",
                       "strings": { "sfz_file": "asset:f12" } },
        "delay": { "schema": 1, "type": "aud.effects.stereo_delay",
                   "params": { "mix": 0.06 } }
      }
    }
  ]
}
```

## document.json

A graph document of `aud_audio_graph` (`aud_graph_document.schema.json`):
its nodes carry the default preset, its `assets` table names every file a
preset names, relative to the content. The host's MIDI arrives at the
graph's event input (`graph`, port 0) at its sample offsets
(aud_plugin_midi.hpp); a document that plays different nodes per MIDI
channel connects the graph to an `aud.graph.channel_split`, whose event
output n carries channel n + 1.

## The state

`encodeState()` writes the state of an instance as a host's project keeps
it: JSON with the preset id, the plain values of the parameters by stable
id, the values of the controls that moved and the settings the editor sent
last (`AudShellSettings`), so that a project never holds a path of the
machine it was saved on.

```json
{ "schema": 1, "preset": "AUD007871_Welcome_Combi",
  "params": { "123456": 0.2 }, "controls": { "slider": 0.5 },
  "editor": "{...}" }
```
