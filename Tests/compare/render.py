"""Render every case x signal through AmpBass and/or the reference plugin with AmpBassHostRender.

Output: <out>/<plugin>/<case>/<signal>.wav

    python3 Tests/compare/render.py                       # both plugins
    python3 Tests/compare/render.py --plugins ampbass     # just AmpBass
    python3 Tests/compare/render.py --self-test           # AmpBass rendered as "reference" too (must null)
    python3 Tests/compare/render.py --cases bass treble   # only cases whose name contains a filter
"""

from __future__ import annotations

import argparse
import json
import logging
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from cases import Case, all_cases

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
DEFAULT_HOST = ROOT / "build-tests/Tests/AmpBassHostRender_artefacts/Release/AmpBassHostRender"

log = logging.getLogger("render")


def interp(points: list[list[float]], x: float) -> float:
    """Piecewise-linear map from faceplate units to a normalised value."""
    for (x0, y0), (x1, y1) in zip(points, points[1:]):
        if x <= x1:
            return y0 + (y1 - y0) * (min(max(x, x0), x1) - x0) / (x1 - x0)
    return points[-1][1]


def to_args(plugin: dict, settings: dict[str, float | bool]) -> list[str]:
    args: list[str] = []
    for name, value in plugin.get("baseline", {}).items():
        # Numbers are normalised values; strings are display text.
        if isinstance(value, str):
            args += ["--set-text", f"{name}={value}"]
        else:
            args += ["--set", f"{name}={float(value):.6f}"]
    for key, value in settings.items():
        m = plugin["map"].get(key)
        if m is None:
            raise SystemExit(f"No mapping for '{key}' in plugins.json")
        if m["as"] == "knob10":
            args += ["--set", f"{m['param']}={float(value) / 10.0:.6f}"]
        elif m["as"] == "norm":
            args += ["--set", f"{m['param']}={float(value):.6f}"]
        elif m["as"] == "points":
            args += ["--set", f"{m['param']}={interp(m['points'], float(value)):.6f}"]
        else:
            text = ("On" if value else "Off") if isinstance(value, bool) else f"{value:g}"
            args += ["--set-text", f"{m['param']}={text}"]
    return args


def check_configured(name: str, plugin: dict) -> None:
    blob = json.dumps(plugin)
    if "TODO" in blob:
        raise SystemExit(f"plugins.json '{name}' still has TODO entries. Fill them in from "
                         f"AmpBassHostRender --plugin <path> --list first.")


def render_one(host: Path, plugin_path: Path, args: list[str], src: Path, dst: Path) -> str | None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    cmd = [str(host), "--plugin", str(plugin_path), "--in", str(src), "--out", str(dst), *args]
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, timeout=600, check=False)
    except subprocess.TimeoutExpired:
        return f"{dst}: timed out"
    warnings = [l for l in res.stderr.splitlines() if l.strip()]
    if res.returncode != 0 or not dst.exists():
        return f"{dst}: failed ({res.returncode}) {' | '.join(warnings)}"
    if warnings:
        log.warning("%s: %s", dst.relative_to(dst.parents[2]), " | ".join(warnings))
    return None


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--plugins", nargs="+", default=["ampbass", "reference"], choices=["ampbass", "reference"])
    ap.add_argument("--self-test", action="store_true", help="render AmpBass in place of the reference")
    ap.add_argument("--cases", nargs="*", default=[], help="substring filters on case names")
    ap.add_argument("--signals", type=Path, default=ROOT / "build-tests/compare/signals")
    ap.add_argument("--out", type=Path, default=ROOT / "build-tests/compare/renders")
    ap.add_argument("--host", type=Path, default=DEFAULT_HOST)
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--force", action="store_true", help="re-render files that already exist")
    args = ap.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(message)s")

    if not args.host.exists():
        raise SystemExit(f"HostRender not found at {args.host}. Build it with: cmake --build build-tests")
    if not (args.signals / "manifest.json").exists():
        raise SystemExit(f"No signals in {args.signals}. Run: python3 Tests/compare/signals.py")

    config = json.loads((HERE / "plugins.json").read_text())
    available = set(json.loads((args.signals / "manifest.json").read_text())["signals"])
    cases: list[Case] = [c for c in all_cases() if not args.cases or any(f in c.name for f in args.cases)]

    jobs = []
    for out_name in args.plugins:
        src_name = "ampbass" if args.self_test else out_name
        plugin = config[src_name]
        check_configured(src_name, plugin)
        plugin_path = Path(plugin["path"]).expanduser()
        if not plugin_path.exists():
            raise SystemExit(f"{src_name}: plugin not found at {plugin_path}")
        for case in cases:
            case_args = to_args(plugin, case.settings)
            for sig in case.signals:
                if sig not in available:
                    continue
                dst = args.out / out_name / case.name / f"{sig}.wav"
                if dst.exists() and not args.force:
                    continue
                jobs.append((args.host, plugin_path, case_args, args.signals / f"{sig}.wav", dst))

    log.info("%d renders to do", len(jobs))
    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        errors = [e for e in pool.map(lambda j: render_one(*j), jobs) if e]
    for e in errors:
        log.error(e)
    log.info("done: %d ok, %d failed", len(jobs) - len(errors), len(errors))
    raise SystemExit(1 if errors else 0)


if __name__ == "__main__":
    main()
