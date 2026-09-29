# PANDUAN KONVERSI MISI KE CO-OP — Pola Asli Dev (deep-learn)

> **Sumber:** pembacaan mendalam 4 misi yang dikonversi DEV upstream:
> `INTRO1` (Big Smoke, 58 calls), `INTRO2` (Ryder, 59), `SWEET1` (Tagging Up Turf, 103),
> `SWEET1B` (Cleaning The Hood, 88). Ini pola KANONIK — tiru persis, jangan invent.
>
> **ARSITEKTUR FUNDAMENTAL (TERUJI — baca ini dulu, semua bergantung ke sini):**
> **MISI SCM JALAN DI HOST DOANG.** Launcher (`SWEET.txt` dll) nge-gate `Mission.LoadAndLaunchInternal(N)`
> di belakang `Coop.LocateAllPlayersOnFoot3D` yg **false buat non-host** → cuma HOST yg launch thread misi.
> Non-host TIDAK menjalankan script misi — dia cuma MENERIMA: (a) opcode auto-sync yg di-broadcast host
> (COpCodeSync.cpp:209 `if (m_bIsHost && ...)` = cuma host broadcast), dan (b) packet `Coop.*ForNetworkPlayer`.
> Konsekuensi (semua nyambung ke sini):
> - `$scplayer` = CJ HOST. `$NETWORK_PLAYER[i]` = ped follower (di game host). Host authoritative penuh.
> - `Coop.IsHost` GAK PERLU di misi → di dalam thread misi lo SELALU host.
> - Follower dimanipulasi lewat packet `Coop.*` (mereka gak run script). Bukan mereka yg "jalanin"nya.
> - Follower MATI = non-event buat logika misi (host gak cek `IsDead($NETWORK_PLAYER)`; layer sync yg respawn).
>   Cuma HOST ($scplayer) mati = misi gagal (vanilla deatharrest).
>
> **Prinsip inti (WAJIB):**
> 1. **ADDITIVE-ONLY.** Jangan ubah/hapus logika vanilla. Host jalanin alur asli sebagai `$scplayer`.
>    Co-op cuma NAMBAH lapisan di atasnya (follower = `$NETWORK_PLAYER[i]`).
> 2. **DINAMIS, bukan hardcode-count.** Selalu loop `for/FOR $temp_int = 0 to 6` + guard
>    `Coop.IsNetworkPlayerActorValid(...)`. Slot yg gak ada otomatis di-skip → jalan buat 2 s/d 8 player.
>    JANGAN nulis logika khusus "kalau 4 player". Satu-satunya yg positional = koordinat spawn kendaraan.
> 3. **KONSISTEN & paired.** Tiap `Update*ForNetworkPlayer` WAJIB ada `Remove*` pasangannya di cleanup/fail.
> 4. **NON-HARDCODE arity.** `$NETWORK_PLAYER` = array 7 (host+7=8). Collect = 7-var. (di-set global di SDK,
>    lihat PLAN_Refactor_8Slots.md — bukan tambal per-misi.)

---

## 0. VOCAB Coop API (frekuensi di 4 misi dev)
```
138x IsNetworkPlayerActorValid   <- guard, HAMPIR tiap blok per-player
 53x PrintNowForNetworkPlayer     <- teks objektif per-player
 18x RemoveCheckpointForNetworkPlayer / 16x UpdateCheckpointForNetworkPlayer
 12x GetVehicleNetworkId / 12x GetPedNetworkId   <- handshake sync entity
 11x UpdateCarBlipForNetworkPlayer / 9x RemoveCarBlipForNetworkPlayer
  9x ClearAllEntityBlipsForNetworkPlayer
  8x UpdateCharBlipForNetworkPlayer / 7x RemoveCharBlipForNetworkPlayer
  4x TeleportPlayersToHostSafely   <- interior / transisi paksa
  4x EnableSyncingThisScript / 4x CollectNetworkPlayersForTheMission
  3x ClearThisPrintForNetworkPlayer / 2x PrintHelpForNetworkPlayer
  2x GetNetworkPlayerInternalId
```
Definisi resmi: `sdk/Sanny Builder 4/data/sa_sbl_coopandreas/opcodes.txt`. Kalau butuh yg gak ada di list → CEK dulu, jangan asumsi.

---

## 0.5 PRINSIP PEMERSATU (TERUJI) — "auto-synced vs mirror manual"
**Ini KENAPA di balik semua pola.** Layer C++ `client/src/COpCodeSync.cpp` punya DAFTAR opcode yang
**otomatis di-replikasi** dari host ke semua client saat script bertanda EnableSyncing menjalankannya.
Jadi aturannya DETERMINISTIK, bukan tebakan:

- **Opcode ADA di list `syncedOpcodes` → BIARKAN VANILLA (jangan mirror, jangan sentuh).** Otomatis nular ke semua.
  Termासuk: `print_big`(0x00BA), `clear_prints`(0x00BE), `print_with_number_big`(0x01E3), `register_mission_passed`(0x0318),
  `print_help`(0x03E5), `load_mission_text`(0x054C), `do_fade`(0x016A), `load/start/clear_cutscene`(0x02E4/E7/EA),
  `set_area_visible`(0x04BB), semua camera ops, `clear_area`, densities, `switch_widescreen`, dan **ped-task**
  (`task_enter_car_as_driver/passenger`, `task_go_to_coord_any_means`, `task_kill_char`, `task_stand_still`, dll —
  task di ped MISSION otomatis jalan sama di semua client).
- **Opcode TIDAK di list + menghadap player → MIRROR MANUAL per-player pakai `Coop.*ForNetworkPlayer`.**
  Yang paling penting: **`Text.PrintNow`(0x00BC show_text_highpriority) TIDAK di list** → makanya dev mirror pakai
  `PrintNowForNetworkPlayer`. Blip & checkpoint juga TIDAK ada di list → pakai `UpdateCar/CharBlip/CheckpointForNetworkPlayer`.

**Konsekuensi praktis saat konversi:**
- Cutscene, camera, fade, print_big/help, area_visible, ped-task → **JANGAN diapa-apain** (auto-sync). Ini kenapa
  4 misi dev gak nge-wrap cutscene sama sekali.
- Cuma `Text.PrintNow`, blip, checkpoint (+ hal non-list yg player-facing) yg perlu loop+guard mirror.
- Kalau ragu opcode X auto-sync apa nggak: **cek `syncedOpcodes[]` di COpCodeSync.cpp**. In-list = vanilla; else = mirror.

## 1. INIT (selalu, urutannya persis)
```
:MISSIONLABEL
script_name 'XXX'
Coop.EnableSyncingThisScript()        // <- baris ~6, TEPAT setelah script_name
...
// di blok init misi (dekat Stat.RegisterMissionGiven / $onmission = 1):
$NETWORK_PLAYER[0], $NETWORK_PLAYER[1], $NETWORK_PLAYER[2], $NETWORK_PLAYER[3], $NETWORK_PLAYER[4], $NETWORK_PLAYER[5], $NETWORK_PLAYER[6] = Coop.CollectNetworkPlayersForTheMission()
```
> Collect HARUS di-panggil SEBELUM blok per-player manapun jalan (isi array dulu).

## 2. HANDSHAKE netID = BARRIER TUNGGU REPLIKASI (bukan "mapping") — TERUJI
**Fakta teruji:** hasil `GetVehicleNetworkId/GetPedNetworkId` selalu DIBUANG (SWEET1 buang ke `$temp_int`
= loop var; INTRO1 simpan ke `86@` lalu di-RESET ke 0 di baris berikutnya). Jadi TUJUANNYA bukan ambil ID —
tapi **BLOCK sampai server replikasi entity** (`!= -1` artinya "entity udah ada di semua client"). Entity
auto-sync karena script EnableSyncing'd + host yg bikin; handshake cuma barrier "tunggu sampai siap".
Taruh DEKAT pembuatan entity (handle dijamin exist di situ).

**Dua idiom sah (pilih salah satu):**
```
// Idiom A (SWEET1) — per-entity, hasil dibuang ke throwaway var:
$temp_int = Coop.GetVehicleNetworkId($sweet_car)
while $temp_int == -1
    wait 0
    $temp_int = Coop.GetVehicleNetworkId($sweet_car)
end
// (ulang blok utk tiap ped/mobil)

// Idiom B (INTRO1) — tunggu SEMUA entity sekaligus dalam 1 loop (lebih efisien):
while true
    wait 0
    $temp_int = Coop.GetVehicleNetworkId(41@)
    86@ = Coop.GetPedNetworkId($big_smoke)
    if and
        $temp_int <> -1
        86@ <> -1
    then
        break
    end
end
```
⚠️ JANGAN handshake entity yg belum dibuat / spawn di coord placeholder (0,0,-100 lalu di-warp) →
gak pernah dapet netID → **HANG selamanya** (persis kenapa entity placeholder rawan; lesson C-02 TestLog).

## 3. IDIOM PER-PLAYER (jantung konversi — SELALU loop+guard)
```
for $temp_int = 0 to 6
    if Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[$temp_int])
    then
        // aksi ke $NETWORK_PLAYER[$temp_int]
    end
end
```
Kegunaan umum:
- **Setup awal:** ClearTasks / SetCoordinates / SetAreaVisible tiap follower.
- **Teks objektif — SELEKTIF (teruji):** vanilla lokal DULU, baru mirror. TAPI dev TIDAK mirror SEMUA teks —
  cuma teks **instruksi/objektif yg tiap player perlu tau** ("go beat up the dealer", "get in car"). Teks
  status/ambient berulang (SWEET1B `SW1B_G` muncul 15x) TIDAK di-mirror. Jangan over-mirror (spam player lain).
  ```
  Text.PrintNow('SWE1_A', 7000, 1)          // buat host (vanilla, JANGAN dihapus)
  for $temp_int = 0 to 6
      if Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[$temp_int])
      then
          Coop.PrintNowForNetworkPlayer('SWE1_A', 7000, 1, $NETWORK_PLAYER[$temp_int])
      end
  end
  ```

## 3.5 MEKANISME START MISI (launcher — biasanya gak diubah pas konversi, tapi WAJIB paham)
Misi di-trigger dari script LAUNCHER (mis. `SWEET.txt`, `INT.txt`), BUKAN body misi. Pola "lingkaran":
```
if Coop.LocateAllPlayersOnFoot3D(0, $sweetX, $sweetY, $sweetZ, 1.2, 1.2, 2.0)   // SEMUA player di radius?
goto_if_false @SKIP
if Player.CanStartMission($player1)
goto_if_false @SKIP
$onmission = 1
Mission.LoadAndLaunchInternal(13)   // launch by NOMOR DEFINE MISSION
```
`Coop.LocateAllPlayersOnFoot3D` (teruji): HOST-ONLY evaluasi (non-host langsung false); cek host + loop
SEMUA `m_pPlayers` di radius; true cuma kalau SEMUA (host+semua follower) berdiri di lingkaran. **Sudah dinamis
(loop m_pPlayers, bukan hardcode-count)** — jadi lingkaran otomatis nunggu berapapun player. Ini INFRA upstream;
biasanya gak disentuh saat konversi body misi.

## 4. BLIP / CHECKPOINT per-player (selalu Update↔Remove berpasangan)
**Konvensi warna (teruji di 4 misi):**
- Mobil objektif/konvoi → `BlipColor.Purple`, `BlipDisplay.Both`, scale 3.
- NPC teman/kawalan → `UpdateCharBlipForNetworkPlayer(..., true, BlipDisplay.Both, BlipColor.Blue, 3)` (friendly=true).
- Target musuh (kejar/bunuh) → `UpdateCharBlipForNetworkPlayer(..., false, BlipDisplay.Both, BlipColor.Red, 3)` (friendly=false).
- Default display = `BlipDisplay.Both` (17/20 pemakaian). BlipOnly/MarkerOnly jarang (kasus khusus).
- Mobil objektif/konvoi: `Coop.UpdateCarBlipForNetworkPlayer($NETWORK_PLAYER[i], <car>, true, BlipDisplay.Both, BlipColor.Purple, 3)`
- NPC yg dikawal/dikejar: `Coop.UpdateCharBlipForNetworkPlayer($NETWORK_PLAYER[i], <ped>, <friendly?>, BlipDisplay.Both, BlipColor.Blue/Red, 3)`
- Titik tujuan on-foot/drive: `Coop.UpdateCheckpointForNetworkPlayer(x, y, z, sx, sy, sz, $NETWORK_PLAYER[i])`
- **Cleanup WAJIB (teruji):** dua cara sah — (a) individual `Remove*ForNetworkPlayer`, ATAU (b) borongan
  `Coop.ClearAllEntityBlipsForNetworkPlayer($NETWORK_PLAYER[i])`. INTRO1 buktinya Update car/char tanpa Remove
  individual sama sekali — dibersihin pakai ClearAllEntityBlips di akhir. Jadi TIDAK wajib 1:1 Update↔Remove.
- **Over-remove itu SENGAJA & aman (teruji):** checkpoint Remove sering LEBIH banyak dari Update (SWEET1 cp U4/R8)
  karena Remove ditaruh di BANYAK path (tiap fail + sukses + transisi fase). Under-remove = blip nyangkut; over-remove aman.

## 5. GATE "TUNGGU SEMUA PLAYER" (biar objektif gak selesai duluan / follower gak ke-tinggal) — TERUJI di 4 misi
Ada DUA idiom sah (dua-duanya dipakai dev; pilih salah satu):
```
// Idiom A (SWEET1) — flag "ada yg belum", tanpa Break:
206@ = 0
for $temp_int = 0 to 6
    if Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[$temp_int])
    then
        if not Char.LocateAnyMeans3D($NETWORK_PLAYER[$temp_int], 0, <destX>, <destY>, <destZ>, 10.0, 10.0, 10.0)
        then
            206@ = 1
        end
    end
end
if or
  not <kondisi vanilla objektif ...>
  206@ <> 0
goto_if_false @NEXT

// Idiom B (INTRO2) — flag "semua ok", pakai Break (berhenti lebih awal, lebih efisien):
150@ = 1
FOR $temp_int = 0 to 6
    if Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[$temp_int])
    then
        if not Char.LocateAnyMeans3D($NETWORK_PLAYER[$temp_int], 0, <destX>, <destY>, <destZ>, 12.0, 12.0, 12.0)
        then
            150@ = 0
            Break
        end
    end
end
if or
  not <kondisi vanilla objektif ...>
  150@ == 0
goto_if_false @NEXT
```
> `Char.LocateAnyMeans3D` = "any means" → follower boleh datang naik mobil sendiri / numpang / jalan kaki. Radius longgar (10-12m). Ada juga varian `Char.LocateOnFoot3D` (INTRO2:668) buat objektif yg wajib jalan kaki.
> Pakai var lokal BEBAS per-misi (SWEET1: 206@, INTRO2: 150@; verifikasi gak dipakai hal lain).
> Idiom B (Break) lebih disaranin utk 8-player (gak iterasi sisa slot begitu ketemu 1 yg jauh).

## 6. KENDARAAN untuk follower (satu-satunya bagian positional)
Dua gaya sah:
- **Ikut mobil misi** (SWEET1/2/3): cukup blip mobil host + gate #5; follower naik/nyusul. Paling simpel.
- **Kendaraan sendiri per-player** (INTRO1 Big Smoke — BMX): spawn 1 kendaraan per slot.
  Dev nulis blok eksplisit per index; buat 8-player pola-nya diperpanjang [0..6], koordinat di-offset:
  ```
  if Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[i])
  then
      $NETWORK_PLAYER_VEHICLE[i] = Car.Create(<model>, baseX, baseY - i*2.0, baseZ)
      Car.SetHealth(...) / SetHeading(...) / SetCanBurstTires(..., False)
  end
  ```
  `$NETWORK_PLAYER_VEHICLE` = array 7.
  > **TERUJI — JANGAN warp follower ke kendaraan!** Dev spawn kendaraan + BLIP-nya, lalu manusia-nya naik SENDIRI.
  > `Task.EnterCar` di INTRO1 CUMA buat host ($scplayer) & AI homie — network player TIDAK di-warp (dia kontrol
  > ped-nya sendiri, hormati agency). Misi cuma CEK: `if not Char.IsInCar($NETWORK_PLAYER[i], $NETWORK_PLAYER_VEHICLE[i])`
  > → kasih teks "naik ke kendaraan" (per-player). Blip kendaraan + IsInCar-check = cukup; jangan force warp.
  > Idealnya loop dgn offset terhitung (Y = base + i*step) drpd blok copy-paste — sama fungsinya, lebih rapi.

## 7. INTERIOR (misi masuk ruangan) — teleport-bareng, BUKAN per-player instance
Engine GTA cuma render 1 interior global → semua player HARUS masuk bareng (share 1 instance):
```
Streaming.SetAreaVisible(<area>)
for $temp_int = 0 to 6
    if Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[$temp_int])
    then
        if Char.IsInAnyCar($NETWORK_PLAYER[$temp_int])
        then
            Char.WarpFromCarToCoord($NETWORK_PLAYER[$temp_int], <inX>, <inY>, <inZ>)
        end
        Char.ClearTasksImmediately($NETWORK_PLAYER[$temp_int])
        // koordinat berbeda per index biar gak numpuk (SWEET1B: if $temp_int==0 .. ==2 SetCoordinates beda)
        Char.SetAreaVisible($NETWORK_PLAYER[$temp_int], <area>)
    end
end
```
Keluar interior akhir misi: set area follower balik 0 + teleport bareng. `Coop.TeleportPlayersToHostSafely(...)` juga dipakai buat transisi paksa.

## 8. CLEANUP (label akhir + SEMUA fail path)
- Hapus tiap blip/checkpoint per-player (pasangan dari #4).
- Entity misi vanilla tetap dibersihin oleh kode vanilla (jangan diutak-atik).
- Pola paling aman: di label cleanup, loop 0..6 + Remove semua yg pernah di-Update.

---

## 9. CHECKLIST KONVERSI 1 MISI (urut)
1. Baca file penuh: petakan init, spawn entity, objektif tiap fase, kondisi menang/kalah, cleanup.
2. `Coop.EnableSyncingThisScript()` di baris ~6.
3. `Collect...()` 7-var di init (sebelum blok per-player).
4. Handshake netID tiap entity misi penting (dekat pembuatannya).
5. Tiap aksi/teks/blip yg vanilla lakuin ke player → tambah loop+guard mirror ke follower (ADDITIVE).
6. Objektif drive/lokasi → tambah gate #5 (tunggu semua).
7. Kendaraan follower (#6) kalau misi butuh.
8. Interior (#7) kalau misi masuk ruangan.
9. Cleanup #8 di akhir + fail path.
10. Compile via CLI (lihat PLAN_SWEET3_Conversion.md §4a) → deploy → playtest.

## 10. ANTI-PATTERN (jangan)
- ❌ Ubah/hapus baris vanilla (mis. ganti `Text.PrintNow` jadi versi network doang) → host rusak.
- ❌ Hardcode "0 to 2" / logika khusus jumlah player tertentu → pakai 0 to 6 + guard.
- ❌ Kasih follower checkpoint buntu tanpa gate → follower "nyampe" tapi misi gak maju (stuck).
- ❌ Handshake netID di entity placeholder-coord → hang.
- ❌ Update blip/checkpoint tanpa Remove di cleanup → blip nyangkut.
- ❌ Misi interior dibikin per-player instance → gak bisa (engine single-interior); pakai teleport-bareng.
- ❌ Force-warp follower ke kendaraan (WarpCharIntoCar/EnterCar) → jangan; spawn+blip+IsInCar-check aja (hormati agency).
- ❌ Mirror SEMUA Text.PrintNow → cuma teks objektif/instruksi; teks status/ambient jangan (spam).

---

## 12. LOG VERIFIKASI (teruji empiris ke kode dev 2026-09-30 — bukan asersi)
Klaim di panduan ini diuji ke INTRO1/INTRO2/SWEET1/SWEET1B:
- ✅ EnableSyncing di baris 6 (keempat misi) + Collect sebelum blok per-player.
- ✅ Handshake = BARRIER tunggu replikasi (netID value DIBUANG: SWEET1→$temp_int loop-var; INTRO1→86@ di-reset ke 0
  baris berikutnya lalu jadi phase-counter). Dua idiom: per-entity `while ==-1` (SWEET1) & combined `while true..break` (INTRO1).
- ✅ Wait-for-all gate ada di KEEMPAT misi (bukan cuma SWEET1). Dua idiom: 206@ set-1-noBreak (SWEET1) & 150@ set-0-Break (INTRO2).
- ✅ Text mirror SELEKTIF (SWEET1B: SW1B_A/SWE1_YH/SW1B_B di-mirror; SW1B_G ambient 15x TIDAK). Ordering lokal-dulu benar.
- ✅ Blip cleanup: individual Remove* ATAU bulk ClearAllEntityBlips (INTRO1 car U2/R0 + clearAll:3). Over-remove sengaja (SWEET1 cp U4/R8).
- ✅ Follower TIDAK di-warp ke kendaraan (INTRO1: EnterCar cuma host+AI; NETWORK_PLAYER via blip+IsInCar-check).
- ✅ Interior: SetAreaVisible(area) masuk + per-index SetCoordinates; exit SetAreaVisible(0) + TeleportPlayersToHostSafely (SWEET1B).
Koreksi yg lahir dari uji: handshake itu barrier bukan mapping; gate & handshake punya 2 idiom; text mirror selektif;
cleanup gak wajib 1:1; follower jangan di-warp. (MD versi awal sempat salah di poin2 ini — sekarang dibetulin.)
- ✅ **PRINSIP PEMERSATU (§0.5):** mirroring itu DETERMINISTIK, bukan judgment. Opcode di `syncedOpcodes[]`
  (COpCodeSync.cpp) auto-replikasi (print_big/help, cutscene, camera, fade, area_visible, ped-task) → vanilla.
  Yg TIDAK (Text.PrintNow 0x00BC, blip, checkpoint) → mirror manual. Verified: 0x00BA in-list, 0x00BC NOT in-list.
- ✅ IsHost = 0x pemakaian di 4 misi. deatharrest/fail = vanilla. Cutscene = 0x wrapping (auto-synced).
- ✅ GetNetworkPlayerInternalId → player-index utk Player.* opcodes (INTRO2 clothes).
- ✅ Start misi (launcher SWEET.txt/INT.txt): `LocateAllPlayersOnFoot3D` host-only + loop m_pPlayers (dinamis, nunggu SEMUA di lingkaran) → `Mission.LoadAndLaunchInternal(N)`.
- ✅ Konvensi blip: Purple=mobil objektif, Blue=NPC teman(friendly true), Red=target musuh(false), Both=default. Konversi kita sudah ikut konvensi ini.
- ✅ **ARSITEKTUR: misi jalan HOST-ONLY** (launcher gate LocateAllPlayers false utk non-host; COpCodeSync host-only broadcast).
  Non-host cuma terima opcode+packet. Ini KOREKSI TERBESAR — dulu sempat kubilang "jalan di 2 client" (SALAH).
  Menjelaskan: IsHost unused, follower via packet, auto-sync, follower-death non-event.
- ✅ Follower mati = non-event (IsDead atas NETWORK_PLAYER = 0x). Mission-passed vanilla (RegisterMissionPassed 0x0318 auto-sync). Money reward gap (belum sync).

## 10.5 CATATAN TAMBAHAN (teruji)
- **`Coop.IsHost` TIDAK dipakai di 4 misi dev** (0x). Misi jalan IDENTIK di semua client; otoritas host dari
  layer sync C++, bukan cabang SCM. JANGAN reach for IsHost pas konversi misi kecuali ada alasan sangat spesifik.
- **deatharrest (fail-on-death) = VANILLA, jangan sentuh** (baris 10 tiap misi: `has_deatharrest_been_executed`).
  Misi gagal kalau HOST ($scplayer) mati (vanilla). Follower mati → respawn via layer sync, gak nge-fail misi.
- **`Coop.GetNetworkPlayerInternalId($NETWORK_PLAYER[i])`** → convert network-player jadi **player-index** buat
  opcode `Player.*` yg butuh index (mis. `Player.GetClothesItem`). Beda dari ped-handle. Dipakai INTRO2 utk baca clothes.
- **`Coop.TeleportPlayersToHostSafely(p0,p1,p2)`** → tarik follower ke host (transisi paksa / interior). CATATAN:
  masih 3-arg di API lama; utk 8-player perlu cek apakah API-nya cukup (lihat R-01 TestLog soal warp saat di mobil).
- **Follower mati mid-misi = NON-EVENT (teruji).** 4 misi dev NOL kali cek `Char.IsDead($NETWORK_PLAYER[...])`.
  Jangan tambah logika fail/respawn per-follower — layer sync yg urus respawn. Cuma NPC misi ($sweet dll) & host yg dicek (vanilla).
- **Mission-passed / reward = VANILLA (teruji).** `Stat.RegisterMissionPassed`(0x0318) ADA di synced-list → auto-sync.
  Big "PASSED" text (print_big) auto-sync. `Mission.Finish`/`PlayerMadeProgress`/tune = vanilla, jangan di-wrap.
  ⚠️ Money reward TIDAK di-sync (TODO `sync money` belum kelar) — kalau misi kasih uang, cuma host yg dapet (gap diketahui, minor).

## 11. BATAS (jujur — bukan semua misi cocok)
- Tempur DRIVE-BY di kendaraan + ped mati (Drive-By/SWEET4) → crash engine ped-group (TestLog). Skip/unsupported.
- Mekanik khusus (dansa/RC/terbang/kereta timed) → butuh keputusan desain dulu, bukan pola mekanis ini.
- Selalu: compile lolos ≠ jalan. Playtest 2+ client wajib (cuma manusia).
