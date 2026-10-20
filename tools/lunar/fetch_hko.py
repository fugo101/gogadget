#!/usr/bin/env python3
"""Fetch the Hong Kong Observatory's Gregorian-lunar tables and derive test data.

HKO publishes the Chinese calendar for 1901-2100 as one text file per year
(https://www.hko.gov.hk/en/gts/time/conversion1_text.htm). This script downloads
T<year>e.txt one at a time with a pause between requests, keeps the files in
$GG_CACHE_DIR/hko (default ~/.cache/gogadget/hko), checks them against
tools/lunar/hko-sha256.txt and derives:

  core/test/data/hko_month_starts.csv  lunar year, month, leap, Gregorian start date
  $GG_CACHE_DIR/hko/solar_terms.csv    Gregorian date of each solar term, for
                                       generate.py --compare-hko

Only these dates go into the repository, never HKO's files.

Usage: fetch_hko.py [--delay SECONDS] [--update-checksums]
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import hashlib
import io
import os
import re
import ssl
import sys
import tempfile
import time
import urllib.error
import urllib.request
from pathlib import Path

import certifi

REPO = Path(__file__).resolve().parents[2]
OUT_MONTHS = REPO / "core" / "test" / "data" / "hko_month_starts.csv"
CHECKSUMS = Path(__file__).resolve().parent / "hko-sha256.txt"
URL = "https://www.hko.gov.hk/en/gts/time/calendar/text/files/T{year}e.txt"
FIRST_YEAR, LAST_YEAR = 1901, 2100
USER_AGENT = "gogadget-lunar/1.0 (+https://github.com/fugo101/gogadget; one-off test data)"

# HKO's English names of the solar terms -> k, the term that begins when the Sun's
# apparent longitude reaches 15k degrees (the numbering of tools/lunar/generate.py).
TERMS = {
    "Vernal Equinox": 0, "Bright & Clear": 1, "Corn Rain": 2, "Summer Commences": 3,
    "Corn Forms": 4, "Corn on Ear": 5, "Summer Solstice": 6, "Moderate Heat": 7,
    "Great Heat": 8, "Autumn Commences": 9, "End of Heat": 10, "White Dew": 11,
    "Autumnal Equinox": 12, "Cold Dew": 13, "Frost": 14, "Winter Commences": 15,
    "Light Snow": 16, "Heavy Snow": 17, "Winter Solstice": 18, "Moderate Cold": 19,
    "Severe Cold": 20, "Spring Commences": 21, "Spring Showers": 22, "Insects Waken": 23,
}  # fmt: skip
WEEKDAYS = ("Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday")
TITLE = re.compile(r"^Gregorian-Lunar Calendar Conversion Table of (\d{4}) ")
ROW = re.compile(
    # Older files write "Lunar month" and zero-pad the Gregorian date.
    r"^(\d{4})/(\d{1,2})/(\d{1,2})\s+(\d{1,2})((?:st|nd|rd|th) Lunar [Mm]onth)?\s+"
    r"(" + "|".join(WEEKDAYS) + r")\s*(.*?)\s*$"
)


def cache_dir() -> Path:
    return Path(os.environ.get("GG_CACHE_DIR") or Path.home() / ".cache" / "gogadget") / "hko"


def fetch(year: int, path: Path, delay: float) -> None:
    """Download one file, with retries that back off; atomic on success."""
    context = ssl.create_default_context(cafile=certifi.where())
    request = urllib.request.Request(URL.format(year=year), headers={"User-Agent": USER_AGENT})
    for attempt in range(4):
        time.sleep(delay * (2**attempt))
        try:
            with urllib.request.urlopen(request, timeout=60, context=context) as response:
                data = response.read(1 << 20)
            break
        except urllib.error.HTTPError as e:
            if e.code not in (429, 500, 502, 503, 504) or attempt == 3:
                raise
        except (urllib.error.URLError, TimeoutError):
            if attempt == 3:
                raise
        print(f"  retrying {path.name}", file=sys.stderr)
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as tmp:
        tmp.write(data)
    os.replace(tmp.name, path)


def read_checksums() -> dict[str, str]:
    if not CHECKSUMS.exists():
        return {}
    out = {}
    for line in CHECKSUMS.read_text().splitlines():
        digest, name = line.split()
        out[name] = digest
    return out


def parse(year: int, text: str) -> list[tuple[dt.date, int, bool, int | None]]:
    """Rows of one file: (date, lunar day or month number, starts a month, term k)."""
    lines = text.splitlines()
    title = TITLE.match(lines[0])
    if not title or int(title.group(1)) != year or not lines[1].startswith("Gregorian date"):
        raise ValueError(f"T{year}e.txt: unexpected header {lines[:2]!r}")
    rows = []
    for n, line in enumerate(lines[2:], start=3):
        if not line.strip():
            continue
        if not re.match(r"^\d{4}/", line):  # a footnote, such as Hong Kong's summer time
            print(f"note: T{year}e.txt:{n}: {line.strip()}", file=sys.stderr)
            continue
        m = ROW.match(line)
        if not m:
            raise ValueError(f"T{year}e.txt:{n}: cannot parse {line!r}")
        date = dt.date(int(m.group(1)), int(m.group(2)), int(m.group(3)))
        if date.year != year or WEEKDAYS[date.weekday()] != m.group(6):
            raise ValueError(f"T{year}e.txt:{n}: wrong year or weekday in {line!r}")
        term = m.group(7)
        if term and term not in TERMS:
            raise ValueError(f"T{year}e.txt:{n}: unknown solar term {term!r}")
        starts = m.group(5) is not None
        rows.append((date, int(m.group(4)), starts, TERMS[term] if term else None))
    return rows


def derive(rows: list[tuple[dt.date, int, bool, int | None]]):
    """Month starts [(lunar year, month, leap, date)], solar terms [(date, k)], gaps."""
    months, terms, gaps = [], [], []
    year = number = None
    day = None  # lunar day of the previous row
    previous = None
    for date, value, starts, term in rows:
        if previous is not None and date != previous + dt.timedelta(days=1):
            missing = (date - previous).days - 1
            gaps.append((previous + dt.timedelta(days=1), missing))
            if missing != 1 or starts or day is None or value != day + 2:
                raise ValueError(f"cannot bridge the gap after {previous}")
            day += 1  # a mid-month day is missing; nothing it holds is lost but a term
        if starts:
            if number is None:  # the first month start: months 11 and 12 before Tết
                year = date.year - 1 if value >= 11 and date.month <= 2 else date.year
                leap = False
                if day is not None:
                    # The first file opens inside a month. The lunar day of its first
                    # row gives that month's day 1, so the comparison covers every day
                    # of the files. The files do not say whether it is a leap month 11;
                    # it is taken as month 11, which the test then checks against ours.
                    first = rows[0][0] - dt.timedelta(days=rows[0][1] - 1)
                    if value != 12 or (date - first).days not in (29, 30):
                        raise ValueError(f"cannot date the month before {date}")
                    months.append((year, 11, False, first))
            else:
                if day not in (29, 30):
                    raise ValueError(f"{date}: the month before has {day} days")
                leap = value == number
                if not leap and value != number % 12 + 1:
                    raise ValueError(f"{date}: month {value} after month {number}")
                if value == 1 and not leap:
                    year += 1
            number, day = value, 1
            months.append((year, number, leap, date))
        else:
            if day is not None and value != day + 1:
                raise ValueError(f"{date}: lunar day {value} after {day}")
            day = value
        if term is not None:
            terms.append((date, term))
        previous = date
    return months, terms, gaps


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--delay", type=float, default=1.0, help="seconds between requests")
    parser.add_argument(
        "--update-checksums", action="store_true", help="accept changed files and rewrite the list"
    )
    args = parser.parse_args()

    directory = cache_dir()
    directory.mkdir(parents=True, exist_ok=True)
    known = read_checksums()
    digests, rows = {}, []
    for year in range(FIRST_YEAR, LAST_YEAR + 1):
        path = directory / f"T{year}e.txt"
        if not path.exists():
            print(f"fetching {path.name}", file=sys.stderr)
            fetch(year, path, args.delay)
        data = path.read_bytes()
        digests[path.name] = hashlib.sha256(data).hexdigest()
        expected = known.get(path.name)
        if expected and expected != digests[path.name] and not args.update_checksums:
            sys.exit(f"{path}: changed since tools/lunar/hko-sha256.txt was written")
        # Some titles hold one Big5 dash; everything the parser matches is ASCII.
        rows += parse(year, data.decode("latin-1"))

    if args.update_checksums or not known:
        CHECKSUMS.write_text("".join(f"{d}  {name}\n" for name, d in digests.items()))
        print(f"wrote {CHECKSUMS.relative_to(REPO)}", file=sys.stderr)

    months, terms, gaps = derive(rows)
    for first, missing in gaps:
        print(f"note: HKO's files skip {missing} day(s) from {first}", file=sys.stderr)

    out = io.StringIO()
    out.write(
        "# First days of the months of the Chinese calendar, from the Hong Kong Observatory's\n"
        f"# Gregorian-lunar conversion tables {FIRST_YEAR}-{LAST_YEAR}\n"
        "# (https://www.hko.gov.hk/en/gts/time/conversion1_text.htm), derived by\n"
        "# tools/lunar/fetch_hko.py; do not edit. HKO's calendar uses UTC+8 from 1929 and\n"
        "# Beijing mean time before.\n"
    )
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(["lunar_year", "month", "leap", "start"])
    for year, number, leap, date in months:
        writer.writerow([year, number, int(leap), date.isoformat()])
    OUT_MONTHS.parent.mkdir(parents=True, exist_ok=True)
    OUT_MONTHS.write_text(out.getvalue())

    with open(directory / "solar_terms.csv", "w", newline="") as f:
        writer = csv.writer(f, lineterminator="\n")
        writer.writerow(["date", "term"])
        for date, k in terms:
            writer.writerow([date.isoformat(), k])
    print(
        f"{len(months)} month starts into {OUT_MONTHS.relative_to(REPO)}, "
        f"{len(terms)} solar terms into {directory / 'solar_terms.csv'}",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
