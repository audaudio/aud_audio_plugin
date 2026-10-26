// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

// Where a plugin is and what it finds around it: the binary that holds this
// code, paths with their links resolved, the locations of its content
// inside or next to the app it ships in, and the system's log - a plugin
// writes no file of its own. Paths are UTF-8.
//
// aud_plugin_bundle.cpp holds what every platform shares;
// aud_plugin_bundle_apple.mm (macOS and iOS) and aud_plugin_bundle_win.cpp
// what differs.

#ifndef AUD_PLUGIN_BUNDLE_HPP
#define AUD_PLUGIN_BUNDLE_HPP

#include <string>
#include <vector>

namespace aud_plugin {

// #############################################################################
// The path of the binary that holds this code, as the host loaded it; empty
// when the system does not know it.
std::string binaryPath();

// #############################################################################
// `path` with every link and junction on its way resolved; `path` when it
// cannot be resolved.
std::string resolvedPath(const std::string& path);

// #############################################################################
// The path a content location names for the bundle at `bundle`:
// `containing-app:<path>` inside the .app that holds the bundle,
// `containing-folder:<path>` inside the folder that holds it,
// `app:<bundle id>:<path>` inside the installed app with that id (macOS),
// `bundle:<path>` inside the bundle; empty for none.
std::string locate(const std::string& location, const std::string& bundle);

// #############################################################################
// The first of `locations` that is a directory, for the bundle at `bundle`
// (locate); empty when none exists.
std::string contentDirectory(const std::vector<std::string>& locations,
                             const std::string& bundle);

// #############################################################################
// The .app that holds `path`, e.g. /Applications/A.app for
// /Applications/A.app/Contents/PlugIns/A.appex; empty when none does.
std::string containingApp(const std::string& path);

// #############################################################################
// The folder that holds `path`, e.g. C:\App for C:\App\A.dll; empty for a
// path without a folder.
std::string containingFolder(const std::string& path);

// #############################################################################
// The last part of `path`, e.g. Contents for /A/Contents/.
std::string lastName(const std::string& path);

// #############################################################################
// `path` and `relative`, whose parts are separated by '/', joined with the
// separator of the platform.
std::string joinPath(const std::string& path, const std::string& relative);

// #############################################################################
// Whether `path` is a directory, or a file.
bool isDirectory(const std::string& path);
bool isFile(const std::string& path);

// #############################################################################
// Logs `message` to the system's log under AUD_PLUGIN_LOG_SUBSYSTEM, and to
// the file AUD_PLUGIN_LOG_ENV names when a test sets it; the realtime thread
// never calls it.
void logInfo(const std::string& message);
void logError(const std::string& message);

// #############################################################################
// The file AUD_PLUGIN_LOG_ENV names, where a test wants the log lines;
// empty without it.
std::string logFile();

// #############################################################################
// Per platform

// .............................................................................
// The installed app with the bundle id `identifier`; empty when there is
// none or the platform has no such lookup (iOS, Windows).
std::string appWithIdentifier(const std::string& identifier);

// .............................................................................
// Writes `message` to the system's log: the unified log on Apple's
// platforms, the debugger's output on Windows.
void systemLog(bool error, const std::string& message);

// .............................................................................
// The id of this process.
long processId();

}  // namespace aud_plugin

#endif  // AUD_PLUGIN_BUNDLE_HPP
