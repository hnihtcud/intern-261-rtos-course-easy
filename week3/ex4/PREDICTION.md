# Dự đoán — Week 3 / Ex4

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379

1. Chạy `app/deadlock_demo.c` y nguyên như được cấp. Dự đoán: hệ thống sẽ
   treo sau khoảng bao lâu (ngay lập tức, hay sau vài chu kỳ)? Vì sao không
   phải ngay lập tức (gợi ý: cần cả 2 task "đua" vào đúng thời điểm)?
   - Deadlock xảy ra sau khoảng **5–10 ms**, ngay trong chu kỳ đầu tiên.
   - Alpha lấy mutex A rồi delay 5 ms, tạo cơ hội cho beta lấy mutex B và cũng delay 5 ms. Khi chạy tiếp, alpha chờ B còn beta chờ A; cả hai không thể tiến tới lệnh trả mutex.
   - Không treo ngay lúc khởi chạy vì phải xuất hiện tình huống mỗi task đã giữ một mutex rồi chờ mutex của task kia.
2. Khi treo, `uxTaskGetSystemState` sẽ báo trạng thái gì cho `task_alpha` và
   `task_beta`? Dự đoán tên mutex mỗi task đang chờ (dựa trên đọc code, chưa
   chạy công cụ).

   - Khi deadlock xảy ra, `uxTaskGetSystemState()` sẽ báo cả `task_alpha` và `task_beta` ở trạng thái `eBlocked`:
      - `task_alpha`: đang giữ `s_mutex_a`, chờ lấy `s_mutex_b`.
      - `task_beta`: đang giữ `s_mutex_b`, chờ lấy `s_mutex_a`.
   - Hai task chờ nhau với `portMAX_DELAY`, nên không thể chạy đến lệnh trả mutex.
3. Tính reload value cho IWDG với `IWDG_TIMEOUT_MS` của bạn (LSI ~40 kHz).

   - Với `IWDG_TIMEOUT_MS = 1000 ms`, giả sử tần số LSI bằng `40000 Hz`, em chọn prescaler = 32.
      - f_đếm = 40000 / 32 = 1250 nhịp/giây

      - T_nhịp = 1000 / 1250 = 0,8 ms, nghĩa là cứ 0,8 ms, bộ đếm giảm một đơn vị.

      - N = 1000 / 0,8 = 1250 nhịp
      
   Thời gian hết hạn của IWDG tương ứng với `Reload + 1` nhịp:
      - Reload = N − 1 = 1250 − 1 = 1249

4. Sau khi sửa lỗi deadlock và thêm supervisor + IWDG, nếu bạn cố tình tái
   tạo lại tình huống deadlock cũ (tạm thời), dự đoán board sẽ phản ứng thế
   nào và sau bao lâu.
   - Khi tái tạo deadlock, `task_alpha` và `task_beta` chờ mutex của nhau nên không hoàn thành chu kỳ và không báo sống cho supervisor. Supervisor không nhận đủ bit trong thời gian chờ nên không gọi `HAL_IWDG_Refresh()`.
   - Với `IWDG_TIMEOUT_MS = 1000 ms`, board sẽ tự reset sau khoảng **1 giây kể từ lần refresh cuối cùng**, không phải chính xác 1 giây kể từ lúc deadlock xảy ra. Nếu chưa refresh lần nào, thời gian được tính từ lúc IWDG bắt đầu chạy.
   - Sau reset, chương trình chạy lại từ đầu. Nếu vẫn giữ code gây deadlock, board có thể tiếp tục treo rồi reset lặp lại.