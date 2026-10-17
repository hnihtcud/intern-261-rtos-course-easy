#!/usr/bin/env python3
"""Ex3: analyze exactly 500 task intervals (501 GPIO transitions).

Python 3 standard library only. Input: PulseView CSV with time header and
one column per digital channel. Both rising and falling transitions count.
The initial CSV level is NOT a transition. No filtering of outlier intervals.
Defaults: PERIOD_MS=17, SAMPLE_COUNT=500, WORK_JITTER_US=1400, PA7 on D0.
WORK_JITTER_US is firmware metadata, not a correction to measured values.
"""
import argparse
import csv
import math
import re
import sys
from pathlib import Path

DETAIL_FIELDS = ['delay', 'tick_hz', 'sample_count', 'period_ms', 'work_jitter_us',
          'channel', 'min_ms', 'max_ms', 'mean_ms', 'jitter_us', 'drift_ms',
          'tick_period_ms', 'drift_vs_tick_ms', 'start_us', 'end_us',
          'duration_ms', 'available_cycles', 'skip_cycles', 'source_csv']
FIELDS = ['delay', 'tick_hz', 'min_ms', 'max_ms', 'mean_ms', 'jitter_us', 'drift_ms']


def configuration(path, args):
    mode = args.mode
    if mode is None:
        name = path.stem.lower()
        if 'xtaskdelayuntil' in name:
            mode = 'until'
        elif 'vtaskdelay' in name:
            mode = 'delay'
        else:
            raise ValueError('Cannot infer delay mode; use --mode delay or until')
    hz = args.tick_hz
    if hz is None:
        match = re.search(r'(?:^|[_\- ])(1000|100)(?:hz)?(?=$|[^0-9])',
                          path.stem, re.IGNORECASE)
        if not match:
            raise ValueError('Cannot infer tick rate; use --tick-hz 1000 or 100')
        hz = int(match.group(1))
    return ('vTaskDelay' if mode == 'delay' else 'xTaskDelayUntil'), hz


def analyze(path, args):
    mode, hz = configuration(path, args)
    edges = []
    previous_level = previous_time = None
    units = {'seconds': 1e6, 's': 1e6, 'milliseconds': 1e3,
             'ms': 1e3, 'microseconds': 1.0, 'us': 1.0,
             'nanoseconds': .001, 'ns': .001}
    with path.open(encoding='utf-8-sig', newline='') as stream:
        lines = (line for line in stream
                 if line.strip() and not line.lstrip().startswith((';', '#')))
        reader = csv.reader(lines)
        header = next(reader, None)
        if not header:
            raise ValueError('Empty CSV')
        unit = args.time_unit or header[0].strip().lower()
        if unit not in units:
            raise ValueError('Unknown time unit; specify --time-unit us/ms/s/ns')
        scale = units[unit]
        for row_number, row in enumerate(reader, 2):
            try:
                time = float(row[0]) * scale
                value = row[args.channel + 1].strip()
                if value not in ('0', '1') or not math.isfinite(time):
                    raise ValueError('invalid time or digital level')
                level = int(value)
            except (ValueError, IndexError) as exc:
                raise ValueError(f'Invalid CSV record {row_number}: {exc}') from exc
            if previous_time is not None and time <= previous_time:
                raise ValueError(f'Time must increase strictly: record {row_number}')
            if previous_level is not None and level != previous_level:
                edges.append(time)
            previous_level, previous_time = level, time
    available = max(0, len(edges) - 1)
    needed = args.skip_cycles + args.samples + 1
    if len(edges) < needed:
        raise ValueError(f'Only {len(edges)} transitions / {available} intervals; '
                         f'need {needed} transitions for the requested window')
    selected = edges[args.skip_cycles:needed]
    intervals = [b - a for a, b in zip(selected, selected[1:])]
    assert len(intervals) == args.samples
    duration = selected[-1] - selected[0]
    minimum, maximum = min(intervals), max(intervals)
    # Default pdMS_TO_TICKS conversion truncates to a whole number of ticks.
    ticks = int(args.period_ms * hz / 1000)
    if ticks < 1:
        raise ValueError('Requested period converts to zero ticks')
    tick_period = ticks * 1000 / hz
    result = dict(zip(DETAIL_FIELDS, [mode, hz, args.samples, args.period_ms,
        args.work_jitter_us, f'D{args.channel}', minimum / 1000,
        maximum / 1000, duration / args.samples / 1000, maximum - minimum,
        duration / 1000 - args.samples * args.period_ms, tick_period,
        duration / 1000 - args.samples * tick_period, selected[0], selected[-1],
        duration / 1000, available, args.skip_cycles, str(path)]))
    print(f'\n{path.name} | {mode} | {hz} Hz | D{args.channel}')
    print(f'  Used: {args.samples} cycles / {args.samples + 1} transitions; '
          f'available: {available} cycles; skipped: {args.skip_cycles}')
    print(f'  Min       : {result["min_ms"]:.6f} ms')
    print(f'  Max       : {result["max_ms"]:.6f} ms')
    print(f'  Mean      : {result["mean_ms"]:.6f} ms')
    print(f'  Jitter    : {result["jitter_us"]:.6f} us')
    print(f'  Drift     : {result["drift_ms"]:+.6f} ms '
          f'(reference: {args.period_ms:g} ms)')
    print(f'  Tick drift: {result["drift_vs_tick_ms"]:+.6f} ms '
          f'(reference: {tick_period:g} ms; supplementary)')
    print(f'  Window    : {selected[0]:.3f} -> {selected[-1]:.3f} us')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('files', nargs='+', type=Path)
    parser.add_argument('--output', type=Path, default=Path('result.csv'))
    parser.add_argument('--samples', type=int, default=500)
    parser.add_argument('--period-ms', type=float, default=17)
    parser.add_argument('--work-jitter-us', type=int, default=1400)
    parser.add_argument('--channel', type=int, default=0, help='Analyzer D0..D7, NOT GPIO pin number')
    parser.add_argument('--skip-cycles', type=int, default=0)
    parser.add_argument('--mode', choices=['delay', 'until'])
    parser.add_argument('--tick-hz', type=int, choices=[100, 1000])
    parser.add_argument('--time-unit', choices=['s', 'ms', 'us', 'ns'])
    parser.add_argument('--replace', action='store_true', help='Rebuild output instead of updating rows')
    args = parser.parse_args()
    if (args.samples <= 0 or not math.isfinite(args.period_ms) or args.period_ms <= 0
            or not 0 <= args.channel <= 7 or args.skip_cycles < 0
            or args.work_jitter_us < 0):
        parser.error('Invalid sample count, period, channel, skip count or workload')
    if args.output.resolve() in [p.resolve() for p in args.files]:
        parser.error('Output must not overwrite an input CSV')
    try:
        results = [analyze(p, args) for p in args.files]
        keys = [(r['delay'], r['tick_hz']) for r in results]
        if len(keys) != len(set(keys)):
            raise ValueError('Multiple inputs for the same configuration; select one capture')
        rows = {}
        if args.output.exists() and not args.replace:
            with args.output.open(encoding='utf-8-sig', newline='') as stream:
                reader = csv.DictReader(stream)
                if reader.fieldnames != FIELDS:
                    raise ValueError('Existing output has a different schema; choose a new output')
                for row in reader:
                    rows[(row['delay'], int(row['tick_hz']))] = row
        for result in results:
            rows[(result['delay'], result['tick_hz'])] = result
        ordered = sorted(rows.values(), key=lambda r:
                         (-int(r['tick_hz']), r['delay'] != 'vTaskDelay'))
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with args.output.open('w', encoding='utf-8-sig', newline='') as stream:
            writer = csv.DictWriter(stream, fieldnames=FIELDS)
            writer.writeheader()
            for row in ordered:
                writer.writerow({k: f'{v:.9f}' if isinstance(v, float) else v
                                 for k, v in row.items() if k in FIELDS})
        print(f'\nSaved: {args.output} ({len(ordered)} configurations)')
    except (OSError, ValueError, csv.Error) as exc:
        parser.exit(1, f'ERROR: {exc}\n')


if __name__ == '__main__':
    main()
