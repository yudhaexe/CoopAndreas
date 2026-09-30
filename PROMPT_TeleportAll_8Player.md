# PROMPT AGEN — Opcode baru `Coop.TeleportAllPlayersToHostSafely()` (dukung 8 pemain)

## PERAN
Kamu engineer C++ CoopAndreas. Tugas: bikin opcode BARU tanpa-parameter yang men-teleport **semua**
network-player misi ke dekat host secara aman — menghapus batas 3-slot pada opcode lama. Ini kerja
**engine (C++ + definisi Sanny)**, BUKAN konversi misi.

## 🧨 ANTI-HALUSINASI (baca dulu, ini paling sering gagal)
- **Kutip, jangan mengingat.** Sebelum nulis, buka file rujukan di bawah dan TIRU pola persisnya. Semua
  fakta di prompt ini sudah diverifikasi dgn `file:line` — kalau ada yang tak cocok saat kamu buka,
  BERHENTI dan lapor, jangan karang.
- **Jangan invent API/opcode/enum.** Opcode baru = **0x1D1D** (sudah dicek bebas; lihat Ground Truth #1).
  Kalau ternyata sudah dipakai, lapor — jangan pilih angka lain diam-diam.
- **COMPILES ≠ TESTED.** Status maksimal yang boleh kamu klaim = **COMPILES** (xmake sukses + link DLL).
  Teleport 8 pemain hanya bisa dibuktikan playtest 8 client (manusia). Jangan tulis "works/tested".
- **Jangan sentuh opcode lama 0x1D1C.** Ia masih dipakai ~55 call-site misi 4-pemain. Biarkan utuh.

## GROUND TRUTH (terverifikasi — buka & konfirmasi sebelum pakai)
1. **Opcode bebas = 0x1D1D.** Registrar terakhir = `0x1D1C` di
   `client/src/Commands/CCustomCommandRegistrar.h:67`. Tambah baris baru SETELAHNYA.
2. **Batas lama ada di** `client/src/Commands/Commands/CCommandTeleportPlayersToHostSafely.cpp`:
   `MAX_TELEPORT_SLOTS = 3` (baris 6), `CollectParameters(3)` (baris 40). **Algoritma
   `CalculateSafePositions` (baris 9–36) sudah generik** terhadap `playerCount` — itu yang kita pakai ulang.
3. **Kapasitas real** `shared/config.h:9` → `MAX_SERVER_PLAYERS = 8` (host + 7 follower). Jadi slot
   teleport maksimal = **7**.
4. **Pola ambil semua network-player** ada di
   `client/src/Commands/Commands/CCommandCollectNetworkPlayersForTheMission.cpp` —
   `for (auto networkPlayer : CNetworkPlayerManager::m_pPlayers) { if (auto player = networkPlayer->m_pPed) ... }`
   dengan `constexpr int MISSION_NETWORK_PLAYERS = 7;`. Tiru cara iterasinya.
5. **Pola paramless command**: header `.h` = `CCommandCollectNetworkPlayersForTheMission.h` (4 baris:
   `#pragma once`, include `../CCustomCommand.h`, class extends `CCustomCommand`, override `Process`).
6. **Definisi Sanny paramless**:
   - `sdk/Sanny Builder 4/data/sa_sbl_coopandreas/sa_coop.db:3396` →
     `EnableSyncingThisScript,1D12,0,0,()`
   - `sdk/Sanny Builder 4/data/sa_sbl_coopandreas/sa_coop.json` blok id `1D12` (num_params 0, tanpa
     input/output). Lihat juga blok `1D1C` (TeleportPlayersToHostSafely) sbg tetangga urutan.

## LANGKAH (urut)
1. **Buat header** `client/src/Commands/Commands/CCommandTeleportAllPlayersToHostSafely.h`
   — tiru persis pola Ground Truth #5, ganti nama kelas jadi `CCommandTeleportAllPlayersToHostSafely`.

2. **Buat** `client/src/Commands/Commands/CCommandTeleportAllPlayersToHostSafely.cpp`:
   - `Process(CRunningScript* script)` — **TANPA** `CollectParameters` (0 param).
   - `assert(CLocalPlayer::m_bIsHost);` (samakan dgn command lama, baris 42).
   - Kumpulkan network-player valid dari `CNetworkPlayerManager::m_pPlayers` (pola Ground Truth #4)
     ke array lokal `CNetworkPlayer* networkPlayers[7]`, hitung `playerCount` (maks 7 = `MAX_SERVER_PLAYERS-1`).
   - Untuk posisi aman: **pakai ulang** logika `CalculateSafePositions` dari command lama (Ground Truth #2).
     Karena helper itu `static` (file-local), duplikasi ke `.cpp` baru ini dgn `MAX_TELEPORT_SLOTS`
     dinaikkan ke `7` — JANGAN ubah file lama. (Boleh juga refactor helper ke util bersama, tapi kalau
     ragu, duplikasi lebih aman & tak menyentuh command 3-slot yang sudah teruji.)
   - Kirim `Packets::Scripts::TeleportPlayerScripted` per pemain (pola command lama baris 74–78:
     set `playerid`, `pos`, `heading = FindPlayerPed(0)->m_fCurrentRotation`, lalu `GetPacketFactory().Send`).
   - Fallback posisi (0,0,0) → `FindPlayerCoors(0)` (pola command lama baris 70–73).
   - Include yang sama seperti command lama: `<CGeneral.h>`, `<CPedPlacement.h>` (untuk `FindZCoorForPed`).

3. **Daftarkan** di `client/src/Commands/CCustomCommandRegistrar.h`:
   - `#include "Commands/CCommandTeleportAllPlayersToHostSafely.h"` (di grup include, dekat baris 32).
   - `CCustomCommandMgr::RegisterCommand(0x1D1D, new CCommandTeleportAllPlayersToHostSafely());`
     tepat setelah baris 67 (`0x1D1C`).

4. **Definisi Sanny** — WAJIB dua file konsisten:
   - `sa_coop.db`: tambah baris
     `TeleportAllPlayersToHostSafely,1D1D,0,0,()` (setelah baris 3406, tiru format `EnableSyncingThisScript`).
   - `sa_coop.json`: tambah objek opcode di array command class Coop (setelah blok `1D1C`), tiru blok
     paramless `1D12`:
     ```json
     {
         "id": "1D1D",
         "name": "coop_teleport_all_players_to_host_safely",
         "num_params": 0,
         "class": "Coop",
         "member": "TeleportAllPlayersToHostSafely",
         "attrs": { "is_static": true },
         "short_desc": "teleport all mission network players safely around the host"
     }
     ```
     Jaga koma/kurung JSON valid.

## VERIFIKASI (WAJIB — urut)
1. **Self-audit**: pastikan tidak ada perubahan di `CCommandTeleportPlayersToHostSafely.cpp` (0x1D1C harus
   utuh). Pastikan `0x1D1D` unik di registrar. Pastikan `.db` & `.json` cocok (opcode, member, num_params).
2. **Compile client (xmake)** — bukti empiris, jangan asumsi:
   ```bash
   export PATH="$PATH:/c/Program Files/xmake"
   cd /c/Users/PC/Documents/GitHub/CoopAndreas
   xmake build -y client
   ```
   Sukses = `build ok` + `CoopAndreasSA.dll` ter-link. Kalau error compile → benerin, ulang sampai bersih.
3. (Opsional) validasi signature opcode baru dikenal Sanny — compile SCM headless. ⚠️ **WAJIB `--game sa`;
   JANGAN `--no-splash`** (bukan flag valid → GUI menggantung, exit 1, main.scm tak ter-regenerate):
   ```bash
   SANNY="/c/Users/PC/Downloads/SannyBuilder-v4.2.0/sanny.exe"
   SCM="C:\\Users\\PC\\Documents\\GitHub\\CoopAndreas\\scm\\main.scm"
   rm -f "/c/Users/PC/Downloads/SannyBuilder-v4.2.0/compile.log"; ls -la "$SCM"
   "$SANNY" --compile "C:\\Users\\PC\\Documents\\GitHub\\CoopAndreas\\scm\\main.txt" "$SCM" --game sa --mode sa_sbl_coopandreas
   ls -la "$SCM"   # timestamp maju + compile.log tak berisi "error:" = signature dikenal. HANYA cek signature, bukan bukti jalan.
   ```

## LAPORAN (jujur)
- Daftar file dibuat/diubah + `file:line` tiap perubahan kunci.
- Hasil `xmake build` apa adanya (tempel baris sukses/gagal).
- Status = **COMPILES** (bukan TESTED). Sebut eksplisit: teleport 8-pemain belum diplaytest.
- Catatan pemakaian: misi 8-pemain baru cukup panggil `Coop.TeleportAllPlayersToHostSafely()` (0 arg)
  menggantikan `Coop.TeleportPlayersToHostSafely($NETWORK_PLAYER[0..2])`. Opcode lama tetap valid untuk
  misi lama; TIDAK perlu migrasi massal.
- Kalau nemu ketidakcocokan dgn Ground Truth di prompt ini → FLAG, jangan lanjut diam-diam.

## JANGAN
- ❌ Ubah/refactor opcode lama 0x1D1C.
- ❌ Pilih opcode selain 0x1D1D tanpa lapor alasan.
- ❌ Klaim "tested/works". Maks COMPILES.
- ❌ Karang nama packet/method/enum. Kalau tak ada di rujukan, cari di source dulu; tak ketemu = FLAG.
