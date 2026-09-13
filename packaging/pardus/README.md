# ProtectEye - Pardus Linux Kurulum Rehberi

ProtectEye, TÜBİTAK ULAKBİM tarafından geliştirilen **Pardus 21** ve **Pardus 23** (hem **XFCE** hem de **GNOME** sürümleri) üzerinde tam uyumlu ve yerel Türkçe arayüzüyle çalışır.

## 1. Hazır DEB Paketi ile Kurulum (Önerilen)

GitHub Releases sayfasından en son `.deb` paketini indirin (`protecteye_X.Y.Z_amd64.deb`):

### Terminal ile:
```bash
sudo apt update
sudo apt install ./protecteye_*.deb
```

### Grafiksel Arayüz ile:
İndirilen `.deb` dosyasına çift tıklayarak **Pardus Paket Kurucu** ile tek tıkla kurabilirsiniz.

## 2. Evrensel Kurulum Scripti ile Kurulum

```bash
curl -fsSL https://raw.githubusercontent.com/alierenaltindag/protect-eye/main/installer.sh | bash
```

## 3. Pardus XFCE Entegrasyonu

Pardus'un varsayılan masaüstü ortamı olan XFCE'de:
- Sistem tepsisi (bildirim alanı) doğrudan desteklenir.
- **Rahatsız Etmeyin (Do Not Disturb):** XFCE panelindeki bildirim eklentisinden açılan "Rahatsız Etmeyin" modu ProtectEye tarafından yerel olarak algılanır ve molalar sessizce ertelenir.
- **Ekran Kilidi / Uyku:** `xfce4-screensaver` veya sistem askıya alma durumlarında ProtectEye zamanlayıcıları otomatik olarak duraklatılır ve açıldığında sıfırlanır.

## 4. Pardus GNOME Entegrasyonu

Pardus GNOME sürümünde sistem tepsisi gösterimi için `gnome-shell-extension-appindicator` paketi kurulu olmalıdır (Pardus GNOME'da varsayılan olarak yüklü gelir):

```bash
sudo apt install gnome-shell-extension-appindicator
```
