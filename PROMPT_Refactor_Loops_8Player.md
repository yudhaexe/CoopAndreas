# PROMPT AGEN — Audit + Refactor loop misi 4-pemain → 8-pemain (0→6)

## PERAN
Kamu editor SCM (Sanny Builder) CoopAndreas. Tugas 2 fase: **(1) AUDIT** misi mana yang masih 4-pemain,
lalu **(2) REFACTOR** loop per-player-nya jadi 7-follower (indeks 0–6). Ini kerja `.txt`, bukan C++.

## 🧨 ANTI-HALUSINASI (baca dulu)
- **Jangan blind-replace `2`→`6`.** `for ... = 0 to 2` BELUM tentu loop pemain. Hanya lebarkan loop yang
  mengindeks `$NETWORK_PLAYER[$temp_int]` (atau var loop apa pun) DAN dijaga `Coop.IsNetworkPlayerActorValid`.
  Loop `0 to 2` yang mengiterasi hal lain (3 mobil, 3 checkpoint statis, dsb.) **JANGAN disentuh** — flag saja.
- **Kutip, jangan mengingat.** Buka tiap file, baca konteks loop, baru ubah. Verifikasi tiap perubahan.
- **COMPILES ≠ TESTED.** Bukti maksimal = compile SCM bersih. Partisipasi 8-pemain hanya terbukti playtest
  8 client (manusia). Jangan klaim "works/tested".
- **Kalau nemu idiom tanpa preseden** (bukan sekadar `for` loop — lihat Fase 2 daftar idiom) → FLAG, jangan karang.

## GROUND TRUTH (terverifikasi per 2026-09-30)
1. **Hanya 3 misi** yang masih punya `for ... = 0 to 2`: `scm/scripts/INTRO2.txt`, `scm/scripts/SWEET1.txt`,
   `scm/scripts/SWEET1B.txt`. **66 misi lain sudah `0 to 6`** (jangan sentuh yang sudah benar).
2. **`Collect` sudah 7-var** di misi ini (mis. `SWEET1B.txt:127`) — jadi slot data sudah tersedia,
   yang kurang cuma loop yang MEMAKAI slot 3–6.
3. **Idiom loop pemain + guard** (contoh `SWEET1B.txt:271`):
   ```
   for $temp_int = 0 to 2
       if
           Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[$temp_int])
       then
           Coop.PrintNowForNetworkPlayer(...)
           Coop.UpdateCheckpointForNetworkPlayer(...)
       end
   end
   ```
   Guard menolak slot invalid/0 → aman dilebarkan ke `0 to 6`.
4. **Ketergantungan teleport**: misi ini memanggil `Coop.TeleportPlayersToHostSafely($NETWORK_PLAYER[0], [1], [2])`
   yang **hard-limit 3**. Melebarkan loop TIDAK memperbaiki teleport. Lihat Fase 2 langkah T.

## FASE 1 — AUDIT (jangan percaya daftar di atas mentah-mentah; verifikasi ulang & catat)
Untuk SETIAP file di `scm/scripts/`, klasifikasikan status 8-pemain dengan mencari **semua** idiom 4-slot,
bukan cuma `for 0 to 2`:
```bash
cd /c/Users/PC/Documents/GitHub/CoopAndreas
# a. loop pemain hardcoded 3
grep -rnE 'for .*= 0 to 2' scm/scripts/
# b. teleport hard-limit 3
grep -rnE 'TeleportPlayersToHostSafely\(' scm/scripts/
# c. idiom 4-slot yang di-UNROLL (bukan loop) — cek referensi eksplisit slot [2] tanpa [3..6]
grep -rnE '\$NETWORK_PLAYER\[2\]' scm/scripts/
# d. konfirmasi Collect sudah 7-var (kalau masih 3-var = misi belum dikonversi penuh, FLAG terpisah)
grep -rnE 'CollectNetworkPlayersForTheMission' scm/scripts/
```
Buat tabel status per misi: **SUDAH-0-6 / MASIH-0-2 / UNROLLED-3 / BELUM-COLLECT-7 / N/A (global, tanpa loop pemain)**.
Simpan hasil audit ke `PLAN_Refactor_8Slots.md` (update tabel di sana; kalau belum ada tabel, tambahkan).
**Untuk tiap loop `0 to 2`, buka konteksnya** dan tandai: pemain (→refactor) vs bukan-pemain (→biarkan+catat).

## FASE 2 — REFACTOR (hanya misi MASIH-0-2 / UNROLLED-3 hasil audit)
Target awal (dari Ground Truth #1): **INTRO2, SWEET1, SWEET1B**. Tapi ikuti hasil AUDIT-mu, bukan asumsi.
Untuk tiap misi target, tangani SEMUA idiom berikut:

- **Langkah L (loop pemain)**: ubah `for $temp_int = 0 to 2` → `0 to 6` **hanya** bila body-nya mengindeks
  `$NETWORK_PLAYER[$temp_int]` dan/atau dijaga `IsNetworkPlayerActorValid`. Pastikan var loop yang dipakai
  memang var iterasi yang sama.
- **Langkah U (unrolled)**: kalau ada blok yang di-copy manual untuk slot 0/1/2 (bukan loop), dan menyangkut
  `$NETWORK_PLAYER[0..2]` player-facing → perluas ke 0..6 mengikuti pola guard yang sama. Kalau ragu
  bentuknya beda dari preseden → FLAG, jangan tebak.
- **Langkah G (gate "tunggu semua")**: kondisi yang cuma cek slot [0..2] harus mencakup [0..6]. Kalau gate
  sudah berbentuk loop ber-guard, cukup ikut Langkah L.
- **Langkah H (handshake netID)**: kalau ada while-loop/blok handshake per follower yang hardcoded 3 →
  perluas ke 7 mengikuti pola guard. FLAG kalau strukturnya tak ada preseden.
- **Langkah T (teleport)**: ganti `Coop.TeleportPlayersToHostSafely($NETWORK_PLAYER[0], [1], [2])` menjadi
  `Coop.TeleportAllPlayersToHostSafely()` **JIKA** opcode 0x1D1D itu sudah ada di build
  (cek `client/src/Commands/CCustomCommandRegistrar.h` + `sa_coop.db`). **KALAU BELUM ADA** → JANGAN pakai
  (nanti compile SCM gagal / opcode tak dikenal). Sebagai gantinya **FLAG dependency**: catat bahwa teleport
  masih mentok 3 sampai 0x1D1D di-implement (lihat `PROMPT_TeleportAll_8Player.md`). Jangan diam-diam.

## VERIFIKASI (WAJIB)
1. **Self-audit diff**: pastikan tiap `0 to 2 → 0 to 6` yang kamu ubah memang loop pemain (buka konteks
   ulang). Pastikan TIDAK ada file "SUDAH-0-6" yang tersentuh.
2. **Compile SCM** (bukti empiris). ⚠️ **WAJIB `--game sa`; JANGAN `--no-splash`** (bukan flag valid → Sanny
   buka GUI & menggantung, exit 1, main.scm TAK ter-regenerate):
   ```bash
   SANNY="/c/Users/PC/Downloads/SannyBuilder-v4.2.0/sanny.exe"
   LOG="/c/Users/PC/Downloads/SannyBuilder-v4.2.0/compile.log"
   SCM="C:\\Users\\PC\\Documents\\GitHub\\CoopAndreas\\scm\\main.scm"
   ls -la "$SCM"   # catat timestamp SEBELUM
   rm -f "$LOG"
   "$SANNY" --compile "C:\\Users\\PC\\Documents\\GitHub\\CoopAndreas\\scm\\main.txt" "$SCM" --game sa --mode sa_sbl_coopandreas
   ls -la "$SCM"   # timestamp SESUDAH harus MAJU
   # BUKTI SUKSES BERLAPIS: (1) compile.log tak ada/tak berisi "error:", DAN (2) timestamp main.scm maju.
   # Timestamp tak berubah = compile TAK PERNAH jalan (perintah salah), BUKAN sukses.
   ```
   Ada `error: <file>:<line>` → benerin, ulang sampai bersih (compiler berhenti di error PERTAMA urut
   `{$INCLUDE}`; bisa ada error lain setelahnya). Deploy: `cp scm/main.scm "C:\Games\GTA - The Original Trilogy\GTASA\CoopAndreas\main.scm"`.

## LAPORAN (jujur)
- Tabel audit lengkap (status per misi) — tempel + simpan ke `PLAN_Refactor_8Slots.md`.
- Daftar file diubah + `file:line` tiap loop yang dilebarkan, dengan alasan "ini loop pemain karena …".
- Semua FLAG: loop `0 to 2` bukan-pemain yang sengaja dibiarkan; idiom tanpa preseden; dependency teleport 0x1D1D.
- Hasil compile apa adanya. Status = **COMPILES**, sebut eksplisit 8-pemain belum diplaytest.

## JANGAN
- ❌ Blind-replace `2`→`6` tanpa buka konteks.
- ❌ Sentuh 66 misi yang sudah `0 to 6`.
- ❌ Pakai `Coop.TeleportAllPlayersToHostSafely()` kalau opcode 0x1D1D belum ada di build.
- ❌ Klaim "tested/works". Maks COMPILES.
