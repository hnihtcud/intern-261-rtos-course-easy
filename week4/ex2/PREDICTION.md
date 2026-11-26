# Dự đoán — Week 4 / Ex2

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379

1. Lỗi gọi API queue không phải `FromISR` bên trong ISR sẽ gây ra hiện
   tượng gì khi chạy (configASSERT? treo? reset?). Giải thích bằng cơ chế
   (ngăn xếp ISR, gọi `vTaskSuspendAll`/scheduler từ ngữ cảnh ngắt), không
   chỉ "nó bị lỗi".
   - Em dự đoán chương trình dừng tại `configASSERT()` khi ISR gọi `xQueueSend()`, kể cả khi queue còn chỗ và thời gian chờ bằng `0`. Trên Cortex-M3, `xQueueSend()` đi qua `vPortEnterCritical()`, nơi kiểm tra `VECTACTIVE` để phát hiện CPU đang chạy trong ngắt.
   - ISR chạy ở Handler mode, dùng stack MSP và không có TCB riêng để chuyển sang `Blocked`. Nếu assert bị tắt, queue đầy và thời gian chờ lớn hơn `0`, `xQueueSend()` có thể gọi `vTaskSuspendAll()` rồi đưa **task vừa bị ngắt** vào danh sách chờ gửi. Điều này có thể làm sai trạng thái lập lịch. Nếu `configASSERT()` giữ CPU trong vòng lặp, board sẽ có biểu hiện treo. 
2. Sau khi sửa đúng API `FromISR`, dự đoán số mẫu ghi UART mỗi giây =
   `SAMPLE_RATE_HZ` / `LOG_BLOCK_SAMPLES` khối/giây. Tính ra con số.
   - Với `SAMPLE_RATE_HZ = 400` và `LOG_BLOCK_SAMPLES = 32`:
   ```
      Số khối mỗi giây = 400 / 32 = 12,5 khối/giây
      Thời gian gom một khối = 32 / 400 = 80 ms
   ```
   - Nếu không mất mẫu và UART theo kịp, hệ thống dự kiến ghi trung bình **12,5 khối/giây**, tương đương **400 mẫu/giây**.

3. Với `UART_BAUD` của bạn, một khối `LOG_BLOCK_SAMPLES` mẫu mất bao lâu để
   truyền xong? Có nguy cơ UART chưa gửi xong khối cũ mà khối mới đã sẵn
   sàng không?
   - Với `LOG_BLOCK_SAMPLES = 32` và định dạng nhị phân 16 byte/mẫu, mỗi khối có `32 × 16 = 512 byte` dữ liệu. Ở `UART_BAUD = 115200`, cấu hình 8N1 cần 10 bit trên dây cho mỗi byte.
   ```
    Thời gian truyền = 512 × 10 / 115.200 ≈ 44,44 ms
    Khoảng cách giữa hai khối = 32 / 400 = 80 ms
    Thời gian còn dư = 80 − 44,44 ≈ 35,56 ms
   ```
   - Do đó, em dự đoán UART gửi xong khối cũ trước khi khối mới sẵn sàng.