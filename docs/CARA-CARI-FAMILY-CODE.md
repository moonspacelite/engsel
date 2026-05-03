# Cara Cari Family Code XL/PRIO/HYBRID (Tanpa Hardcode)

> Tutorial 4 metode discovery family code (UUID) yang dipakai engsel
> dan komunitas. Ditulis dari pengalaman langsung — dengan kode yang
> bisa langsung Anda copy-paste.

---

## Background — Apa itu family code?

Family code adalah UUID v4 (mis. `e2e6c8e7-1aa1-4f3a-9b5e-...`) yang
dipakai server XL untuk grouping paket. Satu family code biasanya
punya 1-12 variant (misal Xtra Combo Flex 7d / 30d / 90d).

Endpoint utama: `POST /api/v8/xl-stores/options/list` dengan body
`{"package_family_code": "<UUID>", "is_enterprise": false, "migration_type": "NORMAL"}`.

Server return `data.package_family.name` + `data.package_variants[]`.
Kalau `name == ""` dan `variants == 0` → UUID tidak valid (atau bukan
family, mungkin campaign/MCCM action_param).

---

## Metode 1 — Source komunitas (paling mudah, yield 30-50)

**Susah-mudah: ⭐ (sangat mudah)**
**Yield: 30-50 UUID per sweep, ~90% sudah pasaran**

### Sumber yang sudah saya verify

| Sumber | Format | Catatan |
|---|---|---|
| [purplemashu/me-cli](https://github.com/purplemashu/me-cli) (archived) | folder `decoy_data/*.json` | Dasar Eve CLI, banyak UUID di decoy data |
| [purplemashu/me-cli-sunset](https://github.com/purplemashu/me-cli-sunset) | folder `decoy_data/*.json` | Fork lebih baru |
| `me.mashu.lol/pg-hot.json` + `pg-hot2.json` | array JSON | Daftar paket "HOT" pilihan komunitas |
| Telegram groups (XL Tipsmart, MyXL Tricks, dll) | screenshot Eve CLI | Manual ekstrak UUID dari image |

### Langkah konkret

```bash
# Clone dua repo komunitas + sweep UUID
git clone https://github.com/purplemashu/me-cli-sunset
git clone https://github.com/purplemashu/me-cli || true

# Pattern UUID v4
UUID_RX='[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}'

# Sweep semua file
grep -rohE "$UUID_RX" me-cli-sunset/ me-cli/ | sort -u > uuid_komunitas.txt
wc -l uuid_komunitas.txt
# Output saya: 47 unik
```

```bash
# Tambah dari pg-hot
curl -s https://me.mashu.lol/pg-hot.json | jq -r '..|.family_code? // empty' | sort -u
curl -s https://me.mashu.lol/pg-hot2.json | jq -r '..|.family_code? // empty' | sort -u
```

### Trik penting

- **Banyak duplicate antara me-cli-sunset dan me-cli** — pakai `sort -u`.
- **Ada UUID di nama file** (mis. `decoy_data/abc12345.json`) — `grep -E "$UUID_RX"` di nama file juga.
- **Field `package_family_code` di pg-hot2** sometimes salah (campaign UUID, bukan family) — verify via metode 4 sebelum pakai.

---

## Metode 2 — APK decompile (medium, yield 10-15 per APK)

**Susah-mudah: ⭐⭐ (medium, butuh JADX)**
**Yield: 10-15 UUID baru per versi APK lama**

UUID hardcoded di binary APK — terutama di string constant pool. Tip
penting: **APK versi lama** sering punya UUID yang dihapus di versi
terbaru tapi server XL **masih kenal** (deprecated tapi belum dihapus
dari catalog).

### Tools

- [APKMirror](https://www.apkmirror.com/apk/pt-xl-axiata-tbk/myxl/) — download APK
- [JADX](https://github.com/skylot/jadx) — decompile

```bash
# Install jadx
sudo apt install -y jadx
# Atau: snap install jadx

# Download APK (gunakan curl atau APKPure)
curl -L 'https://www.apkmirror.com/apk/.../myxl-9-1-0-android-apk-download/' \
     -o myxl-9.1.0.apk
# (Praktiknya APKMirror butuh user-agent + cloudscraper, sometimes pakai browser)

# Decompile
jadx -d myxl_9.1.0/ myxl-9.1.0.apk

# Sweep UUID
UUID_RX='[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}'
grep -rohE "$UUID_RX" myxl_9.1.0/sources/ | sort -u > uuid_apk_9.1.0.txt
```

### Trik penting

- **Ulangi untuk 5 versi APK terakhir** (mis. 9.1.0, 9.0.5, 8.9.1, 8.8.0, 8.7.5)
- **Diff antar versi**:
  ```bash
  diff <(sort uuid_apk_9.1.0.txt) <(sort uuid_apk_8.7.5.txt) | grep '<' | awk '{print $2}'
  # → UUID yang ada di 8.7.5 tapi sudah hilang di 9.1.0
  ```
  UUID yang **dihapus** sering masih valid di server (deprecated, bukan removed dari DB).
- Filter false-positive dengan blacklist UUID umum (Android system UUIDs, library UUIDs Firebase/dll). Cek dulu di [https://www.uuidgenerator.net/version-validator](https://www.uuidgenerator.net/version-validator) untuk konfirmasi format v4.

---

## Metode 3 — Web JS bundle scraping (medium, yield 10-30 PRIO-exclusive)

**Susah-mudah: ⭐⭐ (medium, butuh js-beautify)**
**Yield: 10-30 UUID, banyak PRIO-exclusive**

Web app `prioritas.xl.co.id` adalah portal khusus PRIORITAS subscriber.
JS bundle-nya sering import family_code list untuk rendering catalog.

### Langkah konkret

```bash
# Fetch landing page
curl -sL https://prioritas.xl.co.id/ -o landing.html

# Extract URL JS bundle
grep -oE 'src="[^"]*\.js"' landing.html | sed 's/src="//;s/"//' > js_files.txt

# Download semua bundle
mkdir -p prio_bundle/
while read -r path; do
    out="prio_bundle/$(basename "$path")"
    curl -sL "https://prioritas.xl.co.id$path" -o "$out"
done < js_files.txt

# Beautify (kalau minified)
sudo apt install -y nodejs npm
sudo npm install -g js-beautify

for f in prio_bundle/*.js; do
    js-beautify "$f" > "${f%.js}.pretty.js"
done

# Grep UUID
UUID_RX='[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}'
grep -ohE "$UUID_RX" prio_bundle/*.pretty.js | sort -u > uuid_prio_web.txt
```

### Trik penting

- **Ulangi di subdomain lain**:
  - `https://www.xl.co.id/prio/`
  - `https://www.xl.co.id/paket/`
  - `https://hapyou.xl.co.id/`  (XL HappyOne portal)
  - `https://store.xl.co.id/`
- **Banyak UUID di JS adalah ASSET ID** (image asset di CDN), bukan family code. Filter dengan validasi metode 4.
- **CSP / CORS** kadang block download — pakai `curl -A 'Mozilla/5.0 (X11; Linux x86_64)'` untuk bypass user-agent restrictions.

---

## Metode 4 — API discovery (paling powerful, butuh token aktif)

**Susah-mudah: ⭐⭐⭐ (technical, butuh akses API + bahasa Python)**
**Yield: 10-50 UUID FRESH yang tidak ada di pasaran**

Ini metode utama yang dipakai engsel saat sweep new family. Harus punya:

- Token aktif (id_token + access_token dari login MyXL)
- xl_helper.py atau engsel running (untuk send_api_request + decrypt)

### Tahap A — Endpoint sweep

Setiap endpoint berikut **sering return UUID di payload**:

```
POST /api/v8/store/api/v8/banners
POST /api/v8/store/api/v8/segments/sfy
POST /api/v8/configs/store/segments
POST /api/v8/personalization/dynamic-banners
POST /api/v8/gamification/loyalties/tiering/rewards-catalog
POST /api/v8/store/api/v8/store-landing
POST /api/v8/sharings/api/v8/store/landing-content
```

**Contoh dengan engsel terinstal (langsung CLI):**

Engsel sudah ada Menu 8 > 8 (Discovery) yang call beberapa endpoint
ini. Jalankan, capture output, grep UUID.

**Atau dengan Python langsung:**

```python
import json
from xl_helper import send_api_request   # dari me-cli-sunset
import re

UUID_RX = re.compile(r'[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}')

api_key = "..."           # dari .env
xdata = "..."
sec   = "..."
token = "..."             # id_token

endpoints = [
    "store/api/v8/banners",
    "store/api/v8/segments/sfy",
    "configs/store/segments",
    "personalization/dynamic-banners",
    "gamification/loyalties/tiering/rewards-catalog",
]

found = set()
for ep in endpoints:
    r = send_api_request("https://api.myxl.xlaxiata.co.id",
                         api_key, xdata, sec, ep,
                         {"is_enterprise": False, "lang": "en"},
                         token, "POST")
    text = json.dumps(r)
    for u in UUID_RX.findall(text):
        found.add(u)

print(len(found), "UUIDs found from endpoints")
```

### Tahap B — Keyword search recursive

Endpoint `/api/v9/xl-stores/options/search?keyword=<X>` return list paket
yang match keyword + `action_param` (encrypted package_option_code).

```python
keywords = [
    "Diamond", "Ultima", "Gold", "Platinum",  # PRIO tiers
    "Akrab", "XL SATU", "Vidio", "Disney",    # AKRAB + bundles
    "Combo", "Xtra", "Extra", "Bonus",        # generic
    "Roaming", "Internet", "Voice", "SMS",
    "5G", "Game", "Stream", "Music",
    # ... 200+ keywords yang saya pakai dalam me-cli-sunset extended search
]

option_codes = []
for kw in keywords:
    r = send_api_request(base, api_key, xdata, sec,
                         "api/v9/xl-stores/options/search",
                         {"keyword": kw, "is_enterprise": False, "lang": "en"},
                         token, "POST")
    if r and r.get("status") == "SUCCESS":
        for item in r.get("data", {}).get("items", []):
            ap = item.get("action_param")
            if ap:
                option_codes.append(ap)

print(len(option_codes), "action_param dari keyword search")
```

### Tahap C — Resolve action_param → family_code

```python
for code in option_codes:
    r = send_api_request(base, api_key, xdata, sec,
                         "api/v8/xl-stores/options/detail",
                         {"package_option_code": code, "is_enterprise": False, "lang": "en"},
                         token, "POST")
    if r and r.get("status") == "SUCCESS":
        fc = r.get("data", {}).get("package_family_code")
        if fc:
            found.add(fc)
```

### Tahap D — Validate dengan /options/list

```python
def validate_family(fc):
    """Return (legit, name, num_variants) untuk family code."""
    for mig in ["NORMAL", "PRE_TO_PRIOH", "PRIOH_TO_PRIO"]:
        for ent in [False, True]:
            r = send_api_request(base, api_key, xdata, sec,
                                 "api/v8/xl-stores/options/list",
                                 {"package_family_code": fc,
                                  "is_enterprise": ent,
                                  "migration_type": mig,
                                  "lang": "en"},
                                 token, "POST")
            if r and r.get("status") == "SUCCESS":
                pf = r.get("data", {}).get("package_family", {})
                pv = r.get("data", {}).get("package_variants", [])
                if pf.get("name") and len(pv) > 0:
                    return True, pf["name"], len(pv)
    return False, None, 0

for fc in found:
    ok, name, nv = validate_family(fc)
    if ok:
        print(f"LEGIT  {fc}  {name}  ({nv} variants)")
    else:
        print(f"INVALID {fc}")
```

### Trik penting metode 4

1. **action_param adalah session-encrypted**. Harus diresolve di session
   yang **sama** dengan yang produksinya. Login fresh + langsung resolve.
   Kalau wait > 30 menit, action_param expired.

2. **Server trust client-claimed `migration_type`**: spoof
   `PRE_TO_PRIOH` dari akun PREPAID → unlock catalog PRIORITAS. Ini
   kenapa engsel pakai matrix 7-mig × 2-enterprise saat browse.

3. **Server trust client-claimed `is_enterprise`**: spoof
   `is_enterprise=true` → unlock paket BIZ.

4. **Akun PREPAID lihat 98% catalog, akun PRIORITAS hanya 62%** (server
   filter terbalik karena PRIORITAS sudah comitted, app tidak perlu
   show paket yang tidak relevan).

5. **Filter false-positive ketat**: `name != "" AND len(variants) > 0`.
   UUID dari banner/MCCM **bukan family** (kebanyakan campaign action
   yang trigger flow custom).

6. **Yield realistic dari 4 metode digabung**: 50-150 UUID. ~70% sudah
   ada di pasaran TG, ~30% baru/unik. Untuk fresh, fokus ke metode 4
   keyword search dengan keyword **regional** (mis. "Jabodetabek",
   "Bali", "Sumatra") atau **promo seasonal** (mis. "Lebaran",
   "Independence", "Imlek").

---

## Bonus — MITM live traffic (paling lengkap, butuh device)

Tidak saya tutorial-kan di sini karena butuh:

- Android rooted atau iOS jailbroken
- Frida server installed
- Burp Suite atau Charles Proxy
- SSL pinning bypass script

**Yield: 20-50 UUID per session app browsing.**

Resource: [Frida CodeShare](https://codeshare.frida.re/) untuk SSL
pinning bypass scripts MyXL.

---

## Verifikasi: Active Package Legit vs Decoy

Engsel sekarang punya **Menu 8 > 4: Verify Active Package** yang
otomatis test setiap paket aktif Anda dengan metode validation di atas.

Output:

- `[LEGIT]` — family_code resolve di server, paket asli
- `[DECOY]` — family_code tidak resolve, paket cosmetic only (kuota
  yang Anda lihat hanya di UI, server tidak tahu paket ini)

Banyak screenshot di grup TG dengan "validity 2099" sebenarnya **decoy
visual** dari override file `decoy_data/*.json`. Untuk benar-benar punya
paket validity 2099, harus aktivasi MPD via:

- App MyXL → "Upgrade ke PRIORITAS"
- Atau **Engsel Menu 8 > 9 > 7** (Aktivasi Kontrak MPD) — endpoint
  yang sama yang dipakai App MyXL di balik layar.

---

## Resources

- [me-cli-sunset](https://github.com/purplemashu/me-cli-sunset) — Python reference impl
- [engsel](https://github.com/moonspacelite/engsel) — C/OpenWrt impl
- [JADX](https://github.com/skylot/jadx) — APK decompiler
- [APKMirror](https://www.apkmirror.com/apk/pt-xl-axiata-tbk/myxl/) — APK archive
- [js-beautify](https://github.com/beautify-web/js-beautify) — JS deobfuscator
- [Frida CodeShare](https://codeshare.frida.re/) — SSL pinning bypass

---

*Dokumen ini bagian dari proyek engsel — CLI MyXL untuk OpenWrt.*
*Maintainer: 0x | Lisensi: MIT*
