# ⚙️ Config & Profile Guide

🇬🇧 English · [🇮🇩 Bahasa Indonesia](CONFIG.id.md)

The config file is at `/data/adb/modules/ugspoof/config.txt`.
After editing, just force stop the game and open it again.

## How it works

Every time a game opens, the module picks a profile in this order:
1. Is there a `map.<package>=profile` line? That profile is used.
2. Is the package in `packages=`? It uses the profile from `profile=`.
3. Neither? The game is ignored and not spoofed.

Anything in `config.txt` always **overrides** the profile. So if you only want to change one value, put it in config.txt. No need to edit the profile.

## config.txt options

| Option | What it does |
|---|---|
| `profile=` | Default profile |
| `packages=` | List of games, separated by commas |
| `map.<package>=` | A specific profile for one game |
| `enable_gl=` | `1`/`0`, OpenGL spoof |
| `enable_vulkan=` | `1`/`0`, Vulkan spoof |
| `enable_build=` | `1`/`0`, device info spoof |
| `enable_props=` | `1`/`0`, system property spoof |
| `spoof_system_callers=` | `1` = system UI gets spoofed too (can glitch, keep it `0`) |
| `global_props=` | `1` = model changes for all apps (needs reboot, not recommended) |

## What's in a profile

Profiles live in the `profiles/` folder, named `name.conf`.

**GPU info:**
```ini
gl_vendor=Qualcomm
gl_renderer=Adreno (TM) 750
gl_version=OpenGL ES 3.2 V@0762.10   # leave empty if graphics look weird
vk_vendor_id=0x5143
vk_device_id=0x43051401
vk_device_name=Adreno (TM) 750
```

**Device info (android.os.Build):**
```ini
build.MODEL=NX769J
build.BRAND=nubia
build.MANUFACTURER=nubia
```

**System properties:**
```ini
propall.ro.product.model=NX769J   # seen by all code inside the game
prop.ro.hardware=qcom             # only seen by the game's own code
prop.ro.mediatek.*=               # empty = hidden, * = everything starting with this
```

### `prop.` vs `propall.`?

- **`propall.`** is for identity info, like model, brand, and chipset.
- **`prop.`** is for properties that drivers also use, like `ro.hardware` and `ro.board.platform`. Drivers still read the real value, so the game doesn't crash.
- ❌ **Never** change `ro.hardware.egl` or `ro.hardware.vulkan`. They decide which GPU driver gets loaded.

## Making your own profile

1. Copy `profiles/_template.conf` and give it a new name, e.g. `dream_phone.conf` (lowercase letters, numbers, `_` or `-` only)
2. Fill in the data. The most accurate way is to grab it from the real phone:
   ```sh
   adb shell getprop | grep -E "ro.product|ro.soc|ro.board"
   ```
   For GPU info, use an app like *Device Info HW* or *AIDA64*.
3. Use it: `profile=dream_phone`
4. Force stop the game, open it again, done!

Your own profiles **won't get deleted** when you update the module.

Want your profile added to the repo? Send a Pull Request and include where the data came from.

## Common GPU IDs

| GPU | Vendor ID | Device ID |
|---|---|---|
| Adreno 750 (SD 8 Gen 3) | `0x5143` | `0x43051401` |
| Adreno 740 (SD 8 Gen 2) | `0x5143` | `0x43050a01` |
| ARM Mali | `0x13B5` | varies |
| Samsung Xclipse | `0x144D` | varies |
| PowerVR | `0x1010` | varies |
