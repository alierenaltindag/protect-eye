# ProtectEye - Fedora & Red Hat Linux Kurulum Rehberi

ProtectEye, Fedora Workstation (GNOME), Fedora KDE Spin ve Fedora XFCE üzerinde tam uyumlu şekilde çalışır.

## 1. Hazır RPM Paketi ile Kurulum (Önerilen)

GitHub Releases sayfasından en son `.rpm` paketini indirin (`protecteye-X.Y.Z-1.x86_64.rpm`):

```bash
sudo dnf install ./protecteye-*.rpm
```

## 2. Evrensel Kurulum Scripti ile Kurulum

```bash
curl -fsSL https://raw.githubusercontent.com/alierenaltindag/protect-eye/main/installer.sh | bash
```

## 3. Fedora Üzerinde Kaynak Koddan Derleme

```bash
sudo dnf install -y cmake ninja-build gcc-c++ \
  qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtsvg-devel

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

## 4. Fedora GNOME Wayland İçin Not

Fedora Workstation varsayılan olarak GNOME Wayland ile gelir. GNOME sistem tepsisi (AppIndicator) simgelerini görüntülemek için standart GNOME eklentisinin aktif olduğundan emin olun:

```bash
sudo dnf install -y gnome-shell-extension-appindicator
```
