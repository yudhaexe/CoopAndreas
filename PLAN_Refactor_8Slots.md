# PEDOMAN — Refactor Slot Misi Co-op 4 → 8 (host + 7)

> Tujuan user: misi co-op bisa 8 player (skrg cap 4 = host + 3). Pakai pola SWEET1
> "any means near destination" biar teman GAK wajib naik mobil misi → soal kursi kelar
> (yang gak muat tinggal nyusul ke tujuan). Freeroam SUDAH 8; ini KHUSUS misi.

---

## ⚠️ TEMUAN BLOCKER (wajib dibaca dulu)
Arity opcode `Coop.CollectNetworkPlayersForTheMission` = **3 output** (player1/2/3), didefinisikan di:
- `sdk/Sanny Builder 4/data/sa_sbl_coopandreas/sa_coop.json` (id `1D02`, `num_params: 3`, output 3 Char) — SOURCE
- **`classes.db` / `sa_coop.db` (SQLite, BINARY)** — yang DIPAKAI COMPILER sanny.exe saat compile.

**Blocker:** ubah JSON aja kemungkinan TIDAK cukup — compiler baca dari .db biner. Harus:
- regenerate .db dari json (butuh tooling Sanny yg belum kita punya scripted), ATAU
- edit langsung SQLite `classes.db`/`sa_coop.db` (butuh sqlite tool + tau skema).
Kalau arity di-compiler tetap 3 tapi SCM manggil 7-var → **compile error**. Kalau C++ StoreParameters(7)
tapi SCM baca 3 → **var space script KORUP** (runtime rusak). Ketiganya WAJIB konsisten.

**STATUS: BELUM feasible tanpa beresin arity di compiler DB dulu.** Ini langkah #0 refactor.

### HASIL TES (2026-09-29) — arity resisten diubah via CLI
Dites langsung: opcode arity 3 di-define di file TEKS `sa_coop.db` (mode.xml `<classes>`), `classes.db`,
`sa_coop.json` (mode.xml `<library>`) — line 3380: `CollectNetworkPlayersForTheMission,1D02,0,0,("player1: Char"...)`.
Edit ketiganya ke 7 output + hapus cache `debug.bin` → compiler TETAP "Expected 3 params". Jadi Sanny nge-cache
arity di tempat yg gak ketimpa lewat CLI (kemungkinan perlu regen via GUI, atau cache registry/appdata).
**KESIMPULAN: arity mission-players di-DESAIN 4 oleh pembuat mod; ngubahnya fragile + NON-PORTABLE**
(perubahan ada di install Sanny lokal, BUKAN repo — tiap recompile/reinstall/teman balik ke 4).
**REKOMENDASI: JANGAN kerjakan. Misi tetap 4, freeroam tetap 8.** Ratio effort:risk:value jelek
(untestable + non-portable + misi SP didesain ≤4 orang). Semua file Sanny udah di-restore ke kondisi bersih.

### ✅ UPDATE (2026-09-29 malam) — TERNYATA BISA & SUDAH DIKERJAKAN
Kunci yg tadi kelewat: param count opcode ada di **`SASCM.INI`** (mode.xml `<opcodes>`), baris
`1D02=3,coop_collect_network_players_for_the_mission %1d% %2d% %3d%` — angka `3` = param count yg divalidasi
compiler ("Expected 3 params"). Class DB (sa_coop.db/classes.db) juga list output-nya. Ubah KETIGA
(SASCM.INI count 3→7 + %4d%..%7d%, sa_coop.db & classes.db output player4..7) → 7-var compile SUKSES.
**Portability KEJAWAB:** file2 ini ada di repo `sdk/Sanny Builder 4/data/sa_sbl_coopandreas/` → edit repo + commit = portable
(siapapun copy SDK ke Sanny-nya dapet arity 7).
**DIKERJAKAN:** (1) SDK arity 7 (repo+install), (2) C++ CollectNetworkPlayersForTheMission StoreParameters(7) fill 7,
(3) main.txt `$NETWORK_PLAYER`/`_VEHICLE` array 3→7, (4) SEMUA 56 Collect() call → 7-var, (5) loop `for 0 to 2`→`0 to 6`
di 32 misi konversi (KECUALI upstream SWEET1/SWEET1B/INTRO1/INTRO2 — biarin 4, logika tuned). main.scm compile OK.
**BELUM ditest in-game** (untestable oleh Claude) — butuh playtest >4 player. Deploy: DLL(build 20:50)+main.scm ke game
& kirim ke SEMUA pemain (protocol+arity berubah, wajib seragam).

---

## LANGKAH REFACTOR (urut, semua wajib konsisten)
0. **[BLOCKER] Ubah arity opcode 1D02 di compiler DB** jadi 7 output (player1..player7), num_params 7.
   - Edit sa_coop.json + regenerate/edit classes.db & sa_coop.db. Verifikasi: compile 1 call 7-var sukses.
1. **C++** `client/src/Commands/Commands/CCommandCollectNetworkPlayersForTheMission.cpp`:
   - `memset(ScriptParams, 0, 7 * sizeof(int))`, loop isi sampai 7, `StoreParameters(7)`.
   - Rebuild client DLL (xmake build client).
2. **main.txt** (`scm/main.txt:617`): `$NETWORK_PLAYER: array 3 of integer` → `array 7`.
   (opsional `$NETWORK_PLAYER_VEHICLE: array 3` → 7 kalau dipakai.)
3. **Semua 57 pemanggilan Collect()** (pola IDENTIK, mechanical replace):
   `$NETWORK_PLAYER[0], $NETWORK_PLAYER[1], $NETWORK_PLAYER[2] = Coop.CollectNetworkPlayersForTheMission()`
   → `$NETWORK_PLAYER[0], ..., $NETWORK_PLAYER[6] = Coop.CollectNetworkPlayersForTheMission()`
   (HARUS semua, termasuk upstream SWEET1/1B/INTRO1/INTRO2 — kalau nggak, mismatch arity → compile error.)
4. **Loop per-player** `for $temp_int = 0 to 2` → `for $temp_int = 0 to 6`:
   - **HANYA di misi hasil konversi kita.** JANGAN sentuh loop upstream (SWEET1=35, SWEET1B=27, INTRO2=18)
     — logika mereka di-tune buat 3 network player (special-case index 2, distribusi mobil). Ubah = rawan rusak.
   - Total loop kita: ~50 lokasi tersebar (LA1FIN2, DECON, SWEET2/3/4, SMOKE2, RYDER2, DRUGS1/3/4, HOODS5, CRASH4, TWAR7, dst).
5. **Objektif "any means near destination"** (pola SWEET1): teman GAK wajib naik mobil misi.
   Tiap objektif drive/lokasi, gate pakai loop `Char.LocateAnyMeans3D($NETWORK_PLAYER[i], dest, R)` utk i=0..6
   → set flag kalau ada yg jauh → tambah ke kondisi OR (kayak 206@ di SWEET2). Ini yg bikin kursi gak masalah.
6. **Rebuild DLL + recompile main.scm (CLI) + verify + deploy.** Kirim `main.scm` (+DLL krn C++ berubah) ke teman.

## URUTAN PENGERJAAN (user: FOKUS LOS SANTOS DULU)
Karena arity opcode global (all-or-nothing di langkah 0-3), begitu langkah 0-3 kelar, loop (langkah 4-5)
bisa incremental per-misi. Prioritas misi Los Santos: SWEET2/3/4, SMOKE2, RYDER2, DRUGS1, DRUGS3, DRUGS4,
HOODS5, CRASH4, TWAR7, MUSIC5, LA1FIN2.

## RISIKO
- Untestable runtime oleh Claude → wajib playtest user tiap tahap.
- Arity mismatch = korup var (silent, susah didiagnosa). Compile-check nangkep mismatch SCM↔def, TAPI
  TIDAK nangkep mismatch def↔C++ (StoreParameters) — itu cuma ketauan runtime. HATI-HATI.
- Banyak misi SP cutscene/objektif diracik ≤4 orang; 8 orang bisa aneh walau gak crash.

## REKOMENDASI SEQUENCING (Claude)
Beresin dulu bug misi dasar (SWEET4 crash ped-death, validasi pola SWEET1 di convoy LS) + playtest,
BARU kerjain refactor 8-slot sbg effort terpisah (mulai dari langkah 0 blocker). Refactor gede di atas
misi yg belum stabil = numpuk risiko.
