// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

// The Windows part of aud_plugin_bundle.hpp: the binary through the module
// that holds this code, junctions and links resolved by the file system,
// the debugger's output as the log. Windows keeps no registry of apps by
// bundle id, so `app:` finds nothing.

#include "aud_plugin_bundle.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "aud_plugin_config.hpp"

namespace aud_plugin {

namespace {

std::wstring wide(const std::string& text) {
  if (text.empty()) return {};
  const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(),
                                         static_cast<int>(text.size()), nullptr, 0);
  std::wstring result(static_cast<size_t>(length), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                      &result[0], length);
  return result;
}

std::string utf8(const std::wstring& text) {
  if (text.empty()) return {};
  const int length =
      WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                          nullptr, 0, nullptr, nullptr);
  std::string result(static_cast<size_t>(length), '\0');
  WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                      &result[0], length, nullptr, nullptr);
  return result;
}

// `path` without the prefix of a long path: \\?\C:\... or \\?\UNC\server\...
std::wstring withoutLongPrefix(const std::wstring& path) {
  if (path.compare(0, 8, L"\\\\?\\UNC\\") == 0) return L"\\\\" + path.substr(8);
  if (path.compare(0, 4, L"\\\\?\\") == 0) return path.substr(4);
  return path;
}

}  // namespace

// #############################################################################
std::string binaryPath() {
  HMODULE module = nullptr;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(&binaryPath), &module)) {
    return {};
  }
  std::wstring file(MAX_PATH, L'\0');
  for (;;) {
    const DWORD length =
        GetModuleFileNameW(module, &file[0], static_cast<DWORD>(file.size()));
    if (length == 0) return {};
    if (length < file.size()) {
      file.resize(length);
      break;
    }
    file.resize(file.size() * 2);
  }
  return utf8(withoutLongPrefix(file));
}

// #############################################################################
std::string resolvedPath(const std::string& path) {
  if (path.empty()) return path;
  // A directory opens only with backup semantics; the handle's final path
  // has every junction and link of the way resolved.
  const HANDLE handle =
      CreateFileW(wide(path).c_str(), 0,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                  OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (handle == INVALID_HANDLE_VALUE) return path;
  std::wstring resolved(MAX_PATH, L'\0');
  DWORD length = GetFinalPathNameByHandleW(handle, &resolved[0],
                                           static_cast<DWORD>(resolved.size()),
                                           FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
  if (length >= resolved.size()) {
    resolved.resize(length);
    length = GetFinalPathNameByHandleW(handle, &resolved[0], length,
                                       FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
  }
  CloseHandle(handle);
  if (length == 0 || length >= resolved.size()) return path;
  resolved.resize(length);
  return utf8(withoutLongPrefix(resolved));
}

// #############################################################################
std::string appWithIdentifier(const std::string&) { return {}; }

// #############################################################################
void systemLog(bool error, const std::string& message) {
  const std::string line = std::string("[" AUD_PLUGIN_LOG_SUBSYSTEM "] ") +
                           (error ? "error: " : "") + message + "\n";
  OutputDebugStringW(wide(line).c_str());
}

// #############################################################################
long processId() { return static_cast<long>(GetCurrentProcessId()); }

}  // namespace aud_plugin
