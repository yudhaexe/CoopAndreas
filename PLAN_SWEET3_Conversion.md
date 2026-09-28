# PLAN & GUIDE — Setup + Konversi Misi SWEET3 (Drive-Thru)

> **Fungsi file ini:** working memory buat sesi AI (Claude) + dev. Semua isi di sini
> **sudah diverifikasi langsung dari mesin/repo** pada sesi 2026-09-28/29. Jangan
> tambahin klaim yang belum dicek. Kalau ragu → verifikasi ulang, jangan nebak.
>
> Baca juga: `HANDOVER_CoopAndreas.md` (analisa repo global).

---

## 0. ATURAN ANTI-HALU (buat Claude, baca tiap sesi)
1. **Jangan klaim udah build/compile/test** kalau belum ada output nyata. Claude
   TIDAK bisa jalanin Sanny Builder, xmake build butuh konfirmasi, dan game/playtest
   100% di manusia.
2. **Alamat memory, opcode param, nama file → verifikasi dulu** sebelum ngedit. Salah
   param opcode / label bentrok / index var tabrakan baru ketauan pas build & run.
3. **Nama exe itu `gta_sa.exe` (UNDERSCORE)**, bukan `gta-sa.exe`. Launcher nolak yang strip.
4. Kalau in-game crash/desync, laporannya dari **manusia** — Claude nunggu observasi, bukan ngarang.
5. Update tabel status di file ini tiap ada progress nyata.

---

## 1. STATUS SETUP (per 2026-09-29)

| Item | Status | Detail terverifikasi |
|---|---|---|
| GTA `gta_sa.exe` v1.0 US HOODLUM | ✅ Beres | `C:\Games\GTA - The Original Trilogy\GTASA\gta_sa.exe`, **14.383.616 bytes**, scan bersih. SHA256 `a5112e9c...18d7521` |
| Backup exe repack lama | ✅ Ada | `gta-sa.exe` (hash beda: `f01a00ce...343ac`) masih di folder — bisa diabaikan |
| Sanny Builder 4.2.0 (portable) | ✅ Beres | `C:\Users\PC\Downloads\SannyBuilder-v4.2.0\sanny.exe` |
| Opcode CoopAndreas di Sanny | ✅ Beres | Sudah dicopy ke `...\SannyBuilder-v4.2.0\data\sa_sbl_coopandreas\` |
| VS2022/2026 Desktop C++ | ⏳ Lagi install | Target: `C:\Program Files\Microsoft Visual Studio\18\Community`. **Workload BENAR = "Desktop development with C++"** (BUKAN "Game development with C++") |
| xmake | ❌ Belum | `winget install xmake` (winget tersedia di mesin) |
| Binary CoopAndreas (dll/exe) | ❌ Belum | **Build sendiri** via xmake (repo = source only, tidak ada binary jadi) |
| `GTA_SA_DIR` env var | ❌ Belum di-set | Opsional; kalau di-set, `xmake --build client` auto-copy dll ke game |

### Path penting (hafalkan, jangan nebak)
```
Repo         : C:\Users\PC\Documents\GitHub\CoopAndreas
Game         : C:\Games\GTA - The Original Trilogy\GTASA
Sanny        : C:\Users\PC\Downloads\SannyBuilder-v4.2.0
```

---

## 2. TOOLCHAIN — sisa langkah

1. **VS2022/2026** → tab **Workloads** → centang **"Desktop development with C++"** → Install.
2. **xmake** → di PowerShell: `winget install xmake` → tutup-buka terminal.
3. (opsional) set env: `setx GTA_SA_DIR "C:\Games\GTA - The Original Trilogy\GTASA"` → tutup-buka terminal.

---

## 3. BUILD (setelah VS + xmake siap)

Dari root repo (`xmake.lua` ada di situ). Semua target **x86 + MSVC + Windows**.
```
xmake --build client      # -> CoopAndreasSA.dll  (dll utama yg di-inject)
xmake --build server      # -> server.exe
xmake --build proxy       # -> output di-rename jadi eax.dll (loader inject)
xmake --build launcher    # -> LaunchCoopAndreas.exe
```
> Kalau xmake minta pilih toolchain/versi VS, arahkan ke MSVC yg barusan diinstall.
> Claude bantu baca & benerin error compile.

### Pasang ke game (inject setup)
Di folder game:
1. `eax.dll` bawaan repack → rename jadi `eax_orig.dll`.
2. `eax.dll` hasil build proxy → taro di folder game.
3. `CoopAndreasSA.dll`, `launcher`, dll → sesuai instruksi README (taro di folder game / subfolder `CoopAndreas`).

---

## 4. COMPILE SCM (WAJIB tiap ubah misi)

### 4a. CLI (dipakai Claude buat auto-verify — TERVALIDASI 2026-09-29)
Sanny Builder punya CLI, jadi Claude bisa compile sendiri tanpa GUI:
```bash
SANNY="/c/Users/PC/Downloads/SannyBuilder-v4.2.0/sanny.exe"
LOG="/c/Users/PC/Downloads/SannyBuilder-v4.2.0/compile.log"
rm -f "$LOG"                       # WAJIB hapus dulu — deterministik
"$SANNY" --compile "C:\\...\\scm\\main.txt" "C:\\...\\scm\\main.scm" --mode sa_sbl_coopandreas --no-splash
# CEK: kalau $LOG ada & isinya "error:" -> GAGAL (baca error, benerin). Kalau gak ada -> SUKSES.
```
- **Exit code SELALU 0** — JANGAN andelin exit code. Patokan = ada/tidaknya `compile.log` berisi `error:`.
- Format error: `error: <file>:<line> <pesan>` (mis. `Unknown directive`, dst).
- Mode id (dari mode.xml): `sa_sbl_coopandreas` (title "GTA SA (v1.0 - CoopAndreas)").
- Output `main.scm` ~3MB kalau sukses penuh.
- Konsekuensi: Claude bisa jamin tiap misi `COMPILES` sebelum dikirim. Sisa buat dev = playtest in-game.

### 4b. GUI (buat dev, opsional)
1. Buka `sanny.exe` → Edit Mode = `sa_sbl_coopandreas` → buka `scm\main.txt` → Compile (F6).
2. Output (`main.scm`) → copy ke `${GTA_SA_DIR}\CoopAndreas\`.

---

## 5. ★ KERJAAN INTI: KONVERSI SWEET3 (Drive-Thru) ★

### 5.1 Fakta target (terverifikasi)
- File: `scm/scripts/SWEET3.txt` — **2730 baris**, saat ini **2 Coop calls** (cuma warning stub di baris 6–7).
- Didaftarin: `scm/main.txt:408` → `DEFINE MISSION 15 AT @SWEET3  // Drive-Thru`
- Include: `scm/main.txt:727` → `{$INCLUDE scripts/SWEET3.txt}`
- Label entry: `:SWEET3` (baris 4).
- Ada banyak sub-label (`:SWEET3_38`, `_47`, `_175`, ... `_2213`, dst) — struktur gosub/switch khas SCM.

### 5.2 Template acuan (terverifikasi)
`scm/scripts/SWEET1.txt` (Tagging Up Turf) — 3314 baris, **103 Coop calls**. Idiom kunci:
```
:SWEET1
script_name 'SWEET1'
Coop.EnableSyncingThisScript()          // <- baris ~6, tandain thread buat di-sync
...
// di init misi (SWEET1_468, sblm $onmission = 1):
$NETWORK_PLAYER[0], $NETWORK_PLAYER[1], $NETWORK_PLAYER[2] = Coop.CollectNetworkPlayersForTheMission()
...
// map actor/vehicle misi ke network id biar sync:
$temp_int = Coop.GetVehicleNetworkId($sweet_car)
$temp_int = Coop.GetPedNetworkId($big_smoke)
...
// tiap aksi yg nyentuh player → loop + guard:
for $temp_int = 0 to 2
    if Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[$temp_int])
    then
        // aksi ke $NETWORK_PLAYER[$temp_int]
    end
end
```
Variabel global udah disiapin di `main.txt`:
`$NETWORK_PLAYER: array 3`, `$NETWORK_PLAYER_VEHICLE: array 3`, `$temp_int`.

### 5.3 Coop API yang boleh dipakai (dari repo — jangan ngarang method baru)
```
Coop.IsNetworkPlayerActorValid(handle)         // guard slot player (paling sering)
Coop.EnableSyncingThisScript()
Coop.CollectNetworkPlayersForTheMission()      // isi $NETWORK_PLAYER[0..2]
Coop.IsHost()
Coop.GetPedNetworkId(h) / GetVehicleNetworkId(h)
Coop.GetNetworkPlayerInternalId(h)
Coop.PrintNowForNetworkPlayer(...) / PrintHelpForNetworkPlayer(...) / ClearThisPrintForNetworkPlayer(...)
Coop.AddChatMessage(...)
Coop.UpdateCheckpointForNetworkPlayer(...) / RemoveCheckpointForNetworkPlayer(...)
Coop.UpdateCarBlipForNetworkPlayer(...) / RemoveCarBlipForNetworkPlayer(...)
Coop.UpdateCharBlipForNetworkPlayer(...) / RemoveCharBlipForNetworkPlayer(...)
Coop.ClearAllEntityBlipsForNetworkPlayer(...)
Coop.TeleportPlayersToHostSafely(...)
Coop.LocateAllPlayersOnFoot(...)
Coop.IsSyncingThisPed(h)
Coop.ClaimPedOnRelease(...) / CancelPedClaim(...) / PedResetAllClaims() / PedTakeHost(...)
Coop.GetPedInAreaWithModel(...)
```
Definisi resmi: `sdk/Sanny Builder 4/data/sa_sbl_coopandreas/opcodes.txt`.
> Kalau butuh method yg gak ada di list → CEK opcodes.txt dulu, jangan asumsi ada.

### 5.4 Langkah konversi SWEET3 (rencana)
1. **Baca penuh** `SWEET3.txt` — petakan: init misi, spawn actor/vehicle, objective, teks/blip/checkpoint, kondisi menang/kalah.
2. Tambah `Coop.EnableSyncingThisScript()` di awal thread `:SWEET3`.
3. Di init misi, panggil `Coop.CollectNetworkPlayersForTheMission()` isi `$NETWORK_PLAYER[0..2]`.
4. Map actor & vehicle misi ke network id (`GetPedNetworkId`/`GetVehicleNetworkId`).
5. Wrap tiap aksi yg nyentuh player pakai loop `for 0 to 2` + guard `IsNetworkPlayerActorValid`.
6. Blip/checkpoint/teks → pakai varian `...ForNetworkPlayer`.
7. **Tentukan semantik objective** (lihat DECISION di §7) sebelum ubah kondisi menang/kalah.
8. Hapus/matikan 2 baris warning stub (baris 6–7) setelah misi beneran dikonversi.

### 5.5 Definisi "selesai" 1 misi
Draft SCM ≠ selesai. Selesai = **compile sukses di Sanny + jalan 2 client tanpa desync/crash
di jalur happy-path + objective bener**. Itu butuh playtest manusia.

---

## 6. LOOP KERJA
```
Claude draft SCM  →  dev compile di Sanny  →  dev run server + 2 client
      ↑                                                    │
      └─────────  dev kasih log / observasi in-game  ←─────┘
```

---

## 7. DECISIONS yang perlu diputusin dev (Claude JANGAN asal pilih)
- [ ] **Semantik objective Drive-Thru**: misi dianggap sukses kalau HOST nyampe, SEMUA player
      nyampe, atau SALAH SATU? (default GTA: solo player). Ini nentuin logic menang/kalah.
- [ ] **Kendaraan**: tiap player dikasih kendaraan sendiri, atau numpang 1 mobil (host nyetir)?
- [ ] **Fail condition**: kalau 1 player mati, misi gagal buat semua atau cuma respawn?

---

## 8. LOG PROGRESS (append tiap sesi)
- **2026-09-28/29** — Setup: game exe v1.0 US terpasang & bersih; Sanny + opcode Coop siap;
  VS Desktop C++ lagi install; xmake belum. Belum ada baris SWEET3 yg dikonversi. File plan ini dibuat.

---

## 9. QUICK CHECK COMMANDS
```bash
# kedalaman konversi tiap misi
for f in $(grep -rl "Coop\." scm/scripts/); do echo "$(grep -c 'Coop\.' "$f") $(basename $f)"; done | sort -rn

# semua method Coop yg dipakai
grep -rhoE "Coop\.[A-Za-z]+" scm/scripts/*.txt | sort | uniq -c | sort -rn

# verifikasi exe game
cd "/c/Games/GTA - The Original Trilogy/GTASA" && sha256sum gta_sa.exe && stat -c%s gta_sa.exe
```
