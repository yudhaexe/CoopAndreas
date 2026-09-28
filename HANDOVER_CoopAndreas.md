# HANDOVER — CoopAndreas (fokus: konversi misi ke co-op)

> Dokumen ini nge-capture hasil analisa repo `Tornamic/CoopAndreas` supaya bisa langsung
> lanjut kerja (atau lanjut sesi AI baru) tanpa mulai dari nol. Ditulis buat konteks
> **konversi misi single-player GTA:SA jadi co-op**.
>
> Repo: https://github.com/Tornamic/CoopAndreas · Versi saat dianalisa: `0.3.0-alpha`

---

## 1. Ringkasan proyek

CoopAndreas = mod co-op multiplayer buat GTA: San Andreas. Bukan game baru — dia
**nge-hook engine GTA:SA (closed-source) lewat memory patching** + sinkronisasi
antar-client pakai ENet (UDP). Bahasa: C++ (client DLL + server binary), plus
scripting misi di **Sanny Builder** (dialek SCM).

Dev utama praktis solo: Kirill Tymoshchenko (`@tornamic`).

---

## 2. Fakta teknis kunci (udah diverifikasi dari source)

| Item | Nilai | Lokasi |
|---|---|---|
| **Max player (freeroam)** | **8** (hardcoded) | `shared/config.h` → `MAX_SERVER_PLAYERS = 8` |
| **Max player (dalam misi)** | **4** (host + 3) | `Coop.CollectNetworkPlayersForTheMission()` → `$NETWORK_PLAYER: array 3` |
| **Default port** | **6767** (UDP) | `shared/config.h` → `DEFAULT_PORT = 6767` |
| **Transport** | ENet (UDP), bind `ENET_HOST_ANY` | `server/src/...` |
| **Config maxplayers** | Reader-nya ada, TAPI belum kepakai | `CNetwork.cpp` ada `// TODO: GetConfigMaxPlayers`; `enet_host_create` masih pakai konstanta 8 |

**Catatan penting:** naikin `maxplayers` di config server **nggak ngefek** — enet host
& serialisasi playerid dua-duanya nge-clamp ke `MAX_SERVER_PLAYERS` (8). Mau lebih dari
8 = harus ubah konstanta + audit semua serialisasi packet.

### Port forwarding (buat main jarak jauh)
- Satu PC → `127.0.0.1:6767`, nggak perlu apa-apa.
- Satu LAN → IP lokal host, nggak perlu forwarding.
- Beda jaringan/internet → **perlu** forward **UDP 6767**, ATAU pakai VPN virtual-LAN
  (ZeroTier/Radmin/Hamachi — paling anti-ribet), ATAU VPS public IP.

### Kenapa cuma 1 player bisa di interior (limitasi engine)
GTA:SA cuma render **SATU "area/interior" aktif secara global** — semua interior numpuk
di koordinat map yang sama, dipisah pakai area ID, sisanya di-cull. Opcode
`0x04BB set_area_visible` yang ngontrol, dan mod ini **nge-sync opcode itu ke semua
client** (`client/src/COpCodeSync.cpp`, bagian `// Interiors`). Jadi "area yang
kelihatan" jadi state bersama → nggak bisa render player A di dalam toko + player B di
luar barengan. Fix sebenarnya = **per-player world instancing / interior streaming**,
masih di TODO (`stream in/out players, peds, vehicles`) dan belum dikerjain. Ini butuh
RE binary (IDA/Ghidra), **bukan** kerjaan scripting.

---

## 3. Struktur repo

```
CoopAndreas/
├── client/        # DLL yang di-inject ke gta_sa.exe (C++, x86)
│   └── src/
│       ├── Hooks/            # hook ke fungsi engine
│       ├── PacketHandlers/   # handler packet dari server
│       ├── COpCodeSync.cpp   # <-- daftar opcode SCM yang di-sync (termasuk interior)
│       ├── CPatch.cpp        # <-- memory patch (alamat hardcoded, mis. patch::Nop(0x...))
│       └── game_sa/          # wrapper struktur game
├── server/        # server binary (C++, x86) — accept koneksi, relay state
│   └── src/ConfigManager.*   # baca port/maxplayers dari config .ini
├── proxy/         # eax.dll proxy buat inject DLL utama ke game
├── launcher/      # LaunchCoopAndreas.exe — start game + pass cmd args
├── shared/
│   └── config.h              # <-- KONSTANTA (max player, port, versi)
├── scm/           # ====== INI AREA KERJA KONVERSI MISI ======
│   ├── main.txt              # entry SCM: DEFINE MISSION + {$INCLUDE scripts/*.txt}
│   ├── scripts/              # 303 file .txt — body tiap thread/misi
│   └── .vscode/
├── sdk/
│   └── Sanny Builder 4/data/sa_sbl_coopandreas/
│       └── opcodes.txt       # definisi opcode custom CoopAndreas buat Sanny Builder
├── third_party/   # imgui, enet, INIReader, discordrpc, plugin-sdk
└── xmake.lua      # build config (semua target x86/msvc/windows)
```

**Build targets (xmake):** `client` (shared DLL), `server` (binary), `proxy` (shared DLL),
`launcher`, `plugin_sa` (static). Semua **x86 + MSVC + Windows**.

---

## 4. Build & run (Windows)

```bash
# prasyarat: Visual Studio 2022 (C++ package) + xmake
xmake --build client      # -> CoopAndreasSA.dll (auto-copy ke game kalau $GTA_SA_DIR di-set)
xmake --build server      # -> server.exe
xmake --build proxy       # -> rename output jadi eax.dll, taro di folder game
xmake --build launcher    # -> LaunchCoopAndreas.exe
```

Setup inject: di folder game, rename `eax.dll` bawaan → `eax_orig.dll`, taro `eax.dll`
hasil build proxy.

**Compile SCM (WAJIB tiap ubah misi):**
1. Install Sanny Builder 4.
2. Copy semua isi `sdk/Sanny Builder 4/` ke folder instalasi Sanny Builder (nambahin
   opcode CoopAndreas ke compiler).
3. Buka `scm/main.txt` di Sanny Builder, compile, copy output (`main.scm`, dll) ke
   `${GTA_SA_DIR}/CoopAndreas/`.

**Run:** jalanin `server.exe` dulu → `LaunchCoopAndreas.exe` → isi serial → Launch.
Client connect ke IP server (`127.0.0.1` kalau lokal).

> Build server di Linux: README bilang "TODO" — belum didukung resmi. Sandbox non-Windows
> nggak bisa build/test end-to-end.

---

## 5. ★ Sistem konversi misi (bagian inti) ★

### 5.1 Cara kerja
Tiap misi = satu thread SCM di `scm/scripts/<NAMA>.txt`, di-`{$INCLUDE}` dari `main.txt`
dan didaftarin lewat `DEFINE MISSION <n> AT @<LABEL>`. Ada **135 slot misi** total
(termasuk minigame/race/sekolah); story mission ≈ 100.

Konversi = ambil misi single-player, terus **tiap aksi yang nyentuh player** (spawn,
posisi, kasih kendaraan, cek objective, blip, teks, checkpoint) di-wrap jadi loop atas
`$NETWORK_PLAYER[0..2]` dengan guard `Coop.IsNetworkPlayerActorValid(...)`.

Ini scripting level-tinggi yang readable — **BUKAN reverse-engineering binary**. Itu
kabar baik: polanya bisa ditiru dari misi yang udah kelar.

### 5.2 Coop API (Sanny Builder — dipakai di file misi)
Method yang udah kepakai di repo (frekuensi = seberapa sering muncul):

```
Coop.IsNetworkPlayerActorValid(handle)              # guard: slot player valid?  (paling sering)
Coop.EnableSyncingThisScript()                      # tandain thread ini buat di-sync
Coop.CollectNetworkPlayersForTheMission()           # -> isi $NETWORK_PLAYER[0..2]
Coop.IsHost()                                        # cabang logic khusus host
Coop.GetPedNetworkId(handle) / GetVehicleNetworkId(handle)
Coop.GetNetworkPlayerInternalId(handle)
Coop.PrintNowForNetworkPlayer(...)                  # teks per-player
Coop.PrintHelpForNetworkPlayer(...) / ClearThisPrintForNetworkPlayer(...)
Coop.AddChatMessage(...)
Coop.UpdateCheckpointForNetworkPlayer(...) / RemoveCheckpointForNetworkPlayer(...)
Coop.UpdateCarBlipForNetworkPlayer(...) / RemoveCarBlipForNetworkPlayer(...)
Coop.UpdateCharBlipForNetworkPlayer(...) / RemoveCharBlipForNetworkPlayer(...)
Coop.ClearAllEntityBlipsForNetworkPlayer(...)
Coop.TeleportPlayersToHostSafely(...)
Coop.LocateAllPlayersOnFoot(...)
Coop.IsSyncingThisPed(handle)
Coop.ClaimPedOnRelease(...) / CancelPedClaim(...) / PedResetAllClaims() / PedTakeHost(...)
Coop.GetPedInAreaWithModel(...)
```

Variabel global yang disiapin di `main.txt`:
```
$NETWORK_PLAYER: array 3 of integer
$NETWORK_PLAYER_VEHICLE: array 3 of integer
$temp_int: integer
```

### 5.3 Pola konversi (template dari misi yang udah jadi)
Contoh idiom dari `INTRO1.txt` (Big Smoke) — dipakai berulang:

```
// di awal thread
Coop.EnableSyncingThisScript()
...
$NETWORK_PLAYER[0], $NETWORK_PLAYER[1], $NETWORK_PLAYER[2] = Coop.CollectNetworkPlayersForTheMission()

// tiap kali aksi ke player, loop + guard:
for $temp_int = 0 to 2
    if Coop.IsNetworkPlayerActorValid($NETWORK_PLAYER[$temp_int])
    then
        Char.SetCoordinates($NETWORK_PLAYER[$temp_int], x, y, z)
        Char.ClearTasksImmediately($NETWORK_PLAYER[$temp_int])
        // ...dst
    end
end

// mission actor (mis. Big Smoke) di-map ke network id biar sync:
86@ = Coop.GetPedNetworkId($big_smoke)
```

### 5.4 Status konversi saat ini (diverifikasi dari `grep -c "Coop\."`)

| Status | Misi | Jumlah Coop calls |
|---|---|---|
| ✅ **Fully converted** | Tagging Up Turf (`SWEET1`) | 103 |
| ✅ | Cleaning The Hood (`SWEET1B`) | 88 |
| ✅ | Ryder (`INTRO2`) | 59 |
| ✅ | Big Smoke (`INTRO1`) | 58 |
| 🟡 **Stub / baru mulai** | `SWEET2/3/4/6`, `RYDER2/3`, `SMOKE2/3`, `JFUD`, `TATTO`, `PSHOP`, `BARB` | 2–8 (cuma scaffold, isi masih vanilla) |
| ⬜ **Belum disentuh** | ~80+ story mission sisanya | 0 |

**Unit kerja 1 misi "beneran" ≈ 50–100 penyisipan `Coop.*` di ~3.000–4.000 baris SCM
yang diedit tangan.** (Big Smoke: 4060 baris, 58 calls.)

---

## 6. Penilaian: seberapa jauh AI (Claude) bisa bantu

**Bisa dibantu (pede tinggi):**
- Draft konversi misi tipe "roti-mentega" (pergi ke titik → tembak → balik) — mayoritas
  misi Grove Street awal. Ada 4 template konkret buat ditiru. Draft pertama solid ~80%.
- Ngisi fitur sync kecil yang masih TODO (money sync, dll) — mekanikal, ada contoh serupa.
- Refactor, code review, rancang arsitektur, baca stack trace/log buat cari bug logic.

**Mentok (butuh keputusan/eksekusi manusia):**
- **Interior instancing / streaming** — RE binary, butuh IDA/Ghidra + debugger + observasi
  crash live. AI cuma bisa bantu SETELAH address & perilakunya lo temuin.
- **Misi mekanik-khusus** (terbang: N.O.E./Stowaway/Vertical Bird/Learning to Fly/Freefall;
  RC: Beefy Baron/Supply Lines/Air Raid/New Model Army; timed/solo). Masalahnya **desain**
  ("4 player ngapain di misi RC solo?"), bukan sintaks. Lo putusin desain → AI implementasi.
- **Semantik objective** ("selesai kalau SEMUA / SALAH SATU / host nyampe X?") — keputusan
  per-misi; salah = ke-compile tapi logic rusak.

**Kenapa "SEMUA misi, tested & working" nggak bisa dijamin AI sendirian:**
1. AI nulis buta — nggak bisa compile Sanny Builder, nggak bisa jalanin game. Salah param
   opcode / label bentrok / index var tabrakan baru ketauan pas build & run.
2. **Bottleneck-nya testing, bukan drafting.** Tiap misi wajib playtest 2–4 client bareng
   buat nangkep desync/race condition. Nggak bisa diotomasi → 100% di manusia.
3. Volume + compounding error: ~100 misi × ribuan baris. Pede 80%/misi ≠ semua jalan.

**Framing realistis:** AI = akselerator drafting (mungkin 3–5x throughput). Desain misi
khusus, build, dan loop test-fix tetap di dev.

---

## 7. Rekomendasi langkah berikutnya

**Pilot: konversi 1 misi roti-mentega buat ngukur loop nyata.**
Kandidat: **Drive-Thru (`scm/scripts/SWEET3.txt`)** — udah di-stub (2 Coop calls), tinggal
diisi; secara story urutannya pas setelah misi yang udah kelar; mekaniknya simpel
(nyetir + makan + balapan ringan). Alternatif lain yang lurus: Nines And AK's (`SWEET2`),
Drive-By (`SWEET4`).

**Loop kerja yang realistis:**
```
AI draft konversi  →  lo compile di Sanny Builder  →  lo run server + 2 client
     ↑                                                          │
     └──────────  lo kasih log / hasil observasi in-game  ←─────┘
```

Dari 1 pilot ini kita dapet angka: berapa lama 1 siklus draft→build→test→fix. Angka itu
yang nentuin apakah "semua misi" realistis dikejar — jauh lebih akurat daripada tebakan.

---

## 8. Quick reference — file yang sering disentuh

| Butuh | File |
|---|---|
| Ubah konstanta (max player/port) | `shared/config.h` |
| Liat opcode apa aja yang di-sync | `client/src/COpCodeSync.cpp` |
| Memory patch / alamat hardcoded | `client/src/CPatch.cpp` |
| Config server (port/maxplayers) | `server/src/ConfigManager.cpp` + `.h` |
| Daftar & include misi | `scm/main.txt` (`DEFINE MISSION`, `{$INCLUDE}`) |
| Body misi | `scm/scripts/<NAMA>.txt` |
| Definisi Coop opcode (compiler) | `sdk/Sanny Builder 4/data/sa_sbl_coopandreas/opcodes.txt` |
| Template konversi terbaik | `scm/scripts/SWEET1.txt` (103 calls), `INTRO1.txt` (58) |

**Command cek cepat:**
```bash
# misi mana yang udah dikonversi & sedalam apa
for f in $(grep -rl "Coop\." scm/scripts/); do echo "$(grep -c 'Coop\.' "$f") $(basename $f)"; done | sort -rn

# semua method Coop API yang dipakai
grep -rhoE "Coop\.[A-Za-z]+" scm/scripts/*.txt | sort | uniq -c | sort -rn
```
