import csv
import struct
from pathlib import Path

import matplotlib.pyplot as plt

TICK_RATE_HZ = 1000
QUEUE_LENGTH = 12

# Khớp struct trong app_main.c:
# TickType_t tick: 4 byte
# uint8_t raw_depth: 1 byte
# uint8_t filtered_depth: 1 byte
# padding: 2 byte
RECORD = struct.Struct("<IBB2x")


def plot_queue(input_name, output_name, title):
    source = Path(input_name)
    data = source.read_bytes()

    if not data or len(data) % RECORD.size != 0:
        raise ValueError(
            f"{source}: kích thước dữ liệu không hợp lệ"
        )

    records = list(RECORD.iter_unpack(data))
    first_tick = records[0][0]

    time_s = [
        ((tick - first_tick) & 0xFFFFFFFF) / TICK_RATE_HZ
        for tick, _, _ in records
    ]
    raw = [row[1] for row in records]
    filtered = [row[2] for row in records]

    if any(value > QUEUE_LENGTH for value in raw + filtered):
        raise ValueError(
            "Độ đầy vượt quá 12; kiểm tra cấu trúc dữ liệu xuất."
        )

    # Lưu CSV để nộp và kiểm tra lại.
    csv_path = source.with_suffix(".csv")
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(
            ["time_s", "queue_raw", "queue_filtered"]
        )
        writer.writerows(zip(time_s, raw, filtered))

    fig, ax = plt.subplots(figsize=(10, 4))

    ax.step(
        time_s, raw, where="post",
        label="queue_raw", linewidth=1.5
    )
    ax.step(
        time_s, filtered, where="post",
        label="queue_filtered", linewidth=1.5
    )
    ax.axhline(
        QUEUE_LENGTH, color="red", linestyle="--",
        label="Queue capacity = 12"
    )

    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Queued samples")
    ax.set_title(title)
    ax.set_ylim(-0.5, QUEUE_LENGTH + 1)
    ax.set_yticks(range(QUEUE_LENGTH + 1))
    ax.grid(True, alpha=0.3)
    ax.legend()

    fig.tight_layout()
    fig.savefig(output_name, dpi=200)
    plt.close(fig)

    print(
        f"{input_name}: {len(records)} điểm, "
        f"max raw = {max(raw)}, "
        f"max filtered = {max(filtered)}"
    )
    print(f"Đã lưu {output_name} và {csv_path}")


plot_queue(
    "queue_depth_normal.bin",
    "queue_fill_normal.png",
    "Queue occupancy - UART 115200 baud"
)

plot_queue(
    "queue_depth_slow.bin",
    "queue_fill_slow_uart.png",
    "Queue occupancy - UART 9600 baud"
)

plot_queue(
    "queue_depth_both_max.bin",
    "queue_fill_both_max.png",
    "Queue occupancy - UART 9600 baud, both portMAX_DELAY"
)