# ⚙️ Panduan Config & Profil

🇮🇩 Bahasa Indonesia · [🇬🇧 English](CONFIG.en.md)

File config ada di `/data/adb/modules/ugspoof/config.txt`.
Habis edit, cukup force stop game lalu buka lagi.

## Cara kerjanya

Setiap game dibuka, modul nentuin profil dengan urutan begini:
1. Ada `map.<package>=profil`? Profil itu yang dipakai.
2. Package-nya ada di `packages=`? Pakai profil dari `profile=`.
3. Nggak ada di keduanya? Game dicuekin, nggak di-spoof.

Isi `config.txt` selalu **menimpa** isi profil. Jadi kalau mau ngubah satu nilai aja, tulis di config.txt, nggak perlu edit profilnya.

## Opsi di config.txt

| Opsi | Fungsinya |
|---|---|
| `profile=` | Profil default |
| `packages=` | Daftar game, pisah pakai koma |
| `map.<package>=` | Profil khusus buat satu game |
| `enable_gl=` | `1`/`0`, spoof OpenGL |
| `enable_vulkan=` | `1`/`0`, spoof Vulkan |
| `enable_build=` | `1`/`0`, spoof info device |
| `enable_props=` | `1`/`0`, spoof system property |
| `spoof_system_callers=` | `1` = UI sistem ikut di-spoof (bisa glitch, biarin `0`) |
| `global_props=` | `1` = model diubah buat semua app (perlu reboot, nggak disaranin) |

## Isi profil

Profil ada di folder `profiles/`, formatnya `nama.conf`.

**Info GPU:**
```ini
gl_vendor=Qualcomm
gl_renderer=Adreno (TM) 750
gl_version=OpenGL ES 3.2 V@0762.10   # kosongin kalau grafis aneh
vk_vendor_id=0x5143
vk_device_id=0x43051401
vk_device_name=Adreno (TM) 750
```

**Info device (android.os.Build):**
```ini
build.MODEL=NX769J
build.BRAND=nubia
build.MANUFACTURER=nubia
```

**System property:**
```ini
propall.ro.product.model=NX769J   # dibaca semua kode di dalam game
prop.ro.hardware=qcom             # cuma dibaca kode milik game
prop.ro.mediatek.*=               # kosong = disembunyiin, * = semua yang depannya sama
```

### Bedanya `prop.` sama `propall.`?

- **`propall.`** buat info identitas, kayak model, brand, dan chipset.
- **`prop.`** buat property yang juga dipakai driver, kayak `ro.hardware` dan `ro.board.platform`. Driver HP tetap baca nilai aslinya, jadi game nggak crash.
- ❌ **Jangan pernah** ubah `ro.hardware.egl` atau `ro.hardware.vulkan`. Itu nentuin driver GPU yang dipakai.

## Bikin profil sendiri

1. Copy `profiles/_template.conf`, kasih nama baru, misalnya `hp_impian.conf` (huruf kecil, angka, `_` atau `-` aja)
2. Isi datanya. Paling akurat ambil dari HP aslinya:
   ```sh
   adb shell getprop | grep -E "ro.product|ro.soc|ro.board"
   ```
   Info GPU bisa dicek pakai app *Device Info HW* atau *AIDA64*.
3. Pakai profilnya: `profile=hp_impian`
4. Force stop game, buka lagi, selesai!

Profil buatan sendiri **nggak bakal hilang** waktu modul di-update.

Mau profilmu masuk repo? Kirim Pull Request, sertain juga sumber datanya ya.

## ID GPU yang sering dipakai

| GPU | Vendor ID | Device ID |
|---|---|---|
| Adreno 750 (SD 8 Gen 3) | `0x5143` | `0x43051401` |
| Adreno 740 (SD 8 Gen 2) | `0x5143` | `0x43050a01` |
| ARM Mali | `0x13B5` | beda-beda |
| Samsung Xclipse | `0x144D` | beda-beda |
| PowerVR | `0x1010` | beda-beda |
