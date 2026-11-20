<div align="center">

# 🎮 UGSpoof – Universal Game Spoof

**Make games think your phone is a flagship, so high graphics & FPS options unlock.**

Any arm64 chipset · OpenGL ES + Vulkan · Per-game · Doesn't touch game files

[![Build](https://github.com/handokosyahputra/UGSpoof-MH-Outlander-All-game/actions/workflows/build.yml/badge.svg)](https://github.com/handokosyahputra/UGSpoof-MH-Outlander-All-game/actions/workflows/build.yml)
![Version](https://img.shields.io/github/v/release/handokosyahputra/UGSpoof-MH-Outlander-All-game?label=version)
![License](https://img.shields.io/badge/license-MIT-blue)

🇬🇧 English · [🇮🇩 Bahasa Indonesia](README.md)

</div>

> [!WARNING]
> **Use at your own risk.** Spoofing your device may break a game's rules, and anti-cheat might detect it. Try it on an **alt account** first. This module only unlocks graphics options. It's **not a cheat**.

## 🤔 What is this?

A lot of games lock Ultra graphics or 120 FPS to certain phones. UGSpoof makes the game think you're on a flagship (for example a REDMAGIC 9S Pro+ with an Adreno 750).

The spoof only happens **inside the game**. Your system, drivers, and other apps (banking etc.) still see your real phone.

> Heads up: spoofing **doesn't make your phone faster**. On a mid-range phone, raise graphics step by step so it doesn't lag or overheat.

## ✨ Features

- 🧠 Works on **any chipset**: MediaTek, Snapdragon, Exynos, Unisoc (arm64)
- 🔐 Supports **Magisk, Kitsune, KernelSU, APatch**
- 🖼️ **GPU** spoof: OpenGL ES and Vulkan
- 📱 **Device** spoof: model, brand, chipset
- 🎯 **Per-game**, other apps stay untouched
- 🗂️ **5 flagship profiles** ready to go, or make your own
- 🕵️ Hides your real chipset (e.g. MediaTek) from the game
- ⚡ Change settings **without rebooting**
- 💾 **No data wipe needed**, so no re-downloading

## 📋 Requirements

- **64-bit** Android phone (Android 9+)
- **Root + Zygisk**:
  - Magisk / Kitsune → turn on *Settings → Zygisk*
  - KernelSU / APatch → install [ZygiskNext](https://github.com/Dr-TSNG/ZygiskNext) or [ReZygisk](https://github.com/PerformanC/ReZygisk)
- **Disable** other GPU spoof modules (iUnlocker, COPG, etc.) so they don't clash

> This doesn't work without root. Normal apps can't change what a game reads.

## 📦 Install

1. Download the latest zip from [Releases](https://github.com/handokosyahputra/UGSpoof-MH-Outlander-All-game/releases)
2. Open Magisk/KSU/APatch → **Modules → Install from storage** → pick the zip
3. **Reboot**
4. Open your game and check the graphics menu 🎉

During install, the module checks Zygisk, shows your phone info, and looks for supported games.

### Still locked?

Try **force stopping** the game and opening it again. If it's still locked, the game saved its old detection result. Delete just its settings file. **Don't clear all data**, or you'll have to re-download everything:

```sh
su
PKG=game.package.name
am force-stop $PKG
cp -r /data/data/$PKG/shared_prefs /sdcard/backup_prefs_$PKG
rm -rf /data/data/$PKG/shared_prefs
```

Make sure your game account is linked first.

## ⚙️ Settings

Edit this file with a root file manager (e.g. MT Manager):
```
/data/adb/modules/ugspoof/config.txt
```
After editing, just **force stop the game** and open it again. No reboot needed.

```ini
profile=redmagic_9s_pro                  # which phone profile to use
packages=com.game.one, com.game.two      # your games
map.com.game.three=galaxy_s24_ultra      # a different profile for one game

enable_gl=1        # OpenGL spoof
enable_vulkan=1    # Vulkan spoof
enable_build=1     # device info spoof
enable_props=1     # system property spoof
global_props=0     # 1 = change model for ALL apps (not recommended)
```

**Finding a game's package name:** check its Play Store link (`id=...`), or run `su -c pm list packages | grep -i gamename` in a terminal.

Want to make your own profile or see every option? Check **[docs/CONFIG.en.md](docs/CONFIG.en.md)**.

## 📱 Built-in profiles

All of them are Snapdragon 8 Gen 3 + Adreno 750.

| Profile | Phone |
|---|---|
| `redmagic_9s_pro` | REDMAGIC 9S Pro+ |
| `xiaomi_14` | Xiaomi 14 |
| `galaxy_s24_ultra` | Galaxy S24 Ultra ⚠️ |
| `rog_phone_8_pro` | ROG Phone 8 Pro ⚠️ |
| `oneplus_12` | OnePlus 12 ⚠️ |

⚠️ = some of the data isn't verified yet. Got the real data? Send a PR!

## 🩺 Having issues?

Open the game first, then run this in a terminal:
```sh
su -c sh /data/adb/modules/ugspoof/check.sh
```

| Problem | Try this |
|---|---|
| Graphics still locked | Check `packages=` is correct, force stop the game, or try another profile |
| Game crashes | Set `enable_vulkan=0` |
| Weird graphics / glitches | Leave `gl_version=` empty |
| Empty log | Check Zygisk is on and the package name is correct |

Still stuck? Open an [Issue](https://github.com/handokosyahputra/UGSpoof-MH-Outlander-All-game/issues) and paste your `check.sh` output.

## 🔧 Build it yourself

You'll need Linux/WSL with `clang`, `lld`, `zip`, and `gcc`:
```sh
sudo apt install clang lld zip gcc
./build.sh
```
The output goes to `out/`. No NDK needed.

Releasing a new version? Bump the number in `VERSION`, add notes to `CHANGELOG.md`, then:
```sh
git tag v4.0.1 && git push origin v4.0.1
```
GitHub Actions builds the Release for you.

## 🤝 Contributing

Help is super welcome! Especially:
- New phone profiles, or fixes for existing ones
- Reports of games and phones that work (or don't)
- Code improvements (run `./build.sh test` before opening a PR)

## 💖 Support me

If this module helped you, consider following or supporting me 🙏

[![YouTube](https://img.shields.io/badge/YouTube-red?logo=youtube&logoColor=white)](https://youtube.com/@USERNAME)
[![TikTok](https://img.shields.io/badge/TikTok-black?logo=tiktok&logoColor=white)](https://tiktok.com/@USERNAME)
[![Instagram](https://img.shields.io/badge/Instagram-E4405F?logo=instagram&logoColor=white)](https://instagram.com/USERNAME)
[![Telegram](https://img.shields.io/badge/Telegram-26A5E4?logo=telegram&logoColor=white)](https://t.me/USERNAME)

[![Ko-fi](https://img.shields.io/badge/Ko--fi-FF5E5B?logo=kofi&logoColor=white)](https://ko-fi.com/USERNAME)
[![Saweria](https://img.shields.io/badge/Saweria-FAAE2B?logoColor=black)](https://saweria.co/USERNAME)
[![Trakteer](https://img.shields.io/badge/Trakteer-BE1E2D?logoColor=white)](https://trakteer.id/USERNAME)

A ⭐ on this repo helps a lot too!

## 📜 License

[MIT](LICENSE). Written from scratch, with no code from iUnlocker, COPG, or any other module.
