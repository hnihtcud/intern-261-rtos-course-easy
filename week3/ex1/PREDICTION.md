# Dự đoán — Week 3 / Ex1

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379

1. Với `ISR_NVIC_PRIO` của bạn và nhóm priority `NVIC_PRIORITYGROUP_4`, số này
   nằm trong khoảng cho phép (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` ..
   `configLIBRARY_LOWEST_INTERRUPT_PRIORITY`) không? Trình bày phép so sánh.
   - Trong bài của em, ISR_NVIC_PRIO = 6, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5 và configLIBRARY_LOWEST_INTERRUPT_PRIORITY = 15. Ta có:
   ```
      5 ≤ 6 ≤ 15
   ```  
   - Do đó, priority của EXTI1 nằm trong khoảng cho phép gọi API FreeRTOS từ ISR. Ngắt EXTI1 ở mức 6 có ưu tiên phần cứng thấp hơn mức 5 nên chịu sự che ngắt bằng BASEPRI khi kernel bảo vệ dữ liệu quan trọng, tránh truy cập đồng thời gây sai lệch dữ liệu kernel. Vì vậy, EXTI1 được phép gọi vTaskNotifyGiveFromISR() để đánh thức ack_task.
2. Khi đặt priority EXTI cao hơn ngưỡng cho phép rồi gọi
   `vTaskNotifyGiveFromISR`, bạn dự đoán hành vi quan sát được là gì (reset?
   treo? thông báo gì trên UART/debugger)?
   - Em dự đoán khi đặt priority EXTI bằng 4, cao hơn ngưỡng cho phép 5 về ưu tiên phần cứng, chương trình vẫn chạy cho đến khi có cạnh xuống trên PB1. Khi ISR gọi vTaskNotifyGiveFromISR(), FreeRTOS kiểm tra priority ngắt và kích hoạt configASSERT vì 4 < 5, vi phạm điều kiện gọi API FromISR.
   - Nếu configASSERT được định nghĩa là vô hiệu hóa ngắt rồi lặp vô hạn, chương trình sẽ dừng tại vòng lặp vô hạn assert; ack_task không được đánh thức để đảo PA6. Khi tạm dừng bằng debugger, em dự đoán thấy CPU nằm trong phần xử lý assert, với call stack liên quan đến vPortValidateInterruptPriority().
3. Dự đoán latency ISR → task (có `portYIELD_FROM_ISR`) theo đơn vị us, dựa
   trên tần số CPU 72 MHz và số lệnh ước lượng trong đường đi ISR → context
   switch. So với `LATENCY_BUDGET_US`: đạt hay không?
   - Em ước lượng đường xử lý gồm: CPU tiếp nhận ngắt và vào `EXTI1_IRQHandler()` khoảng 20 chu kỳ; HAL kiểm tra, xóa cờ EXTI và gọi `exti_button_isr_handler()` khoảng 120 chu kỳ; `vTaskNotifyGiveFromISR()` kiểm tra priority, cập nhật notification và đưa `ack_task` sang Ready khoảng 300 chu kỳ; yêu cầu PendSV và chuyển sang xử lý PendSV khoảng 30 chu kỳ; lưu ngữ cảnh task cũ, chọn và khôi phục ngữ cảnh `ack_task` khoảng 200 chu kỳ; task trở về từ hàm chờ và gọi `HAL_GPIO_TogglePin()` khoảng 50 chu kỳ.
   - Tổng cộng khoảng 720 chu kỳ CPU. Với tần số 72 MHz, mỗi µs tương ứng 72 chu kỳ, nên latency dự đoán là `720 / 72 = 10 µs`. Giá trị này nhỏ hơn `LATENCY_BUDGET_US = 80 µs`, vì vậy em dự đoán đạt yêu cầu.
4. Nếu bỏ `portYIELD_FROM_ISR`, `ack_task` sẽ chạy **khi nào** (ngay lập tức
   hay phải chờ tick ngắt kế tiếp)? Latency tăng thêm cỡ bao nhiêu?
   - Nếu bỏ portYIELD_FROM_ISR(), vTaskNotifyGiveFromISR() vẫn đưa ack_task từ Blocked sang Ready nhưng không yêu cầu chuyển ngữ cảnh ngay khi thoát ISR. Trong cấu hình bài này, CPU thường quay lại chạy Idle và ack_task phải chờ đến ngắt SysTick tiếp theo để được lập lịch. Với configTICK_RATE_HZ = 1000, một tick dài 1000 µs, nên latency dự đoán tăng thêm từ gần 0 đến 1000 µs. Lấy trung bình thì thời gian đo được khoảng 510 us.