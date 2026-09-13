# Contributing to ProtectEye

First off, thank you for considering contributing to ProtectEye! Open-source projects thrive because of contributors like you.

## 📋 Table of Contents
- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
  - [Reporting Bugs](#reporting-bugs)
  - [Suggesting Features](#suggesting-features)
  - [Pull Requests](#pull-requests)
- [Development Setup](#development-setup)
  - [Linux (Ubuntu / Debian / Arch / Fedora)](#linux)
  - [Windows](#windows)
- [Coding Guidelines](#coding-guidelines)
- [Commit Convention](#commit-convention)

---

## 📜 Code of Conduct

By participating in this project, you agree to abide by our [Code of Conduct](CODE_OF_CONDUCT.md). Please treat all contributors with respect and kindness.

---

## 💡 How Can I Contribute?

### Reporting Bugs
Before creating bug reports, please check existing issues to ensure the problem has not already been reported.

When filing a bug report, please include:
- Your operating system and desktop environment (e.g. *CachyOS GNOME 46 Wayland*, *Ubuntu 24.04 Wayland*, *Windows 11 23H2*).
- Clear steps to reproduce the issue.
- Expected behavior vs. actual behavior.
- Screenshots or terminal logs (run `protecteye` from a terminal to see debug logs).

### Suggesting Features
Feature ideas are warmly welcomed! Please open an issue with the **Feature Request** template describing:
- The problem you want solved or the enhancement you'd like to see.
- Why it would be valuable to ProtectEye users.
- Possible implementation ideas or UI mockups.

### Pull Requests
1. Fork the repository on GitHub.
2. Create a feature branch: `git checkout -b feature/my-new-feature`.
3. Ensure your code compiles cleanly and existing unit tests pass:
   ```bash
   cmake -B build -G Ninja
   cmake --build build
   ctest --test-dir build --output-on-failure
   ```
4. Commit your changes using [Conventional Commits](https://www.conventionalcommits.org/):
   - `feat: add sound volume slider in settings`
   - `fix: prevent focus steal on Wayland overlay dismissal`
   - `docs: update installation instructions`
5. Push your branch: `git push origin feature/my-new-feature`.
6. Submit a Pull Request describing your changes.

---

## 🛠️ Development Setup

### Linux
Install Qt 6 and build tools:
- **Arch / CachyOS:** `sudo pacman -S qt6-base qt6-multimedia qt6-svg cmake ninja gcc`
- **Ubuntu / Debian:** `sudo apt install qt6-base-dev qt6-multimedia-dev libqt6svg6-dev cmake ninja-build build-essential`
- **Fedora:** `sudo dnf install qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtsvg-devel cmake ninja-build gcc-c++`

Build and test:
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

### Windows
1. Install **Visual Studio 2022** (with C++ Desktop development) or MinGW64.
2. Install **Qt 6.2+** with MSVC/MinGW toolchain (`Qt Core`, `Gui`, `Widgets`, `Multimedia`, `Svg`).
3. Run:
```cmd
cmake -B build_win -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build build_win
```

---

## 📐 Coding Guidelines

- **C++ Standard:** Modern C++20 standard.
- **Qt Guidelines:** Follow Qt naming conventions (camelCase methods, `m_` prefix for private members, signals and slots).
- **Cross-Platform:** Keep platform-specific code cleanly isolated inside conditional blocks (`#ifdef Q_OS_WIN`, `#ifdef Q_OS_LINUX`). Do not break Wayland or Windows compatibility.
- **UI & Accessibility:** Keep overlays and settings intuitive, clean, and legible. Maintain dark mode styling consistency (`#0c0c12`, `#0f172a`, `#38bdf8`).

Thank you for helping make ProtectEye better for everyone!
