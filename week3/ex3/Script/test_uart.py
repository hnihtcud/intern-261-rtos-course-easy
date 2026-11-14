import time
import serial

PORT = "COM5"
BAUD = 57600        # Phải khớp baudrate STM32
COUNT = 1000

commands = [
    f"P20#{seq}\r".encode("ascii")
    for seq in range(1, COUNT + 1)
]
payload = b"".join(commands)

print("File đang chạy:", __file__)
print("Cổng:", PORT, "| Baudrate:", BAUD)
print("Lệnh đầu:", repr(commands[0]))
print("Lệnh cuối:", repr(commands[-1]))
print("Số lệnh:", COUNT)
print("Tổng byte cần gửi:", len(payload))  # 7893 byte

input(
    "\nĐóng kết nối COM trong Serial Debug Assistant.\n"
    "Reset STM32 rồi nhấn Continue, không Pause.\n"
    "Sau đó nhấn Enter tại đây để gửi..."
)

with serial.Serial(
    port=PORT,
    baudrate=BAUD,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=1,
    write_timeout=30,
    xonxoff=False,
    rtscts=False,
    dsrdtr=False,
) as uart:
    time.sleep(1)

    started = time.perf_counter()

    # Gửi toàn bộ lệnh liên tục, không nghỉ giữa các lệnh.
    written = uart.write(payload)
    uart.flush()

    elapsed = time.perf_counter() - started

    if written != len(payload):
        raise RuntimeError(
            f"Chỉ gửi được {written}/{len(payload)} byte"
        )

    # Chờ board xử lý nốt trước khi đọc Watch.
    time.sleep(2)

print(f"\nĐã gửi phía máy tính: {COUNT} lệnh, {written} byte")
print(f"Thời gian ghi và chờ truyền: {elapsed:.3f} s")