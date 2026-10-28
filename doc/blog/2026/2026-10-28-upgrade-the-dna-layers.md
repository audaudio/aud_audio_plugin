<!--
@license
Copyright (c) Audanika. All Rights Reserved.

Use of this source code is governed by terms that can be
found in the LICENSE file in the root of this package.
-->

# Upgrade the DNA layers and dependencies

## Motivation

- The packages of this family are maintained in one pass: every repo moves to
  the current SDK, takes the current versions of its dependencies and rebuilds
  its DNA instances.
- The SDK floor of this repo was current already; its share of the pass is the
  dependencies and the DNA.
- `dna_audanika` ^0.1.0 brings the blog and translate layers, refreshed guides
  and a README check; the old constraint kept all of that out.

## Strategy

- Let `gg do upgrade deps` lift every dependency to its current version and
  tighten the constraints to what resolved.
- Rebuild the DNA with `gg dna build` and let the DNA test of the repo verify
  the instances when the checks run.
- Keep the public API untouched.

## Implementation 1: SDK and dependencies

- `pubspec.yaml`: the SDK constraint stays, it was `>=3.13.0` already.
- Dependencies upgraded: `dna_audanika` ^0.1.0, `helix` ^1.14.0.
- The analyzer and the tests stay green.

## Implementation 2: DNA instances

- `dna/_vars.json` sets what the upgraded layers ask for: company, copyright
  holder, git organisation and URL, Jira prefix and the name of the quick check
  pipeline, `Quick Check`, which the GitHub ruleset requires.
- `gg dna build` instantiated the layers: the blog guide and its templates, the
  translate guide, the refreshed README guides with `scripts/check-readme.js`,
  the updated AI guides and the `/blog`, `/translate` and `/readme` skills.
- gg 17.6 builds with helix 1.13 and folded `.claude/CLAUDE_dna_gg.md` into
  `CLAUDE.md`; the DNA test of the repo runs with helix 1.14 and restored the
  file on the next check. Both landed as `#gg: generated DNA` commits and
  cancel out for `CLAUDE.md`.

## Dependency updates

- `dna_audanika` ^0.1.0: the layers `dna_blog` and `dna_translate`, the README
  check script, refreshed guides and skills, and new required variables (see
  above).
- `helix` 1.14: the DNA engine the test of this repo builds with.

## Open Points

- The README does not follow the new README template yet:
  `scripts/check-readme.js` reports the missing sections and the missing
  language switcher. A follow-up pass brings it in line.
- The translate guide asks for a German copy of every file under `doc`; this
  post has one, the rest follows with the README pass.
- Until gg builds with helix 1.14, `gg dna build` and the DNA test disagree
  about `CLAUDE.md`; the test has the last word.
