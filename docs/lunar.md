# Lunar calendar

The Watchface shows the Lunar date (âm lịch). `core/` computes it with integer code from tables that a Python script generates offline from the JPL DE440s ephemeris; nothing astronomical runs on the device. ADR-0015 records why the calendar is our own, and [research/lunar-calendar.md](research/lunar-calendar.md) holds the research behind it. This page is the reference: rules, time, algorithm, tables, how to regenerate, what was verified, the policies and the open questions.

| Path | What it is |
|---|---|
| `core/include/gg/gg_lunar.h`, `core/src/gg_lunar.c` | The interface and the integer code |
| `core/src/gg_lunar_data.h`, `core/src/gg_lunar_data.c` | Table layout (hand-written) and tables (generated) |
| `core/test/test_lunar.c` | Host tests |
| `core/test/data/lunar_months.csv` | Every month of lunar years 1900-2100, from the generator |
| `core/test/data/lunar_terms.csv` | The date of every solar term of 1900-2100, from the generator |
| `core/test/data/hko_month_starts.csv` | Month starts from the Hong Kong Observatory's tables 1901-2100 |
| `core/test/data/hko_solar_terms.csv` | Solar-term dates from the same tables |
| `core/test/data/hko_differences.csv` | The months in which Vietnam and HKO differ, with the reason |
| `core/test/data/hko_term_differences.csv` | The solar terms on which Vietnam and HKO differ, with the reason |
| `tools/lunar/` | The generator, the HKO fetcher, pinned requirements ([README](../tools/lunar/README.md)) |

## Rules

The Vietnamese calendar uses the same rules as the Chinese calendar, applied in Vietnam's time zone:

1. A month starts on the local date of its new moon: the instant at which the Sun and the Moon have the same apparent geocentric ecliptic longitude.
2. A solar term (tiết khí) begins every 15 degrees of the Sun's apparent longitude. The 12 at multiples of 30 degrees are the principal terms (trung khí). Term k begins at 15k degrees: 0 Xuân phân (March equinox), 6 Hạ chí, 12 Thu phân, 18 Đông chí (December solstice).
3. Month 11 is the month that contains the local date of Đông chí.
4. When 13 months separate two consecutive starts of month 11 (a sui with a leap month), the first of those months without a principal term is the leap month (tháng nhuận). It repeats the number of the month before it.
5. Lunar year Y begins with month 1 (Tết) in Gregorian year Y, between 21 January and 20 February; its months 11 and 12 start in November to January.

Two consequences are tested on every table: months have 29 or 30 days and years 12 or 13 months (353-355 or 383-385 days), and a leap month never holds a principal term. In 1900-2100 the leap month is one of 2-9 or 11; 2033 has the only leap month 11.

## Time

**Zones.** Dates are local: UTC+8 for instants before 1968-01-01 00:00 Hanoi time and UTC+7 from then on, the calendar of the North under Decision 121-CP (8 August 1967). Calendrical Calculations encodes Hanoi the same way. The switch date matters for two new moons, the nearest on either side whose date depends on the zone: 1967 month 11 (16:09:39 UT on 1 December 1967: 00:09 on 2/12 in UTC+8, 23:09 on 1/12 in UTC+7) and Tết 1968 (16:29:10 UT on 29 January 1968). Any switch after the first and up to the second gives the same tables, which start 1967 month 11 on 2/12 and Tết 1968 on 29/1; had UTC+7 applied from the decree date (8 August 1967), 1967 month 11 would start on 1 December. The decree's text is unverified; both sources in the research put the switch at the end of 1967 (histcal: the North used UTC+8 until 1967; Calendrical Calculations: UTC+7 from the Gregorian new year 1968). `generate.py` prints this window and fails if an event lies within two days of the switch; `test_zone_switch` pins both months.

- The South kept UTC+8 until 1975. Its calendar therefore differs from these tables in 1968 month 1 (Tết on 30 January, not 29), 1969 month 1 (Tết 17 February, not 16), 1974 month 11 and 1975 month 3. The tables hold the North's calendar.
- Before 1929 the Chinese calendar, which Vietnam followed until 1945, was computed on Beijing mean time (UT+7:45:40), not UTC+8. ADR-0015 fixes UTC+8 for all years before 1968, so the tables differ from the historical calendar in three months: 1914 month 10 (starts 18/11 instead of 17/11), 1916 month 1 (Tết 4/2 instead of 3/2) and 1920 month 10 (11/11 instead of 10/11). See the open questions.

**Time scales.** The ephemeris gives instants in TT; civil time comes from:

- before 1972: UT1 = TT - ΔT, from Skyfield's ΔT tables. Skyfield's UTC assumes TAI - UTC = 10 s before 1972, up to 13 s off the Earth's rotation in the 1950s; it would move Đông chí 1951 (00:00:01.5 UTC+8 in UT1) to 22/12, against HKO.
- from 1972: UTC with the announced leap seconds, and none after the last one (2016-12-31), so TT - UTC = 69.184 s in the future. JPL Horizons makes the same assumption, and the CGPM decided in 2022 to end leap seconds by 2035.
- If civil time instead followed UT1 after 1972 (UT1 - UTC reaches -10 s in 2072 and -27 s in 2100 in Skyfield's model), no month and no solar term in 1900-2100 would change date (`generate.py --sensitivity`).

The day boundary is local midnight, also for the Can Chi of the day.

## Algorithm

`tools/lunar/generate.py`, with Skyfield 1.55 and DE440s (32 MB, SHA-256 pinned):

1. Search 1899-06-01 to 2102-03-01 (TT) with `find_discrete` (1 ms precision) for every new moon (2508) and every solar term (4866). Longitudes are apparent (light time, aberration, light deflection, IAU 2000A nutation) on the true ecliptic and equinox of date.
2. Convert each instant to civil time (above) and to a local date in the chosen zone (`vn` for the tables; `utc8` and `china` for the comparisons).
3. For every sui from December 1899 to December 2101, find the two month-11 starts, count the months between them and place the leap month by the rules. Months 11 and 12 belong to the lunar year in which month 11 starts.
4. Check the rules again on the result, then write the files. The output is deterministic: the same inputs give byte-identical files, and `--check` fails when a committed file is stale.

The search window is wider than the tables: month 11 of 1899 precedes Tết 1900, and the sui that ends with Đông chí 2101 decides whether lunar year 2100 ends with a leap month (it does not).

## Tables

`gg_lunar_data.c` holds 2034 bytes:

- `gg_lunar_years`: one `uint32_t` per lunar year 1900-2100 (804 bytes). Bits 0-12 are the month lengths in calendar order with the leap month right after the month it repeats (1 = 30 days), bits 13-16 the leap month (0 = none), bits 17-22 the day of the Gregorian year of the same number on which Tết falls (0 = 1 January; the values run from 20 to 50).
- `gg_solar_terms`: the local date of the 24 solar terms of each Gregorian year 1900-2100, in date order from Tiểu hàn (about 6 January) to Đông chí. Each term always falls in the same Gregorian month (position p in month p/2 + 1), so 2 bits per term store its day as an offset of 0-3 from `gg_solar_term_base[p]`, the earliest day it takes in 1900-2100 (6 bytes per year, 1206 bytes, plus 24 base bytes).

The interface (`gg_lunar.h`) converts both ways, gives the leap month and month lengths, the Can Chi of a year, month or day, and the solar term in effect on a date. Conversions cover the Solar dates from Tết 1900 (1900-01-31) to 2100-12-31 and return false outside them; 1-30 January 1900 belong to lunar year 1899, which the tables do not hold. A conversion walks at most 13 month lengths, so it takes constant time. The reducer (`gg_app.c`) recomputes the Lunar date whenever the date in a TIME message changes.

**Can Chi.** Stems Giáp ... Quý are 0-9 and branches Tý ... Hợi 0-11.

| | Stem | Branch | Check |
|---|---|---|---|
| Lunar year Y | (Y + 6) mod 10 | (Y + 8) mod 12 | 2026 Bính Ngọ; year 4 was Giáp Tý |
| Month M of year Y | (12Y + M + 3) mod 10 | (M + 1) mod 12 | month 1 of a Giáp or Kỷ year is Bính Dần |
| Day with Julian Day Number J | (J + 9) mod 10 | (J + 1) mod 12 | 2000-01-01 (J = 2451545) Mậu Ngọ |

The months run through the 60 pairs 12 a year, so their stem advances with 12Y + M; the constant 3 comes from "Giáp Kỷ chi niên Bính tác thủ". A leap month has no pair of its own and reuses the pair of the month it repeats, as Vietnamese almanacs print it. Check value (ADR-0015): 2026-10-20 is day 11 of month 9 of Bính Ngọ, an Đinh Mão day in a Mậu Tuất month.

## Regenerating

From the repository root, after the setup in [tools/lunar/README.md](../tools/lunar/README.md):

```sh
tools/lunar/.venv/bin/python -I tools/lunar/fetch_hko.py   # only when the hko_*.csv test data must change
tools/lunar/.venv/bin/python -I tools/lunar/generate.py    # writes gg_lunar_data.c and the four generated CSV files
make lunar-check                                       # generate.py --check: fails if a committed file is stale
make test
```

Downloads (the ephemeris, the 200 HKO files) go to `$GG_CACHE_DIR`, by default `~/.cache/gogadget`, never into the repository. `fetch_hko.py` waits a second between requests and checks every file against `tools/lunar/hko-sha256.txt`. A different Skyfield, NumPy or ephemeris can change the tables: review the diff of `lunar_months.csv` and `lunar_terms.csv`, then run the verification commands below. `generate.py` stops instead of writing a difference from HKO that the time zone does not explain (or, for the three solar terms below, that `KNOWN_HKO_TERM_DEVIATIONS` does not list).

## Verification

### The rules at UTC+8 against HKO

HKO publishes the Chinese calendar for 1901-2100 ([conversion tables](https://www.hko.gov.hk/en/gts/time/conversion1_text.htm)). Run at UTC+8 (`generate.py --zone utc8 --compare-hko`), the generator reproduces it as follows:

| Compared | Count | Differences |
|---|---|---|
| Months of lunar years 1929-2100 | 2127 | 2: 2057 month 9 (new moon 2057-09-29 00:00:44.2 UTC+8, 44.2 s after midnight; HKO 28/9) and 2097 month 7 (2097-08-08 00:01:49.1, 109.2 s after; HKO 7/8) |
| Solar terms 1929-2100 | 4128 | 2: Đại hàn 1979 (1979-01-20 23:59:54.4 UTC+8, 5.6 s before midnight; HKO 21/1) and Xuân phân 2084 (2084-03-20 00:00:47.0, 47.1 s after; HKO 19/3) |
| Months of lunar years 1900-1928 | 348 | 3 at UTC+8 (1914 month 10, 1916 month 1, 1920 month 10, new moons 101-315 s after midnight). With Beijing mean time (`--zone china`) only 1906 month 4 differs (new moon 23:52:05 Beijing mean time, 00:06:25 UTC+8; HKO 24/4). |
| Solar terms 1901-1928 | 672 | 5 at UTC+8, 6 with Beijing mean time: HKO's early terms follow neither, by up to 26 minutes |

In all four exceptions from 1929 on, DE440s puts the event within two minutes of midnight, on the other side from HKO; the research found Apple's ICU calendar agreeing with DE440s on them, against HKO. None of the four is close to midnight in Hanoi time (they fall between 22:59 and 23:02 UTC+7), so they do not touch the Vietnamese tables.

### Vietnam against HKO

`core/test/test_lunar.c` compares every day from 1 January 1901 to 2100-12-31 with HKO's table; it starts on 1900-12-22, day 1 of the lunar month in progress on 1 January 1901, which `fetch_hko.py` dates from the lunar day of HKO's first row. 68 months give some day a different lunar date, all listed with their reason in `core/test/data/hko_differences.csv`; the test fails if one is missing or stale, if a reason is not one the time zone gives, or if the count is not 68. Changing that count needs a written reason on this page. Every one is explained by the time zone:

- **Before 1929, 6 months**: the 3 Beijing-mean-time new moons above, each making one month start a day later here and the month before it a day longer.
- **From 1968, 55 months start a day earlier in Vietnam**: the new moon falls between 23:00 and 24:00 Hanoi time, after midnight in UTC+8. Among them Tết 1968 (29/1, China 30/1), 1969 (16/2 vs 17/2), 2007 (17/2 vs 18/2), 2030 (2/2 vs 3/2) and 2053 (18/2 vs 19/2), and the leap months 8 of 1995 (24/9 vs 25/9) and 3 of 2031 (21/4 vs 22/4).
- **7 months are numbered differently**:
  - 1984-85: Đông chí 1984 fell at 23:22:48 on 21/12 in Hanoi and 00:22:48 on 22/12 in UTC+8, so China's month 11 began a month later and China inserted leap month 10 (from 23/11/1984). In Vietnam, Xuân phân 1985 fell at 23:13:43 on 20/3, so the month from 21/3/1985 had no principal term and became leap month 2. Tết 1985 was 21/1 in Vietnam and 20/2 in China. The months from 23/11/1984, 22/12/1984, 21/1/1985, 20/2/1985 and 21/3/1985 carry different numbers.
  - 1987: Xử thử fell at 23:09:50 on 23/8 in Hanoi, so the month from 26/7 has a principal term and the one from 24/8 has none. Vietnam has leap month 7 (from 24/8), China leap month 6 (from 26/7).

The research counted 58 start differences between Hồ Ngọc Đức's table and HKO for 1968-2100; this table has 55 because in 2054, 2077 and 2085 it agrees with HKO where his table does not (knife-edge months below). The 7 numbering differences are the same.

<details>
<summary>The 55 months from 1968 that start a day earlier in Vietnam</summary>

| Lunar month | Vietnam | China (HKO) | New moon, Hanoi time |
|---|---|---|---|
| 1968 1 | 1968-01-29 | 1968-01-30 | 23:29:10 |
| 1969 1 | 1969-02-16 | 1969-02-17 | 23:25:02 |
| 1974 11 | 1974-12-13 | 1974-12-14 | 23:24:44 |
| 1975 3 | 1975-04-11 | 1975-04-12 | 23:38:58 |
| 1978 8 | 1978-09-02 | 1978-09-03 | 23:08:53 |
| 1979 2 | 1979-02-26 | 1979-02-27 | 23:45:12 |
| 1984 5 | 1984-05-30 | 1984-05-31 | 23:47:50 |
| 1986 11 | 1986-12-01 | 1986-12-02 | 23:42:36 |
| 1989 7 | 1989-08-01 | 1989-08-02 | 23:05:35 |
| 1995 leap 8 | 1995-09-24 | 1995-09-25 | 23:54:44 |
| 1996 6 | 1996-07-15 | 1996-07-16 | 23:14:50 |
| 1996 11 | 1996-12-10 | 1996-12-11 | 23:56:17 |
| 1997 9 | 1997-10-01 | 1997-10-02 | 23:51:30 |
| 1997 12 | 1997-12-29 | 1997-12-30 | 23:56:31 |
| 2006 6 | 2006-06-25 | 2006-06-26 | 23:05:15 |
| 2007 1 | 2007-02-17 | 2007-02-18 | 23:14:18 |
| 2008 11 | 2008-11-27 | 2008-11-28 | 23:54:35 |
| 2009 3 | 2009-03-26 | 2009-03-27 | 23:05:55 |
| 2018 10 | 2018-11-07 | 2018-11-08 | 23:02:02 |
| 2019 2 | 2019-03-06 | 2019-03-07 | 23:03:58 |
| 2020 11 | 2020-12-14 | 2020-12-15 | 23:16:35 |
| 2030 1 | 2030-02-02 | 2030-02-03 | 23:07:31 |
| 2031 leap 3 | 2031-04-21 | 2031-04-22 | 23:57:06 |
| 2032 2 | 2032-03-11 | 2032-03-12 | 23:24:41 |
| 2034 8 | 2034-09-12 | 2034-09-13 | 23:13:50 |
| 2037 3 | 2037-04-15 | 2037-04-16 | 23:07:45 |
| 2038 3 | 2038-04-04 | 2038-04-05 | 23:43:02 |
| 2039 11 | 2039-12-15 | 2039-12-16 | 23:32:05 |
| 2041 8 | 2041-08-26 | 2041-08-27 | 23:16:13 |
| 2049 10 | 2049-10-26 | 2049-10-27 | 23:15:12 |
| 2051 2 | 2051-03-12 | 2051-03-13 | 23:52:39 |
| 2053 1 | 2053-02-18 | 2053-02-19 | 23:31:39 |
| 2056 4 | 2056-05-14 | 2056-05-15 | 23:06:37 |
| 2057 4 | 2057-05-03 | 2057-05-04 | 23:32:27 |
| 2058 11 | 2058-12-15 | 2058-12-16 | 23:12:15 |
| 2059 12 | 2060-01-03 | 2060-01-04 | 23:40:42 |
| 2060 2 | 2060-03-02 | 2060-03-03 | 23:12:08 |
| 2062 12 | 2062-12-30 | 2062-12-31 | 23:57:30 |
| 2065 8 | 2065-08-31 | 2065-09-01 | 23:40:00 |
| 2070 11 | 2070-12-02 | 2070-12-03 | 23:54:33 |
| 2072 11 | 2072-12-09 | 2072-12-10 | 23:59:56 |
| 2073 12 | 2073-12-28 | 2073-12-29 | 23:56:15 |
| 2078 10 | 2078-11-04 | 2078-11-05 | 23:57:20 |
| 2080 8 | 2080-09-13 | 2080-09-14 | 23:26:11 |
| 2082 5 | 2082-05-27 | 2082-05-28 | 23:48:30 |
| 2085 3 | 2085-03-25 | 2085-03-26 | 23:38:38 |
| 2086 3 | 2086-04-13 | 2086-04-14 | 23:54:37 |
| 2088 7 | 2088-08-16 | 2088-08-17 | 23:16:58 |
| 2090 12 | 2091-01-19 | 2091-01-20 | 23:32:32 |
| 2092 12 | 2092-12-28 | 2092-12-29 | 23:12:05 |
| 2093 4 | 2093-04-25 | 2093-04-26 | 23:14:14 |
| 2095 3 | 2095-04-04 | 2095-04-05 | 23:37:38 |
| 2099 5 | 2099-06-18 | 2099-06-19 | 23:12:13 |
| 2099 8 | 2099-09-14 | 2099-09-15 | 23:52:43 |
| 2100 3 | 2100-04-09 | 2100-04-10 | 23:18:32 |

</details>

### Solar terms against HKO

`core/test/test_lunar.c` checks every date of `core/test/data/lunar_terms.csv` through `gg_lunar_solar_term()`, so a one-day error in any of the 4824 terms fails, and compares each of HKO's 4800 terms of 1901-2100 (`hko_solar_terms.csv`) with the date the C code gives. 143 differ, all listed in `hko_term_differences.csv`; the count is pinned like the months':

- **140 by the time zone**: 137 from 1968 whose instant falls between 23:00 and 24:00 Hanoi time, after midnight in UTC+8, and 3 before 1929 (Đại tuyết 1917, Bạch lộ 1927, Hạ chí 1928) that fall before midnight in Beijing mean time and after it in UTC+8.
- **3 that HKO dates differently from DE440s in its own zone too**, listed in `KNOWN_HKO_TERM_DEVIATIONS` in `generate.py` and named in the test: Tiểu tuyết 1912 (DE440s 23:33:47 Beijing mean time on 22/11, HKO 23/11) and Thu phân 1913 (23:38:21 on 23/9, HKO 24/9), where HKO's early terms follow neither zone by 21-26 minutes; and Đại hàn 1979 (DE440s 5.6 s before midnight UTC+8 on 20/1, HKO 21/1), which is 22:59:54 in Hanoi, so 20/1 there whichever side of UTC+8 midnight the instant lies.

### Knife-edge events

Events close to local midnight, from `generate.py --margins 120` (time truncated to 0.1 s, margin rounded):

| Event | Local time | Margin | Note |
|---|---|---|---|
| New moon, 2054 month 4 | 2054-05-08 00:01:00.6 | +60.6 s | Hồ Ngọc Đức's table: 7/5 |
| New moon, 2072 month 11 | 2072-12-09 23:59:56.4 | -3.6 s | his algorithm: 10/12 |
| New moon, 2077 month 10 | 2077-11-16 00:01:14.5 | +74.6 s | his table: 15/11 |
| New moon, 2085 month 9 | 2085-10-19 00:01:15.9 | +75.9 s | his table and algorithm: 18/10 |
| New moon, 1998 month 8 | 1998-09-21 00:01:29.3 | +89.3 s | the closest in the past |
| New moon, 1914 month 10 | 1914-11-18 00:01:41.3 UTC+8 | +101.3 s | Beijing mean time: 17/11 |
| Đông chí 1951 | 1951-12-23 00:00:01.5 UTC+8 | +1.5 s | needs UT1 (above); HKO agrees |
| Đại thử 2038 | 2038-07-22 23:59:56.1 | -3.9 s | |
| Hàn lộ 2098 | 2098-10-07 23:59:50.0 | -9.9 s | |
| Lập hạ 1911 | 1911-05-07 00:00:16.0 UTC+8 | +16.0 s | |
| Vũ thủy 1923 | 1923-02-19 23:59:40.3 UTC+8 | -19.7 s | |
| Lập thu 2060 | 2060-08-06 23:59:37.7 | -22.2 s | |
| Hàn lộ 2003 | 2003-10-09 00:00:32.7 | +32.8 s | |
| Tiểu mãn 2004 | 2004-05-20 23:59:12.0 | -47.9 s | |
| Cốc vũ 1950 | 1950-04-20 23:59:05.5 UTC+8 | -54.4 s | |
| Đại tuyết 1917 | 1917-12-08 00:00:59.4 UTC+8 | +59.5 s | HKO: 7/12 (Beijing mean time) |
| Mang chủng 2080 | 2080-06-04 23:58:56.7 | -63.2 s | |

Further events within 120 s: Bạch lộ 2014 (+86.0 s), Tiểu thử 2041 (-87.0 s), Tiểu mãn 1917 (-88.9 s), Xử thử 2074 (+90.5 s), Hàn lộ 1970 (+92.4 s), Kinh trập 2076 (+114.7 s). The next new moons after the four knife-edge months are 2062-12-30 (-149.0 s) and 2078-11-04 (-159.6 s). The tests pin the four months and the six solar terms within 25 s of midnight.

### Host tests

`core/test/test_lunar.c` (18 tests, under 0.1 s): the check value with its Can Chi; the Tết and leap-month cases above; the knife-edge months and terms; the two months the zone switch decides; an exhaustive round trip of every day in the range, checking that each day follows the one before; the rules (month and year lengths, Đông chí in month 11 every year, no principal term in any of the 74 leap months, solar terms one at a time 14-16 days apart); every row of `lunar_months.csv` and `lunar_terms.csv`; and the HKO comparisons of months and solar terms. `core/test/test_app.c` checks that the reducer fills the Lunar date and the leap flag when the date changes, and the leap-month golden screenshot in `ui/test/test_screens.c` renders a real leap-month day (2025-07-27, day 3 of leap month 6 of Ất Tỵ).

## Policies

- **Knife-edge months follow DE440s.** A month starts on the local date of the JPL DE440s instant, in UTC with no leap second after 2016. For 2054 month 4, 2072 month 11, 2077 month 10 and 2085 month 9 this differs from Hồ Ngọc Đức's widely used table or simplified algorithm, so the device can disagree with Vietnamese web calendars on those months. JPL Horizons and Apple's Vietnamese calendar give the same dates as these tables.
- **The tables encode the North's calendar** with UTC+8 before 1968, also for 1900-1928 (ADR-0015). The South (until 1975) and the historical Chinese calendar before 1929 differ in the months listed under Time.
- **A leap month reuses the Can Chi of the month it repeats.**
- **No holidays yet.** Lunar holidays (Tết, Giỗ Tổ 10/3, Trung Thu 15/8, ...) belong to the calendar screen (roadmap v0.3) and are always counted in the regular month, never the leap month.

## Open questions

- **1900-1928.** Beijing mean time would reproduce HKO and the historical calendar for these years (except 1906 month 4) and move Tết 1916 to 3 February; UTC+8, as ADR-0015 says, does not. Changing it means editing ADR-0015 and the `vn` zone in `generate.py`.
- **Future leap seconds.** Leap seconds may still be inserted before 2035. Đại thử 2038 (-3.9 s) and 2072 month 11 (-3.6 s) change date only if UTC gains 4 or more negative leap seconds; every other event has at least 9.9 s of margin. If the leap-second rule changes, rerun `generate.py --sensitivity` and `--margins`.
- **The official calendar.** No current body publishes a machine-readable Vietnamese almanac to check the knife-edge months against (research, open questions).
- **HKO's 2057 and 2097 months** disagree with DE440s at UTC+8 by 44 and 109 s. They do not affect Vietnam, but a test against a future HKO table should expect them.
- **Hour Can Chi** and the convention for 23:00-24:00 (giờ Tý) are not implemented.
