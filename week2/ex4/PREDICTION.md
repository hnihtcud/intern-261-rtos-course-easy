# Dự đoán — Week 2 / Ex4

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379    `EVENT_BIT_COUNT` = 4  `SYNC_TIMEOUT_MS` = 120

1. RAM event group chiếm (bytes) dự đoán: 28 . Dựa vào đâu (cấu trúc dữ
   liệu nào trong FreeRTOS)? 
   - Em dựa vào cấu trúc EventGroup_t, gồm EventBits_t uxEventBits (4 byte), List_t xTasksWaitingForBits (20 byte) và trường định danh trace uxEventGroupNumber (4 byte khi configUSE_TRACE_FACILITY = 1).
2. RAM thêm khi dùng task notification cho 1 điểm đồng bộ: dự đoán = 0
   bytes. Vì sao?
   - Vì giá trị và trạng thái notification đã nằm trong TCB của sync_task khi bật configUSE_TASK_NOTIFICATIONS = 1, nên không cần cấp phát thêm object riêng.
3. Dự đoán thời gian đánh thức (producer gọi → sync_task chạy) của notification
   so với event-group-bit: nhanh hơn / chậm hơn / xấp xỉ? Vì sao (nghĩ đến số
   lớp hàm bên dưới mỗi API)?
   - Em dự đoán task notification có thời gian đánh thức ngắn hơn event-group-bit, vì đường xử lý bên dưới API trực tiếp và ít lớp trung gian hơn. xTaskNotifyGive() cập nhật notification ngay trong TCB của task nhận và bỏ chặn task nếu đang chờ. Trong khi đó, xEventGroupSetBits() phải đi qua object event group, cập nhật bit, duyệt danh sách task chờ, kiểm tra điều kiện ANY/ALL rồi mới bỏ chặn các task phù hợp. Vì vậy, event group thường thực hiện nhiều bước và lời gọi xử lý nội bộ hơn. 
4. Nếu `sync_task` bị timeout (`SYNC_TIMEOUT_MS` hết hạn) vì một producer
   không set bit kịp, `xEventGroupWaitBits` trả về gì? Làm sao phân biệt được
   "đủ bit" với "timeout"?
   - Khi chờ hết 120 ms mà chưa đủ sự kiện, xEventGroupWaitBits() trả về các bit đã được set. Với bốn producer, em kiểm tra đủ cả bốn bit chưa: 1111 là đồng bộ thành công; nếu thiếu bất kỳ bit nào, ví dụ 1011 thiếu bit 2, thì lượt chờ đã timeout mà chưa đủ sự kiện. 
5. Nêu trước một tình huống bạn nghĩ **không thể** thay toàn bộ event group
   bằng notification — sẽ kiểm chứng lại ở REPORT.md.
   - Em dự đoán không thể thay trực tiếp event group bằng một task notification khi nhiều task cùng chờ một sự kiện chung. Ví dụ, task A và task B đều đang chờ bit SYSTEM_READY: một lần gọi xEventGroupSetBits() có thể đánh thức cả hai. Trong khi đó, xTaskNotifyGive() chỉ gửi đến một task cụ thể; nếu gửi cho A thì B vẫn chờ. Muốn đánh thức cả hai bằng notification phải gửi riêng cho từng task.