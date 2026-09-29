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

### [2026-09-29] ✅ PLAYTEST BERHASIL — mod jalan, 2 player konek main bareng (setelah ASI dibuang)
Milestone lolos. 2 bug misi ditemukan (di bawah).

### [2026-09-29] SWEET2 (Nines and AK's) — non-host STUCK di marker Big Smoke
- Gejala: player non-host nyampe marker Big Smoke tapi mission gak lanjut buat dia (stuck). Host jalan normal.
- Diagnosa: DESAIN. Objektif vanilla ngecek `Char.IsSittingInCar($scplayer, $big_smoke_car)` — $scplayer = HOST doang. Follower kukasih mobil sendiri + checkpoint, jadi mereka nyampe marker tapi ARRIVAL gak trigger apa2 (cuma host yg majuin fase). Follower idle "stuck".
- Fix kandidat: (a) buang checkpoint follower yg misleading, sisain car-blip aja (follower ngerti cuma ngikut); ATAU (b) pola teleport-together (tarik follower ke host di transisi fase) kayak misi skeleton — lebih pas buat misi yg objektifnya keiket host-in-car. Butuh keputusan + test.
- Lesson umum? Y → pakai POLA SWEET1 (ride-together): teman naik mobil misi bareng + objektif NUNGGU semua teman deket tujuan (gate 206@). Bukan teleport, bukan own-car+checkpoint-buntu.
- **FIX DITERAPKAN (SWEET2, 2026-09-29):** buang checkpoint buntu (sisain car blip Big Smoke), tambah gate 206@ di loop objektif drive (@SWEET2_1949): loop cek semua $NETWORK_PLAYER pakai Char.LocateAnyMeans3D deket tujuan (12m), kalau ada yg jauh 206@=1, tambah `206@ <> 0` ke kondisi OR objektif. Compile OK, deployed. Var 206@ bebas di SWEET2 (niru SWEET1).
- Status: FIXED (nunggu re-test). Kalau lolos → terapin pola ke SWEET3/SMOKE2/RYDER2/DRUGS1/DRUGS4.

### [2026-09-29] SWEET4 (Drive-By) — CRASH di non-host
- Crash log: Downloads/2026-09-29_17-02-10.log (dari mesin teman, path C:\apps download\GTASA = non-host).
- Exception 0x00642067 gta_sa.exe AV **read**, EAX=0x00000000 (null-ptr deref). Backtrace: gta_sa 0x642067 <- 0x841ADB <- 0x83BBF6 <- **CoopAndreasSA.dll (0x7484A6A3, 0x72FFB)** <- 0x53E986. Jadi crash lewat jalur SYNC mod, di kode ped/vehicle/task game.
- Active scripts: SWEET4 TIDAK ada di list (thread misi udah ilang/entity invalid pas crash).
- Diagnosa (hipotesis): entity yg di-sync (gang ped 394@/401@/408@ atau drive-by car 543@) jadi invalid/null di sisi non-host → mod proses task ped null → crash. Konversiku map ke-4 entity itu ke network id (handshake) = nambah sync surface. Drive-by (AI ped nembak dari mobil gerak) emang berat buat sync.
- Fix kandidat: kurangi sync surface — JANGAN handshake/map gang PEDS (394@/401@/408@), sisain car 543@ blip aja. ATAU guard null. Belum pasti akar; butuh test ulang.
- **CLUE dari user (2026-09-29): crash terjadi PAS NON-HOST MEMBUNUH (ped/musuh).** → ini ped-DEATH sync: pas ped mati, jalur sync mod proses ped (null) → deref. Kemungkinan besar isu CORE CoopAndreas (sync kematian ped), BUKAN dari edit SCM-ku (aku cuma map gang friendly + car, gak map Balla musuh). Fix butuh investigasi C++ (guard null di ped-death sync handler), kemungkinan gak bisa dari SCM.
- Lesson umum? Ped-death sync di client rawan null-deref. Perlu cek C++ PacketHandlers/peds.cpp (death) + CNetworkPed destroy.
- Status: OPEN (butuh investigasi C++ ped-death, bukan SCM)
- **KONFIRMASI (2026-09-29):** Drive-By ASLINYA ditandai UNSUPPORTED oleh dev upstream (commit "add some unadapted mission warnings" — warning "may cause crashes"). Dev SENGAJA gak convert. User konfirmasi: Tagging Up Turf & Cleaning the Hood (dev-converted, JALAN KAKI, enemy statis) kill-ped AMAN; Drive-By (DI MOBIL + enemy WAVE/respawn) crash. → Akar = limitasi engine sync utk ped-mati-di-kendaraan + churn wave, BUKAN bug SCM-ku. Fix = C++ engine (guard ped-death-in-vehicle), butuh debug live.
- **KATEGORISASI MISI (pedoman konversi):**
  - ✅ AMAN diconvert: tempur JALAN KAKI (gang war, kill-target on-foot), drive-to-destination, escort. Contoh proven: Tagging Up Turf, Cleaning the Hood.
  - ⚠️ DICURIGAI (bukan divonis — TEST dulu): tempur DRIVE-BY di kendaraan (TASK_SIMPLE_GANG_DRIVEBY), misi mekanik-khusus (dansa/RC/terbang/kereta).
  - ❌ KONFIRMASI crash: Drive-By (in-vehicle driveby + ped-group death).
  - **KOREKSI:** "wave/enemy respawn" BUKAN penyebab crash (dulu sempat kutulis begitu, SALAH). Bukti: Tagging Up Turf & Cleaning the Hood punya kill-geng banyak/berkelompok tapi AMAN. Aturan: JANGAN pre-ban kategori; convert → test → hanya yg beneran crash & susah difix yg di-unsupported.
- **INVESTIGASI C++ SELESAI (2026-09-29, via PDB symbol + llvm-symbolizer):** Chain crash Drive-By =
  CGame::Process (0x53E981) → Events::gameProcessEvent (hook per-frame mod, injector call_hooks @DLL RVA 0x42FFB) →
  lambda mod (DLL RVA 0x1A6A3, Main.cpp gameProcessEvent handler) → game 0x83BBF6 → 0x841ADB → **0x642067 null-deref
  (area CPedGroups)**. Pas geng-LAWAN mati, proses per-frame mod micu kode ped-group game yg deref null di non-host.
  Fungsi game 0x83BBF6/0x841ADB/0x642067 UNLABELED di plugin-sdk (engine internal) → gak bisa dipetakan lebih jauh
  tanpa debugger live. Build symbol (PDB) di-deploy; crash log dari mesin ber-PDB ke-simbolisasi di frame mod.
  KESIMPULAN: bug engine-level ped-group↔sync saat ped mati; kategori yg upstream tandai unsupported. Bukan quick-fix.
  Next kalau mau lanjut: buka .dmp di Visual Studio (ada disassembly+register di 0x642067) — effort multi-sesi.
  TODO housekeeping: balikin xmake client ke set_strip("all") (skrg set_symbols debug utk diagnosis).

### [2026-09-29] STARTUP CRASH — modloader.asi (bentrok mod repack, BUKAN kode kita)
- Kapan/Gejala: crash pas startup game (sebelum masuk), "Active scripts: (empty)" = SEBELUM SCM load. Player: host. Konsisten (3x berturut 14:38–14:39).
- Crash log: `CoopAndreas_crashes/2026-09-29_14-38-27.log`. Exception 0x0074872E AV read.
- Diagnosa: backtrace 0x5C17xxxx–0x5C180xxx = **modloader.asi** (base 0x5C160000) lagi scan path file (string game path + AppData\Local keliatan di stack dump). Repack "Original Trilogy" punya ~15 ASI (SilentPatchSA, skygfx, WidescreenFix, III.VC.SA.LimitAdjuster, MixSets, MobileHud, RealTrafficFix, SALodLights, modloader, GInputSA, skygfx, dll) yg bentrok sama hook CoopAndreas. Game versi bener (1.0.0.0 US) tapi ke-mod berat. Crash lama 01:28 (sebelum perubahan apapun hari ini) JUGA startup crash → pre-existing.
- BUKAN dari: serial removal (itu crash 0xDEAD, beda), BUKAN dari main.scm/misi (crash sebelum SCM load).
- Fix: TEST di GTA SA v1.0 US BERSIH, atau nonaktifkan ASI repack (pindahkan `scripts/*.asi` + `modloader/` keluar sementara, sisakan cuma eax.dll/eax_orig.dll/CoopAndreasSA.dll). CoopAndreas gak butuh CLEO/modloader.
- Lesson umum? Y → §2 C-03. Revisi: N/A (bukan kode kita; masalah environment).
- Status: MITIGATED (2026-09-29) — ASI repack dipindah ke `C:\Games\_coop_asi_backup` (root/ + scripts/ + modloader/ + cleo/). Game sekarang cuma load CoopAndreas. Nunggu user re-test.
- **RESTORE mod repack** (kalau mau balikin): pindahkan balik dari `C:\Games\_coop_asi_backup`:
  `_coop_asi_backup/root/*` -> game root, `_coop_asi_backup/scripts/*` -> game/scripts/, `modloader/` & `cleo/` -> game root.

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

## 3b. DEEPENING SKELETON (2026-09-29) — kenapa cuma sebagian
Cloud session bikin ~22 misi "skeleton" (EnableSyncing + CollectNetworkPlayers + TeleportPlayersToHostSafely,
TANPA per-player blip). Investigasi buat perdalam: MAYORITAS gak aman diperdalam buta:
- **Placeholder-coord entity** (DRIV2 34@ di 0,0,-100; WUZI1 dummy pedtype 8) → netID handshake bisa HANG (L-01/C-02).
- **Blip phase-toggled** (SYND4: `Blip.AddForChar(39@)` langsung `ChangeDisplay(Neither)` → disembunyiin, dibuka/tutup per fase) → per-player blip naif = bocorin target kepagian.
- **Multi-target shooting gallery** (SYN5 6 target, MUSIC5 6 target) → blip banyak, low value.
- **Dance/race/interior/terbang** (CESAR1, CPRACE, MUSIC2, CATCUT, SYN6/7, WUZI2) → teleport-together JUSTRU pola yg bener.
**Yang AMAN diperdalam & udah dikerjain:** DECON (car 85@) + SYN2 (car 59@) — objektif = 1 car, real-coord, blip friendly normal → netID handshake aman + per-player convoy blip + cleanup. COMPILES.
**Kesimpulan:** sisa skeleton DIBIARIN (aman apa adanya). Perdalam sisanya HARUS setelah playtest (biar tau handshake mana aman & fidelity blip per fase).

## 3c. TEMUAN RISIKO (dari baca source, belum tentu crash — cek pas playtest)
- **R-01 Teleport player di dalam mobil:** `TeleportPlayerScripted` handler (scripts.cpp:124) cuma panggil
  `pPlayerPed->Teleport(pos)` — TIDAK warp player keluar mobil dulu. Kalau follower lagi NYETIR pas host
  trigger `TeleportPlayersToHostSafely`, ped-nya bisa ke-yank keluar/desync dari mobilnya (ghost car / glitch).
  Semua misi skeleton pakai teleport-together di awal — RISIKO kalau player mulai misi sambil di mobil.
  Mitigasi kalau kejadian: sebelum teleport, warp player keluar mobil dulu (belum diimplement).
- **R-02 Player limit >8:** butuh refactor SEDANG (bukan kecil). Detail di PLAN_AllMissions §7 / jawaban chat.
- **LA1FIN2 (Green Sabre) DIPERDALAM:** car 40@ (drive ke docks) → per-player convoy blip (fase car-blip @390) +
  handshake real-coord + cleanup. Fase coord-blip tetap teleport-together. COMPILES.
- **MUSIC5 (House Party) DIBIARIN skeleton:** 8 target musuh × blip toggle 3-4× di 8 label terpisah (20 add point).
  Mirror faithful = kompleks & rawan; value rendah (defense 1 lokasi, player udah co-located via teleport). Skip sampai playtest.

## 4. RINGKASAN MISI TER-KONVERSI (status test)
Sinkron dengan tracker `PLAN_AllMissions_Campaign.md`. Status test: ⬜ belum · 🟡 sebagian · ✅ lolos happy-path.

| Misi | Pola | Compile | Playtest |
|------|------|---------|----------|
| SWEET3 Drive-Thru | convoy 1-mobil +SWEET1-gate | ✅ | ⬜ (re-test) |
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
| LA1FIN2 The Green Sabre | multi-phase (teleport-bareng tiap relokasi + shootout) | 🟡 draft (belum compile: cloud tanpa CLI) | ⬜ |
