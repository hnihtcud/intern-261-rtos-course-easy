# Dự đoán — Week 2 / Ex3

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379    `LOW_HOLD_MS` = 20  `MID_WORK_MS` = 30
`HIGH_PERIOD_MS` = 25 `DEADLINE_MS` = 20

**Cách set up hiện tượng:**
- Chọn ưu tiên: Low = 1, Mid = 2, High = 3. Các mốc thời gian tính từ cùng một mốc khởi động.
- Low lấy khóa tại 20 ms và thực hiện lượng công việc tương đương 20 ms CPU; nếu bị chen vào thì tiếp tục làm phần còn lại.
- High yêu cầu khóa lần đầu tại 25 ms, sau đó theo chu kỳ 25 ms.
- Mid chờ đến 26 ms mới chạy công việc 30 ms. Low và Mid lặp lại mỗi 100 ms.
1. Cấu hình binary semaphore: thời gian `task_high` bị block dự đoán = 45 ms.
   Phép tính dựa trên đâu (liệt kê các thành phần cộng lại)?
- Tại 25 ms, Low đã thực hiện khoảng 5 ms công việc, còn 15 ms. High bị block chờ khóa. Khi Mid thức dậy, binary semaphore không hỗ trợ priority inheritance nên Mid chen vào trước Low trong 30 ms.
- Thời gian High bị block dự đoán:
```
T_block = phần công việc còn lại của Low + thời gian Mid chen vào
        = (20 − 5) + 30
        = 45 ms.
```
- Low dự kiến nhả khóa tại khoảng 70 ms; High chờ từ 25 đến 70 ms.
2. `task_high` có đạt `DEADLINE_MS` ở cấu hình binary semaphore không? Vì sao?
- Dự đoán task_high không đạt DEADLINE_MS = 20 ms trong lượt xảy ra priority inversion. Khi High yêu cầu tài nguyên đang do Low giữ, High phải chuyển sang Blocked. Binary semaphore không có priority inheritance nên Mid có thể chạy trước Low và chiếm CPU trong MID_WORK_MS = 30 ms, khiến Low chậm nhả tài nguyên.
- Nếu Mid bắt đầu chạy ngay sau khi High bị block, riêng thời gian chờ Mid đã khoảng 30 ms > 20 ms, chưa kể phần công việc còn lại của Low và thời gian High xử lý tài nguyên. Vì vậy, High sẽ trễ deadline ở lượt này.
3. Cấu hình mutex: thời gian `task_high` bị block dự đoán = 15 ms. Vì sao
   ngắn hơn (hoặc không) so với câu 1?
- Low lấy mutex tại 20 ms và thực hiện công việc trong 20 ms. Khi High yêu cầu mutex tại 25 ms, High bị block và Low được kế thừa ưu tiên của High. Vì vậy, Mid thức dậy tại 26 ms không thể chen vào; Low tiếp tục chạy và nhả mutex tại khoảng 40 ms.
- Thời gian High bị block = 40 − 25 = 15 ms, ngắn hơn mức 45 ms của binary semaphore vì Mid không thể trì hoãn Low nhả tài nguyên.
4. Trên ảnh logic analyzer ở cấu hình binary semaphore, kênh `task_mid` sẽ ở
   trạng thái nào trong lúc `task_high` bị chặn — chạy liên tục, chạy ngắt
   quãng, hay không chạy? Vì sao?
- Ở cấu hình binary semaphore, dự đoán task_mid chạy liên tục trong khoảng 30 ms của nó, nằm trong thời gian task_high bị chặn.
- High đang Blocked để chờ tài nguyên do Low giữ. Binary semaphore không có priority inheritance nên Low vẫn có ưu tiên thấp hơn Mid. Vì vậy, khi Mid thức dậy, Mid chiếm CPU và trì hoãn Low nhả tài nguyên, khiến High tiếp tục chờ.
5. Nếu đổi `task_mid` sang ưu tiên **thấp hơn** `task_low`, inversion còn xảy ra
   không? Vì sao (đây là cách phân biệt priority inversion với việc chỉ đơn
   giản là "task_low chạy lâu").
- Nếu đặt ưu tiên task_mid thấp hơn task_low, hiện tượng High bị kéo dài thời gian chờ do Mid chen vào sẽ không còn xảy ra. Khi High bị block chờ tài nguyên, Low có ưu tiên cao hơn Mid nên tiếp tục chạy đến khi nhả tài nguyên.
