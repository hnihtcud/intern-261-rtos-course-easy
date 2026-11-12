# Dự đoán — Week 3 / Ex3

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379

1. Thời gian truyền 1 byte ở `RX_BAUD` của bạn là bao nhiêu us? (8N1, tính cả
   start/stop bit). Trong thời gian đó `parser_task` có kịp xử lý 1 byte từ
   stream buffer không, nếu nó là task ưu tiên cao nhất hệ thống?
   - Với RX_BAUD = 57600 baud và cấu hình 8N1, một byte gồm 1 bit start, 8 bit dữ liệu và 1 bit stop, tổng cộng 10 bit. Thời gian truyền một byte là:
   ```T_byte = 10 / 57600 × 10^6 ≈ 173,61 µs.```
   - CPU 72 MHz có khoảng 12.500 chu kỳ CPU trong 173,61 µs. Với mỗi byte, parser chỉ cần:
      - Đọc byte từ stream buffer.
      - Kiểm tra có phải \r không.
      - Lưu byte vào mảng hoặc xử lý lệnh nếu đã gặp \r.
   - Đây là lượng xử lý nhỏ nên em dự đoán parser xử lý kịp. Vì có priority cao nhất trong các task, khi được đánh thức, parser được scheduler ưu tiên chạy để lấy dữ liệu ra.
2. `STREAM_BUFFER_BYTES` của bạn chứa tối đa bao nhiêu lệnh dài `MAX_CMD_LEN`
   byte trước khi đầy?
   - Stream buffer có dung lượng 192 byte. Nếu mỗi lệnh dài tổng cộng 16 byte, buffer chứa được tối đa 192 / 16 = 12 lệnh khi parser chưa đọc dữ liệu ra. Sau 12 lệnh, buffe đầy; byte tiếp theo không thể được ghi thêm nếu parser chưa giải phóng chỗ trống.
3. Nếu `xStreamBufferSendFromISR` trả về 0 (stream buffer đầy) trong
   `uart_rx_isr_handler`, byte đó sẽ bị mất. Theo bạn, trường hợp nào thực tế
   khiến điều này xảy ra (gửi quá nhanh / `parser_task` ưu tiên quá thấp /
   cái khác)?
   - Stream buffer đầy khi dữ liệu từ UART được đưa vào nhanh hơn parser_task lấy ra, khiến lượng dữ liệu chờ tích lũy đủ 192 byte. Các trường hợp thực tế gồm:
      - Gửi liên tục không nghỉ: parser không xử lý kịp tốc độ dữ liệu đến.
      - parser_task có ưu tiên thấp: các task ưu tiên cao hơn chiếm CPU, khiến parser không được chạy để đọc buffer.
      - Parser xử lý lệnh quá lâu, chẳng hạn in nhiều log bằng UART blocking hoặc gọi delay, trong khi ISR vẫn tiếp tục nhận byte.
   - Ở 57600 baud, UART nhận khoảng một byte mỗi 173,61 µs. Nếu buffer ban đầu rỗng và parser không đọc trong khoảng 192 × 173,61 ≈ 33,33 ms, buffer sẽ đầy khi dữ liệu đến liên tục. Khi đó, lần gửi một byte tiếp theo trả về 0 và byte không được lưu. Chương trình cần đếm byte bị mất, tiếp tục đăng ký nhận UART, không đứng chờ buffer trống trong ISR.
4. Dự đoán: gửi lệnh bằng gõ tay bình thường có mất byte không? Gửi bằng script gửi liên tục không nghỉ thì sao?
   - Gõ tay bình thường: em dự đoán không mất byte vì khoảng cách giữa các lần gõ lớn, parser có đủ thời gian đọc và xử lý dữ liệu, nên stream buffer 192 byte khó bị đầy.
   - Script gửi liên tục không nghỉ: ở 57600 baud, mỗi byte đến cách nhau khoảng 173,61 µs. Em dự đoán vẫn không mất byte nếu parser chỉ thực hiện các thao tác ngắn và được ưu tiên chạy để đọc buffer.Tuy nhiên nếu tăng tốc độ gửi mỗi byte vào lớn hơn `RX_BAUD` hiện tại của em thì parser sẽ không xử lý đủ nhanh, dữ liệu sẽ tích lũy và mất byte.