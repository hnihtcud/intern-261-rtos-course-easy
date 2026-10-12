# Dự đoán — Week 1 / Ex2

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379   Ưu tiên của tôi: A=3 B=1 C=2

1. Bước 2 (`configUSE_PREEMPTION`=1, ưu tiên khác nhau): task nào chạy? task nào
   **không bao giờ** chạy? CPU share dự đoán:

- Task chạy: A vì có ưu tiên cao nhất và không block.
- Task không bao giờ chạy: B, C. Vì task A có ưu tiên cao nhất, dùng busy-loop nên sẽ luôn được Scheduler chọn. 
   | Task | CPU share dự đoán (%) |
   |---|---|
   | A | 100 |
   | B | 0 |
   | C | 0 |

2. Bước 3 (`configUSE_PREEMPTION`=0): dự đoán thay đổi gì so với bước 2? Vì sao?

- Khi `configUSE_PREEMPTION`=0, RTOS dùng lập lịch hợp tác thì task A vẫn sẽ luôn được running còn task B và C thì vẫn bị starvation vì khi Scheduler bắt đầu, A có ưu tiên cao nhất nên sẽ được chọn đồng thời do tắt preemption và A cũng không block hay yield nên A sẽ giữ luôn CPU  

3. Bước 4 (3 task cùng ưu tiên): time slice dự đoán = 1 ms. Dựa vào đâu?

- Time slice dự đoán là 1 ms, dựa trên configTICK_RATE_HZ = 1000 Hz, nên chu kỳ tick bằng 1000/1000 = 1 ms. Khi bật preemption và time slicing, ba task cùng ưu tiên và luôn Ready sẽ luân phiên theo mỗi tick.

4. Bước 5: tôi sẽ sửa starvation bằng cách: thêm vTaskDelay() sau mỗi vòng làm việc của từng task . Cách này đánh đổi gì?

- Sửa starvation bằng cách thêm vTaskDelay() với số tick khác 0 sau mỗi vòng làm việc của cả 3 task. Khi task có ưu tiên cao hơn bị đưa vào blocked, các task có ưu tiên thấp hơn sẽ được chọn để chuyển sang running.
- Đánh đổi: thêm delay giúp task thấp có cơ hội chạy, nhưng làm giảm số lượt xử lý mỗi giây và có thể tăng thời gian phản hồi với công việc đến trong lúc task đang Blocked. Delay quá ngắn có thể chưa giải quyết starvation; delay quá dài làm tăng thời gian Idle và giảm thông lượng xử lý.
5. Chân nào sẽ không bao giờ đổi trạng thái trên logic analyzer ở bước 2? PA4 (Task B) và PA6 (Task C) do bị starvation.
