#!/usr/bin/env python3
import argparse
import csv
import json
import math
import re
import struct
import sys
from pathlib import Path


FRAME_SAMPLES = 1024
ADC_MIDPOINT = 2048
CHANNEL_KEYS = (
    "adc1_ch13",
    "adc2_ch18",
    "adc3_ch13",
    "adc5_ch3",
)
CHANNEL_ALIASES = {
    "adc1_ch13": 0,
    "adc1ch13": 0,
    "adc1": 0,
    "ch1": 0,
    "channel1": 0,
    "input1": 0,
    "adc2_ch18": 1,
    "adc2ch18": 1,
    "adc2": 1,
    "ch2": 1,
    "channel2": 1,
    "input2": 1,
    "adc3_ch13": 2,
    "adc3ch13": 2,
    "adc3": 2,
    "ch3": 2,
    "channel3": 2,
    "input3": 2,
    "adc5_ch3": 3,
    "adc5ch3": 3,
    "adc5": 3,
    "ch4": 3,
    "channel4": 3,
    "input4": 3,
}


def normalize_header(name: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", name.strip().lower())


def find_first_number(text: str):
    match = re.search(r"[-+]?\d+(?:[.,]\d+)?", text)
    if not match:
        return None
    return int(float(match.group(0).replace(",", ".")))


def parse_single_column_file(path: Path):
    values = []
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        value = find_first_number(line)
        if value is None:
            continue
        values.append(value)
    return values


def parse_csv_table(path: Path):
    with path.open("r", encoding="utf-8", newline="") as handle:
        sample = handle.read(4096)
        handle.seek(0)
        try:
            dialect = csv.Sniffer().sniff(sample, delimiters=",;\t")
        except csv.Error:
            dialect = csv.excel

        has_header = False
        try:
            has_header = csv.Sniffer().has_header(sample)
        except csv.Error:
            has_header = False

        rows = list(csv.reader(handle, dialect))

    rows = [row for row in rows if any(cell.strip() for cell in row)]
    if not rows:
        raise ValueError(f"{path} does not contain any data rows")

    if has_header:
        header = rows[0]
        data_rows = rows[1:]
        mapping = {}
        for index, name in enumerate(header):
            normalized = normalize_header(name)
            if normalized in CHANNEL_ALIASES:
                mapping[CHANNEL_ALIASES[normalized]] = index
        if len(mapping) != 4:
            missing = [CHANNEL_KEYS[idx] for idx in range(4) if idx not in mapping]
            raise ValueError(
                f"{path} is missing ADC columns for: {', '.join(missing)}"
            )
    else:
        if len(rows[0]) < 4:
            raise ValueError(f"{path} must contain at least 4 columns")
        data_rows = rows
        mapping = {0: 0, 1: 1, 2: 2, 3: 3}

    channels = [[] for _ in range(4)]
    for row in data_rows:
        for channel_index in range(4):
            column_index = mapping[channel_index]
            if column_index >= len(row):
                raise ValueError(
                    f"{path} row has fewer than {column_index + 1} columns"
                )
            value = find_first_number(row[column_index])
            if value is None:
                raise ValueError(
                    f"{path} row '{row}' does not contain a numeric value for channel {channel_index + 1}"
                )
            channels[channel_index].append(value)
    return channels


def parse_source(source: Path):
    if source.is_file():
        if source.suffix.lower() != ".csv":
            raise ValueError(f"Unsupported file source: {source}")
        return parse_csv_table(source)

    if not source.is_dir():
        raise ValueError(f"ADC source path does not exist: {source}")

    table_candidates = [source / "adc_table.csv", source / "adc.csv"]
    for candidate in table_candidates:
        if candidate.exists():
            return parse_csv_table(candidate)

    channels = []
    for channel_key in CHANNEL_KEYS:
        candidate = None
        for suffix in (".csv", ".txt"):
            path = source / f"{channel_key}{suffix}"
            if path.exists():
                candidate = path
                break
        if candidate is None:
            raise ValueError(
                f"Could not find input for {channel_key} in {source}"
            )
        channels.append(parse_single_column_file(candidate))
    return channels


def clamp_sample(value: int):
    return max(0, min(0xFFFFFFFF, value))


def write_frame_binary(path: Path, samples):
    with path.open("wb") as handle:
        for sample in samples:
            handle.write(struct.pack("<I", clamp_sample(sample)))


def build_manifest(channels, output_dir: Path, loop_frames: bool):
    max_length = max(len(channel) for channel in channels)
    if max_length == 0:
        raise ValueError("ADC source does not contain any samples")

    frame_count = int(math.ceil(max_length / float(FRAME_SAMPLES)))
    output_dir.mkdir(parents=True, exist_ok=True)

    frames = []
    for frame_index in range(frame_count):
        start = frame_index * FRAME_SAMPLES
        stop = start + FRAME_SAMPLES
        channel_paths = []
        for channel_index, channel_key in enumerate(CHANNEL_KEYS):
            chunk = list(channels[channel_index][start:stop])
            if len(chunk) < FRAME_SAMPLES:
                chunk.extend([ADC_MIDPOINT] * (FRAME_SAMPLES - len(chunk)))
            frame_path = output_dir / (
                f"frame_{frame_index:04d}_{channel_key}.bin"
            )
            write_frame_binary(frame_path, chunk)
            channel_paths.append(str(frame_path.resolve()))
        frames.append(channel_paths)

    manifest = {
        "channel_keys": list(CHANNEL_KEYS),
        "frame_count": frame_count,
        "frame_samples": FRAME_SAMPLES,
        "frame_bytes": FRAME_SAMPLES * 4,
        "total_samples": max_length,
        "loop_frames": loop_frames,
        "startup_delay_s": 0.05,
        "half_period_s": FRAME_SAMPLES / 250000.0,
        "frames": frames,
    }
    manifest["run_time_s"] = (
        manifest["startup_delay_s"] + frame_count * manifest["half_period_s"] + 0.02
    )

    manifest_path = output_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest_path


def parse_args():
    parser = argparse.ArgumentParser(
        description="Prepare Renode ADC frame binaries from txt/csv inputs."
    )
    parser.add_argument(
        "--source",
        required=True,
        help="Path to ADC source directory or a multi-column CSV table.",
    )
    parser.add_argument(
        "--output-dir",
        required=True,
        help="Directory where frame binaries and manifest will be written.",
    )
    parser.add_argument(
        "--loop",
        action="store_true",
        help="Mark the generated manifest as looping for live Renode feeds.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    source = Path(args.source).resolve()
    output_dir = Path(args.output_dir).resolve()

    try:
        channels = parse_source(source)
        manifest_path = build_manifest(channels, output_dir, args.loop)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    print(str(manifest_path))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
