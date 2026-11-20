ui_print "==============================================="
ui_print " Universal Game Spoof (UGSpoof)"
ui_print "==============================================="
[ "$ARCH" = "arm64" ] || abort "! Hanya untuk HP arm64 (64-bit)"

# ---- Root & Zygisk ----
ZOK=0
if [ "$KSU" = "true" ] || [ "$APATCH" = "true" ]; then
  [ "$KSU" = "true" ] && ui_print "- Root: KernelSU" || ui_print "- Root: APatch"
  for z in zygisksu rezygisk zygisk_next zygisknext; do
    [ -d "/data/adb/modules/$z" ] && [ ! -f "/data/adb/modules/$z/disable" ] && ZOK=1
  done
  [ $ZOK = 1 ] || ui_print "! Pasang ZygiskNext atau ReZygisk dulu (wajib)"
else
  ui_print "- Root: Magisk $MAGISK_VER ($MAGISK_VER_CODE)"
  Z=$(magisk --sqlite "SELECT value FROM settings WHERE key='zygisk'" 2>/dev/null | cut -d= -f2)
  for z in zygisksu rezygisk zygisk_next; do
    [ -d "/data/adb/modules/$z" ] && [ ! -f "/data/adb/modules/$z/disable" ] && Z=1
  done
  [ "$Z" = "1" ] && ZOK=1 || ui_print "! Zygisk BELUM aktif: Magisk > Settings > Zygisk"
fi
[ $ZOK = 1 ] && ui_print "- Zygisk: OK"

# ---- Info HP asli ----
SOC=$(getprop ro.soc.model); [ -z "$SOC" ] && SOC=$(getprop ro.board.platform)
GPU=$(getprop ro.hardware.vulkan); [ -z "$GPU" ] && GPU=$(getprop ro.hardware.egl)
ui_print "- HP asli: $(getprop ro.product.brand) $(getprop ro.product.model)"
ui_print "  SoC: $SOC | GPU driver: $GPU | Android $(getprop ro.build.version.release)"
case "$GPU" in
  adreno*) ui_print "  GPU asli Adreno: spoof GPU paling stabil";;
  mali*)   ui_print "  GPU Mali: kalau grafis aneh, kosongkan gl_version / enable_vulkan=0";;
  *)       ui_print "  GPU lain: kalau crash, coba enable_vulkan=0";;
esac

# ---- Config: pertahankan yang lama (ugspoof, atau versi lama rm9spro_spoof) ----
for D in /data/adb/modules/ugspoof /data/adb/modules/rm9spro_spoof; do
  OLD="$D/config.txt"; [ -f "$OLD" ] || continue
  if grep -q '^profile=' "$OLD"; then
    cp -f "$OLD" "$MODPATH/config.txt"; ui_print "- Config lama dipertahankan ($(basename $D))"
  else
    P=$(grep '^packages=' "$OLD" | head -n1 | cut -d= -f2)
    [ -n "$P" ] && sed -i "s|^packages=.*|packages=$P|" "$MODPATH/config.txt" && ui_print "- Daftar game dari versi lama dipindahkan"
  fi
  [ -d "$D/profiles" ] && for f in "$D"/profiles/*.conf; do
    [ -f "$f" ] && [ ! -f "$MODPATH/profiles/$(basename $f)" ] && cp -f "$f" "$MODPATH/profiles/"
  done
  break
done
if [ -d /data/adb/modules/rm9spro_spoof ]; then
  touch /data/adb/modules/rm9spro_spoof/remove
  ui_print "- Modul lama (rm9spro_spoof) akan dihapus otomatis saat reboot"
fi

# ---- Deteksi game ----
if ! grep -q '^packages=..*' "$MODPATH/config.txt"; then
  PK=$(pm list packages 2>/dev/null | sed 's/^package://' | grep -iE 'monsterhunter|outlander|\.mho' | tr '\n' ',' | sed 's/,$//')
  if [ -n "$PK" ]; then
    sed -i "s|^packages=.*|packages=$PK|" "$MODPATH/config.txt"; ui_print "- Game terdeteksi: $PK"
  else
    ui_print "! Isi 'packages=' di config.txt dengan package game"
  fi
fi
ui_print "- Profil: $(grep '^profile=' "$MODPATH/config.txt" | cut -d= -f2)"
ui_print "  Tersedia: $(ls $MODPATH/profiles | grep -v '^_' | sed 's/.conf$//' | tr '\n' ' ')"

# ---- Modul lain yang bentrok ----
for m in /data/adb/modules/*; do
  n=$(basename "$m"); case "$n" in ugspoof|rm9spro_spoof) continue;; esac; [ -f "$m/disable" ] && continue
  case "$n" in Mjg2MjU1ODg0Mwo|*[Cc][Oo][Pp][Gg]*) ui_print "! Matikan modul '$n' (sama-sama spoof GPU, bisa crash)";; esac
done

set_perm_recursive "$MODPATH" 0 0 0755 0644
for s in service.sh post-fs-data.sh check.sh action.sh; do set_perm "$MODPATH/$s" 0 0 0755; done
ui_print "- Selesai. Reboot, lalu buka game."
