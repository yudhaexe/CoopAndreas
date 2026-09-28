# PLAN — Campaign Konversi SEMUA Misi ke Co-op

> **Tujuan user:** konversi semua misi ke co-op, jalan autonomous, resumable lintas
> rate-limit (limit 5 jam habis → tunggu reset → lanjut dari titik terakhir).
>
> **Baca dulu:** `PLAN_SWEET3_Conversion.md` (§0 aturan anti-halu, Coop API, idiom, build/compile).
> File ini KHUSUS ngatur strategi batch + antrean + progress semua misi.

---

## 0. KONTRAK REALISTIS (baca tiap sesi, jangan overpromise)

**⛔ ATURAN #1 (dari user): BUKAN MODE AUTO.** Tiap misi WAJIB tanya keputusan desain dulu
(via AskUserQuestion) sebelum konversi — sama kayak SWEET3. JANGAN rantai beberapa misi tanpa
nanya. Alur per misi: analisa struktur → TANYA design forks → draft → self-compile (CLI) →
update tracker → misi berikutnya (tanya lagi). Lihat memory [[coopandreas-ask-per-mission]].

Yang Claude kirim = **DRAFT konversi**, BUKAN "tested & working".
- ❌ Tidak bisa compile Sanny Builder → draft belum tentu ke-compile.
- ❌ Tidak bisa playtest 2 client → desync/crash cuma ketauan dari dev.
- ❌ Semantik objective & misi mekanik-khusus = keputusan dev, bukan tebakan Claude.
- ✅ Bisa: draft pola roti-mentega niru `SWEET1.txt`, satu per satu, resumable.

Definisi status per misi (dipakai di tracker §4):
- `DRAFTED` = Claude udah sisipin Coop.* pola dasar. **Belum di-compile.**
- `COMPILES` = dev udah compile di Sanny, sukses (atau Claude udah benerin error compile).
- `TESTED` = dev udah playtest 2 client, happy-path jalan, objective bener. ← DEFINISI "SELESAI".

---

## 1. INVENTARIS (terverifikasi 2026-09-29)
- 136 misi terdaftar (`DEFINE MISSION` di `main.txt`), 303 file .txt (banyak non-misi/helper).
- Sudah kelar (>50 calls): `SWEET1`(103), `SWEET1B`(88), `INTRO2`(59), `INTRO1`(58).
- Stub (2–8 calls): `JFUD`(8), `TATTO/PSHOP/BARB`(4), `SWEET2/3/4/6`, `SMOKE2/3`, `RYDER2/3` (2).
- Belum disentuh: sisanya (~116).

---

## 2. STRATEGI: PILOT GATE → baru BATCH (WAJIB urut)

### FASE 0 — PILOT (gate, TIDAK boleh dilewati)
Konversi **SWEET3 (Drive-Thru)** SATU misi sampai **COMPILES** (idealnya TESTED).
Tujuan: buktiin pola Claude beneran ke-compile SEBELUM diterapin ke 100 misi.
Kalau pilot nemu kesalahan sistematis → benerin pola dulu. **Ini yang nyegah "100 draft salah semua".**
Butuh dev: compile di Sanny + kasih daftar error → Claude fix → ulang sampai clean.

### FASE 1 — BATCH misi roti-mentega
Baru boleh mulai SETELAH minimal 1 pilot `COMPILES`. Proses antrean §4 satu per satu,
tiap misi commit progress ke tracker. Prioritas: misi Grove Street awal / pola sederhana.

### FASE 2 — Misi butuh keputusan
Misi mekanik-khusus / objective ambigu → JANGAN di-draft buta. Tulis pertanyaan di §5,
tunggu jawaban dev, baru implementasi.

### FASE 3 — Misi RE-binary (interior/streaming)
Di luar jangkauan scripting. Skip, catat di §5. Butuh IDA/Ghidra + dev.

---

## 3. LOOP KERJA AUTONOMOUS + RESUME
```
[Claude] ambil misi berikutnya dari antrean (§4)
   → draft konversi (niru SWEET1)
   → update status jadi DRAFTED + catat di LOG (§6)
   → lanjut misi berikutnya
        │
   (rate-limit 5 jam habis? sesi mati?)
        │
   [resume] baca tracker §4 → lanjut dari misi ber-status paling rendah
```
- **Resumable:** semua state ada di file ini (§4 tracker). Interupsi apapun → lanjut dari sini.
- **Rate-limit:** Claude tak bisa nembus sendiri. Saat sesi hidup lagi → baca §4 → lanjut.
- **Auto-pacing:** bisa pakai skill `/loop` (self-paced) biar Claude nerusin antrean otomatis.
- **Aturan commit:** SATU misi = SATU unit kerja. Jangan pindah misi sebelum status ke-update.

---

## 4. ANTREAN + TRACKER (Claude update kolom Status tiap selesai)
Status: `TODO` → `DRAFTED` → `COMPILES` → `TESTED` | atau `BLOCKED`(butuh keputusan) | `SKIP`(RE-binary)

| # | Misi (file) | Nama | Tipe | Status | Catatan |
|---|---|---|---|---|---|
| — | SWEET1 | Tagging Up Turf | butter | TESTED? | referensi template (103 calls) |
| — | INTRO1/INTRO2/SWEET1B | — | — | done | referensi |
| **PILOT** | SWEET3 | Drive-Thru | convoy(1-car) | COMPILES | 21 Coop calls, **compile sukses (CLI)**, main.scm ke-deploy ke game. Isi: sync enable + collect players + entity netID map (sweet_car/smoke/sweet/ryder) + convoy blip per-player + checkpoint per-player @3 leg (2404.1,-1891.5 / 2513.3,-1671.9 / 2066.465,-1695.444) + cleanup (remove checkpoint+blip). Desain: host nyetir sweet_car+AI gang, player lain ngikut mobil sendiri, objective any-player. **Nunggu playtest 2-client (desync/crash/objective).** Belum dikonversi: per-player objective TEXT (masih host-only via Text.PrintNow) — bisa ditambah kalau test butuh. |
| 1 | SWEET2 | Nines and AK's | convoy+foot | COMPILES | 14 Coop calls, compile OK, deployed. Host nyetir $big_smoke_car (Big Smoke penumpang), player lain mobil sendiri + convoy blip + checkpoint (dest 2453.07,-2003.96 & on-foot 2448.96,-1973.545) + cleanup. Nunggu playtest. |
| 2 | SWEET4 | Drive-By | convoy+combat | COMPILES | 14 Coop calls, compile OK, deployed. Host nyetir gang car 543@ (gang 394@/401@/408@ penumpang, drive-by), player lain mobil sendiri + convoy blip ke 543@. Objektif=kill Balla (world-state, any-player natural). Checkpoint di-skip (musuh roaming, gak cocok fixed checkpoint). Nunggu playtest. |
| 3 | SMOKE2 | Running Dog | convoy+chase | COMPILES | 10 Coop calls, compile OK, deployed. Host nyetir mobil Smoke 34@ (Big Smoke 35@), player lain mobil sendiri + convoy blip. Objektif kejar-bunuh target (world-state, any-player). ⚠️RISIKO: entity spawn coord placeholder (0,0,-100) lalu di-warp; netID handshake ditaruh tepat setelah create — kalau in-game HANG, pindahin ke setelah warp. Nunggu playtest. |
| 4 | RYDER2 | Robbing Uncle Sam | convoy+combat | COMPILES | 10 Coop calls, compile OK, deployed. Host nyetir truk 95@ (Ryder penumpang), player lain mobil sendiri + convoy blip. Angkut krat = AI-scripted (Ryder), objektif world-state. CATATAN: blip pindah ke getaway car 99@ di fase akhir — belum di-sync per-player (refinement). Nunggu playtest. |
| — | SWEET6 | **Cesar Vialpando** | 🎯 MEKANIK-KHUSUS | BLOCKED | minigame dansa lowrider (rhythm+hidraulik). Butuh keputusan "4 player di misi dansa solo?" — TANYA dulu. 5571 baris. |
| — | RYDER3 | **Catalyst** | 🎯 MEKANIK-KHUSUS | BLOCKED | heist kereta/motor chase — cek mekanik, mungkin butuh keputusan. |
| — | SMOKE3 | **Wrong Side of the Tracks** | 🎯 MEKANIK-KHUSUS | BLOCKED | misi kereta legendaris (timing ketat di motor, Smoke nembak). Butuh keputusan desain. |
| 5 | HOODS5 | Sweet's Girl | escort+combat | COMPILES | 8 Coop calls, compile OK, deployed. Bukan convoy — objektif lokasi (ikuti Sweet 75@ + LocateAnyMeans). Pola: netID map Sweet + per-player CHAR blip (UpdateCharBlipForNetworkPlayer) + cleanup. Nunggu playtest. |
| 6 | CRASH4 | Doberman | territory+kill | COMPILES | 8 Coop calls, compile OK, deployed. Gang war Glen Park + kejar-bunuh target 34@. Pola: netID map target + per-player char blip (merah) + cleanup. Objektif world-state (kill). Nunggu playtest. |
| 7 | DRUGS3 | Gray Imports | location+kill | COMPILES | 8 calls, compile OK, deployed. Solo docks shootout + kejar boss 101@. netID map boss + char blip merah + cleanup. Nunggu playtest. |
| 8 | TWAR7 | OG Loc | convoy+chase-kill | COMPILES | 8 calls, compile OK, deployed. netID map target Freddy 34@ + char blip + cleanup. Nunggu playtest. |
| 9 | DRUGS1 | Just Business | convoy+combat | COMPILES | 10 calls, compile OK, deployed. Host nyetir 34@ (Big Smoke penumpang) + convoy blip + cleanup. Nunggu playtest. |
| 10 | DRUGS4 | Reuniting the Families | convoy+combat+escort | COMPILES | 10 calls, compile OK, deployed. Host nyetir $sweet_car (Sweet), + convoy blip + cleanup. Misi kompleks (ambush SWAT, rooftop) — blip fase lanjut ($sweet on-foot 2142, getaway) belum di-sync per-player (refinement). Nunggu playtest. |
| 11 | SWEET7 | **Los Sepulcros** | escort+combat | DRAFTED | 28 Coop calls. ⚠️ **DRAFTED, belum COMPILES** (Sanny CLI ga ada di sesi cloud ini — sanity compile di sesi local). Desain (user): (a) fase kabur = host nyetir escape car 78@ + Sweet penumpang, player lain mobil sendiri + convoy blip ke 78@ + checkpoint dest Grove (799.01,-1074.03,23.01), objektif any-player; (b) 3 Ballas target (289@/290@/291@) = per-player CHAR blip merah, kill world-state. Isi: enable sync + collect + netID map (78@/$sweet @SWEET7_3089, 289/290/291 di branch spawn) + per-player char blip 3 Ballas (dibuang per-kematian di handler vanilla 296@/297@/298@) + convoy blip+checkpoint fase drive-home + cleanup (car blip+checkpoint) di shared cleanup @SWEET7_32503. **Prinsip aman: tiap Coop blip call ditaruh persis sebelah vanilla Blip.* sepadan → reachability/validity identik (hindari hang netID & handle-0).** CATATAN: 289/290/291 spawn branch-dependent (`FindMaxNumberOfGroupMembers()>1`). Teks objektif per-player belum di-backfill. Nunggu: compile local + playtest 2-client. |
| 12 | BCRASH1 | **Badlands** | location+kill | DRAFTED | 10 Coop calls (pola DRUGS3/CRASH4). ⚠️ DRAFTED, belum COMPILES (cloud tanpa CLI). Target 284@ (Char model 147 @ farm -2814,-1522) = per-player char blip merah, kill world-state any-player. Isi: enable sync + collect + netID map 284@ + per-player char blip (di sebelah `285@ = Blip.AddForChar`) + removal pas target mati (@BCRASH1_12477) + cleanup. CATATAN: mekanik foto+snipe (Char.HasBeenPhotographed) host-primary (kamera), kill bisa any-player. Additive-only. Nunggu compile local + playtest. |
| 13 | GROVE2 | **Grove 4 Life** | escort/companion+combat | DRAFTED | 8 Coop calls (pola HOODS5). ⚠️ DRAFTED, belum COMPILES (cloud tanpa CLI). Follow-Sweet gang war: $sweet companion = per-player FRIENDLY char blip biru (BlipColor.Blue, friendly True) biar follower nemu grup. Musuh Ballas spawn dinamis (gang war, gak ada blip fix) → objektif world-state. Isi: enable sync + collect (di Stat.RegisterMissionGiven, misi ini gak ada `$onmission=1`) + netID map $sweet + per-player friendly blip + cleanup di shared routine @GROVE2_3381 (unconditional, jalan pass/fail). Additive-only. Nunggu compile local + playtest. |
| 14 | TRUTH1 | **Body Harvest** | convoy/escort-vehicle | DRAFTED | 8 Coop calls (pola DRUGS4/SMOKE2). ⚠️ DRAFTED, belum COMPILES (cloud tanpa CLI). Curi Combine Harvester 34@ (model 532) dari ladang, host nyetir balik ke barn dikejar traktor musuh; follower escort + convoy blip ke 34@. Objektif world-state (deliver harvester). Isi: enable sync + collect + netID map 34@ + per-player convoy blip (di sebelah vanilla Blip.AddForCar 34@) + cleanup di shared routine (sblm Mission.Finish). Additive-only. Nunggu compile local + playtest. |
| 15 | MANSIO3 | **Home Coming** | territory-clear+kill | DRAFTED | 29 Coop calls (pola SWEET7 multi-target). ⚠️ DRAFTED, belum COMPILES (cloud tanpa CLI). Bersihin dealer di Grove: 6 target (40@ + 5 dealer 41@-45@) = per-player char blip merah, kill world-state. Isi: enable sync + collect + netID map 6 target + per-player enemy blip (di sebelah vanilla Blip.AddForChar) + removal SEMUA di final cleanup @MANSIO3_7272 (unconditional, jalan pass/fail) → gak ada blip nyangkut. CATATAN: $sweet companion blip di-SKIP (vanilla hide-nya BlipDisplay.Neither, peran unclear — Sweet baru pulang RS). Enemy blip persist di mayat sampai misi kelar (kosmetik, vanilla batch-remove pas fase; non-fatal). Additive-only. Nunggu compile local + playtest. |
| 16 | STEAL1 | **Zeroing In** | track/follow-car | DRAFTED | 8 Coop calls. ⚠️ DRAFTED, belum COMPILES (cloud tanpa CLI). Track mobil target 435@ pakai radar lalu ikutin ke garasi; per-player car blip (kuning) di 435@ biar semua player bisa ikut. Isi: enable sync + collect + netID map 435@ + per-player car blip + cleanup SEBELUM `Car.Delete(435@)` @STEAL1_7730 (no stuck). Vanilla juga nge-blip 435@ (radar), jadi konsisten. Additive-only. Nunggu compile local + playtest. |
| 18 | SYN3 | **Outrider** | convoy (outrider gauntlet) | DRAFTED | 8 Coop calls. ⚠️ DRAFTED, belum COMPILES. Host nyetir mission car 34@ (OMEGA, model 421) lewat gauntlet Vagos; follower convoy blip ke 34@. Isi: enable sync + collect + netID map 34@ (di sebelah vanilla `212@=Blip.AddForCar(34@)` @ real coords line 306, aman) + cleanup @shared (Blip.Remove 212@ block). CATATAN: outrider bikes 35@/36@ spawn placeholder (0,0,-100) lalu warp → per-player blip di-SKIP (risiko L-01), refinement. Additive-only. Nunggu compile local + playtest. |
| — | CASIN10 | Saint Mark's Bistro | ✈️ plane + interior | BLOCKED | Shamal (519) terbang ke Liberty City + bistro interior (Z~1370). Flying + single-interior engine-limit → di luar scripting butter. Skip. |
| 17 | CAT3 | **Tanker Commander** | convoy/escort-vehicle | DRAFTED | 8 Coop calls (pola TRUTH1). ⚠️ DRAFTED, belum COMPILES (cloud tanpa CLI). Curi Tanker 327@ (model 514) + trailer, host nyetir balik ke depot dikejar; follower escort + convoy blip ke 327@. Objektif world-state. Isi: enable sync + collect + netID map 327@ + per-player convoy blip (di sebelah vanilla Blip.AddForCar 327@ @CAT3_5488, one-time phase setup — BUKAN di init krn 327@ dibuat di gosub CAT3_710 SETELAH $onmission=1, taruh di init = HANG) + cleanup @CAT3_17366. Additive-only (1 diff whitespace `if `→`if`, sama kayak konversi lama, non-logic). Nunggu compile local + playtest. |
| — | CAT4 | Against All Odds | 🏠 interior (store robbery) | BLOCKED | `Char.SetAreaVisible($catalina, 3)` Z~1003 = interior toko. Single-interior engine-limit → butuh pola teleport-bareng (SWEET1B) atau skip. Belum digarap. |
| SKIP | JFUD/TATTO/PSHOP/BARB | (shop/minigame, bukan story) | — | SKIP | gak ada header "Originally". |
| … | (sisa story missions) | — | — | TODO | diisi bertahap |

> Tabel ini bakal dilengkapi pas FASE 1 mulai (Claude enumerate dari `main.txt` DEFINE MISSION).
> Nama misi di atas = perkiraan urutan story; VERIFIKASI dari file sebelum ngedraft.

---

## 5. BLOCKED / BUTUH KEPUTUSAN DEV (Claude JANGAN nebak)
Diisi saat ketemu. Format: `[misi] pertanyaan`.
- [ ] **SWEET3 (Drive-Thru)** — sukses kalau HOST nyampe garis finish, SEMUA player, atau salah satu? Tiap player mobil sendiri atau numpang 1 mobil?
- [ ] Misi mekanik-khusus yang bakal ketemu nanti (terbang: NOE/Stowaway/Vertical Bird/Learning to Fly/Freefall; RC: Beefy Baron/Supply Lines/Air Raid/New Model Army; timed/solo) → butuh desain "4 player ngapain".
- [ ] Interior/streaming missions → SKIP (RE-binary, di luar scripting).

---

## 6. LOG PROGRESS (append tiap sesi, jangan hapus)
- **2026-09-29** — Plan campaign dibuat. Inventaris terverifikasi (136 misi, 4 done, ~16 stub).
  Belum ada draft baru. Nunggu: (a) VS Desktop C++ selesai install, (b) keputusan objective SWEET3,
  (c) pilot SWEET3 COMPILES sebelum batch. Rekomendasi Claude: JANGAN mass-draft sebelum pilot lolos gate.
- **2026-09-29 (lanjut)** — VS + xmake selesai; `server.exe` BUILD OK (toolchain C++ tervalidasi).
  Keputusan desain SWEET3: host nyetir + player lain mobil sendiri, objective any-player.
  SWEET3 core scaffolding di-DRAFT (12 Coop calls, pola SWEET1). **GATE: nunggu dev compile SWEET3 di Sanny
  Builder buat validasi opcode/param SEBELUM lanjut per-player checkpoint/text & sebelum sentuh SWEET2/4.**
- **2026-09-29 (lanjut 2)** — BREAKTHROUGH: Sanny Builder CLI tervalidasi → Claude bisa compile sendiri.
  Semua 4 target C++ ke-build & mod ke-INSTALL penuh di game folder; user konfirmasi game LAUNCH + CONNECT OK
  (Milestone 1 lolos, versi game kompatibel). SWEET3 (21 calls), SWEET2 (14), SWEET4 (14) semua **COMPILES**
  (self-compiled via CLI) & ke-deploy. Aturan baru dari user: (a) NOT auto — tanya fork desain baru per misi,
  (b) ikuti pola misi yg udah kelar (jgn invent) — cek [[coopandreas-follow-existing-patterns]].
  Catatan: teks objective per-player (Coop.PrintNowForNetworkPlayer, dipakai 53× di misi kelar) BELUM di-backfill
  ke SWEET2/3/4 — masih host-only Text.PrintNow. Interior pattern (teleport-bareng SWEET1B) dipelajari utk misi interior.
  **Nunggu: playtest 2-client SWEET3/2/4.**
- **2026-09-29 (lanjut 3)** — SMOKE2 (Running Dog, 10 calls) & RYDER2 (Robbing Uncle Sam, 10 calls) COMPILES & deployed.
  Total 5 misi COMPILES: SWEET3, SWEET2, SWEET4, SMOKE2, RYDER2 (semua pola convoy proven). Stub butter HABIS.
  Stub tersisa (SWEET6 Cesar Vialpando, RYDER3 Catalyst, SMOKE3 Wrong Side of the Tracks) = MEKANIK-KHUSUS → BLOCKED,
  butuh keputusan desain dari user. Buat lanjut butter lain harus enumerate untouched story missions dari main.txt.
- **2026-09-29 (lanjut 4)** — Untouched butter: HOODS5 Sweet's Girl (8 calls, varian escort/char-blip) & CRASH4 Doberman
  (8 calls, territory+kill). COMPILES & deployed. **Total 7 misi COMPILES.** Kandidat butter berikut: DRUGS3 Gray Imports,
  TWAR7 OG Loc, DRUGS1 Just Business, DRUGS4 Reuniting the Families, SWEET7 Los Sepulcros. ⚠️ 7 misi BELUM ada yg PLAYTEST —
  risiko compounding kalau terus tanpa test.
- **2026-09-29 (lanjut 5)** — DRUGS3 Gray Imports (8), TWAR7 OG Loc (8), DRUGS1 Just Business (10) COMPILES & deployed.
  **Total 10 misi COMPILES.** Test log dibuat (PLAN_CoopAndreas_TestLog.md). Kandidat berikut: DRUGS4, SWEET7, GUNS1, MUSIC5.
  ⚠️ MASIH 0 playtest dari 10 misi — risiko compounding makin gede.
- **2026-09-29 (lanjut 6)** — DRUGS4 Reuniting the Families (10 calls) COMPILES & deployed. **Total 11 misi COMPILES.**
  Panduan test dibuat: PLAN_HowToTest.md (2-laptop, console+crash log, lompat misi via savegame). Console real-time
  (AllocConsole) + crash log auto ke <game>\CoopAndreas_crashes\*.log tervalidasi dari source. Kandidat butter tersisa
  makin masuk ranah stealth/khusus (GUNS1 Home Invasion stealth, SWEET7 Los Sepulcros, MUSIC*, dst). MASIH 0 playtest dari 11.
- **2026-09-28 (sesi cloud)** — SWEET7 Los Sepulcros (28 Coop calls) **DRAFTED** (escort+combat: drive Sweet home dari kuburan
  + funeral ambush 3 Ballas). ⚠️ Sesi CLOUD ga punya Sanny CLI → status DRAFTED, belum COMPILES; sanity compile nyusul di sesi
  local. Desain diputusin user via AskUserQuestion (host nyetir escape car 78@ + Sweet; 3 Ballas per-player char blip merah).
  Prinsip konversi diperketat user: (a) commit tanpa atribusi Claude (git message polos), (b) pastikan cleanup + end-to-end
  after-mission + gak ada stuck/hang, (c) TANPA refactor code vanilla (light-touch insert doang). Prinsip aman baru:
  **tiap Coop per-player blip call ditaruh PERSIS sebelah vanilla Blip.* yg sepadan** → reachability & entity-validity identik
  sama vanilla (nutup risiko hang netID handshake `while==-1` & RemoveCharBlip(handle-0)). Cleanup di shared routine @SWEET7_32503
  (jalan di pass & fail) → gak ada blip/checkpoint nyangkut. Lanjut autonomous sampai limit habis.
  Diverifikasi: konversi lama (commit ab3a6e4) TIDAK ubah vanilla — 426 insert / 11 delete, semua delete = stub warning
  `Coop.AddChatMessage("...unsupported...")` + 1 whitespace `if `→`if` di SWEET3. Aturan run ini: additive-only, no refactor.
- **2026-09-28 (sesi cloud, lanjut)** — BCRASH1 Badlands (10 Coop calls) **DRAFTED**. Location+kill (pola DRUGS3/CRASH4):
  target 284@ per-player char blip merah + removal pas mati + cleanup. 28 insert / 0 delete (100% additive). Belum COMPILES
  (cloud tanpa Sanny CLI). Kandidat berikut: MUSIC3 (Management Issues, 8-target party — kompleks), MUSIC5 (House Party, defense),
  CAT1-4 (Catalina robberies), CATALIN (First Date). Prioritas pola bersih (single/multi target + blip, atau convoy).
- **2026-09-28 (sesi cloud, batch besar)** — 7 misi DRAFTED + 1 fix, semua additive-only, no-refactor, per instruksi user
  (commit git-message polos tanpa atribusi, lanjut sampai limit, pastikan cleanup end-to-end & no stuck):
  SWEET7 Los Sepulcros (28), BCRASH1 Badlands (10), GROVE2 Grove 4 Life (8), TRUTH1 Body Harvest (8),
  MANSIO3 Home Coming (29), STEAL1 Zeroing In (8), CAT3 Tanker Commander (8). Semua nunggu compile Sanny CLI di sesi local + playtest.
  **Lesson C-02 lahir**: netID handshake `while==-1` di entity yg belum dibuat = HANG (STEAL1 & CAT3 ketauan: entity dibuat di
  gosub SETELAH $onmission=1). Aturan tetap: taruh tiap Coop blip call PERSIS di sebelah vanilla Blip.* sepadan. STEAL1 di-fix.
  BLOCKED (butuh keputusan/di luar scripting): CASIN10 (plane+interior), CAT4 (interior toko), CRASH1 Burning Desire (interior+kompleks arr=405).
  **STATUS POOL**: butter Grove-arc HABIS (semua done/blocked). Sisa untouched = mid/late-game (San Fierro/Desert/Venturas) yg
  mayoritas mekanik-khusus (terbang: NOE/Stowaway/Learning to Fly/Vertical Bird/Freefall; RC; race: Monster/Quarry/Kickstart;
  tailing: Snail Trail; interior: Jizzy/Madd Dogg's Rhymes/bistro; heist) ATAU target-count gede (WUZI1 char=26, RIOT2 char=65,
  MAF4 char=68). Ini butuh keputusan desain per-misi (ATURAN #1 ask-per-mission) atau RE-binary (interior/streaming) → TANYA user.
  Kandidat mid-game yg MASIH bisa pola-bersih (kalau user mau lanjut): SYN3 Outrider (escort/convoy), SYN1 Photo Opportunity
  (chase, tapi foto host-only), DRIV2/DRIV3 (chase-kill, array-heavy), MANSON5 Cut Throat Business, SCRASH2 Snail Trail (tailing).
```
```
```
```
```
```
```

---

## 7. YANG DIBUTUHIN DARI DEV BIAR CAMPAIGN JALAN
1. **Sekarang:** jawaban objective SWEET3 (§5) → biar pilot bisa mulai.
2. **Per batch:** compile di Sanny + kasih daftar error → Claude fix.
3. **Per misi "selesai":** playtest 2 client → kasih observasi (desync/crash/objective salah).
4. **Keputusan desain** buat misi mekanik-khusus saat ketemu.
```
