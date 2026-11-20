<div align="center">

# 🎮 UGSpoof – Universal Game Spoof

**Bikin game ngira HP kamu itu HP flagship, biar opsi grafis & FPS tinggi kebuka.**

Semua chipset arm64 · OpenGL ES + Vulkan · Per-game · Nggak ngubah file game

[![Build](https://github.com/handokosyahputra/UGSpoof-MH-Outlander-All-game/actions/workflows/build.yml/badge.svg)](https://github.com/handokosyahputra/UGSpoof-MH-Outlander-All-game/actions/workflows/build.yml)
![Versi](https://img.shields.io/github/v/release/handokosyahputra/UGSpoof-MH-Outlander-All-game?label=versi)
![Lisensi](https://img.shields.io/badge/lisensi-MIT-blue)

[🇬🇧 English](README.md) · 🇮🇩 Bahasa Indonesia

</div>

### 💖 Dukung aku

Kalau modul ini ngebantu, follow aku buat update & mod lainnya ya 🙏

[![Instagram](https://img.shields.io/badge/Instagram-@118harilagi-E4405F?logo=instagram&logoColor=white)](https://www.instagram.com/118harilagi/)
[![TikTok](https://img.shields.io/badge/TikTok-@118harilagi-black?logo=tiktok&logoColor=white)](https://www.tiktok.com/@118harilagi)

Kasih ⭐ di repo ini juga udah ngebantu banget!


> [!WARNING]
> **Risiko tanggung sendiri ya.** Spoof device bisa melanggar aturan game, dan anti-cheat bisa aja deteksi. Cobain dulu di **akun cadangan**. Modul ini cuma buat buka opsi grafis, **bukan buat cheat**.

## 🤔 Ini apa sih?

Banyak game ngunci grafis Ultra atau 120 FPS cuma buat HP tertentu. UGSpoof bikin game ngira kamu pakai HP flagship (misalnya REDMAGIC 9S Pro+ dengan Adreno 750).

Yang dipalsuin cuma **di dalam game**. Sistem, driver, dan app lain (bank, dll) tetap lihat HP asli kamu.

> Catatan: spoof **nggak nambah tenaga HP**. Kalau HP-mu kelas menengah, naikin grafisnya pelan-pelan biar nggak lag atau panas.

## ✨ Fiturnya

- 🧠 Jalan di **semua chipset**: MediaTek, Snapdragon, Exynos, Unisoc (arm64)
- 🔐 Support **Magisk, Kitsune, KernelSU, APatch**
- 🖼️ Spoof **GPU**: OpenGL ES dan Vulkan
- 📱 Spoof **device**: model, brand, chipset
- 🎯 **Per-game**, app lain aman
- 🗂️ **5 profil HP flagship** siap pakai, bisa bikin sendiri juga
- 🕵️ Jejak chipset asli (misalnya MediaTek) disembunyiin dari game
- ⚡ Ganti setting **tanpa reboot**
- 💾 **Nggak wajib clear data**, jadi nggak perlu download ulang

## 📋 Yang dibutuhin

- HP Android **64-bit** (Android 9+)
- **Root + Zygisk**:
  - Magisk / Kitsune → nyalain di *Settings → Zygisk*
  - KernelSU / APatch → pasang [ZygiskNext](https://github.com/Dr-TSNG/ZygiskNext) atau [ReZygisk](https://github.com/PerformanC/ReZygisk)
- **Matiin** modul spoof GPU lain (iUnlocker, COPG, dll) biar nggak bentrok

> Nggak bisa tanpa root ya, karena app biasa nggak punya akses buat ngubah info yang dibaca game.

## 📦 Cara pasang

1. Download zip terbaru di [Releases](https://github.com/handokosyahputra/UGSpoof-MH-Outlander-All-game/releases)
2. Buka Magisk/KSU/APatch → **Modules → Install from storage** → pilih zip-nya
3. **Reboot**
4. Buka game, cek menu grafis 🎉

Pas install, modul bakal otomatis ngecek Zygisk, nampilin info HP kamu, dan nyari game yang didukung.

### Masih kekunci?

Coba **force stop** game terus buka lagi. Kalau masih kekunci, berarti game nyimpen hasil deteksi lama. Hapus file setting-nya aja, **jangan clear data** biar nggak download ulang:

```sh
su
PKG=nama.package.game
am force-stop $PKG
cp -r /data/data/$PKG/shared_prefs /sdcard/backup_prefs_$PKG
rm -rf /data/data/$PKG/shared_prefs
```

Pastikan akun game udah di-bind dulu ya.

## ⚙️ Pengaturan

Edit file ini pakai file manager root (misalnya MT Manager):
```
/data/adb/modules/ugspoof/config.txt
```
Habis edit, cukup **force stop game** lalu buka lagi. Nggak perlu reboot.

```ini
profile=redmagic_9s_pro                  # profil HP yang dipakai
packages=com.game.satu, com.game.dua     # daftar game
map.com.game.tiga=galaxy_s24_ultra       # profil khusus buat 1 game

enable_gl=1        # spoof OpenGL
enable_vulkan=1    # spoof Vulkan
enable_build=1     # spoof info device
enable_props=1     # spoof system property
global_props=0     # 1 = ubah model buat SEMUA app (nggak disaranin)
```

**Cara tahu nama package game:** lihat di link Play Store (`id=...`), atau ketik `su -c pm list packages | grep -i namagame` di terminal.

Mau bikin profil sendiri atau lihat semua opsi? Cek **[docs/CONFIG.id.md](docs/CONFIG.id.md)**.

## 📱 Profil yang tersedia

Semuanya Snapdragon 8 Gen 3 + Adreno 750.

| Profil | HP |
|---|---|
| `redmagic_9s_pro` | REDMAGIC 9S Pro+ |
| `xiaomi_14` | Xiaomi 14 |
| `galaxy_s24_ultra` | Galaxy S24 Ultra ⚠️ |
| `rog_phone_8_pro` | ROG Phone 8 Pro ⚠️ |
| `oneplus_12` | OnePlus 12 ⚠️ |

⚠️ = sebagian datanya belum diverifikasi. Punya data aslinya? Kirim PR ya!

## 🩺 Ada masalah?

Buka game dulu, terus jalanin ini di terminal:
```sh
su -c sh /data/adb/modules/ugspoof/check.sh
```

| Masalah | Coba ini |
|---|---|
| Grafis masih kekunci | Cek `packages=` udah bener, force stop game, atau ganti profil |
| Game crash | Set `enable_vulkan=0` |
| Grafis aneh / glitch | Kosongin `gl_version=` |
| Log kosong | Cek Zygisk udah nyala dan nama package udah bener |

Masih error? Bikin [Issue](https://github.com/handokosyahputra/UGSpoof-MH-Outlander-All-game/issues) dan tempel hasil `check.sh`-nya.

## 🔧 Build sendiri

Butuh Linux/WSL dengan `clang`, `lld`, `zip`, dan `gcc`:
```sh
sudo apt install clang lld zip gcc
./build.sh
```
Hasilnya ada di folder `out/`. Nggak perlu NDK.

Mau rilis versi baru? Ganti angka di `VERSION`, tulis catatan di `CHANGELOG.md`, terus:
```sh
git tag v4.0.1 && git push origin v4.0.1
```
GitHub Actions bakal otomatis bikin Release-nya.

## 🤝 Kontribusi

Bantuan sangat diterima! Terutama:
- Profil HP baru, atau koreksi data profil yang ada
- Laporan game atau HP yang berhasil maupun gagal
- Perbaikan kode (jalanin `./build.sh test` dulu sebelum PR)

## 📜 Lisensi

[MIT](LICENSE). Kodenya ditulis dari nol, nggak pakai kode dari iUnlocker, COPG, atau modul lain.
