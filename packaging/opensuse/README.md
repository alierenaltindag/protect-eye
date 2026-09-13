# ProtectEye - openSUSE (Tumbleweed & Leap) Kurulum Rehberi

ProtectEye, openSUSE Tumbleweed ve openSUSE Leap üzerinde tam uyumlu şekilde çalışır.

## 1. Hazır RPM Paketi ile Kurulum (Önerilen)

GitHub Releases sayfasından en son `.rpm` paketini indirin (`protecteye-X.Y.Z-1.x86_64.rpm`):

```bash
sudo zypper install ./protecteye-*.rpm
```

## 2. Evrensel Kurulum Scripti ile Kurulum

```bash
curl -fsSL https://raw.githubusercontent.com/alierenaltindag/protect-eye/main/installer.sh | bash
```

## 3. openSUSE Üzerinde Kaynak Koddan Derleme

```bash
sudo zypper in -y cmake ninja gcc-c++ \
  qt6-base-devel qt6-multimedia-devel qt6-svg-devel

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```
