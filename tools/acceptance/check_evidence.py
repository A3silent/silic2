#!/usr/bin/env python3
"""Audit actual runtime JSONL/CSV evidence; never substitutes for human playtests."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import statistics
import sys


def require(condition, message):
    if not condition:
        raise ValueError(message)


def records(path):
    with path.open(encoding="utf-8") as stream:
        return [json.loads(line) for line in stream if line.strip()]


def configuration(rows, uncapped=False):
    found = [r for r in rows if r.get("event") == "configuration"]
    require(len(found) == 1, "exactly one configuration record required")
    config = found[0]
    require(config.get("audio_available") is True, "real audio backend unavailable")
    require(config.get("synthetic_input") is True, "acceptance input must be labelled")
    require((config["internal_width"], config["internal_height"]) == (320, 200),
            "unexpected internal resolution")
    if uncapped:
        require(config.get("vsync") is False, "performance requires VSync off")
        require((config["logical_width"], config["logical_height"]) == (1280, 720),
                "performance requires 1280x720 logical window")
    return config


def restart_report(directory):
    rows = records(directory / "restarts.jsonl")
    config = configuration(rows)
    before = [r for r in rows if r.get("event") == "before_reset"]
    reset = [r for r in rows if r.get("event") == "reset"]
    require(len(before) >= 50 and len(reset) == len(before) + 1,
            "need at least 50 runtime restarts plus initial clean state")
    require([r["cycle"] for r in before] == list(range(1, len(before) + 1)),
            "missing restart cycle")
    fields = ("entities", "hostile_projectiles", "player_projectiles", "impact_lights",
              "character_effects", "warnings", "voices", "stats_time", "stats_kills",
              "stats_shots", "stats_damage", "stats_hostile_shots", "stats_hostile_hits",
              "director_time", "environment_time")
    for row in reset:
        require(row.get("clean") is True, "runtime reported dirty reset")
        require(all(row.get(key) == 0 for key in fields), "nonzero/missing reset field")
        require(row.get("hp") == 100 and row.get("fatal_source") == "unknown",
                "player state not reset")
    for row in before:
        require(all(row.get(key, 0) > 0 for key in
                    ("entities", "hostile_projectiles", "player_projectiles", "voices",
                     "damage_taken", "stats_shots", "audio_frames")),
                "restart did not exercise live runtime state")
    require(any(r.get("warnings", 0) > 0 for r in before), "no active warning restart")
    rss = [r["rss_bytes"] for r in reset[11:]]
    require(all(x > 0 for x in rss), "resident memory measurements unavailable")
    # Report the trend for inspection; do not invent an acceptance limit absent
    # from the plan or claim a statistical proof of no leaks from one short run.
    mean_x = (len(rss) - 1) / 2
    mean_y = statistics.mean(rss)
    slope = sum((i - mean_x) * (y - mean_y) for i, y in enumerate(rss)) / sum(
        (i - mean_x) ** 2 for i in range(len(rss)))
    return {"configuration": config, "restarts": len(before), "all_resets_clean": True,
            "warning_cycles": sum(r.get("warnings", 0) > 0 for r in before),
            "post_warmup_rss_min": min(rss), "post_warmup_rss_max": max(rss),
            "post_warmup_rss_last_minus_first": rss[-1] - rss[0],
            "post_warmup_rss_slope_bytes_per_cycle": round(slope, 2)}


def performance_report(directory, mode):
    rows = records(directory / f"{mode}.jsonl")
    config = configuration(rows, uncapped=True)
    reported = [r for r in rows if r.get("event") == "performance"]
    expected = 3 if mode == "performance" else 1
    require(len(reported) == expected, f"need {expected} completed measurement records")
    summary = []
    for result in reported:
        with (directory / f"frames-{mode}-{result['run']}.csv").open() as stream:
            frames = [float(r["wall_clock_ms"]) for r in csv.DictReader(stream)]
        require(len(frames) == result["frames"] and len(frames) > 100,
                "missing or incomplete raw frame evidence")
        require(all(math.isfinite(x) and x >= 0 for x in frames), "invalid frame time")
        ordered = sorted(frames)
        p95, p99 = [ordered[int((len(ordered) - 1) * q)] for q in (.95, .99)]
        streak = longest = 0
        for value in frames:
            streak = streak + 1 if value > 50 else 0
            longest = max(longest, streak)
        require(p95 <= 20 and p99 <= 33.3 and longest < 3, "frame-time gate failed")
        require(abs(p95 - result["p95_ms"]) < .01 and abs(p99 - result["p99_ms"]) < .01,
                "reported percentiles do not match raw CSV")
        require(result.get("audio_frames", 0) > 0, "audio callback did not advance")
        require(result["combat_seconds"] >= (90 if mode == "performance" else 25),
                "run too short for this acceptance mode")
        summary.append({"run": result["run"], "combat_seconds": result["combat_seconds"],
                        "frames": len(frames), "p95_ms": p95, "p99_ms": p99,
                        "max_ms": max(frames), "longest_over_50ms": longest})
    if mode == "performance":
        runs = records(directory / "runs.jsonl")
        complete = [r for r in runs if r.get("reason") == "completed"]
        require(len(complete) == 3, "three complete combat runs required")
        require(all(r.get("synthetic_input") is True and r.get("acceptance_mode") == mode
                    and r.get("seed") == 42 and r.get("content_version") for r in complete),
                "run context missing or incorrect")
    else:
        load = [r for r in rows if r.get("event") == "stress_load"]
        require(len(load) == 1 and all(load[0].get(k) == v for k, v in
                {"chasers": 18, "sentries": 3, "hostile_projectiles": 32,
                 "environment_particles": 2000}.items()), "stress load incomplete")
    return {"configuration": config, "runs": summary}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--restarts", type=Path, required=True)
    parser.add_argument("--performance", type=Path, required=True)
    parser.add_argument("--stress", type=Path, required=True)
    args = parser.parse_args()
    try:
        directories = (args.restarts, args.performance, args.stress)
        hashes = [(directory / "binary.sha256").read_text().split()[0]
                  for directory in directories]
        require(len(set(hashes)) == 1, "acceptance modes used different binaries")
        binary = Path(__file__).resolve().parents[2] / "silic2-release"
        require(hashlib.sha256(binary.read_bytes()).hexdigest() == hashes[0],
                "current Release binary differs from acceptance evidence")
        report = {"restarts": restart_report(args.restarts),
                  "performance": performance_report(args.performance, "performance"),
                  "stress": performance_report(args.stress, "stress"),
                  "release_sha256": hashes[0],
                  "evidence_directories": [str(path) for path in directories],
                  "human_playtest": "pending; not measured by this tool"}
    except (ValueError, KeyError, OSError, TypeError) as error:
        print(f"Evidence check failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(report, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
