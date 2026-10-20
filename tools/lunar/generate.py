#!/usr/bin/env python3
"""Generate the Vietnamese lunar calendar tables from the JPL DE440s ephemeris.

Skyfield finds every new moon and every solar term (each 15 degrees of the Sun's
apparent longitude) from mid-1899 to early 2102. Each instant becomes a local
date, and the rules in docs/lunar.md turn those dates into months. Writes:

  core/src/gg_lunar_data.c            compact tables for the firmware
  core/test/data/lunar_months.csv     every month of lunar years 1900-2100
  core/test/data/hko_differences.csv  the months in which this calendar and the
                                      Hong Kong Observatory's Chinese calendar
                                      differ, each with the event that explains it

Run it with the tool's venv (tools/lunar/README.md):

  generate.py                              regenerate the files above
  generate.py --check                      exit 1 if any of them is out of date
  generate.py --zone utc8 --compare-hko    the same rules at UTC+8, checked against HKO
  generate.py --margins 120                every event within 120 s of local midnight

The ephemeris is downloaded once into $GG_CACHE_DIR/jpl (default ~/.cache/gogadget).
"""

from __future__ import annotations

import argparse
import bisect
import csv
import datetime as dt
import hashlib
import io
import json
import math
import os
import platform
import ssl
import sys
import tempfile
import urllib.request
from dataclasses import dataclass
from pathlib import Path

import certifi
import jplephem
import numpy as np
import skyfield
from skyfield.api import Loader
from skyfield.framelib import ecliptic_frame
from skyfield.searchlib import find_discrete

REPO = Path(__file__).resolve().parents[2]
OUT_C = REPO / "core" / "src" / "gg_lunar_data.c"
OUT_MONTHS = REPO / "core" / "test" / "data" / "lunar_months.csv"
OUT_DIFFS = REPO / "core" / "test" / "data" / "hko_differences.csv"
HKO_MONTHS = REPO / "core" / "test" / "data" / "hko_month_starts.csv"

# Lunar years in the tables; core/include/gg/gg_lunar.h has the same numbers.
FIRST_YEAR = 1900
LAST_YEAR = 2100
LAST_SOLAR_DAY = dt.date(LAST_YEAR, 12, 31)  # the C code converts up to this date
# The search window (TT). Month 11 of 1899 precedes Tết 1900, and the sui that ends
# with the December 2101 solstice decides whether lunar year 2100 ends with a leap month.
SEARCH_FROM = (1899, 6, 1)
SEARCH_TO = (2102, 3, 1)

EPHEMERIS = "de440s.bsp"
EPHEMERIS_URL = "https://ssd.jpl.nasa.gov/ftp/eph/planets/bsp/de440s.bsp"
EPHEMERIS_SHA256 = "c1c7feeab882263fc493a9d5a5b2ddd71b54826cdf65d8d17a76126b260a49f2"

DAY_S = 86400
HOUR_S = 3600
EPOCH = dt.date(1970, 1, 1)
TAU = 2.0 * math.pi

# Solar terms, indexed by k: term k begins when the Sun's apparent longitude reaches
# 15k degrees. Even k are the principal terms (trung khí).
TERM_NAMES = (
    "Xuân phân", "Thanh minh", "Cốc vũ", "Lập hạ", "Tiểu mãn", "Mang chủng",
    "Hạ chí", "Tiểu thử", "Đại thử", "Lập thu", "Xử thử", "Bạch lộ",
    "Thu phân", "Hàn lộ", "Sương giáng", "Lập đông", "Tiểu tuyết", "Đại tuyết",
    "Đông chí", "Tiểu hàn", "Đại hàn", "Lập xuân", "Vũ thủy", "Kinh trập",
)  # fmt: skip
WINTER_SOLSTICE = 18
FIRST_TERM_OF_YEAR = 19  # Tiểu hàn (about 6 January) opens each row of the term table


def days(d: dt.date) -> int:
    """Days since 1970-01-01, like gg_days_from_civil()."""
    return (d - EPOCH).days


def date_of(day: int) -> dt.date:
    return EPOCH + dt.timedelta(days=day)


# ---- Time zones ------------------------------------------------------------------
#
# Instants are kept as seconds since 1970-01-01 00:00 on the civil time scale (see
# civil_seconds). A zone maps an instant to a UTC offset.

# Vietnam (the North; Decision 121-CP): UTC+8 until the end of 1967, UTC+7 from
# 1968-01-01 00:00 Hanoi time. The South kept UTC+8 until 1975; see docs/lunar.md.
VN_SWITCH_S = days(dt.date(1968, 1, 1)) * DAY_S - 7 * HOUR_S
# The Chinese calendar as HKO publishes it: Beijing mean time (116°25' E) before
# 1929, UTC+8 from then on. Only used to explain the HKO comparison.
CN_SWITCH_S = days(dt.date(1929, 1, 1)) * DAY_S - 8 * HOUR_S
BEIJING_MEAN_S = 7 * HOUR_S + 45 * 60 + 40

ZONES = {
    "vn": "UTC+8 before 1968-01-01, UTC+7 from then on (Vietnam, the tables)",
    "utc8": "UTC+8 throughout",
    "china": "Beijing mean time before 1929, UTC+8 from 1929 (HKO's Chinese calendar)",
}


def utc_offset_s(zone: str, civil_s: float) -> int:
    if zone == "vn":
        return 8 * HOUR_S if civil_s < VN_SWITCH_S else 7 * HOUR_S
    if zone == "utc8":
        return 8 * HOUR_S
    if zone == "china":
        return BEIJING_MEAN_S if civil_s < CN_SWITCH_S else 8 * HOUR_S
    raise ValueError(zone)


def offset_name(offset_s: int) -> str:
    if offset_s == BEIJING_MEAN_S:
        return "Beijing mean time"
    return f"UTC+{offset_s // HOUR_S}"


# ---- Events ----------------------------------------------------------------------


@dataclass(frozen=True)
class Event:
    term: int  # -1 for a new moon, otherwise the solar term k (longitude 15k degrees)
    tt: float  # Julian date, TT
    civil_s: float  # civil time scale: UT1 before 1972, UTC from 1972
    ut1_s: float  # UT1, for the sensitivity check

    @property
    def name(self) -> str:
        return "new moon" if self.term < 0 else f"solar term {TERM_NAMES[self.term]}"

    @property
    def principal(self) -> bool:
        return self.term >= 0 and self.term % 2 == 0

    def local(self, zone: str, ut1: bool = False) -> tuple[int, float]:
        """(day, seconds after local midnight) in a zone."""
        s = (self.ut1_s if ut1 else self.civil_s) + utc_offset_s(zone, self.civil_s)
        day = math.floor(s / DAY_S)
        return day, s - day * DAY_S

    def day(self, zone: str, ut1: bool = False) -> int:
        return self.local(zone, ut1)[0]

    def margin(self, zone: str) -> float:
        """Signed seconds from the nearest local midnight (negative: before it)."""
        sod = self.local(zone)[1]
        return sod if sod < DAY_S / 2 else sod - DAY_S

    def when(self, zone: str) -> str:
        day, sod = self.local(zone)
        sod = math.floor(sod * 10) / 10  # never round 23:59:59.96 up to the next day
        h, rest = divmod(sod, HOUR_S)
        m, s = divmod(rest, 60)
        zone_name = offset_name(utc_offset_s(zone, self.civil_s))
        return f"{date_of(day)} {int(h):02d}:{int(m):02d}:{s:04.1f} {zone_name}"


def cache_dir() -> Path:
    return Path(os.environ.get("GG_CACHE_DIR") or Path.home() / ".cache" / "gogadget")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def download(url: str, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    context = ssl.create_default_context(cafile=certifi.where())
    request = urllib.request.Request(url, headers={"User-Agent": "gogadget-lunar/1.0"})
    with urllib.request.urlopen(request, timeout=120, context=context) as response:
        with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as tmp:
            for chunk in iter(lambda: response.read(1 << 20), b""):
                tmp.write(chunk)
    os.replace(tmp.name, path)


def ephemeris_path() -> Path:
    path = cache_dir() / "jpl" / EPHEMERIS
    if not path.exists():
        print(f"downloading {EPHEMERIS_URL} into {path.parent}", file=sys.stderr)
        download(EPHEMERIS_URL, path)
    digest = sha256(path)
    if digest != EPHEMERIS_SHA256:
        sys.exit(f"{path}: SHA-256 {digest}, expected {EPHEMERIS_SHA256}")
    return path


def tool_versions() -> str:
    # Only Python's minor version: a patch release does not change the tables, and the
    # generated header must not go stale with every interpreter update.
    python = ".".join(platform.python_version_tuple()[:2])
    return (
        f"Python {python}, Skyfield {skyfield.__version__}, "
        f"jplephem {jplephem.__version__}, NumPy {np.__version__}"
    )


def civil_seconds(ts, t) -> np.ndarray:
    """Seconds since 1970-01-01 on the civil time scale.

    Before 1972 civil time followed the Earth's rotation, so this is UT1 = TT - ΔT
    (Skyfield's .utc would assume TAI - UTC = 10 s, up to 13 s off in the 1950s).
    From 1972 it is UTC with the announced leap seconds, and with none after the
    last one (TT - UTC = 69.184 s), as JPL Horizons assumes for the future.
    """
    ut1 = ut1_seconds(t)
    year, month, day, hour, minute, second = t.utc
    utc = np.array(
        [
            days(dt.date(int(y), int(mo), int(d))) * DAY_S + int(h) * 3600 + int(mi) * 60 + s
            for y, mo, d, h, mi, s in zip(year, month, day, hour, minute, second)
        ]
    )
    return np.where(t.tt < ts.utc(1972, 1, 1).tt, ut1, utc)


def ut1_seconds(t) -> np.ndarray:
    return ((t.whole - 2440587.5) + t.ut1_fraction) * DAY_S


def search_events() -> list[Event]:
    """Every new moon and solar term in the search window, sorted by time."""
    eph_path = ephemeris_path()
    key = hashlib.sha256(
        json.dumps([EPHEMERIS_SHA256, tool_versions(), SEARCH_FROM, SEARCH_TO, 1]).encode()
    ).hexdigest()[:16]
    memo = cache_dir() / "events" / f"events-{key}.json"
    if memo.exists():
        rows = json.loads(memo.read_text())
        return [Event(*row) for row in rows]

    load = Loader(str(eph_path.parent), verbose=False)
    ts = load.timescale(builtin=True)
    eph = load(EPHEMERIS)
    earth, sun, moon = eph["earth"], eph["sun"], eph["moon"]

    def longitude(observer, body):
        # Apparent geocentric longitude, true ecliptic and equinox of date.
        return observer.observe(body).apparent().frame_latlon(ecliptic_frame)[1].radians

    def half_lunation(t):  # 0 from new moon to full moon, 1 from full moon to new moon
        e = earth.at(t)
        elongation = (longitude(e, moon) - longitude(e, sun)) % TAU
        return (elongation >= math.pi).astype(int)

    half_lunation.step_days = 5.0

    def solar_term(t):  # k while the Sun's longitude is in [15k, 15k + 15) degrees
        return (longitude(earth.at(t), sun) // (TAU / 24)).astype(int) % 24

    solar_term.step_days = 5.0

    t0, t1 = ts.tt(*SEARCH_FROM), ts.tt(*SEARCH_TO)
    print("searching new moons and solar terms (about 30 s)", file=sys.stderr)
    tm, ym = find_discrete(t0, t1, half_lunation)
    new_moons = tm[ym == 0]
    tt, yt = find_discrete(t0, t1, solar_term)

    events = []
    for t, terms in ((new_moons, np.full(len(new_moons), -1)), (tt, yt)):
        civil, ut1 = civil_seconds(ts, t), ut1_seconds(t)
        for i in range(len(terms)):
            events.append(Event(int(terms[i]), float(t.tt[i]), float(civil[i]), float(ut1[i])))
    events.sort(key=lambda e: e.tt)

    memo.parent.mkdir(parents=True, exist_ok=True)
    memo.write_text(json.dumps([[e.term, e.tt, e.civil_s, e.ut1_s] for e in events]))
    return events


# ---- The calendar ----------------------------------------------------------------


@dataclass(frozen=True)
class Month:
    year: int  # lunar year
    month: int  # 1-12
    leap: bool
    start: int  # days since 1970-01-01
    length: int

    @property
    def label(self) -> tuple[int, int, bool]:
        return self.year, self.month, self.leap

    def __str__(self) -> str:
        return label_text(self.label)


def label_text(label: tuple[int, int, bool]) -> str:
    year, month, leap = label
    return f"{year} {'leap ' if leap else ''}month {month}"


def build_months(events: list[Event], zone: str, ut1: bool = False) -> list[Month]:
    """The months of lunar years FIRST_YEAR-LAST_YEAR under the Vietnamese rules.

    A month starts on the local date of its new moon. Month 11 is the month that
    contains the December solstice. When 13 months separate two month-11 starts (a
    sui with a leap month), the first of them without a principal term is the leap
    month and repeats the number of the month before it.
    """
    moons = sorted(e.day(zone, ut1) for e in events if e.term < 0)
    principal = sorted(e.day(zone, ut1) for e in events if e.principal)
    solstice = {}
    for e in events:
        if e.term == WINTER_SOLSTICE:
            day = e.day(zone, ut1)
            solstice[date_of(day).year] = day

    def month11(year: int) -> int:  # index in `moons` of the month holding the solstice
        return bisect.bisect_right(moons, solstice[year]) - 1

    def has_principal(first: int, end: int) -> bool:  # a principal term on a day in [first, end)
        i = bisect.bisect_left(principal, first)
        return i < len(principal) and principal[i] < end

    months = []
    for sui in range(FIRST_YEAR - 1, LAST_YEAR + 1):
        i0, i1 = month11(sui), month11(sui + 1)
        count = i1 - i0
        if count not in (12, 13):
            raise RuntimeError(f"{count} months between the solstices of {sui} and {sui + 1}")
        leap_at = None
        if count == 13:
            leap_at = next(
                (i for i in range(1, 13) if not has_principal(moons[i0 + i], moons[i0 + i + 1])),
                None,
            )
            if leap_at is None:
                raise RuntimeError(f"13 months after the {sui} solstice, all with a principal term")
        number, year = 11, sui
        for i in range(count):
            leap = i == leap_at
            if i > 0 and not leap:
                number = number % 12 + 1
                if number == 1:
                    year = sui + 1
            start = moons[i0 + i]
            months.append(Month(year, number, leap, start, moons[i0 + i + 1] - start))
    return [m for m in months if FIRST_YEAR <= m.year <= LAST_YEAR]


def check_rules(months: list[Month], events: list[Event], zone: str) -> None:
    """Re-check the invariants the C tests also check; raise on any violation."""
    principal = sorted(e.day(zone) for e in events if e.principal)
    solstices = sorted(e.day(zone) for e in events if e.term == WINTER_SOLSTICE)
    for a, b in zip(months, months[1:]):
        if a.start + a.length != b.start:
            raise RuntimeError(f"gap or overlap between {a} and {b}")
    by_year: dict[int, list[Month]] = {}
    for m in months:
        if m.length not in (29, 30):
            raise RuntimeError(f"{m} has {m.length} days")
        by_year.setdefault(m.year, []).append(m)
        holds = [d for d in principal if m.start <= d < m.start + m.length]
        if m.leap and holds:
            raise RuntimeError(f"leap {m} holds a principal term")
        has_solstice = any(m.start <= d < m.start + m.length for d in solstices)
        if has_solstice != (m.month == 11 and not m.leap):
            raise RuntimeError(f"{m}: winter solstice rule broken")
    for year, ms in by_year.items():
        numbers = [m.month for m in ms if not m.leap]
        leaps = [m for m in ms if m.leap]
        if numbers != list(range(1, 13)) or len(leaps) > 1:
            raise RuntimeError(f"lunar year {year}: months {[str(m) for m in ms]}")
        for m in leaps:
            before = ms[ms.index(m) - 1]
            if before.month != m.month or before.leap:
                raise RuntimeError(f"{m} does not follow the month it repeats")
    # Where exactly a zone switches must not matter: an event near a switch has to
    # fall on the same date under the offsets on both sides of it.
    for e in events:
        for switch, offsets in ((VN_SWITCH_S, (8, 7)), (CN_SWITCH_S, (BEIJING_MEAN_S / HOUR_S, 8))):
            if abs(e.civil_s - switch) < 2 * DAY_S:
                dates = {math.floor((e.civil_s + h * HOUR_S) / DAY_S) for h in offsets}
                if len(dates) > 1:
                    raise RuntimeError(f"{e.name} at {e.when(zone)}: the zone switch decides its date")


# ---- Tables ----------------------------------------------------------------------

LEAP_SHIFT = 13  # see core/src/gg_lunar_data.h
TET_SHIFT = 17


def year_records(months: list[Month]) -> list[tuple[int, int, int, int]]:
    """(lunar year, Tết day, leap month, record) for each year of the table."""
    by_year: dict[int, list[Month]] = {}
    for m in months:
        by_year.setdefault(m.year, []).append(m)
    records = []
    for year in range(FIRST_YEAR, LAST_YEAR + 1):
        ms = by_year[year]
        tet = ms[0].start
        offset = tet - days(dt.date(year, 1, 1))
        leap = next((m.month for m in ms if m.leap), 0)
        lengths = sum(1 << i for i, m in enumerate(ms) if m.length == 30)
        if not 0 <= offset < 64:
            raise RuntimeError(f"Tết {year} is {offset} days after 1 January")
        records.append((year, tet, leap, lengths | leap << LEAP_SHIFT | offset << TET_SHIFT))
    return records


def term_dates(events: list[Event], zone: str) -> dict[int, dict[int, dt.date]]:
    """Gregorian year -> {term k: local date} for FIRST_YEAR-LAST_YEAR."""
    out: dict[int, dict[int, dt.date]] = {}
    for e in events:
        if e.term >= 0:
            d = date_of(e.day(zone))
            if FIRST_YEAR <= d.year <= LAST_YEAR:
                out.setdefault(d.year, {})[e.term] = d
    return out


def term_table(events: list[Event], zone: str) -> tuple[list[int], list[list[int]]]:
    """Per-term base day of month, and per year 6 bytes of 2-bit offsets."""
    dates = term_dates(events, zone)
    for year in range(FIRST_YEAR, LAST_YEAR + 1):
        if sorted(dates[year]) != list(range(24)):
            raise RuntimeError(f"{year} does not have the 24 solar terms once each")
    position_day = {}  # (year, position) -> day of month
    for year, terms in dates.items():
        for k, d in terms.items():
            p = (k - FIRST_TERM_OF_YEAR) % 24
            if d.month != p // 2 + 1:
                raise RuntimeError(f"{TERM_NAMES[k]} {d} is not in month {p // 2 + 1}")
            position_day[year, p] = d.day
    years = range(FIRST_YEAR, LAST_YEAR + 1)
    base = [min(position_day[y, p] for y in years) for p in range(24)]
    rows = []
    for y in years:
        offsets = [position_day[y, p] - base[p] for p in range(24)]
        if max(offsets) > 3:
            raise RuntimeError(f"{y}: a solar term is more than 3 days after its base day")
        rows.append([sum(offsets[4 * j + i] << (2 * i) for i in range(4)) for j in range(6)])
    return base, rows


def render_c(events: list[Event], months: list[Month]) -> str:
    records = year_records(months)
    base, rows = term_table(events, "vn")
    out = io.StringIO()
    w = out.write
    w("/*\n")
    w(f" * Vietnamese lunar calendar tables for lunar years {FIRST_YEAR}-{LAST_YEAR}")
    w(" (docs/lunar.md).\n")
    w(" *\n")
    w(" * GENERATED by tools/lunar/generate.py from the JPL DE440s ephemeris. Do not edit:\n")
    w(" * change the generator and run it again.\n")
    w(" *\n")
    w(f" *   {tool_versions()}\n")
    w(f" *   {EPHEMERIS} SHA-256 {EPHEMERIS_SHA256}\n")
    w(" *   Instants in UT1 before 1972 and in UTC from 1972, with no leap second after the\n")
    w(" *   last announced one; dates in UTC+8 before 1968-01-01 and UTC+7 from then on.\n")
    w(" */\n")
    w('#include "gg_lunar_data.h"\n\n')
    w(f"_Static_assert(GG_LUNAR_FIRST_YEAR == {FIRST_YEAR} && GG_LUNAR_LAST_YEAR == {LAST_YEAR},\n")
    w('               "tools/lunar/generate.py covers other years");\n\n')
    w("/* Layout: gg_lunar_data.h. */\n")
    w("const uint32_t gg_lunar_years[] = {\n")
    for year, tet, leap, record in records:
        leap_text = f", leap month {leap}" if leap else ""
        w(f"    0x{record:06x}, /* {year}: Tết {date_of(tet)}{leap_text} */\n")
    w("};\n\n")
    w("const uint8_t gg_solar_term_base[] = {\n")
    w("    " + ", ".join(str(d) for d in base) + ",\n")  # one line, as clang-format wants it
    w("};\n\n")
    w("const uint8_t gg_solar_terms[][GG_SOLAR_TERM_COUNT / 4] = {\n")
    for year, row in zip(range(FIRST_YEAR, LAST_YEAR + 1), rows):
        w("    {" + ", ".join(f"0x{b:02x}" for b in row) + f"}}, /* {year} */\n")
    w("};\n")
    return out.getvalue()


def render_months_csv(months: list[Month]) -> str:
    out = io.StringIO()
    out.write(
        f"# Vietnamese lunar months of lunar years {FIRST_YEAR}-{LAST_YEAR}, generated by\n"
        "# tools/lunar/generate.py (docs/lunar.md); do not edit. start: the Solar date of\n"
        "# day 1; length: days in the month.\n"
    )
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(["lunar_year", "month", "leap", "start", "length"])
    for m in months:
        writer.writerow([m.year, m.month, int(m.leap), date_of(m.start), m.length])
    return out.getvalue()


# ---- Comparison with the Hong Kong Observatory -----------------------------------


def read_hko_months(path: Path = HKO_MONTHS) -> list[tuple[tuple[int, int, bool], int]]:
    """[(label, start day)] from core/test/data/hko_month_starts.csv, sorted by date."""
    rows = []
    with open(path, newline="") as f:
        for row in csv.DictReader(line for line in f if not line.startswith("#")):
            label = (int(row["lunar_year"]), int(row["month"]), row["leap"] == "1")
            rows.append((label, days(dt.date.fromisoformat(row["start"]))))
    rows.sort(key=lambda r: r[1])
    return rows


def read_hko_terms(path: Path) -> dict[tuple[int, int], int]:
    """{(Gregorian year, term k): day} from the solar_terms.csv fetch_hko.py writes."""
    out = {}
    with open(path, newline="") as f:
        for row in csv.DictReader(line for line in f if not line.startswith("#")):
            d = dt.date.fromisoformat(row["date"])
            out[d.year, int(row["term"])] = days(d)
    return out


def new_moon_on(events: list[Event], zone: str, day: int) -> Event:
    return next(e for e in events if e.term < 0 and e.day(zone) == day)


def nearest_new_moon(events: list[Event], zone: str, day: int) -> Event:
    return min((e for e in events if e.term < 0), key=lambda e: abs(e.day(zone) - day))


def compare_months(months, events, zone, hko, first_year, last_year) -> list[str]:
    """Every HKO month of lunar years first_year-last_year whose number or start differs."""
    ours = {m.label: m for m in months}
    problems = []
    for label, start in hko:
        if not first_year <= label[0] <= last_year:
            continue
        m = ours.get(label)
        if m is not None and m.start == start:
            continue
        e = nearest_new_moon(events, zone, start)
        here = f"starts {date_of(m.start)}" if m else "does not exist here"
        problems.append(
            f"{label_text(label)}: HKO starts it {date_of(start)}, this calendar {here}; "
            f"new moon {e.when(zone)}, {e.margin(zone):+.1f} s from midnight"
        )
    hko_labels = {label for label, _ in hko}
    for m in months:
        in_span = hko[0][1] <= m.start <= hko[-1][1]  # HKO: the month in progress on 1901-01-01 on
        if in_span and first_year <= m.year <= last_year and m.label not in hko_labels:
            problems.append(f"{m} (from {date_of(m.start)}) is not in HKO's table")
    return problems


def compare_terms(events, zone, hko_terms, first_year, last_year) -> tuple[int, list[str]]:
    count, problems = 0, []
    for e in events:
        if e.term < 0:
            continue
        d = date_of(e.day(zone))
        if not first_year <= d.year <= last_year:
            continue
        hko_day = hko_terms.get((d.year, e.term))
        if hko_day is None:
            # A term in the first days of January belongs to HKO's row of that year too.
            raise RuntimeError(f"HKO has no {TERM_NAMES[e.term]} in {d.year}")
        count += 1
        if hko_day != e.day(zone):
            problems.append(
                f"{TERM_NAMES[e.term]} {d.year}: HKO {date_of(hko_day)}, here {d}; "
                f"{e.when(zone)}, {e.margin(zone):+.1f} s from midnight"
            )
    return count, problems


def hko_differences(months, events, hko) -> list[tuple[Month, str, str]]:
    """The months of this calendar in which some day has another lunar date in HKO's.

    This is the comparison core/test/test_lunar.c repeats day by day. Each month
    comes with HKO's start for the month of the same number and the reason.
    """
    hko_starts = [start for _, start in hko]
    hko_by_label = {label: start for label, start in hko}
    hko_on_day = {start: label for label, start in hko}
    our_starts = [m.start for m in months]
    differing: dict[tuple[int, int, bool], Month] = {}
    for day in range(hko_starts[0], days(LAST_SOLAR_DAY) + 1):
        i = bisect.bisect_right(hko_starts, day) - 1
        theirs = (hko[i][0], day - hko_starts[i])
        m = months[bisect.bisect_right(our_starts, day) - 1]
        if (m.label, day - m.start) != theirs:
            differing[m.label] = m

    # The same rules in HKO's zone ("china") must reproduce HKO wherever a reason
    # below blames the zone; anything else is reported as HKO disagreeing with DE440s.
    china = {m.start: m.label for m in build_months(events, "china")}

    def moved(e: Event) -> str:
        return f"{e.name} {e.when('vn')} = {e.when('china')}"

    out = []
    for m in sorted(differing.values(), key=lambda m: m.start):
        hko_start = hko_by_label.get(m.label)
        hko_label = hko_on_day.get(m.start)
        if hko_label is None and hko_start is None:
            raise RuntimeError(f"{m} starts on a day on which no HKO month starts")
        if hko_label is None:  # HKO starts this month on another day
            e = new_moon_on(events, "vn", m.start)
            reason = f"starts {'earlier' if hko_start > m.start else 'later'}: {moved(e)}"
            explained = e.day("china") == hko_start
        elif hko_label == m.label:  # same start and number; HKO's next month starts elsewhere
            e = new_moon_on(events, "vn", m.start + m.length)
            reason = f"ends {'earlier' if e.day('china') > e.day('vn') else 'later'}: {moved(e)}"
            explained = e.day("china") != e.day("vn")
        else:  # the leap month sits elsewhere: a principal term changes month
            near = (e for e in events if e.principal and abs(e.day("vn") - m.start) < 400)
            terms = [e for e in near if e.day("vn") != e.day("china")]
            reason = f"HKO numbers it {label_text(hko_label)}: " + "; ".join(map(moved, terms))
            explained = bool(terms) and china.get(m.start) == hko_label
        if not explained:
            reason = "HKO differs from DE440s: " + reason
        out.append((m, str(date_of(hko_start)) if hko_start is not None else "-", reason))
    return out


def render_differences_csv(diffs: list[tuple[Month, str, str]]) -> str:
    out = io.StringIO()
    out.write(
        "# Months in which this calendar and the Hong Kong Observatory's Chinese calendar\n"
        "# (core/test/data/hko_month_starts.csv) give some day a different lunar date,\n"
        "# generated by tools/lunar/generate.py (docs/lunar.md); do not edit.\n"
        "# start: day 1 here; hko_start: day 1 of the month with the same number in HKO's\n"
        "# table (- when it has none); reason: the instant that falls on different dates.\n"
    )
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(["lunar_year", "month", "leap", "start", "hko_start", "reason"])
    for m, hko_start, reason in diffs:
        writer.writerow([m.year, m.month, int(m.leap), date_of(m.start), hko_start, reason])
    return out.getvalue()


# ---- Reports ---------------------------------------------------------------------


def margins_report(events: list[Event], zone: str, limit_s: float) -> list[str]:
    """Events within limit_s of local midnight, and whether UT1 would move them."""
    lines = []
    for e in events:
        year = date_of(e.day(zone)).year
        if not FIRST_YEAR <= year <= LAST_YEAR or abs(e.margin(zone)) > limit_s:
            continue
        alt = e.day(zone, ut1=True)
        note = "" if alt == e.day(zone) else f"; on {date_of(alt)} if civil time followed UT1"
        lines.append(f"{e.name:24} {e.when(zone)}  {e.margin(zone):+7.1f} s{note}")
    return lines


def ut1_sensitivity(events: list[Event], zone: str) -> list[str]:
    """Months and solar terms that change if civil time followed UT1 after 1972."""
    lines = []
    utc = {m.label: m.start for m in build_months(events, zone)}
    for m in build_months(events, zone, ut1=True):
        if utc.get(m.label) != m.start:
            lines.append(f"{m}: {date_of(m.start)} instead of {date_of(utc[m.label])}")
    for e in events:
        if e.term >= 0 and e.day(zone) != e.day(zone, ut1=True):
            year = date_of(e.day(zone)).year
            if FIRST_YEAR <= year <= LAST_YEAR:
                lines.append(f"{e.name}: {date_of(e.day(zone, ut1=True))} instead of "
                             f"{date_of(e.day(zone))}")
    return lines


def write_or_check(path: Path, text: str, check: bool) -> bool:
    """Write path, or with check report whether it is up to date. True when unchanged."""
    current = path.read_text() if path.exists() else None
    if current == text:
        return True
    if check:
        print(f"{path.relative_to(REPO)} is out of date", file=sys.stderr)
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        print(f"wrote {path.relative_to(REPO)}", file=sys.stderr)
    return False


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--zone", choices=ZONES, default="vn", help="time-zone rule")
    parser.add_argument("--check", action="store_true", help="fail if outputs are stale")
    parser.add_argument("--compare-hko", action="store_true", help="compare with HKO's tables")
    parser.add_argument("--hko-terms", type=Path, help="solar_terms.csv from fetch_hko.py")
    parser.add_argument("--margins", type=float, metavar="S", help="list events near midnight")
    parser.add_argument("--sensitivity", action="store_true", help="UT1 instead of UTC")
    args = parser.parse_args()

    events = search_events()
    months = build_months(events, args.zone)
    check_rules(months, events, args.zone)
    leaps = [m for m in months if m.leap]
    print(
        f"{ZONES[args.zone]}: {len(months)} months, {len(leaps)} leap months, "
        f"lunar years {FIRST_YEAR}-{LAST_YEAR}",
        file=sys.stderr,
    )

    status = 0
    report_only = args.compare_hko or args.margins is not None or args.sensitivity
    if args.zone == "vn" and not report_only:
        hko = read_hko_months()
        outputs = (
            (OUT_C, render_c(events, months)),
            (OUT_MONTHS, render_months_csv(months)),
            (OUT_DIFFS, render_differences_csv(hko_differences(months, events, hko))),
        )
        for path, text in outputs:
            if not write_or_check(path, text, args.check) and args.check:
                status = 1
        records = year_records(months)
        size = 4 * len(records) + 24 + 6 * len(records)
        print(f"tables: {size} bytes ({4 * len(records)} for the years)", file=sys.stderr)

    if args.compare_hko:
        hko = read_hko_months()
        for first, last in ((hko[0][0][0], 1928), (1929, LAST_YEAR)):
            problems = compare_months(months, events, args.zone, hko, first, last)
            n = sum(1 for label, _ in hko if first <= label[0] <= last)
            print(f"\nHKO months of lunar years {first}-{last}: {n}, differences: {len(problems)}")
            for p in problems:
                print("  " + p)
        terms_path = args.hko_terms or cache_dir() / "hko" / "solar_terms.csv"
        if terms_path.exists():
            hko_terms = read_hko_terms(terms_path)
            for first, last in ((1901, 1928), (1929, LAST_YEAR)):
                n, problems = compare_terms(events, args.zone, hko_terms, first, last)
                print(f"\nHKO solar terms {first}-{last}: {n}, differences: {len(problems)}")
                for p in problems:
                    print("  " + p)
        if args.zone == "vn":
            diffs = hko_differences(months, events, hko)
            print(f"\nmonths that differ from HKO: {len(diffs)}")
            for m, hko_start, reason in diffs:
                print(f"  {str(m):22} {date_of(m.start)}  HKO {hko_start:10}  {reason}")

    if args.margins is not None:
        print(f"\nevents within {args.margins:g} s of local midnight ({ZONES[args.zone]}):")
        for line in margins_report(events, args.zone, args.margins):
            print("  " + line)

    if args.sensitivity:
        lines = ut1_sensitivity(events, args.zone)
        print(f"\nchanges if civil time followed UT1 after 1972: {len(lines)}")
        for line in lines:
            print("  " + line)
    return status


if __name__ == "__main__":
    sys.exit(main())
