import argparse
import csv
from pathlib import Path

import numpy as np


def read_segments(path):
    """Đọc ba cột: seq, raw_ax, filtered_ax.
    Tách đoạn khi seq không liên tục hoặc có dòng dữ liệu lỗi.
    """
    segments = []
    current = []
    invalid = 0

    def finish_segment():
        nonlocal current
        if current:
            segments.append(np.asarray(current, dtype=np.int64))
            current = []

    with Path(path).open("r", encoding="utf-8-sig",
                         errors="replace") as f:
        for line in f:
            line = line.strip()

            if not line:
                continue

            if line.lower() == "seq,raw_ax,filtered_ax":
                continue

            try:
                fields = line.split(",")
                if len(fields) != 3:
                    raise ValueError()

                seq, raw, filtered = map(int, fields)

                if not 0 <= seq <= 0xFFFFFFFF:
                    raise ValueError()
                if not (-32768 <= raw <= 32767
                        and -32768 <= filtered <= 32767):
                    raise ValueError()

            except ValueError:
                invalid += 1
                finish_segment()
                continue

            if current:
                expected = (current[-1][0] + 1) & 0xFFFFFFFF
                if seq != expected:
                    finish_segment()

            current.append((seq, raw, filtered))

    finish_segment()
    return segments, invalid


def check_moving_average(segment, window):
    """Tính lại trung bình từ raw để đối chiếu với filtered.
    Bỏ window-1 đầu đoạn vì chưa có đủ lịch sử trong file.
    """
    raw = segment[:, 1].astype(np.float64)
    filtered = segment[:, 2].astype(np.float64)

    if len(raw) < window:
        return None

    expected = np.convolve(
        raw, np.ones(window) / window, mode="valid"
    )

    # Phép chia số nguyên có dấu trong C làm tròn về 0.
    expected = np.trunc(expected)
    actual = filtered[window - 1:]

    error = actual - expected

    return {
        "mae": float(np.mean(np.abs(error))),
        "match_percent": float(np.mean(error == 0) * 100),
    }


def estimate_delay(segment, period_ms, max_lag, skip):
    """Tương quan Pearson theo từng độ dịch.

    raw[i] được so với filtered[i + lag].
    lag dương nghĩa là tín hiệu lọc đi sau tín hiệu thô.
    """
    raw = segment[skip:, 1].astype(np.float64)
    filtered = segment[skip:, 2].astype(np.float64)

    if len(raw) < max_lag + 50:
        raise ValueError("Đoạn dữ liệu quá ngắn.")

    scores = []

    for lag in range(max_lag + 1):
        if lag == 0:
            x = raw
            y = filtered
        else:
            x = raw[:-lag]
            y = filtered[lag:]

        x = x - np.mean(x)
        y = y - np.mean(y)

        denominator = np.sqrt(np.dot(x, x) * np.dot(y, y))

        if denominator == 0:
            scores.append(np.nan)
        else:
            scores.append(float(np.dot(x, y) / denominator))

    scores = np.asarray(scores)

    if not np.any(np.isfinite(scores)):
        raise ValueError("Tín hiệu không thay đổi đủ để đo độ trễ.")

    best = int(np.nanargmax(scores))
    fractional_lag = float(best)

    # Nội suy parabol quanh đỉnh để ước lượng phần lẻ của một mẫu.
    if 0 < best < max_lag:
        left, center, right = scores[best - 1:best + 2]
        denominator = left - 2 * center + right

        if np.all(np.isfinite([left, center, right])):
            if denominator < 0:
                offset = 0.5 * (left - right) / denominator
                if abs(offset) <= 0.5:
                    fractional_lag += float(offset)

    return {
        "lag_samples": fractional_lag,
        "delay_ms": fractional_lag * period_ms,
        "correlation": float(scores[best]),
        "at_boundary": best in (0, max_lag),
    }


def analyze_file(path, window, period_ms, max_lag, skip):
    segments, invalid = read_segments(path)
    results = []

    print(f"\nFILE: {path}")
    print(f"  Số đoạn liên tục: {len(segments)}")
    print(f"  Dòng lỗi bị loại: {invalid}")

    for index, segment in enumerate(segments, start=1):
        print(
            f"  Đoạn {index}: {len(segment)} mẫu, "
            f"seq {segment[0, 0]} -> {segment[-1, 0]}"
        )

        if len(segment) < skip + max_lag + 50:
            print("    Bỏ qua: quá ngắn để ước lượng.")
            continue

        check = check_moving_average(segment, window)

        try:
            estimate = estimate_delay(
                segment, period_ms, max_lag, skip
            )
        except ValueError as error:
            print(f"    Bỏ qua: {error}")
            continue

        print(
            f"    Khớp trung bình {window} mẫu: "
            f"{check['match_percent']:.2f}%; "
            f"sai số tuyệt đối trung bình = {check['mae']:.3f}"
        )
        print(
            f"    Độ dịch = {estimate['lag_samples']:.4f} mẫu; "
            f"độ trễ = {estimate['delay_ms']:.3f} ms; "
            f"tương quan = {estimate['correlation']:.6f}"
        )

        if estimate["at_boundary"]:
            print(
                "    Cảnh báo: đỉnh tương quan ở biên tìm kiếm; "
                "cần kiểm tra tín hiệu hoặc phạm vi độ dịch."
            )

        if check["match_percent"] < 99:
            print(
                "    Cảnh báo: đầu ra chưa khớp hoàn toàn "
                "với cửa sổ khai báo."
            )

        row = {
            "file": str(path),
            "window": window,
            "segment": index,
            "samples": len(segment),
            "seq_start": int(segment[0, 0]),
            "seq_end": int(segment[-1, 0]),
            "period_ms_assumed": period_ms,
            "skipped_samples": skip,
            "theory_delay_ms": (window - 1) * period_ms / 2,
            "estimated_lag_samples": estimate["lag_samples"],
            "estimated_delay_ms": estimate["delay_ms"],
            "correlation": estimate["correlation"],
            "moving_average_match_percent": check["match_percent"],
            "moving_average_mae": check["mae"],
            "peak_at_boundary": estimate["at_boundary"],
            "selected": False,
        }
        results.append(row)

    if not results:
        raise ValueError(f"{path}: không có đoạn đủ dữ liệu.")

    # Chọn đoạn dài nhất theo quy tắc cố định, không chọn đoạn
    # chỉ vì kết quả gần lý thuyết nhất.
    selected = max(results, key=lambda row: row["samples"])
    selected["selected"] = True

    print(
        f"  Chọn đoạn {selected['segment']} "
        f"(đoạn đủ điều kiện dài nhất)."
    )

    return results, selected


def main():
    parser = argparse.ArgumentParser(
        description="Ước lượng độ trễ tín hiệu bộ lọc từ log UART."
    )
    parser.add_argument("window8_file")
    parser.add_argument("window16_file")
    parser.add_argument("--period-ms", type=float, default=8.0)
    parser.add_argument("--max-lag", type=int, default=20)
    parser.add_argument("--skip", type=int, default=32)
    parser.add_argument(
        "--output", default="filter_delay_result.csv"
    )
    args = parser.parse_args()

    if args.period_ms <= 0 or args.max_lag < 1 or args.skip < 0:
        parser.error("Tham số thời gian/độ dịch không hợp lệ.")

    rows8, chosen8 = analyze_file(
        args.window8_file, 8, args.period_ms,
        args.max_lag, args.skip
    )
    rows16, chosen16 = analyze_file(
        args.window16_file, 16, args.period_ms,
        args.max_lag, args.skip
    )

    rows = rows8 + rows16
    output = Path(args.output)

    with output.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)

    measured_difference = (
        chosen16["estimated_delay_ms"]
        - chosen8["estimated_delay_ms"]
    )
    predicted_difference = (16 - 8) * args.period_ms / 2

    print("\nKẾT QUẢ SO SÁNH")
    print(
        f"  Cửa sổ 8 : "
        f"{chosen8['estimated_delay_ms']:.3f} ms"
    )
    print(
        f"  Cửa sổ 16: "
        f"{chosen16['estimated_delay_ms']:.3f} ms"
    )
    print(f"  Tăng thêm ước lượng: {measured_difference:.3f} ms")
    print(f"  Tăng thêm lý thuyết: {predicted_difference:.3f} ms")
    print(
        f"  Độ lệch: "
        f"{measured_difference - predicted_difference:+.3f} ms"
    )
    print(f"  CSV: {output.resolve()}")
    print(
        "\nLưu ý: đây là độ trễ tín hiệu, giả sử lấy mẫu đều; "
        "không đo thời gian chờ queue hoặc truyền UART."
    )


if __name__ == "__main__":
    main()