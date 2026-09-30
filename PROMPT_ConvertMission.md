# PROMPT — Agent Konversi Misi CoopAndreas (self-contained)

> **Cara pakai:** salin seluruh isi file ini sebagai prompt ke sesi/agent baru, ganti
> `<NAMA_MISI>` di bagian TUGAS. Agent harus baca file-file referensi dulu sebelum ngedit.

---

## PERAN
Kamu meng-konversi misi single-player GTA:SA jadi **co-op** di repo `CoopAndreas`
(C:\Users\PC\Documents\GitHub\CoopAndreas). Kode misi = Sanny Builder SCM di `scm/scripts/<CODE>.txt`.
Tujuan: nambah lapisan co-op **tanpa** ngerusak logika vanilla, konsisten dengan pola dev, non-hardcode,
gak tambal-sulam. Bikin senang buat dimainkan rame-rame.

## WAJIB BACA DULU (jangan skip)
1. `PLAN_HowToConvert.md` — **panduan konversi KANONIK & teruji.** Ini sumber kebenaran. Ikuti persis.
2. `PLAN_CoopAndreas_TestLog.md` — lessons dari crash nyata (§2) + tier kesiapan (§3d). Baca sebelum mulai.
3. Referensi kode dev (contoh pola benar): `scm/scripts/SWEET1.txt` (103 Coop calls), `SWEET1B.txt` (88),
   `INTRO1.txt` (58), `INTRO2.txt` (59). Grep untuk situasi spesifik (interior, convoy, kill-target, dll).

## ⚠️ WHITELIST OPCODE (pakai HANYA nama ini — agent lain pernah HALU & semua gagal compile)
Method Coop yg ADA (28): AddChatMessage, CancelPedClaim, ClaimPedOnRelease, ClearAllEntityBlipsForNetworkPlayer,
ClearThisPrintForNetworkPlayer, CollectNetworkPlayersForTheMission, EnableSyncingThisScript, GetNetworkPlayerChar,
GetNetworkPlayerInternalId, GetPedInAreaWithModel, **GetPedNetworkId**, **GetVehicleNetworkId**, IsHost,
IsNetworkPlayerActorValid, IsSyncingThisPed, LocateAllPlayersOnFoot3D, PedResetAllClaims, PedTakeHost,
PrintBigForNetworkPlayer, PrintForNetworkPlayer, PrintHelpForNetworkPlayer, PrintNowForNetworkPlayer,
RemoveCarBlipForNetworkPlayer, RemoveCharBlipForNetworkPlayer, RemoveCheckpointForNetworkPlayer,
TeleportPlayersToHostSafely, **UpdateCarBlipForNetworkPlayer**, **UpdateCharBlipForNetworkPlayer**,
**UpdateCheckpointForNetworkPlayer**.
NAMA HALU yg TIDAK ADA (JANGAN dipakai): GetCarNetworkId(→GetVehicleNetworkId), GetCharNetworkId(→GetPedNetworkId),
SetCheckpointForNetworkPlayer(→UpdateCheckpointForNetworkPlayer), AddCarBlipForNetworkPlayer(→UpdateCarBlip...),
AddCharBlipForNetworkPlayer(→UpdateCharBlip...). Ragu? cek `sdk/Sanny Builder 4/data/sa_sbl_coopandreas/sa_coop.db`.
**COMPILE-VERIFY (§VERIFIKASI) WAJIB tiap misi — itu yg nangkep nama halu instan. JANGAN skip.**

## PRINSIP NON-NEGOTIABLE (ringkasan dari PLAN_HowToConvert.md — teruji empiris)
- **Misi jalan HOST-ONLY.** `$scplayer` = host. `$NETWORK_PLAYER[0..6]` = follower (teman, di-drive via packet).
  `Coop.IsHost` gak perlu di misi. Follower mati = non-event (jangan cek IsDead-nya). Cuma host mati = gagal (vanilla).
- **ADDITIVE-ONLY.** JANGAN ubah/hapus baris vanilla. Cuma NAMBAH blok co-op.
- **DINAMIS.** Selalu `for/FOR $temp_int = 0 to 6` + `if Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[$temp_int])`.
  Slot kosong auto-skip → jalan 2–8 player. JANGAN hardcode jumlah player.
- **PRINSIP PEMERSATU (auto-sync vs mirror):** opcode yg ADA di `syncedOpcodes[]` (client/src/COpCodeSync.cpp)
  auto-replikasi host→semua → BIARIN VANILLA (print_big/help, cutscene, camera, fade, area_visible, mission_passed,
  ped-task task_enter_car/goto/kill). Yang TIDAK di list + player-facing → mirror manual `Coop.*ForNetworkPlayer`:
  terutama **`Text.PrintNow` (0x00BC)**, **blip**, **checkpoint**. Kalau ragu: cek `syncedOpcodes[]`.
- **Handshake netID = BARRIER** (tunggu server replikasi entity), hasil dibuang. Taruh dekat pembuatan entity.
  JANGAN di entity coord-placeholder (0,0,-100) → hang.
- **Blip:** Purple=mobil objektif, Blue=NPC teman(friendly true), Red=target musuh(false), Both=default.
  Tiap Update* WAJIB ada cleanup (Remove* individual ATAU ClearAllEntityBlips) di akhir + tiap fail path. Over-remove aman.
- **Gate "tunggu semua":** objektif drive/lokasi pakai loop LocateAnyMeans3D + flag (idiom 206@/150@) biar follower gak ke-tinggal.
- **Follower JANGAN di-force-warp** ke kendaraan. Spawn kendaraan + blip + cek `IsInCar` aja (hormati agency).
- **Interior:** teleport-bareng (SetAreaVisible + per-index SetCoordinates; keluar SetAreaVisible(0)+TeleportPlayersToHostSafely).
  Bukan per-player instance (engine single-interior).

## LANGKAH KONVERSI (urut — lihat checklist §9 di PLAN_HowToConvert.md)
1. **Baca file misi penuh.** Petakan: init, spawn entity, tiap fase objektif, kondisi menang/kalah, cleanup.
2. **Klasifikasi + TANYA dulu** kalau ada fork desain BARU (objektif ambigu, mekanik khusus dansa/RC/terbang/kereta,
   drive-by-in-vehicle). Pola yg udah diputus (convoy ride-together, objektif any/all) pakai langsung. JANGAN nebak desain.
3. `Coop.EnableSyncingThisScript()` di baris ~6 (setelah `script_name`). Buang warning-stub "unsupported" kalau ada.
4. `Collect...()` 7-var di init (sebelum blok per-player):
   `$NETWORK_PLAYER[0], ...[1],...[2],...[3],...[4],...[5],...[6] = Coop.CollectNetworkPlayersForTheMission()`
5. Handshake netID tiap entity misi penting (dekat pembuatannya).
6. Tiap teks-objektif/blip/checkpoint player-facing yg TIDAK auto-sync → mirror loop+guard ke follower (ADDITIVE).
7. Objektif drive/lokasi → tambah gate "tunggu semua" (var lokal BEBAS, verifikasi gak dipakai).
8. Kendaraan follower kalau perlu (spawn+blip+IsInCar, JANGAN warp). Interior kalau perlu (teleport-bareng).
9. Cleanup di label akhir + SEMUA fail path.

## VERIFIKASI (WAJIB tiap selesai — Sanny CLI, kamu bisa jalanin sendiri)
```bash
SANNY="/c/Users/PC/Downloads/SannyBuilder-v4.2.0/sanny.exe"
LOG="/c/Users/PC/Downloads/SannyBuilder-v4.2.0/compile.log"
rm -f "$LOG"
"$SANNY" --compile "C:\\Users\\PC\\Documents\\GitHub\\CoopAndreas\\scm\\main.txt" "C:\\Users\\PC\\Documents\\GitHub\\CoopAndreas\\scm\\main.scm" --mode sa_sbl_coopandreas --no-splash
# SUKSES kalau $LOG TIDAK ada / tidak berisi "error:". Exit code SELALU 0 — JANGAN andelin exit code.
```
- Kalau `error: <file>:<line> ...` → benerin, ulang sampai clean.
- Deploy hasil: `cp scm/main.scm "C:\Games\GTA - The Original Trilogy\GTASA\CoopAndreas\main.scm"`.
- **Compile lolos ≠ jalan.** Playtest 2+ client cuma bisa manusia. Status max = COMPILES; TESTED butuh dev.

## SETELAH SELESAI
- Update tracker di `PLAN_AllMissions_Campaign.md` (status misi → COMPILES + catatan).
- Kalau nemu situasi tanpa preseden di 4 misi dev → FLAG, jangan invent.
- Commit per-misi. Pesan git ringkas; jangan tambahin atribusi AI kalau dev minta gitu (cek preferensi user).
- Kalau ada crash saat test → catat ke `PLAN_CoopAndreas_TestLog.md` (§0 format), diagnosa, apply lesson ke misi lain.

## ANTI-PATTERN (dilarang)
- ❌ Ubah/hapus baris vanilla.  ❌ Hardcode jumlah player / loop 0 to 2 (pakai 0 to 6).
- ❌ Mirror opcode yg auto-sync (cutscene/print_big/camera) — double/salah.
- ❌ Force-warp follower ke kendaraan.  ❌ Checkpoint buntu tanpa gate (follower stuck).
- ❌ Handshake di entity placeholder-coord (hang).  ❌ Update blip tanpa Remove (nyangkut).
- ❌ Nebak semantik objektif / desain mekanik khusus tanpa nanya.

## BATAS JUJUR
- Drive-by-in-vehicle + ped mati (mis. Drive-By/SWEET4) → crash engine ped-group. Skip/unsupported.
- Mekanik khusus (dansa/RC/terbang/kereta-timed) → butuh keputusan desain dulu.
- Kamu bisa compile, TIDAK bisa playtest. Jangan klaim "tested/working" tanpa hasil in-game dari dev.

---

## TUGAS
Konversi misi: **<NAMA_MISI>** (`scm/scripts/<CODE>.txt`).
Ikuti LANGKAH di atas. Baca `PLAN_HowToConvert.md` dulu. Compile-verify via CLI. Update tracker. Lapor ringkas.
