// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

// Where a plugin finds its content (aud_plugin_bundle.hpp): the folders
// around a bundle, joined paths, and the content locations of plugin.json
// on a temporary tree.

#include "aud_plugin_bundle.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "aud_plugin_harness.hpp"

using namespace aud_plugin;
using aud_plugin_test::expectEqual;
using aud_plugin_test::group;
using aud_plugin_test::test;

namespace fs = std::filesystem;

namespace {

#if _WIN32
const std::string sep = "\\";
#else
const std::string sep = "/";
#endif

// A tree of an app with its plugin and its content, removed at the end.
struct Tree {
  Tree() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    root = fs::temp_directory_path() / ("aud_plugin_bundle_" + std::to_string(stamp));
    fs::create_directories(root / "App" / "content" / "sfz");
    fs::create_directories(root / "App" / "Audanika.plugin" / "Contents" / "Resources" / "own");
    fs::create_directories(root / "Mac.app" / "Contents" / "Resources" / "sfz");
    fs::create_directories(root / "Mac.app" / "Contents" / "PlugIns" / "A.plugin");
    std::ofstream(root / "App" / "Audanika.exe") << "MZ";
  }

  ~Tree() {
    std::error_code error;
    fs::remove_all(root, error);
  }

  std::string path(const std::string& relative) const {
    return joinPath(root.u8string(), relative);
  }

  fs::path root;
};

}  // namespace

int main() {
  group("containingFolder", [] {
    test("is the folder above a bundle", [] {
      expectEqual(containingFolder("/Apps/A/A.plugin"), std::string("/Apps/A"));
      expectEqual(containingFolder("C:\\Apps\\A\\A.plugin"), std::string("C:\\Apps\\A"));
    });
    test("ignores a trailing separator", [] {
      expectEqual(containingFolder("C:\\Apps\\A.plugin\\"), std::string("C:\\Apps"));
    });
    test("keeps the separator of a root", [] {
      expectEqual(containingFolder("C:\\A.plugin"), std::string("C:\\"));
      expectEqual(containingFolder("/A.plugin"), std::string("/"));
    });
    test("is empty without a folder", [] {
      expectEqual(containingFolder("A.plugin"), std::string());
    });
  });

  group("lastName", [] {
    test("is the last part, also before a trailing separator", [] {
      expectEqual(lastName("/A.plugin/Contents/"), std::string("Contents"));
      expectEqual(lastName("C:\\A\\x86_64-win"), std::string("x86_64-win"));
      expectEqual(lastName("A"), std::string("A"));
    });
  });

  group("resolvedPath", [] {
    test("is the path itself when nothing resolves it", [] {
      expectEqual(resolvedPath(""), std::string());
      expectEqual(resolvedPath("/no/such/path"), std::string("/no/such/path"));
    });
  });

  group("binaryPath", [] {
    test("is the executable of the test", [] {
      expectEqual(isFile(binaryPath()), true);
    });
  });

  group("joinPath", [] {
    test("joins with the separator of the platform", [] {
      expectEqual(joinPath("base", "a/b"), "base" + sep + "a" + sep + "b");
    });
    test("adds no second separator", [] {
      expectEqual(joinPath("base/", "a"), "base/a");
      expectEqual(joinPath("base\\", "a"), "base\\a");
    });
  });

  group("containingApp", [] {
    test("is the outermost app", [] {
      expectEqual(containingApp("/Applications/A.app/Contents/PlugIns/B.plugin"),
                  std::string("/Applications/A.app"));
    });
    test("is empty outside an app", [] {
      expectEqual(containingApp("C:\\Apps\\A.plugin"), std::string());
    });
  });

  group("locate", [] {
    test("names a file next to the bundle, e.g. the editor on Windows", [] {
      Tree tree;
      const std::string editor =
          locate("containing-folder:Audanika.exe", tree.path("App/Audanika.plugin"));
      expectEqual(editor, tree.path("App/Audanika.exe"));
      expectEqual(isFile(editor), true);
      expectEqual(isDirectory(editor), false);
    });
    test("names nothing for an unknown kind or an app outside one", [] {
      expectEqual(locate("unknown:x", "C:\\A.plugin"), std::string());
      expectEqual(locate("containing-app:x", "C:\\A.plugin"), std::string());
    });
  });

  group("contentDirectory", [] {
    test("finds the content in the folder that holds the bundle", [] {
      Tree tree;
      const std::string bundle = tree.path("App/Audanika.plugin");
      expectEqual(contentDirectory({"containing-folder:content/sfz"}, bundle),
                  tree.path("App/content/sfz"));
    });
    test("finds the content inside the app that holds the bundle", [] {
      Tree tree;
      const std::string bundle = tree.path("Mac.app/Contents/PlugIns/A.plugin");
      expectEqual(contentDirectory({"containing-app:Contents/Resources/sfz"}, bundle),
                  tree.path("Mac.app/Contents/Resources/sfz"));
    });
    test("finds the content inside the bundle", [] {
      Tree tree;
      const std::string bundle = tree.path("App/Audanika.plugin");
      expectEqual(contentDirectory({"bundle:Contents/Resources/own"}, bundle),
                  tree.path("App/Audanika.plugin/Contents/Resources/own"));
    });
    test("takes the first location that exists", [] {
      Tree tree;
      const std::string bundle = tree.path("App/Audanika.plugin");
      expectEqual(contentDirectory({"containing-app:Contents/Resources/sfz",
                                    "containing-folder:missing",
                                    "containing-folder:content/sfz",
                                    "bundle:Contents/Resources/own"},
                                   bundle),
                  tree.path("App/content/sfz"));
    });
    test("is empty when no location exists", [] {
      Tree tree;
      const std::string bundle = tree.path("App/Audanika.plugin");
      expectEqual(contentDirectory({"containing-folder:missing", "unknown:content"}, bundle),
                  std::string());
    });
  });

  return aud_plugin_test::finish();
}
