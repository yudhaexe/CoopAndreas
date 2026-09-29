# PANDUAN KONVERSI MISI KE CO-OP — Pola Asli Dev (deep-learn)

> **Sumber:** pembacaan mendalam 4 misi yang dikonversi DEV upstream:
> `INTRO1` (Big Smoke, 58 calls), `INTRO2` (Ryder, 59), `SWEET1` (Tagging Up Turf, 103),
> `SWEET1B` (Cleaning The Hood, 88). Ini pola KANONIK — tiru persis, jangan invent.
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

## 2. HANDSHAKE netID (entity misi yg harus muncul & sync di semua client)
Tiap mobil/ped penting (yg vanilla bikin & pakai) di-map ke network id, TARUH DEKAT tempat entity dibuat
(di situ handle dijamin exist — lihat lesson C-02 di TestLog):
```
$temp_int = Coop.GetVehicleNetworkId($sweet_car)
while $temp_int == -1
    wait 0
    $temp_int = Coop.GetVehicleNetworkId($sweet_car)
end
// idem GetPedNetworkId utk ped ($sweet, $big_smoke, dst)
```
⚠️ JANGAN taruh handshake di entity yg belum dibuat / spawn di coord placeholder (0,0,-100 lalu di-warp) →
`while ==-1` bisa hang. Taruh sebelah `Blip.AddFor*` vanilla yg sepadan.

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
- **Teks objektif:** pasangkan dgn vanilla. Dev SELALU nulis lokal DULU baru mirror:
  ```
  Text.PrintNow('SWE1_A', 7000, 1)          // buat host (vanilla, JANGAN dihapus)
  for $temp_int = 0 to 6
      if Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[$temp_int])
      then
          Coop.PrintNowForNetworkPlayer('SWE1_A', 7000, 1, $NETWORK_PLAYER[$temp_int])
      end
  end
  ```

## 4. BLIP / CHECKPOINT per-player (selalu Update↔Remove berpasangan)
- Mobil objektif/konvoi: `Coop.UpdateCarBlipForNetworkPlayer($NETWORK_PLAYER[i], <car>, true, BlipDisplay.Both, BlipColor.Purple, 3)`
- NPC yg dikawal/dikejar: `Coop.UpdateCharBlipForNetworkPlayer($NETWORK_PLAYER[i], <ped>, <friendly?>, BlipDisplay.Both, BlipColor.Blue/Red, 3)`
- Titik tujuan on-foot/drive: `Coop.UpdateCheckpointForNetworkPlayer(x, y, z, sx, sy, sz, $NETWORK_PLAYER[i])`
- **Cleanup WAJIB** (di label akhir misi + tiap fail path): `Coop.RemoveCarBlipForNetworkPlayer / RemoveCharBlipForNetworkPlayer / RemoveCheckpointForNetworkPlayer`,
  atau borongan `Coop.ClearAllEntityBlipsForNetworkPlayer($NETWORK_PLAYER[i])`.

## 5. GATE "TUNGGU SEMUA PLAYER" (biar objektif gak selesai duluan / follower gak ke-tinggal)
Pola `206@` dari SWEET1 — tambahkan ke kondisi objektif drive/lokasi:
```
206@ = 0
for $temp_int = 0 to 6
    if Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[$temp_int])
    then
        if not Char.LocateAnyMeans3D($NETWORK_PLAYER[$temp_int], 0, <destX>, <destY>, <destZ>, 12.0, 12.0, 12.0)
        then
            206@ = 1     // ada yg belum sampai
        end
    end
end
if or
  not <kondisi vanilla objektif ...>
  206@ <> 0            // <- gate: jangan lanjut sampai semua sampai
goto_if_false @NEXT
```
> `Char.LocateAnyMeans3D` = "any means" → follower boleh datang naik mobil sendiri / numpang / jalan kaki. Radius agak longgar (10-12m).
> Pakai var lokal yg BEBAS di misi itu (SWEET1 pakai 206@; verifikasi gak dipakai hal lain).

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
  lalu warp/track pakai loop+guard biasa. `$NETWORK_PLAYER_VEHICLE` = array 7.
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

## 11. BATAS (jujur — bukan semua misi cocok)
- Tempur DRIVE-BY di kendaraan + ped mati (Drive-By/SWEET4) → crash engine ped-group (TestLog). Skip/unsupported.
- Mekanik khusus (dansa/RC/terbang/kereta timed) → butuh keputusan desain dulu, bukan pola mekanis ini.
- Selalu: compile lolos ≠ jalan. Playtest 2+ client wajib (cuma manusia).
