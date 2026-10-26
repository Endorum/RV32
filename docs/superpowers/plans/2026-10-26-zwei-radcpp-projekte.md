# rv32 als zwei RAD-C++-Projekte — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Das Repo besteht nur noch aus `platform/`, `emulator/` (Qt, RAD C++) und `software/` (Firmware + Kernel als ein Cross-Build, RAD C++); alles andere ist weg.

**Architecture:** Reiner Umzug mit `git mv`, keine Logikänderung an Firmware/Kernel. `software/` hat eine von Hand geschriebene `CMakeLists.txt` (Toolchain vor `project()`), die RAD C++ als "fremd" erkennt und trotzdem baut; Play startet ein von CMake erzeugtes Skript `build/software`, das den Qt-Emulator aufruft. Der Qt-Emulator bekommt `-headless` (Konsolenbetrieb wie früher). Beweis der Gleichheit: `rom.bin`, `kernel.bin`, `disk.img` byteweise gleich vor und nach dem Umzug.

**Tech Stack:** CMake ≥ 3.20 + Ninja, RISC-V GCC (`riscv64-elf-*` aus Homebrew), Qt 6 Widgets, RAD C++ (`~/Programming/VibeCoding/RAD_CPP`).

**Spec:** `docs/superpowers/specs/2026-10-26-zwei-radcpp-projekte-design.md`

## Global Constraints

- Alle Verschiebungen mit `git mv` (Historie bleibt erhalten).
- Keine Änderung an RAD C++.
- Emulator-Projektname `rv32emu`, Binary `emulator/build/rv32emu`. Software-Projektname `software`, Typ `console-c`, Startskript `software/build/software`.
- Header bleiben neben ihren Quellen; kein `software/include/`.
- `software/build/` ist der einzige Build-Ordner der Software (IDE und Terminal-Preset teilen ihn).
- Gelöscht werden: alter `emulator/`, `emulator/tests/`, `tests/` (ISA), Root-`CMakeLists.txt`, Root-`CMakePresets.json`, `sw/*/CMakePresets.json`, alle `.vscode/`, `emulator/CMakeLists.txt.old`.
- Configure-Argumente von RAD C++ (`Toolchain::configureArgs`), für jede Prüfung "wie RAD C++":
  `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_PREFIX_PATH=/opt/homebrew;$HOME/Programming/VibeCoding/RAD_CPP/build/sdk" -DCMAKE_MAKE_PROGRAM=$(which ninja) -DCMAKE_COLOR_DIAGNOSTICS=ON`
  danach `cmake --build build`. Im Folgenden `RADCFG` genannt.
- Der Repo-Pfad enthält `C++` und liegt neben `RISC-V Kopie` (Leerzeichen): jeder Pfad in Skripten und CMake-Befehlen wird gequotet.
- Commit-Nachrichten enden mit `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

## Review Focus

1. **Play in `software/`, bevor der Emulator gebaut ist** → Skript gibt `emulator not built: <pfad> -- open emulator/rv32emu.radcpp in RAD C++ and build it` auf stderr aus und endet mit 1 (Test in Task 3, Step 6).
2. **Frischer Checkout / gelöschtes `software/build`, erster Build aus RAD C++** → Configure mit `RADCFG` findet die Cross-Toolchain und baut alles (Task 3, Step 4 baut immer von einem leeren Ordner).
3. **`-headless` mit stdin als Pipe statt Terminal** → Eingabe kommt an, kein Hängen beim Start (Task 2 testet genau über eine Pipe).
4. **Zusätzliche Run-Argumente aus `software.radcpp`** (z. B. `-d`) → werden vom Skript an den Emulator durchgereicht (Task 3, Step 7).
5. **Pfade mit `+` und Leerzeichen** → Startskript und CMake-Befehle quoten alles (Task 3, Step 7 startet das Skript aus einem anderen Arbeitsverzeichnis heraus).

---

### Task 0: Stand sichern, Worktree, Ausgangsmessung

**Files:** keine Änderungen im Code; Commit von `rv32emu-qt/` im Haupt-Checkout.

**Interfaces:**
- Produces: `$SCRATCH/baseline.sha` mit den SHA-1 von `rom.bin`, `kernel.bin`, `disk.img`; `$SCRATCH/baseline/` mit Kopien dieser drei Dateien (Task 2 bootet damit). `$SCRATCH` = Scratchpad-Ordner der Sitzung.

- [ ] **Step 1: `rv32emu-qt/` im Haupt-Checkout unverändert committen** (ist untracked und wäre sonst im Worktree nicht da). Die lokale Änderung an `emulator/include/devices/UART.hpp` nicht committen; sie steckt schon in `rv32emu-qt/` und der alte Emulator wird gelöscht.

```bash
git add rv32emu-qt && git commit -m "import: Qt emulator rv32emu-qt as RAD C++ project (snapshot before restructure)"
```

- [ ] **Step 2: Prüfen, dass die Sicherung existiert**

Run: `ls "/Users/alex/Programming/C++/EMU_DEV/RISC-V Kopie/rv32/emulator/src/main.cpp"`
Expected: Datei existiert. Sonst STOP und Alex fragen.

- [ ] **Step 3: Worktree anlegen** mit dem Skill `superpowers:using-git-worktrees`, Branch `two-radcpp-projects` von `radcpp-emulator`. Alle weiteren Tasks laufen im Worktree.

- [ ] **Step 4: Ausgangsmessung im Worktree mit dem alten Layout**

```bash
(cd sw/firmware && cmake --preset default && cmake --build --preset default)
(cd sw/kernel   && cmake --preset default && cmake --build --preset default)
mkdir -p "$SCRATCH/baseline"
cp build/firmware/rom.bin build/kernel/kernel.bin build/disk.img "$SCRATCH/baseline/"
(cd "$SCRATCH/baseline" && shasum rom.bin kernel.bin disk.img) > "$SCRATCH/baseline.sha"
```

Expected: drei Zeilen in `baseline.sha`. Danach `rm -rf build` im Worktree (der alte Root-Build wird nicht mehr gebraucht).

---

### Task 1: Emulator umziehen und umbenennen

**Files:**
- Delete: `emulator/` (alt, ganz), `tests/` (ganz)
- Move: `rv32emu-qt/` → `emulator/`, `emulator/rv32emu-qt.radcpp` → `emulator/rv32emu.radcpp`
- Modify: `emulator/rv32emu.radcpp`, `emulator/CMakeLists.txt`, `emulator/project.cmake`

**Interfaces:**
- Produces: `emulator/build/rv32emu` (Qt-Binary), Run-Argumente `-firmware=../software/build/firmware/rom.bin`, `-harddisk=../software/build/disk.img`.

- [ ] **Step 1: Alt löschen, neu verschieben**

```bash
git rm -r -q emulator tests
git mv rv32emu-qt emulator
git mv emulator/rv32emu-qt.radcpp emulator/rv32emu.radcpp
```

- [ ] **Step 2: `emulator/rv32emu.radcpp`**: `name = "rv32emu"`; `[run] args = ["-firmware=../software/build/firmware/rom.bin", "-harddisk=../software/build/disk.img"]`. Rest unverändert.

- [ ] **Step 3: `emulator/CMakeLists.txt`**: jedes `rv32emu-qt` durch `rv32emu` ersetzen (auch in der Kopfzeile `from rv32emu.radcpp` und im Skin-Kommentar). Sonst nichts ändern; das ist exakt die Ausgabe, die RAD C++ für den neuen Namen erzeugt. `emulator/.gitignore` bleibt.

- [ ] **Step 4: `emulator/project.cmake`**: Kopfkommentar "Manual CMake additions for rv32emu."; Target-Namen `rv32emu-qt` → `rv32emu`; Kommentar zu `platform/` ohne "shared with emulator/", stattdessen "shared with software/"; Kommentar "emulator core: -std=c++23 instead of gnu++23" (ohne Verweis auf den alten Emulator).

- [ ] **Step 5: Bauen wie RAD C++**

Run: `cd emulator && rm -rf build && <RADCFG>`
Expected: Build ohne Fehler, `emulator/build/rv32emu` existiert und ist ausführbar.

- [ ] **Step 6: Keine Reste**

Run: `git grep -n "rv32emu-qt" -- emulator`
Expected: keine Treffer.

- [ ] **Step 7: Commit**

```bash
git add -A emulator tests
git commit -m "refactor: Qt emulator becomes emulator/ (rv32emu); drop console emulator and all tests"
```

---

### Task 2: `-headless`

**Files:**
- Modify: `emulator/include/CONFIG.hpp` (Feld), `emulator/src/CONFIG.cpp` (`parse_arguments`), `emulator/src/main.cpp`

**Interfaces:**
- Consumes: `Machine(const Config&)`, `Machine::start()`, `Machine::step()`, `Machine::halted()`, `Machine::uart_output() -> std::string` (alle vorhanden).
- Produces: `bool Config::headless = false;`, Schalter `-headless`.

- [ ] **Step 1: Boot-Test schreiben** als `$SCRATCH/headless_boot.sh` (Testskript, nicht im Repo):

```sh
#!/bin/sh
# usage: headless_boot.sh <emulator> <rom.bin> <disk.img>
out="$(printf 'boot\n' | perl -e 'alarm shift; exec @ARGV' 20 "$1" -headless -firmware="$2" -harddisk="$3" 2>&1)"
for s in "RUNNING FIRMWARE" "starting firmware shell" "Kernel start" "returning from kernel"; do
  printf '%s' "$out" | grep -q "$s" || { echo "MISSING: $s"; printf '%s\n' "$out" | tail -20; exit 1; }
done
echo HEADLESS_OK
```

- [ ] **Step 2: Test laufen lassen, muss fehlschlagen**

Run: `sh "$SCRATCH/headless_boot.sh" emulator/build/rv32emu "$SCRATCH/baseline/rom.bin" "$SCRATCH/baseline/disk.img"`
Expected: `MISSING: ...` (heute bricht `-headless` in `parse_arguments` mit "Expected right side after '='" ab bzw. es öffnet sich ein Fenster ohne Konsolenausgabe).

- [ ] **Step 3: Umsetzen**
  - `Config`: `bool headless = false;` neben `debug/log/step`; `Config::str()` gibt zusätzlich ` HEADLESS: <0|1>` aus.
  - `parse_arguments`: `else if(arg == "headless") config.headless = true;` im selben Stil wie `d/l/s`.
  - `showUsage`: Zeile `  -headless             no window, run in the console (UART on stdout/stdin)` unter "Flags".
  - `main.cpp`: vor `QApplication` prüfen, ob ein Argument genau `-headless` ist. Wenn ja: kein `QApplication`, kein Skin, `parse_arguments`, dann die Schleife aus Spec 5.1 (`start()`, `step()` bis `halted()`, nach jedem Schritt `std::cout << machine.uart_output() << std::flush;`), `return 0`. `argc < 2` → Usage gilt in beiden Fällen. Ohne `-headless` bleibt der Ablauf exakt wie heute.

- [ ] **Step 4: Bauen und Test laufen lassen**

Run: `cmake --build emulator/build && sh "$SCRATCH/headless_boot.sh" emulator/build/rv32emu "$SCRATCH/baseline/rom.bin" "$SCRATCH/baseline/disk.img"`
Expected: `HEADLESS_OK`; es erscheint kein Fenster und kein Dock-Symbol.

- [ ] **Step 5: Fenster-Modus unverändert**

Run: `emulator/build/rv32emu` ohne Argumente
Expected: Usage mit `-headless`-Zeile, Exit-Code 1.

- [ ] **Step 6: Commit**

```bash
git add emulator/include/CONFIG.hpp emulator/src/CONFIG.cpp emulator/src/main.cpp
git commit -m "feat(emulator): -headless runs the machine in the console without a window"
```

---

### Task 3: Software als ein Cross-Build

**Files:**
- Move: `sw/common` → `software/src/common`, `sw/firmware` → `software/src/firmware`, `sw/kernel` → `software/src/kernel`, `cmake/` → `software/cmake/`, `tools/` → `software/tools/`
- Delete: `software/src/firmware/CMakePresets.json`, `software/src/kernel/CMakePresets.json`, `software/src/firmware/.vscode/`, `software/src/kernel/.vscode/`
- Create: `software/CMakeLists.txt`, `software/CMakePresets.json`, `software/software.radcpp`
- Modify: `software/cmake/rv32-image.cmake` (Pfadblock), `software/src/firmware/CMakeLists.txt`, `software/src/kernel/CMakeLists.txt`, Kommentare in `software/cmake/riscv-toolchain.cmake` und `software/tools/mkimage/mkimage.sh`, falls sie alte Pfade nennen

**Interfaces:**
- Consumes: `emulator/build/rv32emu` (Task 1), `-headless` (Task 2).
- Produces: `software/build/firmware/rom.bin`, `software/build/kernel/kernel.bin`, `software/build/disk.img`, `software/build/compile_commands.json`, `software/build/software` (ausführbar), CMake-Ziele `launcher` und `run`.

- [ ] **Step 1: Verschieben und Überflüssiges löschen**

```bash
mkdir -p software/src
git mv sw/common software/src/common
git mv sw/firmware software/src/firmware
git mv sw/kernel software/src/kernel
git mv cmake software/cmake
git mv tools software/tools
git rm -r -q software/src/firmware/CMakePresets.json software/src/kernel/CMakePresets.json \
             software/src/firmware/.vscode software/src/kernel/.vscode
```

Expected: `sw/` existiert nicht mehr (nur ggf. `.DS_Store`, dann löschen).

- [ ] **Step 2: `rv32-image.cmake` Pfadblock** genau nach der Tabelle in Spec 4.2 (`RV32_ROOT` = Ordner über `cmake/`, also `software/`). Kopfkommentar: Ausgaben in `software/build/<image-ordner>/`. Rest der Datei unverändert.

- [ ] **Step 3: Unter-CMakeLists** (`src/firmware`, `src/kernel`): `cmake_minimum_required`, `set(CMAKE_TOOLCHAIN_FILE …)`, `project(…)` und `include(…rv32-image.cmake)` entfernen; in der Firmware das `add_custom_target(run …)` entfernen. `rv32_image(…)`-Aufrufe, Modul-Glob des Kernels und `disk`-Ziel bleiben Zeichen für Zeichen. Kopfkommentare neu: gebaut über `software/` (RAD C++ oder `cmake --preset default` in `software/`), Ausgaben `software/build/firmware/…` bzw. `software/build/kernel/…` und `software/build/disk.img`, "Sees:"-Zeilen mit `src/common/` statt `sw/common/`.

- [ ] **Step 4: `software/CMakeLists.txt`** nach Spec 4.1, plus:
  - Startskript mit `file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/software" CONTENT "…" FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)`, Inhalt exakt das Skript aus Spec 4.3 mit eingesetzten absoluten Pfaden `${RV32_EMULATOR}`, `${CMAKE_BINARY_DIR}/firmware/rom.bin`, `${RV32_DISK_IMAGE}`, alle in doppelten Anführungszeichen.
  - `add_custom_target(launcher ALL DEPENDS rom_bin disk)`.
  - `add_custom_target(run COMMAND "${CMAKE_BINARY_DIR}/software" DEPENDS launcher USES_TERMINAL VERBATIM)`.
  - Kopfkommentar: was das Projekt ist, wie man es in RAD C++ öffnet und im Terminal baut, dass `build/software` der Play-Einstieg ist und warum (RAD C++ startet `build/<name>`).

- [ ] **Step 5: `software/CMakePresets.json`** (Version 6): Configure-Preset `default` (Ninja, `binaryDir` `${sourceDir}/build`); Build-Presets `default` und `run` (`targets: ["run"]`). **`software/software.radcpp`**: Kopfkommentar wie die anderen `.radcpp`, `name = "software"`, `standard = "c17"`, `type = "console-c"`, `[run] args = []`, `working_dir = "."`.

- [ ] **Step 6: Bauen wie RAD C++ und Bytevergleich**

```bash
cd software && rm -rf build && <RADCFG>
cmp build/firmware/rom.bin  "$SCRATCH/baseline/rom.bin"
cmp build/kernel/kernel.bin "$SCRATCH/baseline/kernel.bin"
cmp build/disk.img          "$SCRATCH/baseline/disk.img"
```

Expected: alle drei `cmp` ohne Ausgabe, Exit-Code 0; `build/compile_commands.json` existiert; `build/software` ist ausführbar. Weicht ein Hash ab: Ursache finden (Spec 9), nicht weitermachen.

- [ ] **Step 7: Startskript prüfen**
  - Aus einem anderen Ordner: `cd / && printf 'boot\n' | perl -e 'alarm 20; exec @ARGV' "<repo>/software/build/software" -headless` → Ausgabe enthält `RUNNING FIRMWARE` und `Kernel start` (Review Focus 4 und 5: `-headless` kommt als Zusatzargument durch).
  - Emulator fehlt: `mv emulator/build/rv32emu /tmp/rv32emu.bak; software/build/software; echo $?; mv /tmp/rv32emu.bak emulator/build/rv32emu` → Meldung `emulator not built: …` auf stderr, Exit-Code `1` (Review Focus 1).

- [ ] **Step 8: Commit**

```bash
git add -A software
git commit -m "refactor: software/ is one RAD C++ cross build (firmware + kernel), Play starts the emulator"
```

---

### Task 4: Root aufräumen, README, .gitignore

**Files:**
- Delete: `CMakeLists.txt`, `CMakePresets.json` (Root)
- Modify: `README.md`, `.gitignore`

- [ ] **Step 1: Löschen**: `git rm CMakeLists.txt CMakePresets.json`. Root-`build/` lokal löschen, falls vorhanden.

- [ ] **Step 2: `README.md`** neu nach Spec 6: Kurzbeschreibung, Baum (`platform/`, `emulator/`, `software/` mit `src/common|firmware|kernel`), Tabelle "Who may see what" (emulator → platform; firmware → platform, src/common, src/firmware; kernel → platform, src/common, src/kernel), "Arbeiten": beide `.radcpp` in RAD C++ öffnen, erst Emulator bauen, dann Play in `software`; Terminal: `cd software && cmake --preset default && cmake --build --preset default` bzw. `--preset run`; `-headless` mit Beispiel. Englisch wie bisher.

- [ ] **Step 3: `.gitignore`**: `.DS_Store`, `build/`, `.cache/`, `*.radcpp.user`, je mit kurzem Kommentar wie heute.

- [ ] **Step 4: Keine alten Pfade mehr**

Run: `git grep -nE "sw/|rv32emu-qt|tests/isa|build/all|run_riscv_tests|\.vscode" -- ':!docs/superpowers'`
Expected: keine Treffer (Treffer in `docs/superpowers/` sind Geschichte und bleiben).

Run: `git ls-files | cut -d/ -f1 | sort -u`
Expected: genau `.gitignore`, `README.md`, `docs`, `emulator`, `platform`, `software`.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "chore: root is just platform/, emulator/, software/; new README"
```

---

### Task 5: Abnahme durch Alex in RAD C++

Keine Codeänderung. Alex prüft und meldet zurück:

- [ ] `emulator/rv32emu.radcpp` öffnen, Play: Fenster erscheint, Firmware läuft (nachdem `software/` einmal gebaut ist).
- [ ] `software/software.radcpp` öffnen: Baum zeigt `common`, `firmware`, `kernel`; `kernel/mm/page` ist eine Unit mit `.c` und `.h`.
- [ ] Play in `software`: Build läuft (grauer Hinweis "fremde CMakeLists" ist erwartet), Qt-Emulator startet, Firmware-Shell erscheint, `boot` lädt den Kernel.
- [ ] Danach Branch abschließen mit `superpowers:finishing-a-development-branch`.
