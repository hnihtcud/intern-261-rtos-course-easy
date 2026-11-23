# Dự đoán — Week 4 / Ex1

> **Commit TRƯỚC khi gửi design doc cho mentor duyệt.**

`student_id`: 2510379

1. Với `SAMPLE_RATE_HZ` của bạn, chu kỳ lấy mẫu là bao nhiêu us? Dựa trên
   latency ISR→task thật bạn đã đo ở `week3/ex1`, dự đoán task xử lý có kịp
   `ISR_TO_PROC_DEADLINE_US` không? Trình bày phép tính, không đoán.
Với `SAMPLE_RATE_HZ = 400 Hz`, chu kỳ lấy mẫu là `1.000.000 / 400 = 2.500 µs`.
Deadline **300 µs** tính từ lúc mẫu đã nằm trong RAM đến khi `process_task` lọc và đóng gói xong mẫu. 
Ở `week3/ex1`, latency ISR → task đo được khi có `portYIELD_FROM_ISR()` là **10,667 µs**. Dùng số này làm tham chiếu, ngân sách thời gian được tính như sau:

    10,667 + T_lọc + T_đóng_gói ≤ 300 µs
    T_lọc + T_đóng_gói ≤ 289,333 µs

- **Dự đoán kịp deadline.** `process_task` chỉ lọc và đóng gói từng mẫu, nên em kỳ vọng các bước này cùng độ trễ phát sinh nằm trong ngân sách **289,333 µs**. 

2. Dự đoán tổng RAM (queue + stack mọi task) theo thiết kế của bạn, tính
   bằng bytes. So với `MAX_HEAP_BYTES`: còn dư bao nhiêu?

- Mỗi phần tử `sensor_sample_t` gồm một timestamp `uint32_t` chiếm 4 B và sáu giá trị gia tốc/gyro kiểu `int16_t`, mỗi giá trị chiếm 2 B. Vì vậy, mỗi mẫu dự kiến chiếm `4 + 6 × 2 = 16 B`.

   - Queue đầu vào giữ tối đa 16 mẫu thô, cần `16 × 16 = 256 B`.
   - Queue đầu ra giữ tối đa 32 mẫu đã xử lý, cần `32 × 16 = 512 B`.

- Hai queue cần **`256 + 512 = 768 B`** để chứa dữ liệu mẫu.

- Stack được cấu hình theo word. Trên hệ thống này, mỗi word stack chiếm 4 B.

   - `process_task` được cấp 256 words, tương đương `256 × 4 = 1.024 B`.
   - `logger_task` được cấp 384 words, tương đương `384 × 4 = 1.536 B`.
   - Idle task dự kiến dùng 128 words, tương đương `128 × 4 = 512 B`.

- Tổng stack là **`1.024 + 1.536 + 512 = 3.072 B`**. Cộng với vùng dữ liệu queue, tổng `queue + stack` là **`768 + 3.072 = 3.840 B`**.

- Trong Week 1 / Ex4, khi tạo động một task có stack 512 B, heap giảm 624 B. Phần chênh `624 − 512 = 112 B` là chi phí cho TCB, header cấp phát và căn chỉnh bộ nhớ. Giả sử giữ cùng cấu hình FreeRTOS, overhead của các task là `3 × 112 = 336 B`.
- Mỗi queue được dự phòng thêm 128 B cho cấu trúc quản lý và chi phí cấp phát, nên hai queue dự phòng `2 × 128 = 256 B`. Cộng thêm 8 B quản lý cuối heap theo cấu hình `heap_4` ở Week 1, tổng overhead dự kiến là **`336 + 256 + 8 = 600 B`**. Mức 128 B mỗi queue là khoản dự phòng, chưa phải số đo.

- Tổng heap dự kiến dùng là **`3.840 + 600 = 4.440 B`**. Với `MAX_HEAP_BYTES = 10.240 B`, thiết kế **dự kiến còn dư `10.240 − 4.440 = 5.800 B`**.
3. Nếu task xử lý tạm thời chậm hơn ISR lấy mẫu (ví dụ do một task khác
   chiếm CPU), hàng đợi của bạn đầy sau bao nhiêu mẫu? Điều gì xảy ra với
   mẫu tiếp theo trong thiết kế của bạn?
- `raw_q` có 16 ô. Nếu queue đang rỗng và `process_task` tạm ngừng đọc, 16 mẫu đầu sẽ lấp hết queue. Ở 400 Hz, các mẫu cách nhau 2,5 ms. Lấy mẫu đầu tiên làm mốc `t = 0`, queue đầy tại `(16 − 1) × 2,5 = 37,5 ms`. Khi mẫu tiếp theo đến, ISR chỉ lưu được mẫu đó nếu task đã giải phóng ít nhất một ô. Nếu queue vẫn đầy, `xQueueSendFromISR()` báo `errQUEUE_FULL`. ISR bỏ mẫu vừa đến và tăng `dropped_samples`.

4. Bạn dự đoán priority nào (ISR task xử lý vs. task ghi UART) sẽ cho kết
   quả đạt deadline tốt hơn? Vì sao?
- Em đặt `process_task` ở priority 3 và `logger_task` ở priority 2. Khi ISR đưa mẫu vào queue và đánh thức `process_task`, task này có thể giành CPU sau khi ISR kết thúc, kể cả khi logger đang truyền UART bằng polling.
- Một khối UART chứa `32 × 16 = 512 B`. Ở 115200 baud, 8N1, thời gian truyền lý tưởng là `512 × 10 / 115200 ≈ 44,44 ms`. Nếu một mẫu đến sau khi logger bắt đầu truyền và task xử lý phải đợi hết lượt gửi thì sẽ không đạt deadline.
- Vì vậy, ưu tiên `process_task` cao hơn giúp giảm thời gian chờ CPU.