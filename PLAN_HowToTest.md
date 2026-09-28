# CARA TEST CoopAndreas — 2 Laptop, Lompat Misi, & Ambil Log

> Panduan praktis buat playtest hasil konversi. Fakta di sini diverifikasi dari source
> (bukan nebak). Terkait: `PLAN_CoopAndreas_TestLog.md` (tempat lapor crash),
> `PLAN_AllMissions_Campaign.md` (daftar misi ter-konversi).

---

## 1. SETUP 2 LAPTOP

### Yang harus ada di TIAP laptop
Folder game lengkap + file mod (sama kayak yang udah dipasang):
`eax.dll` (proxy coop), `eax_orig.dll`, `CoopAndreasSA.dll`, `LaunchCoopAndreas.exe` (+`.manifest`),
`CoopAndreas\main.scm`, `gta_sa.exe` v1.0 US. **Cara paling gampang: copy seluruh folder game** dari
laptop 1 ke laptop 2 (flashdisk/share), biar versi & mod persis sama. Kalau `main.scm` beda → desync/crash.

### Siapa jalanin server
Cukup **SATU** laptop yang jalanin `server.exe` (jadi "host machine"). Dua-duanya jalanin
`LaunchCoopAndreas.exe` dan connect ke IP server yang sama.

### Skenario jaringan (pilih satu)
| Situasi | IP yang dipakai client | Perlu apa |
|---|---|---|
| **Satu WiFi/LAN yang sama** (paling gampang) | IP lokal laptop-server, mis. `192.168.1.x` | Cek IP server: buka `cmd` → `ipconfig` → "IPv4 Address". Firewall: izinkan `server.exe` (UDP 6767). |
| **Beda jaringan / internet** | IP virtual dari VPN | Install **ZeroTier / Radmin VPN / Hamachi** di dua laptop → join network sama → pakai IP virtual (mis. ZeroTier `25.x.x.x`). Paling anti-ribet, gak perlu port forward. |
| **Beda jaringan tanpa VPN** | IP publik server | Port forward **UDP 6767** di router laptop-server. Ribet, VPN lebih disaranin. |

Port default **6767** (UDP). Di laptop yang sama sebagai server, client bisa pakai `127.0.0.1`.

### Langkah connect
1. Laptop-server: dobel-klik `server.exe` (biarin kebuka).
2. Dua laptop: `LaunchCoopAndreas.exe` → isi serial → Launch.
3. In-game: Start Game → nickname → isi **IP server** + port **6767** → connect.
4. Cek dua-duanya kelihatan di dunia (spawn bareng). Baru mulai test misi.

---

## 2. AMBIL LOG / CRASH LOG (ini yang bikin debug enak) ⭐

Mod ini **udah punya sistem log bawaan** (diverifikasi dari source):

### a. Console real-time (paling berguna pas dev)
Client manggil `AllocConsole()` → **muncul jendela console item** barengan game, nampilin
log `logger::info/warn` live (koneksi, sync, error). **Biarin jendela itu kebuka pas main** —
kalau ada yang aneh, isinya kelihatan langsung. Screenshot / copy teksnya buat aku.

### b. Crash log otomatis (WAJIB kirim kalau crash) ⭐⭐
Kalau game **crash**, mod auto-nulis file ke:
```
C:\Games\GTA - The Original Trilogy\GTASA\CoopAndreas_crashes\
    2026-09-29_14-03-21.log   <- teks: alamat crash, module, stack
    2026-09-29_14-03-21.dmp   <- minidump (buat analisa dalam)
```
**Pas crash → buka folder `CoopAndreas_crashes\`, ambil file `.log` paling baru, kirim isinya ke aku.**
Itu petunjuk paling akurat buat nemu akar crash.

### c. Log lain yang mungkin ada (kalau repack lo pakai)
- `modloader\modloader.log`, `cleo.log` di folder game — kadang kecatat error ASI/script.
- Config coop (ada opsi `report-crashlogs`) tersimpan di folder *GTA San Andreas User Files*
  (Documents). Gak perlu diapa-apain buat test lokal.

### d. Format lapor ke aku (biar cepat)
Tulis di `PLAN_CoopAndreas_TestLog.md` §0 atau langsung chat: misi, kapan kejadian, gejala,
host/client, jumlah player, + **isi crash .log** kalau crash. Aku diagnosa → fix → revisi misi lain yang kena.

---

## 3. LOMPAT KE MISI TERTENTU (biar gak main dari awal)

⚠️ **Gak ada command mission-warp bawaan** di mod (chat F6 cuma buat ngobrol; folder "Commands"
itu opcode `Coop.*`, bukan cheat). Jadi opsi realistis:

### Opsi A — Savegame (paling stabil, disaranin)
Misi GTA di-gate sama progress story. Cara paling aman: punya **save tepat sebelum misi target**.
- Main normal sampe misi target kebuka, terus **save** di save-point terdekat (rumah CJ / dll).
  Sekali punya save itu, tiap mau test tinggal load.
- Atau download **"GTA SA mission start savegames"** (savepack per-misi, banyak beredar) → taruh di
  *GTA San Andreas User Files*. Tinggal load save yang pas sebelum misi yang mau dites.
- **Co-op:** host load save di titik yang bener; misi kebuka lewat marker corona di dunia,
  dua player samperin marker bareng → misi mulai.

### Opsi B — Trainer buat mempercepat (bukan pengganti save)
Trainer/CLEO SA (mis. yang ada teleport-to-waypoint, health, weapon, spawn car) bikin gampang
**nyampe marker misi** & bertahan pas test. Tapi trainer **gak nge-unlock gating story** —
tetap butuh save di titik yang bener. Hati-hati: CLEO/ASI tambahan bisa bentrok sama coop; test
seminimal mungkin mod lain kalau bisa.

### Opsi C — Debug mission-launcher ✅ SUDAH DIBIKIN & ke-deploy
File `scm/scripts/DEBUG_LAUNCHER.txt` (di-INCLUDE + `start_new_script` di MAIN.txt). Aktif HANYA
di freeroam (`$onmission == 0`). **Cara pakai: tahan LEFT SHIFT + tekan numpad:**
```
SHIFT+NUM1 = SWEET3 Drive-Thru        SHIFT+NUM7 = CRASH4 Doberman
SHIFT+NUM2 = SWEET2 Nines and AK's    SHIFT+NUM8 = DRUGS3 Gray Imports
SHIFT+NUM3 = SWEET4 Drive-By          SHIFT+NUM9 = TWAR7 OG Loc
SHIFT+NUM4 = SMOKE2 Running Dog       SHIFT+NUM0 = DRUGS1 Just Business
SHIFT+NUM5 = RYDER2 Robbing Uncle Sam SHIFT+NUM(.) = DRUGS4 Reuniting the Families
SHIFT+NUM6 = HOODS5 Sweet's Girl
```
- Force-start dari mana aja (misi biasanya teleport player sendiri di awal).
- Cuma buat build TEST. Buat rilis: hapus `{$INCLUDE scripts/DEBUG_LAUNCHER.txt}` di main.txt
  + baris `start_new_script @DEBUG_LAUNCHER` di scripts/MAIN.txt.
- ⚠️ Kalau force-start misi bikin crash/aneh (karena prasyarat story gak ada), itu WAJAR utk test —
  lapor aja, atau pakai savegame (Opsi A) buat kondisi yang lebih natural.

### Nomor misi ter-konversi (dari main.txt `DEFINE MISSION`)
| Misi | No. | Misi | No. |
|---|---|---|---|
| SWEET3 Drive-Thru | 15 | HOODS5 Sweet's Girl | 18 |
| SWEET2 Nines and AK's | 16 | CRASH4 Doberman | 21 |
| SWEET4 Drive-By | 17 | DRUGS3 Gray Imports | 23 |
| RYDER2 Robbing Uncle Sam | 26 | TWAR7 OG Loc | 27 |
| SMOKE2 Running Dog | 28 | DRUGS1 Just Business | 30 |

---

## 4. URUTAN TEST YANG DISARANIN
1. **Konek dulu** (2 client di dunia bareng, gak usah misi) → pastiin mod + jaringan OK.
2. Test **1 misi paling gampang** dulu: **SWEET4 Drive-By** atau **DRUGS1 Just Business**.
3. Perhatiin momen rawan: **pas misi mulai** (netID handshake — kalau freeze di sini, itu tersangka
   utama, lihat TestLog §3), **pas blip/checkpoint muncul di client**, **pas objektif selesai**.
4. Ada masalah → ambil crash `.log` / screenshot console → lapor. Aku fix + revisi retroaktif.
