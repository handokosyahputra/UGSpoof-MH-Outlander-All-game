#!/system/bin/sh
# Pakai (su): sh /data/adb/modules/ugspoof/check.sh   -> buka game dulu
MODDIR=/data/adb/modules/ugspoof
echo "== Config =="; grep -E '^(profile|packages|map\.|enable_|global_props)' $MODDIR/config.txt
echo "== HP asli =="; echo "$(getprop ro.product.model) | $(getprop ro.hardware) | $(getprop ro.hardware.egl)"
echo "== Log modul (UGSpoof) =="; logcat -d -s UGSpoof | tail -n 30
