// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

/// The format-agnostic core of the plugin shells of the Audanika Audio
/// Engine. The core itself is C++ (`src/`) that a shell's build compiles
/// with `scripts/plugin-build.js`; this library is the Dart end of the
/// shell protocol between a plugin and its editor: the messages and their
/// framing.
library;

export 'src/aud_shell_codec.dart';
export 'src/aud_shell_message.dart';
