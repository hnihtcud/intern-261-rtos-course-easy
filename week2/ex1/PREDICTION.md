# Dự đoán — Week 2 / Ex1

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379

1. Phép tính kích thước queue tối thiểu (dùng `SAMPLE_PERIOD_MS`, thời gian xử lý
   ước lượng của `filter_task` và `uart_task` ở `UART_BAUD`):
- Giả sử sensor_task > filter_task > uart_task về mức ưu tiên, mẫu được tạo đều mỗi 8 ms và không có chậm trễ lập lịch đáng kể. Ước lượng thời gian lọc là 0,05 ms/mẫu, định dạng chuỗi là 0,5 ms/mẫu và đọc cảm biến là 0,5 ms/mẫu.
   ```
   UART dùng 8N1, in theo định dạng `seq,ax,ay,az\r\n`:
   Độ dài tối đa = 10 + 3 × 6 + 3 + 2 = 33 byte.
   Thời gian truyền = 33 × 10 / 115200 × 1000
                    ≈ 2,865 ms.
   Thời gian uart_task ≈ 2,865 + 0,5 = 3,365 ms/mẫu.
   Tổng thời gian xử lý ước lượng: T ≈ 0,5 + 0,05 + 3,365 = 3,915 ms < SAMPLE_PERIOD_MS = 8 ms.
   ```

   Kích thước tối thiểu dự đoán: `queue_raw` = 1 , `queue_filtered` = 1 .
   So với `QUEUE_LENGTH` được cấp: đủ hay không? Vì sao?
   - Theo mô hình này, mỗi mẫu được đọc, lọc và truyền xong trước khi mẫu tiếp theo đến, nên không có mẫu tồn đọng qua các chu kỳ. Mỗi queue chỉ cần một chỗ chứa mẫu chờ nhận; mẫu đã được lấy ra xử lý không còn chiếm chỗ trong queue.
   - So với `QUEUE_LENGTH = 12`: dự đoán đủ ở cấu hình bình thường, đồng thời có thêm chỗ chứa mẫu khi task xử lý bị chậm tạm thời. 
2. Khi hạ `UART_BAUD` xuống 9600 (tạm thời), queue nào sẽ đầy trước? Vì sao?
- Dự đoán queue_filtered đầy trước, với thứ tự ưu tiên đã chọn: sensor_task > filter_task > uart_task.
   + Ở baud 9600, thời gian truyền một dòng tối đa 33 byte là: T = 33 × 10 / 9600 × 1000 ≈ 34,375 ms.
   + Trong khi đó, cứ 8 ms lại có một mẫu mới. Vì UART xử lý chậm hơn tốc độ mẫu đi vào, dữ liệu tích tụ trong queue_filtered và làm queue này đầy.
   + Nếu filter_task dùng xQueueSend(..., portMAX_DELAY), khi queue_filtered đầy, filter phải chờ chỗ trống nên giảm tốc độ lấy mẫu từ queue_raw. Khi đó, queue_raw cũng sẽ dần đầy.
   + Nếu filter dùng timeout bằng 0 và bỏ mẫu khi gửi thất bại, queue_raw có thể vẫn ít dữ liệu, nhưng các mẫu sẽ mất đi tại lúc gửi vào filter_task.
3. Với `xQueueSend(..., 0)` (timeout = 0) khi queue đầy: điều gì xảy ra với mẫu đó?
- Khi queue đầy, xQueueSend(..., 0) trả về ngay errQUEUE_FULL, không block task và không đưa mẫu mới vào queue; các mẫu cũ vẫn giữ nguyên. Mẫu mới vẫn còn trong biến của task, nhưng nếu không lưu hoặc gửi lại trước khi biến bị ghi đè thì mẫu đó sẽ bị bỏ, khiến số thứ tự mẫu phía nhận bị nhảy cóc (ví dụ thứ tự mẫu là 1 2 4 thì mẫu 3 đã không gửi thành công). 
4. Với `xQueueSend(..., portMAX_DELAY)` khi queue đầy: `sensor_task` sẽ ở trạng thái
   nào? Có nguy cơ gì cho chu kỳ lấy mẫu?
- Khi queue đầy, sensor_task chuyển sang trạng thái Blocked để chờ chỗ trống. Với portMAX_DELAY sẽ cho phép chờ vô hạn, không hết hạn chờ.
- Trong lúc đó, sensor không thể tiếp tục đọc cảm biến. Nếu thời gian chờ kéo dài qua mốc lấy mẫu tiếp theo, chu kỳ 8 ms bị trễ, làm bỏ lỡ các thời điểm lấy mẫu. Mẫu đang chờ gửi không bị bỏ do timeout nhưng đánh đổi là không bảo đảm lấy đủ mẫu cảm biến theo chu kỳ quy định từ trước. 
5. Độ đầy `queue_raw` theo thời gian ở cấu hình bình thường sẽ trông như thế nào
   (tăng dần / dao động quanh một mức / luôn gần 0)? Vì sao?
- Ở cấu hình bình thường, dự đoán độ đầy queue_raw luôn gần 0, dao động chủ yếu giữa 0 và 1 phần tử, không tăng dần theo thời gian.
- Sensor tạo một mẫu mỗi 8 ms, trong khi thời gian lọc ước lượng chỉ 0,05 ms/mẫu. Khi sensor gửi mẫu, độ đầy tăng lên 1; sau đó filter nhận mẫu thì độ đầy trở về 0, ngay trước khi thực hiện lọc. Với UART 115200 baud xử lý kịp tốc độ mẫu đến, queue_filtered không bị đầy kéo dài nên filter có thể tiếp tục nhận mẫu từ queue_raw. Đồ thị dự đoán có các xung ngắn từ 0 lên 1 rồi về 0