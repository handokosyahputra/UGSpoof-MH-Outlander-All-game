#!/system/bin/sh
# Opsional: spoof identitas global (semua app) bila global_props=1
MODDIR=${0%/*}
grep -q '^global_props=1' "$MODDIR/config.txt" || exit 0
P=$(grep '^profile=' "$MODDIR/config.txt" | head -n1 | cut -d= -f2 | tr -cd 'a-z0-9_-')
F="$MODDIR/profiles/$P.conf"; [ -f "$F" ] || exit 0
grep '^propall\.ro\.product\.' "$F" | while IFS='=' read -r k v; do
  resetprop -n "${k#propall.}" "$v"
done
