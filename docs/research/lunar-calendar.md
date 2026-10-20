# Vietnamese lunar calendar

Date: 2026-10-20

Method: multi-agent research workflow: one research agent, then an adversarial verifier that re-checked each critical claim and the recommendation against primary sources

Status: verified: 9 confirmed / 1 partially / 0 refuted; the verifier says the recommendation holds

Notes: the settled decisions follow this report (own integer C code and 1900-2100 tables generated from Skyfield + DE440s, UTC+8 before 1968 and UTC+7 from 1968, no Ho Ngoc Duc code or tables, CI against HKO plus the Vietnam-specific years). The verifier's points on an HKO allowlist, a written time-scale and knife-edge policy, and the 1900-1928 Beijing-mean-time caveat are still open.

## Research report

### 1. Rules, and algorithm vs table

**Rules** (Hồ Ngọc Đức, HND): a month starts on the local day containing the new moon. Month 11 contains the winter solstice. In a 13‑month year, the first month after the solstice with no principal term (trung khí) is the leap month. The calculation uses the 105°E meridian ([calrules_en](https://www.xemamlich.uhm.vn/calrules_en.html)). HND's original Leipzig site now returns 404; only the mirror above and the Wayback Machine still have it.

**Time zones:** the North used UTC+8 until 1967 and UTC+7 from 1968. The South used UTC+8 until 1975. The whole country has used UTC+7 since 1976 ([histcal](https://www.xemamlich.uhm.vn/histcal.html)). Calendrical Calculations (CC) encodes Hanoi the same way: UTC+8 before 1968, UTC+7 after ([calendar.l](https://github.com/EdReingold/calendar-code2/blob/main/calendar.l)). The legal basis is [Decision 121‑CP](https://thuvienphapluat.vn/van-ban/Linh-vuc-khac/Quyet-dinh-121-CP-tinh-lich-quan-ly-lich-cua-Nha-nuoc-18212.aspx) of 8/8/1967.

| | HND astronomical (simplified Meeus) | Precomputed table |
|---|---|---|
| Accuracy | HND says this published version is less accurate than his own program ([calrules](https://www.xemamlich.uhm.vn/calrules.html)). **My diffs:** it matches HND's table for every month 1968–2053. Run at UTC+8, it matches HKO for every month 1929–2100 except 2097‑08. Its solar-term dates are one day off for 30 of 4128 HKO terms. | As good as its generator. HND's tables TK19–TK22 cover 1800–2199 ([amlich-hnd.js](https://www.xemamlich.uhm.vn/JavaScript/amlich-hnd.js)). Before 1968 they follow the historical (≈Chinese) calendar and equal HKO except in 1906‑04 and 1925‑01. |
| Cost | About 100 lines. Needs `double`, which ESP32‑S3 emulates in software ("very slow", [Espressif](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/performance/speed.html)). | 24 bits per year (Tết offset, month lengths, leap month). About 0.6 KB for 1900–2100. Integer-only, constant time. |
| Pitfalls | The lunar→solar code lets day 30 overflow a 29-day month and rejects a leap month 12 ([doxuta](https://github.com/doxuta/amlich/blob/main/DESIGN.md); I confirmed both in the code). | Fixed year range. |

**Knife-edge months:** in these, the new moon falls within about 75 s of Hanoi midnight. I checked each with [JPL Horizons](https://ssd-api.jpl.nasa.gov/doc/horizons.html) (DE441, UTC with no new leap seconds):

| Month | New moon, Hanoi time (Horizons) | Month starts | Disagreeing source |
|---|---|---|---|
| 2054 m4 | 00:01:00 | 8/5 | HND table: 7/5 |
| 2072 m11 | 23:59:56 | 9/12 | HND algorithm: 10/12 |
| 2077 m10 | 00:01:14 | 16/11 | HND table: 15/11 |
| 2085 m9 | 00:01:15 | 19/10 | HND table and algorithm: 18/10 |

The answer depends on future ΔT and UTC policy ([CGPM 2022](https://www.bipm.org/en/cgpm-2022/resolution-4)), so set a policy and pin these dates.

### 2. VN≠CN test vectors

I compared HND's table with [HKO 1901–2100](https://www.hko.gov.hk/en/gts/time/conversion1_text.htm). Of 1645 months in 1968–2100, 58 start on a different day and 7 have a different number.
- **Tết:** 1968 29/1 vs 30/1; 1969 16/2 vs 17/2; **1985 21/1 vs 20/2**; 2007 17/2 vs 18/2; 2030 2/2 vs 3/2; 2053 18/2 vs 19/2.
- **Leap month:**
  - China has leap‑10 in 1984; Vietnam has leap‑2 in 1985.
  - 1987: Vietnam leap‑7, China leap‑6.
  - Same leap month but different start: 8/1995 (24/9 vs 25/9) and 3/2031 (21/4 vs 22/4).
  - Both calendars have leap‑11 in 2033.
- **Holidays:** Trung Thu 2034 is 26/9 vs 27/9.
- Apple's ICU "vietnamese" calendar ([vietnamesecal.cpp](https://github.com/apple-open-source/macos/blob/master/ICU/icu/icu4c/source/i18n/vietnamesecal.cpp), tested on macOS 27) puts Tết 1985 on 20/2 and labels two consecutive months "11". No reference here is infallible.

### 3. Implementations and licenses

- **HND's license (2004):** *"Permission to use, copy, modify, and redistribute this software and its documentation for personal, non-commercial use is hereby granted provided that this copyright notice appears in all copies."* The 2006 [amlich-aa98.js](https://github.com/vanng822/amlich/blob/master/lib/amlich-aa98.js) has the same terms plus "and appropriate documentation".
- Because of the non-commercial term, this is neither OSI- nor GPL-compatible. The GNOME extension rewrote its code from scratch for that reason ([NOTICE](https://github.com/tdn2kit/gnome-amlich/blob/main/NOTICE.md)).
- **Ports that inherit the restriction:**
  - [klunar](https://github.com/khoanguyen-3fc/klunar), a C app for Flipper Zero: MIT code, but the HND tables it ships are marked "non-commercial".
  - npm `amlich`: no license.
  - MIT/BSD ports of HND's code ([lunar.h](https://github.com/hnthap/lunar.h), [vnlunar](https://github.com/nguyen703/vnlunar)): unclear provenance.
- **Clean options:** CC's Lisp code (Apache‑2.0); [doxuta/amlich](https://github.com/doxuta/amlich) (Go, MIT).
- Calendar rules and dates are facts, so write fresh code and generate your own table. My reading (not legal advice) is that HND's compiled table may also be covered by EU database rights.

### 4. Can Chi, tiết khí, holidays

**Can Chi formulas** (HND):

| | Can | Chi |
|---|---|---|
| Year | (Y+6)%10 | (Y+8)%12 |
| Month | (12Y+M+3)%10 | (M+1)%12 |
| Day | (JDN+9)%10 | (JDN+1)%12 |
| Hour | Tý hour: (JDN−1)·2%10, then add the branch index | — |

- A leap month uses the name of the month before it, plus "nhuận".
- Test vectors: 1/1/2000 is a Mậu Ngọ day. 20/10/2026 is 11/9 Bính Ngọ, an Đinh Mão day, in a Mậu Tuất month.

**Tiết khí:** a new term begins every 15° of apparent solar longitude (0° = Xuân phân). The trung khí are the multiples of 30°. Use UTC+7 dates; HKO's are UTC+8.

**Paid public holidays** (Labor Code 2019, Art. 112, [text](https://luatvietnam.vn/lao-dong/bo-luat-lao-dong-2019-179015-d1.html)):
- 1/1
- Tết, 5 days
- Giỗ Tổ, 10/3 lunar
- 30/4
- 1/5
- 2/9 plus one adjacent day
- 24/11 (Ngày Văn hóa), a paid day off from 1/7/2026 under the National Assembly resolution of 24/4/2026 ([VnExpress](https://vnexpress.net/chot-ngay-van-hoa-viet-nam-24-11-nguoi-lao-dong-nghi-huong-luong-5066507.html)). The matching Labor Code amendment is UNVERIFIED.

**Observances (not days off):**
- Lunar: Rằm tháng Giêng 15/1, Hàn Thực 3/3, Phật Đản 15/4, Đoan Ngọ 5/5, Vu Lan 15/7, Trung Thu 15/8, Ông Công Ông Táo 23/12, and Giao thừa on 29 or 30/12. These are counted in the regular month, never the leap month.
- Solar: 8/3, 20/10, 20/11, 22/12.

### 5. Test strategy

1. **Generator:** a host-side Python script (Skyfield with DE440s, or Horizons) writes the C tables and a golden CSV.
2. **CI checks:**
   - At UTC+8, output must equal HKO for 1929–2100, including solar terms.
   - At UTC+7, output must equal HND's table for 1968–2053 and reproduce the VN≠CN list.
   - From 2054 on, only the documented knife-edge months may differ. Spot-check them against the [NASA catalog](https://eclipse.gsfc.nasa.gov/phase/phases2001.html) and Horizons.
3. **Host unit tests:**
   - Round-trip every day from 1900 to 2100.
   - Invariants: months have 29 or 30 days, years have 12 or 13 months, month 11 contains Đông chí, and the leap month has no trung khí.
   - The Can Chi and holiday test vectors.

## Recommendation

On the device, write new integer-only C code under a permissive license, driven by two lookup tables covering at least 1900–2100: a 24-bit record per year (Tết offset, 12 month-length bits, leap month and its length; about 0.6 KB) and the day of each of the 24 tiết khí per year (about 4.8 KB). Can Chi then comes from simple modular arithmetic and holidays from a small rule table. Build both tables offline with your own Python script, using a high-precision ephemeris (Skyfield with DE440s, or JPL Horizons) and the CC/HND rules (UTC+8 before 1968, UTC+7 from 1968), and do not copy Hồ Ngọc Đức's non-commercial code or tables. Check the generator in CI: at UTC+8 it must reproduce HKO for 1929–2100, and at UTC+7 it must reproduce HND's table for 1968–2053 plus the Vietnam≠China cases (1968, 1969, 1984–85, 1987, 2007, 2030, 2053), with the four knife-edge months (2054, 2072, 2077, 2085) fixed by a written policy. On the firmware side, add host tests: an exhaustive 1900–2100 round-trip, the structural invariants, and golden test vectors.

## Open questions

- Which body computes Vietnam's official lunar calendar today? Decision 121-CP (1967) reportedly gave the task to the Nha Khí tượng (meteorological service), but I could not fetch the decree text (the site blocks automated access) and found no current, machine-readable official almanac. With no official arbiter, the policy for the knife-edge months (2054-05, 2072-12, 2077-11, 2085-10) is a judgment call: match HND's table, which is compatible with most Vietnamese sites and apps, or use the JPL/UTC result.
- Policy before 1968: HND's table differs from HKO in 1906-04 and 1925-01 (source unknown), and South Vietnam used UTC+8 until 1975 (Tết 1968 was 30/1 in the South and 29/1 in the North). Should the app show the North's official calendar, or limit its range to 1968 or 1976 onward?
- Did the Labor Code amendment adding 24/11 to Article 112 pass at the October 2026 National Assembly session? Secondary sources give the April 2026 resolution as No. 28/2026/QH16. UNVERIFIED.
- Conventions with no single standard: the Can Chi of the 23:00–24:00 half of giờ Tý (current day or next day), and whether a leap month has its own Can Chi (HND reuses the preceding month's).
- On-device cost of the double-precision astronomical algorithm on ESP32-S3 was not benchmarked (UNVERIFIED). This only matters if a runtime fallback beyond the table range is wanted.
- Legal: whether HND's compiled table is covered by EU sui generis database rights, and whether MIT/BSD ports translated from HND's code are lawfully licensed. This is not legal advice.
- The full list of national commemorative days (Decree 145/2013/NĐ-CP and later amendments) was not verified in detail.

## Critical claims and verification

### C1: confirmed

**Claim:** Ho Ngoc Duc's original code is licensed only for personal, non-commercial use: 'Copyright 2004 Ho Ngoc Duc ... Permission to use, copy, modify, and redistribute this software and its documentation for personal, non-commercial use is hereby granted provided that this copyright notice appears in all copies.' The 2006 amlich-aa98.js adds 'and appropriate documentation'.

**Source:** https://www.xemamlich.uhm.vn/JavaScript/amlich-hnd.js (mirror of the 404'd informatik.uni-leipzig.de/~duc/amlich/); https://github.com/vanng822/amlich/blob/master/lib/amlich-aa98.js

**Why it matters:** His code and TK tables cannot be put in an OSI or GPL licensed repo, so the code must be written fresh and the table generated by your own tool. The GNOME extension rewrote its code for the same reason.

**Correction:** No correction. Only the byte-for-byte match between the mirror and the Leipzig original is UNVERIFIED.

**Evidence:** I downloaded the mirror's JavaScript/amlich-hnd.js (Version 0.8, with the TK19–TK22 tables for 1800–2199). It opens with the exact 2004 notice that was quoted. The 2006 file vanng822/amlich lib/amlich-aa98.js says 'Copyright (c) 2006 Ho Ngoc Duc ... for personal, non-commercial use ... provided that this copyright notice and appropriate documentation appears in all copies'. Neither that repo nor npm 'amlich' 0.0.2 has a license. Independent corroboration: klunar's lib/klunar/klunar.c says 'Copyright 2004 Ho Ngoc Duc, used for non-commercial purposes' even though the repo is MIT. tdn2kit/gnome-amlich (GPL-2.0) NOTICE.md says lib/lunar.js was 'viết lại hoàn toàn' because HND's 'chỉ dùng cá nhân, phi thương mại' license is not compatible with the GPL or with extensions.gnome.org. The Leipzig original returns 404. The Wayback availability API lists 2025-01 snapshots, but downloading them returned HTTP 429, so the mirror was not byte-compared with the original.

### C2: confirmed

**Claim:** The Vietnamese rules: a month starts on the local day containing the new moon, the winter solstice falls in month 11, the first month after it with no principal term is the leap month, and calculation uses the 105°E meridian. Documented Vietnam≠China cases: Tết 1985 was 21/1 in Vietnam vs 20/2 in China, and 2007, 2030 and 2053 differ by one day.

**Source:** https://www.xemamlich.uhm.vn/calrules_en.html

**Why it matters:** This defines the algorithm the generator implements and supplies the core golden test vectors.

**Evidence:** calrules_en.html states the rules: the month starts on the day of the New Moon; 12 or 13 months; the Winter Solstice always falls in month 11; the first month after the solstice with no Principal Term is the leap month; calculations use 105°E. The same page gives Tết 1985 as 21/01 (VN) vs 20/02 (CN), and 2007 (17/2 vs 18/2), 2030 (2/2 vs 3/2) and 2053 (18/2 vs 19/2). I built the calendar from these rules using DE440s events. The result matches HND's table for every month from 1929 to 2053.

### C3: confirmed

**Claim:** Zone history: North Vietnam computed the calendar at UTC+8 until 1967 and at UTC+7 from 1968. The South stayed on UTC+8 until 1975, and the whole country has used UTC+7 since 1976. Calendrical Calculations encodes vietnamese-location as Hanoi at UTC+8 before 1968 and UTC+7 after.

**Source:** https://www.xemamlich.uhm.vn/histcal.html ; https://github.com/EdReingold/calendar-code2/blob/main/calendar.l

**Why it matters:** The generator must switch time zone at 1968, or the app's range must be restricted. A proleptic UTC+7 calendar disagrees with historical dates such as Tết 1965 and 1968 in the South.

**Correction:** There is one caveat. The same histcal page says Vietnam used the Qing-derived Hiệp Kỷ calendar, identical to China's, from 1813 to 1945. China computed on Beijing mean time before 1929 (CC's own chinese-location uses 1397/180 h). So 'UTC+8 before 1968' does not reproduce the historical calendar for 1900–1928.

**Evidence:** histcal.html, 1955–1975 period: the North used zone 8 until 1967 and zone 7 from 1968; the South used zone 8; from 1976 the whole country used zone 7. calendar.l (vietnamese-location): z = 8 if tee < (gregorian-new-year 1968), otherwise 7, at Hanoi 21°02′N 105°51′E. A proleptic UTC+7 calendar differs from HND's table for 1900–1967 in 23 start days and 6 month numbers, including Tết 1965 (1/2 vs 2/2). Tết 1968 is 29/1 at UTC+7 and 30/1 at UTC+8.

### C4: confirmed

**Claim:** The Calendrical Calculations reference code (CALENDRICA 4.0) is Apache-2.0 licensed and includes a vietnamese-location parameter for its Chinese-calendar functions.

**Source:** https://github.com/EdReingold/calendar-code2/blob/main/calendar.l

**Why it matters:** It is a legally clean, authoritative algorithm to port or compare against in place of Ho Ngoc Duc's code.

**Correction:** There are three caveats. (1) The Apache header dates only from commit e53c2d91 (2021-12-09, 'Harmonize license header with license file'). That commit removed an older 'personal use / non-commercial re-use only' license text, so older CALENDRICA copies and ports are not clean. (2) For 2051–2150, ephemeris-correction uses -20+32u²+0.5628(2150−y). The NASA polynomial it cites uses −0.5628(2150−y), so ΔT jumps from about 93 s in 2050 to about 206 s in 2051. (3) The astronomy is series-based. CC is authoritative for the rules, not for knife-edge instants.

**Evidence:** github.com/EdReingold/calendar-code2 has an Apache-2.0 LICENSE (the GitHub API reports spdx Apache-2.0). The calendar.l header reads 'CALENDRICA 4.0 -- Common Lisp ... Licensed under the Apache License, Version 2.0', and the file defines and exports vietnamese-location.

### C5: confirmed

**Claim:** HKO publishes official Gregorian–lunar (Chinese, UTC+8) conversion tables with solar-term dates for 1901–2100. My diff: Ho Ngoc Duc's simplified algorithm run at UTC+8 reproduces every HKO month for 1929–2100 except 2097-08. At UTC+7 it matches Ho Ngoc Duc's own table for every month 1968–2053.

**Source:** https://www.hko.gov.hk/en/gts/time/conversion1_text.htm

**Why it matters:** Running the generator at UTC+8 lets it be checked against an independent official table, which in turn validates its UTC+7 output.

**Correction:** Two nuances. HKO's 1901–1928 rows follow Beijing mean time, not UTC+8: DE440s at 116°25′E matches HKO except 1906-04, while at UTC+8 it misses 1914-11, 1916-02 and 1920-11. Also, T2069e.txt is missing the row for 2069-12-30; the Chinese file T2069c.txt has it.

**Evidence:** HKO's conversion1_text.htm links T1901e.txt through T2100e.txt, which include a solar-term column, and I downloaded all 200 files. I ported amlich-aa98.js to Python and diffed it. At UTC+8 against HKO for 1929–2100 (2128 months), the only difference is 2097-08 (algorithm 8/8, HKO 7/8). At UTC+7 against HND's decoded table there are no differences from 1968 to 2053; the first ones are 2054-05, 2072-12 and 2077-11. I also reproduced the report's figure that HND's algorithm gets 30 of 4128 HKO solar terms one day off.

### C6: confirmed

**Claim:** Ho Ngoc Duc's Vietnamese table vs HKO's Chinese table for 1968–2100: of 1645 months, 58 start on a different day and 7 are numbered differently. Tết differs in 1968, 1969, 1985, 2007, 2030 and 2053. Leap months differ in 1984/85 (China leap 10 in 1984, Vietnam leap 2 in 1985) and 1987 (Vietnam leap 7, China leap 6).

**Source:** https://www.xemamlich.uhm.vn/JavaScript/amlich-hnd.js + https://www.hko.gov.hk/en/gts/time/conversion1_text.htm (my computed diff)

**Why it matters:** These are the golden test vectors, and they show that no Chinese-calendar library (UTC+8) is correct for Vietnam.

**Evidence:** I decoded HND's TK20/TK21/TK22 tables and diffed them against HKO for 1968–2100. Result: 1645 months, 58 with a different start day and 7 numbered differently (1984-11-23, 1984-12-22, 1985-01-21, 1985-02-20, 1985-03-21, 1987-07-26, 1987-08-24). Tết differs only in 1968 (29/1 vs 30/1), 1969 (16/2 vs 17/2), 1985 (21/1 vs 20/2), 2007, 2030 and 2053. China has leap 10 in 1984 and Vietnam leap 2 in 1985 (from 21/3). In 1987 Vietnam has leap 7 (24/8) and China leap 6 (26/7). The report's other vectors also check out: leap 8 in 1995 (24/9 vs 25/9), leap 3 in 2031 (21/4 vs 22/4), leap 11 in 2033 in both, Trung Thu 2034 (26/9 vs 27/9), and 20/10/2026 = 11/9 Bính Ngọ, a Đinh Mão day in a Mậu Tuất month.

### C7: partially

**Claim:** Four months between 2054 and 2085 have a new moon within about 75 s of Hanoi midnight (JPL Horizons, DE441, UTC): 2054-05 at 00:01:00, 2072-12 at 23:59:56, 2077-11 at 00:01:14 and 2085-10 at 00:01:15. Ho Ngoc Duc's table, his simplified algorithm, the NASA/Espenak catalog and Apple's ICU disagree on these months.

**Source:** https://ssd-api.jpl.nasa.gov/doc/horizons.html ; https://eclipse.gsfc.nasa.gov/phase/phases2001.html

**Why it matters:** No method is right by construction here. These months need an explicit, documented policy, which a generated table makes visible and testable.

**Correction:** Apple is wrongly listed as a dissenter. Calendar(.vietnamese) on macOS 27 gives 8/5, 9/12, 16/11 and 19/10, which equals Horizons on all four months. The NASA/Espenak catalog cannot settle these cases: it prints UT rounded to the minute ('17:00' = 00:00 ICT for 2054, 2077 and 2085; '16:59' for 2072) and uses a ΔT of 2–3 minutes. The real disagreement is between HND's table and algorithm on one side and Horizons, DE440s and Apple on the other. All four dates are the same under Skyfield's UT1/ΔT model.

**Evidence:** Horizons API (DE441; header says 'the last known leap-second is used as a constant over future intervals'): I took Moon and Sun ObsEcLon at 1-minute steps and interpolated the crossings. New moons in Hanoi time: 2054-05-08 00:01:00.63, 2072-12-09 23:59:56.44, 2077-11-16 00:01:14.58, 2085-10-19 00:01:15.92. Skyfield with DE440s agrees within 0.02 s. No other new moon in 1968–2100 falls within ±76 s of Hanoi midnight; the next closest are 2062-12-30 (−149 s) and 2078-11-04 (−160 s). HND's table differs on 2054, 2077 and 2085 (7/5, 15/11, 18/10). HND's algorithm differs on 2072 and 2085 (10/12, 18/10).

### C8: confirmed

**Claim:** ESP32-S3 has only a single-precision hardware FPU. Double-precision math is emulated in software and is 'very slow'.

**Source:** https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/performance/speed.html

**Why it matters:** Julian-day astronomical code needs double precision, so an integer-only table at runtime is simpler and deterministic and wakes fast from deep sleep.

**Correction:** Speed is a weak reason. One conversion per wake probably costs milliseconds (UNVERIFIED on hardware). The stronger reasons for the table are determinism and testability.

**Evidence:** ESP-IDF stable docs (now v6.1), ESP32-S3 'Maximizing Execution Speed': 'Even though ESP32-S3 has a single precision hardware floating point unit...' and 'Avoid using double precision floating point arithmetic double. These calculations are emulated in software and are very slow.'

### C9: confirmed

**Claim:** Paid public holidays under Labor Code 2019 Art. 112: 1/1 (1 day), Tết Âm lịch (5 days), 30/4 (1), 1/5 (1), 2/9 plus one adjacent day (2), and 10/3 lunar Giỗ Tổ Hùng Vương (1). From 1/7/2026, 24/11 Ngày Văn hóa Việt Nam is also a paid day off under the National Assembly resolution of 24/4/2026.

**Source:** https://luatvietnam.vn/lao-dong/bo-luat-lao-dong-2019-179015-d1.html ; https://vnexpress.net/chot-ngay-van-hoa-viet-nam-24-11-nguoi-lao-dong-nghi-huong-luong-5066507.html

**Why it matters:** This sets the official versus observance flags in the holiday table. 24/11 is new in 2026.

**Correction:** Add two facts. The resolution is NQ 28/2026/QH16. Art. 112(3) lets the Prime Minister set the specific Tết days and the adjacent 2/9 day each year. For example, VPCP letter 10065/VPCP-KGVX of 2/10/2026 set Tết 2027 as 4–10/2/2027 and 24/11/2026 (a Tuesday) as a single day off.

**Evidence:** Official signed PDF of Law 45/2019/QH14 (datafiles.chinhphu.vn/cpp/files/vbpq/2019/12/45.signed.pdf, pp. 41–42), Art. 112(1): 1/1 (1 day), Tết Âm lịch (5), 30/4 (1), 1/5 (1), Quốc khánh (2: 2/9 plus 1 day before or after), Giỗ Tổ 10/3 lunar (1). Resolution 28/2026/QH16 (text PDF; passed 24/4/2026 at the 1st session of the 16th National Assembly), Art. 2(1): 'Ngày 24 tháng 11 hằng năm là Ngày Văn hóa Việt Nam; người lao động được nghỉ làm việc và hưởng nguyên lương'. Art. 13(1) makes it effective 01/7/2026. VnExpress (24/4/2026) agrees and says the Labor Code amendment is expected at the 2nd session; I could not confirm that it has passed.

### C10: confirmed

**Claim:** Ho Ngoc Duc states that the algorithms published on his rules page are heavily simplified and less precise than his online calendar program.

**Source:** https://www.xemamlich.uhm.vn/calrules.html

**Why it matters:** Porting the simplified algorithm is not the same as matching his table, which supports the generator-plus-table approach.

**Evidence:** calrules.html (last modified 18/08/2008): 'Các thuật toán mô tả ở đây đã được đơn giản hóa nhiều ... do đó độ chính xác của chúng thấp hơn độ chính xác của chương trình âm lịch trực tuyến tại http://www.informatik.uni-leipzig.de/~duc/amlich/'. My diffs agree: the algorithm differs from HND's own table in 2054-05, 2072-12 and 2077-11. At UTC+7 it also gives a different tiết khí day than DE440s on 24 of 3192 term days in 1968–2100, always one day early.

## Verifier's assessment of the recommendation

The recommendation holds, but some parts need changes. I built the proposed generator myself: Skyfield 1.55 with DE440s and HND's rules, UTC+8 before 1968 and UTC+7 after, UT1 before 1972, and UTC with no new leap seconds after 2026. It matches HND's table for every month from 1929 to 2053, so all the Vietnam≠China vectors are reproduced. After 1968 it differs from HND only in 2054-05, 2077-11 and 2085-10, and in those months it agrees with Horizons and with Apple's macOS 27 Vietnamese calendar. No month number or leap month changes when the post-2026 events are shifted by up to ±180 s.

Two parts of the plan don't work as written. First, the HKO check will fail. Against HKO for 1929–2100 the generator differs on 2 month starts: 2057-09 (new moon at 00:00:44 UTC+8, HKO gives 28/9) and 2097-08 (00:01:49, HKO gives 7/8). It also differs on 2 solar-term days: Đại hàn 1979 (23:59:54 UTC+8 on 20/1, HKO gives 21/1) and Xuân phân 2084 (00:00:47 on 20/3, HKO gives 19/3). The check therefore needs a documented allowlist. Second, the time-scale policy has to be written down for both the past and the future, and it has to cover tiết khí near midnight as well as new moons.

A decision is also needed for 1900–1928. The options are Beijing mean time or starting the table at 1929 (or 1968). Finally, the holiday table can flag occasions, but it cannot say which days are actually off.

All work stayed in a temporary folder (downloads, a venv and scripts such as sky2.py and sens.py). No project files were touched.

## What the research missed (verifier)

1) HKO data quirks. As shown above, the generator differs from HKO on 2 months and 2 terms; Skyfield and Apple ICU agree with each other there, against HKO. The English T2069e.txt also drops the row for 2069-12-30. Pin checksums for the 200 HKO files rather than fetching them live in CI.

2) Pre-1972 time scale. Skyfield's .utc assumes TAI−UTC = 10 s before 1972, which is about 12.5 s off UT1 in 1951. That alone moves the 1951 Đông chí (UT1 16:00:01.5) from 23/12, which is HKO's date, to 22/12 at UTC+8. The generator must use UT1 = TT−ΔT before 1972 and a stated rule after that. Horizons holds TT−UTC at 69.184 s for the future; Skyfield's ΔT model gives the same four knife-edge dates.

3) Knife-edge solar terms. The tiết khí table has its own near-midnight cases, besides the four months: Đại thử 2038-07-22 at 23:59:56 ICT (−3.9 s), Lập thu 2060-08-06 at 23:59:38 and Hàn lộ 2098-10-07 at 23:59:50. Within 2 minutes there are also 2041-07-06, 2074-08-23, 2076-03-05 and 2080-06-04. The generator should automatically flag every event within N seconds of local midnight, instead of hard-coding four months.

4) 1900–1928 is not historical under 'UTC+8 before 1968'. HND's own history page says Vietnam used the Chinese calendar from 1813 to 1945, and China then computed on Beijing mean time. At UTC+8 the generator gives Tết 1916 as 4/2 (HND and HKO: 3/2), and 1914-11 and 1920-11 one day later than both references. It gives 1906-04 as 24/4 (HND: 23/4). DE440s at Beijing mean time matches HKO for 1901–1928 except 1906-04. HND's Tết 1925 of 25/1 has no astronomical support (the new moon was 24/1/1925 at 14:45 UT, and HKO gives 24/1). For 1968–1975 the South followed UTC+8 (Tết 1968 on 30/1, 1969 on 17/2); document that the table encodes only the North's calendar.

5) CC is not a precision oracle. Because of the ΔT sign discrepancy, a CC port run as-is would probably side with HND's table on 2054, 2077 and 2085 (inferred from the size of the shift; UNVERIFIED, not executed). Port only from the Apache version, after commit e53c2d91.

6) CI licensing hygiene. Do not vendor HND's TK arrays as test fixtures. Either run the comparison offline or fetch the table at test time with a checksum (the Leipzig original is already 404), and commit only the derived dates. The proposed 24-bit record uses HND's exact TK bit layout. Reusing the format is fine; the values must come from your own generator.

7) Holidays. Art. 112(3) lets the Prime Minister set the Tết and 2/9 days off each year, and the nghỉ bù rule in Art. 111(3) also applies. Firmware can mark occasions, but it cannot show official days off without data it can update. The 24/11 holiday currently rests on NQ 28/2026/QH16 (its Art. 13 makes it prevail over conflicting laws), not on the Labor Code.

8) Visible disagreement with other apps. Most Vietnamese web calendars use HND's simplified sun formula, which has no aberration or nutation correction. At UTC+7 it lands one day early on 24 tiết khí days in 1968–2100, for example Thanh minh 2034 (4/4 vs 5/4), Tiểu tuyết 2040 and Đại hàn 2041. The device will disagree with those apps on these days.

9) Range edge. 1–30 January 1900 belong to lunar year 1899, so a 1900–2100 round-trip needs the 1899 record. Tết offsets for 1899–2100 run from 20 to 50, so they fit in 6 bits. No leap month 12 occurs in 1800–2199 (leap 11 appears only in 2033 within 1900–2100), so HND's leap-12 lunar-to-solar bug does not matter in this range.
