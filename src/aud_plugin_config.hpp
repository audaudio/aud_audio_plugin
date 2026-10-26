// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

// The settings a shell's build gives the shared core: the defines below are
// set by the build of the shell, the defaults serve the native tests.
//
// - AUD_PLUGIN_LOG_SUBSYSTEM: the subsystem of the system's log, the bundle
//   id of the plugin
// - AUD_PLUGIN_LOG_ENV: the environment variable that names the file a test
//   wants the log lines in
// - AUD_PLUGIN_RTSAN: marks the realtime path for RealtimeSanitizer

#ifndef AUD_PLUGIN_CONFIG_HPP
#define AUD_PLUGIN_CONFIG_HPP

#ifndef AUD_PLUGIN_LOG_SUBSYSTEM
#define AUD_PLUGIN_LOG_SUBSYSTEM "io.audaudio.plugin"
#endif

#ifndef AUD_PLUGIN_LOG_ENV
#define AUD_PLUGIN_LOG_ENV "AUD_PLUGIN_LOG"
#endif

// #############################################################################
// With AUD_PLUGIN_RTSAN the realtime path carries Clang's nonblocking effect,
// so that RealtimeSanitizer checks what it calls.
#if AUD_PLUGIN_RTSAN && defined(__clang__)
#define AUD_PLUGIN_NONBLOCKING [[clang::nonblocking]]
#else
#define AUD_PLUGIN_NONBLOCKING
#endif

#endif  // AUD_PLUGIN_CONFIG_HPP
