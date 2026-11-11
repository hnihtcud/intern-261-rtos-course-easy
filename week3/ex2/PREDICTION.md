# Dự đoán — Week 3 / Ex2

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379

1. Với nút nhấn cơ khí thông thường, nảy phím thường kéo dài bao lâu
   (tra cứu/ước lượng)? So với `DEBOUNCE_MS` của bạn: đủ lớn để lọc hết không?
   - Em ước lượng nảy phím cơ khí thường kéo dài từ hàng trăm µs đến vài ms, một số loại có thể tới khoảng 10 ms. Với `DEBOUNCE_MS = 30 ms`, em dự đoán khoảng chờ này đủ để lọc nảy của nút thông thường: mỗi cạnh mới đều Reset timer, nên nút chỉ được xác nhận khi không có cạnh mới trong 30 ms. 
2. Nếu `DEBOUNCE_MS` quá lớn, điều gì xảy ra khi người dùng nhấn nhả rất
   nhanh liên tiếp (double-click)?
   - Nếu `DEBOUNCE_MS` quá lớn, hai lần nhấn nhanh có thể bị gộp thành một lần hoặc bị bỏ sót. Mỗi cạnh nhấn/nhả đều Reset timer; nếu trạng thái chưa giữ ổn định đủ lâu thì callback chưa xác nhận được nó. Ví dụ với `DEBOUNCE_MS = 30 ms`, nếu khoảng thả giữa hai lần nhấn ngắn hơn 30 ms, chương trình có thể không nhận ra trạng thái thả và không phân biệt được hai lần nhấn. Vì vậy, tăng thời gian debounce giúp lọc nảy nhưng làm phản hồi chậm và giảm khả năng nhận double-click nhanh.
3. Khi callback của `debounce_timer` bị block 50 ms (thử nghiệm bước 4), bạn
   dự đoán `longpress_timer` sẽ bị trễ bao nhiêu? Giải thích bằng cơ chế
   daemon task + queue lệnh, không chỉ đoán số.
   - Hai callback dùng chung Timer Daemon Task, nên khi callback debounce gọi `vTaskDelay(50)`, Daemon Task không thể xử lý callback khác hoặc các lệnh Start/Stop/Reset trong queue trong khoảng chờ đó. Tuy nhiên, với debounce 30 ms và long-press 600 ms, một lần nhấn giữ bình thường khiến khoảng Blocked kết thúc khoảng 80 ms sau cạnh cuối, trước hạn long-press. Vì vậy, em dự đoán chưa thấy `HELD` bị trễ do lần Blocked này. Nếu một timer đến hạn trong khoảng Daemon Task bị Blocked, callback của nó mới bị trì hoãn.
4. `LONGPRESS_MS` của bạn là bao nhiêu? Giữ phím đúng ngưỡng này, bạn dự đoán
   callback `longpress_callback` chạy lần đầu ở thời điểm nào tính từ lúc
   nhấn xuống?
   - LONGPRESS_MS của em là 600 ms. Nếu nút vẫn được giữ qua ngưỡng, em dự đoán longpress_callback chạy lần đầu khoảng 600 ms sau cạnh xuống cuối cùng mà ISR gửi START. Nếu nảy phím làm lần START cuối muộn 5 ms, hạn đầu tiên sẽ ở khoảng 605 ms tính từ cạnh nhấn đầu tiên nên giả sử ta giữ đúng ngưỡng 600ms thì sẽ không in được `HELD`.