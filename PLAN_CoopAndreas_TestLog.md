# TEST & CRASH LOG — CoopAndreas Konversi Misi

> **Fungsi:** catat SETIAP crash / desync / bug / hasil playtest in-game, biar (1) jadi bahan
> self-learning buat Claude, (2) tiap "lesson" bisa diterapin BALIK ke misi yang udah dikonversi
> (revisi), bukan cuma misi baru. Baca file ini tiap sesi SEBELUM konversi misi baru.
>
> Terkait: `PLAN_AllMissions_Campaign.md` (tracker), `PLAN_SWEET3_Conversion.md` (pola & build/compile).

---

## 0. CARA LAPOR (buat dev) — makin detail makin cepat ke-fix
Tiap ada masalah in-game, kasih info ini (copas format bawah):
```
Misi        : (nama / file, mis. SWEET4)
Kapan       : (pas load misi / pas nyetir / pas cutscene / pas objektif X / pas selesai)
Gejala      : (crash total / freeze / desync / blip gak muncul / objektif gak jalan / player ke-stuck / dll)
Player      : (host / client / dua-duanya)
Jumlah      : (1 client / 2 client)
Log/console : (kalau ada pesan error, screenshot, atau isi console)
```
Claude bakal: reproduce logika → cari akar → fix misi itu → cek apakah lesson-nya berlaku ke misi lain → revisi semua yang kena → append ke §1 & §2.

---

## 1. LOG KEJADIAN (append, jangan hapus — histori)
Format entry:
```
### [tanggal] MISI — gejala singkat
- Kapan/Gejala/Player: ...
- Diagnosa: ...
- Fix: ... (file:baris / opcode)
- Lesson umum? (Y/T) → kalau Y, catat di §2 + daftar misi yg perlu revisi
- Status: OPEN / FIXED / MONITOR
```

_(belum ada entry — nunggu playtest pertama)_

<!-- contoh (hapus/isi saat ada kejadian nyata):
### [2026-09-29] SWEET4 — freeze pas load misi
- Kapan: pas misi mulai (sebelum kontrol player). Gejala: game freeze. Player: dua-duanya. Jumlah: 2 client.
- Diagnosa: netID handshake `while $temp_int == -1` di 543@ nyangkut — entity spawn di coord placeholder blm ke-register.
- Fix: pindahin blok handshake ke SETELAH entity di-warp ke posisi nyata (SWEET4.txt:~980).
- Lesson umum? Y → §2 L-01. Revisi: SMOKE2 (entity juga spawn di 0,0,-100).
- Status: FIXED
-->

---

## 2. LESSONS → REVISI RETROAKTIF (aturan yang lahir dari crash nyata)
> Tiap lesson di sini WAJIB dicek & diterapin ke SEMUA misi yang udah/akan dikonversi.
> Kolom "Applied to" dicentang saat misi udah direvisi.

| ID | Lesson (dari crash apa) | Aturan koreksi | Applied to |
|----|--------------------------|----------------|-----------|
| C-01 | Compile "A jump to offset 0 found" — bukan crash game, tapi COMPILE. Salah opcode buat thread & mission-launch. | Bikin background thread = `start_new_script @label` (004F), BUKAN `create_thread`. Start misi = `load_and_launch_mission_internal <number>` (0417) pakai NOMOR DEFINE MISSION, BUKAN `launch_mission @missionlabel` (label misi beda offset-space → offset 0). | DEBUG_LAUNCHER ✅ |
| C-02 | Potensi HANG: netID handshake `while GetXNetworkId(h)==-1` di entity yg BELUM dibuat (handle=0/garbage) → -1 selamanya → misi stuck. Label order ≠ runtime order (entity sering dibuat di gosub SETELAH $onmission=1). | JANGAN taruh netID handshake di init cuma krn keliatan "setelah $onmission=1". Taruh PERSIS di sebelah vanilla `Blip.AddFor{Car,Char}(h)` yg sepadan — di situ h dijamin exist (vanilla nge-blip-nya). Prinsip umum: tiap Coop per-player blip call = tetangga langsung vanilla Blip.* yg sepadan. | CAT3 ✅ (ketauan pas draft: 327@ dibuat di CAT3_710, dipindah ke @CAT3_5488) / semua misi baru ikut prinsip ini |

<!-- contoh format:
| L-01 | Handshake nyangkut utk entity yg spawn di coord placeholder | netID handshake HARUS setelah entity di-warp ke posisi nyata, jangan pas Char/Car.Create di (0,0,-100) | SWEET4 ✅ / SMOKE2 ⬜ |
-->

---

## 3. HAL YANG BELUM DI-CONVERT (potensi masalah yg udah diketahui, belum tentu crash)
Catatan dari proses konversi — bukan crash, tapi kandidat isu saat test:
- **Teks objektif per-player** belum di-backfill (SWEET2/3/4, SMOKE2, RYDER2, HOODS5, CRASH4) — player non-host belum liat teks misi. Non-fatal.
- **SMOKE2 / (entity placeholder coord)**: netID handshake ditaruh tepat setelah create di coord (0,0,-100) lalu di-warp. Kalau HANG in-game → ini tersangka utama (calon L-01).
- **RYDER2**: blip objektif pindah ke getaway car 99@ di fase akhir, belum di-sync per-player (follower bisa kehilangan arah di fase itu).
- **Interior missions** (belum digarap): wajib pakai pola teleport-bareng SWEET1B, bukan per-player.

### POLA INTERIOR (dari SWEET1B — dipakai buat konvert misi interior, BUKAN auto-BLOCKED)
Engine GTA cuma render 1 interior aktif global → co-op interior JALAN kalau SEMUA player masuk barengan (share 1 instance). Pola:
1. Di tiap titik host di-teleport masuk misi/interior (`Char.SetCoordinates($scplayer, x,y,z)`), tepat SETELAHNYA panggil:
   `Coop.TeleportPlayersToHostSafely($NETWORK_PLAYER[0], $NETWORK_PLAYER[1], $NETWORK_PLAYER[2])` → semua player ikut.
2. Buat transisi interior, samain area follower dulu:
   `for $temp_int = 0 to 2 / if Coop.IsNetworkPlayerActorValid(...) then Char.SetAreaVisible($NETWORK_PLAYER[$temp_int], <area>) end end` lalu teleport.
3. Cleanup akhir misi: set area follower balik ke 0 (exterior) + teleport bareng (SWEET1B line ~2500).
Jadi CRASH1/MUSIC2/CAT4/CASIN10 dll BISA dikonvert pakai pola ini (bukan cuma di-skip) — asal semua player masuk bareng.
- **SWEET7 (branch-dependent Ballas)**: 3 target Ballas (289@/290@/291@) cuma di-spawn di salah satu branch (`Game.FindMaxNumberOfGroupMembers()>1` → branch tanpa Ballas). Per-player char-blip create ditaruh di branch yg pasti spawn; removal mirror gate vanilla (296@/297@/298@). Kalau in-game funeral phase gak ada Ballas / blip aneh → cek branch mana yg jalan. Non-fatal (mirror vanilla).
- **SWEET7 (netID handshake escape car)**: 78@/$sweet netID di-handshake di @SWEET7_3089 (sblm fase kabur). Kalau HANG pas funeral cutscene → tersangka 78@ belum ke-register sync (mirip L-01 risk). Escape car 78@ dibikin awal & di-freeze, harusnya aman.

---

## 4. RINGKASAN MISI TER-KONVERSI (status test)
Sinkron dengan tracker `PLAN_AllMissions_Campaign.md`. Status test: ⬜ belum · 🟡 sebagian · ✅ lolos happy-path.

| Misi | Pola | Compile | Playtest |
|------|------|---------|----------|
| SWEET3 Drive-Thru | convoy 1-mobil | ✅ | ⬜ |
| SWEET2 Nines and AK's | convoy+kaki | ✅ | ⬜ |
| SWEET4 Drive-By | convoy+combat | ✅ | ⬜ |
| SMOKE2 Running Dog | convoy+chase | ✅ | ⬜ |
| RYDER2 Robbing Uncle Sam | convoy+combat | ✅ | ⬜ |
| HOODS5 Sweet's Girl | escort/char-blip | ✅ | ⬜ |
| CRASH4 Doberman | territory+kill | ✅ | ⬜ |
| DRUGS3 Gray Imports | location+kill | ✅ | ⬜ |
| TWAR7 OG Loc | convoy+chase-kill | ✅ | ⬜ |
| DRUGS1 Just Business | convoy+combat | ✅ | ⬜ |
| DRUGS4 Reuniting the Families | convoy+combat | ✅ | ⬜ |
| SWEET7 Los Sepulcros | escort+combat (drive Sweet home + funeral ambush) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
| BCRASH1 Badlands | location+kill (snipe target + escape) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
| GROVE2 Grove 4 Life | escort/companion+combat (follow Sweet, gang war) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
| TRUTH1 Body Harvest | convoy/escort-vehicle (curi harvester, drive back) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
| MANSIO3 Home Coming | territory-clear+kill (6 dealer targets di Grove) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
| STEAL1 Zeroing In | track/follow-car (radar cari mobil target) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
| CAT3 Tanker Commander | convoy/escort-vehicle (curi tanker, drive back) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
| SYN3 Outrider | convoy (host nyetir mission car 34@, gauntlet) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
| MUSIC1 Life's a Beach | convoy (curi van, drive back; dance minigame host-solo) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
| MUSIC2 Madd Dogg's Rhymes | INTERIOR (teleport-bareng masuk/keluar mansion; stealth) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
| MUSIC3 Management Issues | multi-kill (8 target pesta, per-player enemy blip) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
| MUSIC5 House Party | wave-defense (teleport-bareng ke pesta, fight bareng) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
