<!--
@license
Copyright (c) Audanika. All Rights Reserved.

Use of this source code is governed by terms that can be
found in the LICENSE file in the root of this package.
-->

# DNA-Schichten und Abhängigkeiten aktualisieren

## Motivation

- Die Pakete dieser Familie werden in einem Durchgang gepflegt: Jedes Repo
  wechselt auf das aktuelle SDK, übernimmt die aktuellen Versionen seiner
  Abhängigkeiten und baut seine DNA-Instanzen neu.
- Die SDK-Untergrenze dieses Repos war bereits aktuell; sein Anteil am
  Durchgang sind die Abhängigkeiten und die DNA.
- `dna_audanika` ^0.1.0 bringt die Blog- und Translate-Schichten, überarbeitete
  Guides und eine README-Prüfung; die alte Bedingung hielt all das draußen.

## Strategie

- `gg do upgrade deps` jede Abhängigkeit auf ihre aktuelle Version heben und
  die Bedingungen auf das Aufgelöste festziehen lassen.
- Die DNA mit `gg dna build` neu bauen und den DNA-Test des Repos die Instanzen
  prüfen lassen, wenn die Checks laufen.
- Die öffentliche API unangetastet lassen.

## Realisierung 1: SDK und Abhängigkeiten

- `pubspec.yaml`: Die SDK-Bedingung bleibt, sie war bereits `>=3.13.0`.
- Aktualisierte Abhängigkeiten: `dna_audanika` ^0.1.0, `helix` ^1.14.0.
- Analyzer und Tests bleiben grün.

## Realisierung 2: DNA-Instanzen

- `dna/_vars.json` setzt, was die aktualisierten Schichten verlangen: Firma,
  Rechteinhaber, Git-Organisation und -URL, Jira-Präfix und den Namen der
  Quick-Check-Pipeline, `Quick Check`, den das GitHub-Ruleset verlangt.
- `gg dna build` hat die Schichten instanziiert: den Blog-Guide mit seinen
  Vorlagen, den Translate-Guide, die überarbeiteten README-Guides mit
  `scripts/check-readme.js`, die aktualisierten KI-Guides und die Skills
  `/blog`, `/translate` und `/readme`.
- gg 17.6 baut mit helix 1.13 und hat `.claude/CLAUDE_dna_gg.md` in `CLAUDE.md`
  eingefaltet; der DNA-Test des Repos läuft mit helix 1.14 und hat die Datei
  beim nächsten Check wiederhergestellt. Beides landete als Commits `#gg:
  generated DNA` und hebt sich für `CLAUDE.md` auf.

## Aktualisierte Abhängigkeiten

- `dna_audanika` ^0.1.0: die Schichten `dna_blog` und `dna_translate`, das
  README-Prüfskript, überarbeitete Guides und Skills sowie neue
  Pflichtvariablen (siehe oben).
- `helix` 1.14: die DNA-Engine, mit der der Test dieses Repos baut.

## Offene Punkte

- Die README folgt der neuen README-Vorlage noch nicht:
  `scripts/check-readme.js` meldet die fehlenden Abschnitte und den fehlenden
  Sprachumschalter. Ein Folgedurchgang bringt sie in Form.
- Der Translate-Guide verlangt eine deutsche Kopie jeder Datei unter `doc`;
  dieser Post hat eine, der Rest folgt mit dem README-Durchgang.
- Bis gg mit helix 1.14 baut, sind sich `gg dna build` und der DNA-Test über
  `CLAUDE.md` uneins; der Test hat das letzte Wort.
