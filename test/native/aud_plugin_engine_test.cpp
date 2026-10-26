// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

// The engine over the headless host (aud_plugin_engine.hpp) with the
// graph's own nodes: opening a document, its parameters, presets, blocks
// rendered offline as a host renders them, and the graph's diagnostics in
// the log.

#include "aud_plugin_engine.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include "aud_plugin_harness.hpp"
#include "aud_plugin_midi.hpp"

using namespace aud_plugin;
using aud_plugin_test::expectEqual;
using aud_plugin_test::expectNear;
using aud_plugin_test::group;
using aud_plugin_test::test;

namespace {

const char* const kConfig = R"({ "schema": 1, "name": "Test" })";

const char* const kDocument = R"({
  "schema": 1,
  "name": "test",
  "outputChannels": [2],
  "nodes": [
    { "id": "osc", "type": "aud.graph.oscillator",
      "preset": { "schema": 1, "type": "aud.graph.oscillator",
                  "params": { "amplitude": 0.3, "waveform": 1 } } },
    { "id": "filter", "type": "aud.graph.filter",
      "preset": { "schema": 1, "type": "aud.graph.filter",
                  "params": { "cutoff": 1200, "resonance": 0.3,
                              "mode": 0 } } },
    { "id": "out", "type": "aud.graph.mixer",
      "inputChannels": [2, 2, 2, 2, 2, 2, 2, 2], "outputChannels": [2],
      "preset": { "schema": 1, "type": "aud.graph.mixer",
                  "params": { "master": 0.5 } } }
  ],
  "connections": [
    { "from": "osc", "to": "filter" },
    { "from": "filter", "to": "out" },
    { "from": "out", "to": "graph" }
  ],
  "eventConnections": [{ "from": "graph", "to": "osc" }]
})";

const char* const kPresets = R"({
  "schema": 1,
  "default": "saw",
  "presets": [
    { "id": "saw", "nodes": {
        "filter": { "schema": 1, "type": "aud.graph.filter",
                    "params": { "cutoff": 1200 } } } },
    { "id": "sine", "nodes": {
        "filter": { "schema": 1, "type": "aud.graph.filter",
                    "params": { "cutoff": 4000 } } } }
  ]
})";

constexpr double kSampleRate = 48000;
constexpr uint32_t kFrames = 256;

// The settings of the test plugin.
Settings settings() {
  Settings result;
  std::string error;
  if (!parseSettings(kConfig, kDocument, kPresets, &result, &error)) {
    aud_plugin_test::fail("settings: " + error, __FILE__, __LINE__);
  }
  return result;
}

// An engine with the test document, active.
struct Opened {
  Opened() {
    std::string error;
    ok = engine.open(kDocument, "", kSampleRate, kFrames, &error) &&
         engine.setActive(true);
    if (!ok) aud_plugin_test::fail("open: " + error, __FILE__, __LINE__);
  }

  // Renders `blocks` blocks offline and returns the peak of the left channel.
  float render(int blocks) {
    std::vector<float> left(kFrames), right(kFrames);
    float* outputs[] = {left.data(), right.data()};
    float peak = 0;
    for (int i = 0; i < blocks; ++i) {
      EngineBlock block;
      block.outputs = outputs;
      block.channels = 2;
      block.frames = kFrames;
      block.offline = true;
      engine.waitForParams();
      engine.render(block);
      for (const float sample : left) peak = std::max(peak, std::fabs(sample));
    }
    return peak;
  }

  Engine engine;
  bool ok = false;
};

// Points the log of the plugin at `file` (AUD_PLUGIN_LOG_ENV), or nowhere
// for an empty one.
void logTo(const std::string& file) {
#if defined(_WIN32)
  _putenv_s(AUD_PLUGIN_LOG_ENV, file.c_str());
#else
  if (file.empty()) {
    unsetenv(AUD_PLUGIN_LOG_ENV);
  } else {
    setenv(AUD_PLUGIN_LOG_ENV, file.c_str(), 1);
  }
#endif
}

// The text of `file`; empty when it does not exist.
std::string textOf(const std::string& file) {
  std::ifstream in(file);
  std::stringstream text;
  text << in.rdbuf();
  return text.str();
}

const EngineParam* paramNamed(const Engine& engine, const std::string& node,
                              const std::string& param) {
  for (const EngineParam& p : engine.params()) {
    if (p.nodeId == node && p.paramId == param) return &p;
  }
  return nullptr;
}

}  // namespace

int main() {
  group("open", [] {
    test("loads the document and lists its parameters by id", [] {
      Opened opened;
      expectEqual(opened.ok, true);
      const auto& params = opened.engine.params();
      expectEqual(params.empty(), false);
      bool sorted = true;
      for (size_t i = 1; i < params.size(); ++i) {
        sorted = sorted && params[i - 1].id < params[i].id;
      }
      expectEqual(sorted, true);
      const EngineParam* cutoff = paramNamed(opened.engine, "filter", "cutoff");
      expectEqual(cutoff != nullptr, true);
      if (cutoff == nullptr) return;
      expectEqual(cutoff->id, aud_host_param_id("filter", "cutoff"));
      expectEqual(opened.engine.param(cutoff->id) == cutoff, true);
      expectNear(cutoff->defaultValue, 1200, 0.01);
    });
    test("refuses a document it cannot load", [] {
      Engine engine;
      std::string error;
      expectEqual(engine.open("{}", "", kSampleRate, kFrames, &error), false);
      expectEqual(error.empty(), false);
    });
  });

  group("parameters", [] {
    test("a set value reads back, clamped to the range", [] {
      Opened opened;
      const uint32_t id = aud_host_param_id("out", "master");
      opened.engine.setParam(id, 0.25f);
      expectNear(opened.engine.value(id), 0.25, 1e-6);
      const EngineParam* master = opened.engine.param(id);
      expectEqual(master != nullptr, true);
      if (master == nullptr) return;
      opened.engine.setParam(id, master->max + 100);
      expectNear(opened.engine.value(id), master->max, 1e-6);
    });
    test("a pushed change reaches the host before the next offline block", [] {
      Opened opened;
      const uint32_t id = aud_host_param_id("filter", "cutoff");
      opened.render(1);
      opened.engine.pushParam(id, 2000);
      opened.render(1);
      expectNear(opened.engine.value(id), 2000, 0.01);
      expectEqual(opened.engine.droppedParams(), 0u);
    });
  });

  group("presets", [] {
    test("applyPreset sets the values of its nodes", [] {
      Opened opened;
      const Settings s = settings();
      std::string error;
      expectEqual(opened.engine.applyPreset(s.presets[1], &error), true);
      expectNear(opened.engine.value(aud_host_param_id("filter", "cutoff")),
                 4000, 0.01);
    });
    test("a requested preset is applied and reported", [] {
      Opened opened;
      const Settings s = settings();
      int applied = -1;
      opened.engine.setPresets(&s.presets,
                               [&](uint32_t index) { applied = int(index); });
      opened.engine.requestPreset(1);
      expectEqual(opened.engine.waitForPreset(2000), true);
      expectEqual(applied, 1);
      expectNear(opened.engine.value(aud_host_param_id("filter", "cutoff")),
                 4000, 0.01);
    });
  });

  group("render", [] {
    test("plays the oscillator through filter and mixer", [] {
      Opened opened;
      expectEqual(opened.render(8) > 0.01f, true);
    });
    test("is silent before a document is open", [] {
      Engine engine;
      std::vector<float> left(kFrames, 1), right(kFrames, 1);
      float* outputs[] = {left.data(), right.data()};
      EngineBlock block;
      block.outputs = outputs;
      block.channels = 2;
      block.frames = kFrames;
      engine.render(block);
      expectEqual(left[0], 0.0f);
      expectEqual(right[kFrames - 1], 0.0f);
    });
    test("has no latency and a finite tail", [] {
      Opened opened;
      expectEqual(opened.engine.latency(), 0);
      expectEqual(opened.engine.tail() < 48000u * 60u, true);
    });
  });

  group("diagnosticName(code)", [] {
    test("names the codes the graph reports", [] {
      expectEqual(diagnosticName(AUD_ERROR_UNDERFLOW), std::string("stream underflow"));
      expectEqual(diagnosticName(AUD_ERROR_OVERLOAD), std::string("overload"));
      expectEqual(diagnosticName(AUD_ERROR_LATE), std::string("late events"));
      expectEqual(diagnosticName(AUD_ERROR_QUEUE_FULL), std::string("queue full"));
      expectEqual(diagnosticName(AUD_ERROR_CAPACITY), std::string("notes refused"));
      expectEqual(diagnosticName(AUD_ERROR_RETIRED), std::string("events of a retired node"));
      expectEqual(diagnosticName(AUD_ERROR_STATE), std::string("parked for state"));
      expectEqual(diagnosticName(AUD_ERROR_UNSUPPORTED), std::string("unsupported"));
      expectEqual(diagnosticName(AUD_GRAPH_ERROR_REALTIME_VIOLATION),
                  std::string("realtime violation"));
    });
    test("gives a code without a name its number", [] {
      expectEqual(diagnosticName(-999), std::string("code -999"));
    });
  });

  group("diagnostics", [] {
    test("the control thread logs what the graph reports", [] {
      const std::string file =
          (std::filesystem::temp_directory_path() / "aud_plugin_engine_test.log").string();
      std::filesystem::remove(file);
      logTo(file);
      {
        Opened opened;
        // A block with more notes than the graph's queue holds: it drops
        // the rest and reports the queue full.
        std::vector<float> left(kFrames), right(kFrames);
        float* outputs[] = {left.data(), right.data()};
        std::vector<AudEvent> events;
        for (uint32_t i = 0; i < 20000; ++i) {
          events.push_back(midiEvent(i % 2 == 0 ? 0x90 : 0x80, 60, 100, 0));
        }
        EngineBlock block;
        block.outputs = outputs;
        block.channels = 2;
        block.frames = kFrames;
        block.events = events.data();
        block.numEvents = static_cast<uint32_t>(events.size());
        opened.engine.render(block);
        for (int i = 0; i < 100 && textOf(file).find("Diagnostics:") == std::string::npos; ++i) {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
      }
      logTo("");
      const std::string text = textOf(file);
      expectEqual(text.find("Diagnostics:") != std::string::npos, true);
      expectEqual(text.find(" cache ") != std::string::npos, true);
      std::filesystem::remove(file);
    });
  });

  return aud_plugin_test::finish();
}
