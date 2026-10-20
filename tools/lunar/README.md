# tools/lunar

Generates the Vietnamese lunar calendar tables in `core/src/gg_lunar_data.c` from the JPL DE440s ephemeris, and the test data in `core/test/data/`. [docs/lunar.md](../../docs/lunar.md) explains the rules, the time zones, the table format and the verification; ADR-0015 records why the calendar is our own.

| File | What it does |
|---|---|
| `generate.py` | Finds every new moon and solar term from mid-1899 to early 2102 with Skyfield, applies the rules and writes `core/src/gg_lunar_data.c` and, in `core/test/data/`, `lunar_months.csv`, `lunar_terms.csv`, `hko_differences.csv` and `hko_term_differences.csv`. It stops on a difference from HKO that the time zone does not explain. It also compares with the Hong Kong Observatory and lists events near midnight. |
| `fetch_hko.py` | Downloads HKO's Gregorian-lunar tables for 1901-2100 (one request per second, cached) and derives `core/test/data/hko_month_starts.csv` and `core/test/data/hko_solar_terms.csv`. |
| `hko-sha256.txt` | SHA-256 of the 200 HKO files as fetched; `fetch_hko.py` stops if one changes. |
| `requirements.txt` | Exact versions. A different Skyfield or NumPy can change the output: regenerate and review the diff. |

Downloads go to `$GG_CACHE_DIR` (default `~/.cache/gogadget`): `jpl/de440s.bsp` (32 MB, SHA-256 pinned in `generate.py`), `hko/T<year>e.txt`, and `events/`, a memo of the computed events that makes later runs take a second. Nothing downloaded is committed.

## Setup

Python 3.14, any patch release (Homebrew: `/opt/homebrew/opt/python@3.14/libexec/bin/python3`). The generated header records only the minor version, so a Python update does not make the tables look stale; another minor version needs a regeneration and a review of the diff:

```sh
python3.14 -m venv tools/lunar/.venv
tools/lunar/.venv/bin/python -m pip install -r tools/lunar/requirements.txt
```

## Regenerate

From the repository root (`-I` keeps the current directory off Python's import path):

```sh
tools/lunar/.venv/bin/python -I tools/lunar/fetch_hko.py     # only when the HKO data is missing
tools/lunar/.venv/bin/python -I tools/lunar/generate.py      # writes the five files
make lunar-check                                         # generate.py --check with the venv's Python
make test
```

The first `generate.py` run downloads the ephemeris and takes about 30 seconds. The output is deterministic: the same inputs give byte-identical files, and `--check` exits 1 when a committed file is out of date.

## Verify

```sh
tools/lunar/.venv/bin/python -I tools/lunar/generate.py --zone utc8 --compare-hko   # the rules at UTC+8 vs HKO
tools/lunar/.venv/bin/python -I tools/lunar/generate.py --zone china --compare-hko  # Beijing mean time before 1929
tools/lunar/.venv/bin/python -I tools/lunar/generate.py --compare-hko               # Vietnam vs HKO, with reasons
tools/lunar/.venv/bin/python -I tools/lunar/generate.py --margins 120               # events within 120 s of midnight
tools/lunar/.venv/bin/python -I tools/lunar/generate.py --sensitivity               # if civil time followed UT1
```

## Licensing

The code here is written from the published rules and from standard astronomy; it does not use Hồ Ngọc Đức's code or tables (personal, non-commercial licence; see ADR-0015). Skyfield (MIT), jplephem (MIT), NumPy (BSD) and DE440s (JPL, public) are only used offline; the firmware contains integer tables, not their code. From HKO's files only derived dates are committed.
