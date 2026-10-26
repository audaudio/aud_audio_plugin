// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

// The macOS and iOS part of aud_plugin_bundle.hpp: the binary through
// dladdr, links through realpath, apps through NSWorkspace (macOS only),
// the unified log.

#include "aud_plugin_bundle.hpp"

#include <TargetConditionals.h>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#endif
#include <dlfcn.h>
#include <limits.h>
#include <os/log.h>
#include <stdlib.h>
#include <unistd.h>

#include "aud_plugin_config.hpp"

namespace aud_plugin {

namespace {

os_log_t pluginLog() {
  static os_log_t log = os_log_create(AUD_PLUGIN_LOG_SUBSYSTEM, "plugin");
  return log;
}

}  // namespace

// #############################################################################
std::string binaryPath() {
  Dl_info info{};
  if (dladdr(reinterpret_cast<const void*>(&binaryPath), &info) == 0 ||
      info.dli_fname == nullptr) {
    return {};
  }
  return info.dli_fname;
}

// #############################################################################
std::string resolvedPath(const std::string& path) {
  char resolved[PATH_MAX];
  if (path.empty() || realpath(path.c_str(), resolved) == nullptr) return path;
  return resolved;
}

// #############################################################################
std::string appWithIdentifier(const std::string& identifier) {
#if TARGET_OS_OSX
  @autoreleasepool {
    NSString* id = [NSString stringWithUTF8String:identifier.c_str()];
    NSURL* url = [[NSWorkspace sharedWorkspace]
        URLForApplicationWithBundleIdentifier:id];
    return url != nil ? std::string(url.path.UTF8String) : std::string();
  }
#else
  (void)identifier;
  return {};
#endif
}

// #############################################################################
void systemLog(bool error, const std::string& message) {
  if (error) {
    os_log_error(pluginLog(), "%{public}s", message.c_str());
  } else {
    os_log_info(pluginLog(), "%{public}s", message.c_str());
  }
}

// #############################################################################
long processId() { return static_cast<long>(getpid()); }

}  // namespace aud_plugin
