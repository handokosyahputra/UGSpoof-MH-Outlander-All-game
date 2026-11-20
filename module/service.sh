#!/system/bin/sh
MODDIR=${0%/*}
until [ "$(getprop sys.boot_completed)" = "1" ]; do sleep 2; done
sleep 3
CFG="$MODDIR/config.txt"
P=$(grep '^profile=' "$CFG" | head -n1 | cut -d= -f2)
PK=$(grep '^packages=' "$CFG" | head -n1 | cut -d= -f2)
NM=$(grep -c '^map\.' "$CFG")
[ -f "$MODDIR/profiles/$P.conf" ] && PS="✅ $P" || PS="❌ profil '$P' tidak ada"
[ -n "$PK" ] || [ "$NM" -gt 0 ] && GS="game: ${PK:-} (+$NM map)" || GS="❌ packages kosong"
sed -i "s|^description=.*|description=$PS · $GS|" "$MODDIR/module.prop"
