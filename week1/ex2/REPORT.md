# Report — Week 1 / Ex2

`student_id`: 2510379    Commit của PREDICTION.md: 6a33811

## 1. CPU share đo được
| Cấu hình | A (%) | B (%) | C (%) | Time slice đo được | Ảnh |
|---|---|---|---|---|---|
| B2: preempt=1, ưu tiên khác nhau | 100 | 0 | 0 | — | [ex2_1.png](image/ex2_1.jpg) |
| B3: preempt=0, ưu tiên khác nhau | 100 | 0 | 0 | — | [ex2_2.png](image/ex2_2.jpg) |
| B4: preempt=1, cùng ưu tiên | ≈33.33 | ≈33.33 | ≈33.33 | 1 ms | [ex2_3.png](image/ex2_3_1.jpg) |
| B5: sau khi sửa starvation | ≈30 | ≈18 | ≈42 | — | [ex2_4.png](image/ex2_4.jpg) |

Cách tính CPU share từ ảnh PulseView (mô tả ngắn):

**Trả lời:**
- CPU Share = (Tổng thời gian của Task chiếm CPU)/(Tổng thời gian cửa sổ đang đo)*100.

- **Minh chứng bước 4:** [minh_chung_1](image/ex2_3_1.jpg), [minh_chung_2](image/ex2_3_2.jpg), [minh_chung_3](image/ex2_3_3.jpg) 

## 2. Preemption nhìn thấy ở đâu
Trên ảnh **bước 4** (cùng ưu tiên, round-robin), chỉ ra ít nhất **2 thời điểm**
preemption xảy ra. Với mỗi thời điểm: dấu thời gian, task bị cắt, task chiếm CPU,
và vì sao (hết time slice, không phải block).

| t (us) | Task bị preempt | Task chiếm CPU | Lý do |
|---|---|---|---|
| 3328 | B | C | Hết lượt time slice 1 ms; Scheduler chọn C cùng ưu tiên, đang Ready. B không block/yield. |
| 4328 | C | A | Hết lượt time slice 1 ms; Scheduler chọn task tiếp theo là A cùng ưu tiên, đang Ready. C không block/yield. |

Ở **bước 2** (ưu tiên khác nhau, đều busy-loop không block): giải thích vì sao ảnh
của bạn cho thấy **0 thời điểm preemption**, dù chạy bao lâu đi nữa — dựa vào điều kiện preemption của FreeRTOS, không phải "vì ảnh chụp chưa đủ dài".

**Trả lời:**
A có ưu tiên cao nhất và luôn Ready vì chỉ busy-loop, không block và task B, C có ưu tiên thấp hơn nên không thể chiếm CPU từ A. Vì không có task cùng ưu tiên với A để chia lượt và cũng không có task ưu tiên cao hơn trở thành Ready để preempt A nên trong cấu hình này, dù quan sát lâu hơn cũng không xuất hiện chuyển task do preemption. Ngắt SysTick vẫn xảy ra nhưng sau khi xử lý ngắt, CPU tiếp tục chạy A.
## 3. Time slice
Time slice đo được ở bước 4: 1 ms. `configTICK_RATE_HZ` = 1000.
Sai số: 0.002 %. Giải thích sai số: Sai số có thể đến từ độ phân giải lấy mẫu, vị trí đặt con trỏ, thời gian xử lý ngắt, chuyển ngữ cảnh.

## 4. DWT wrap-around
Tại sao so sánh `DWT->CYCCNT > target` bị sai sau ~59 giây, và cách viết đúng:

**Trả lời:**
- DWT->CYCCNT là một biến đếm có 32 bit nên nó sẽ đếm từ 0 đến 2^32 - 1 rồi trở về 0. Với clock 72MHz, T = 2^32/72000000 = 59,65(s). Nếu tính target = start + cycles, target có thể tràn thành số nhỏ. Khi đó, phép so sánh DWT->CYCCNT > target có thể báo hết thời gian ngay dù chưa chờ đủ. Còn 1 trường hợp nữa là target ở gần mốc reset bộ đếm, nếu DWT->CYCCNT bỏ lỡ lúc vượt qua target (CPU tạm dừng task để xử lý ngắt hoặc chạy task khác nên DWT vẫn tiếp tục đếm và có thể tràn về 0 trước) thì sẽ phải đợi thêm khoảng 59s nữa. Vì vậy, cách so sánh mốc tuyệt đối có thể sai.
- Cách viết đúng: 
```text
uint32_t start = DWT->CYCCNT;
while ((uint32_t)(DWT->CYCCNT - start) < cycles) {
}
```
- Giải thích: Cách viết đúng dùng (uint32_t)(DWT->CYCCNT - start) để tính số chu kỳ đã trôi qua. Phép trừ số nguyên không dấu hoạt động theo modulo 2^32, nên khi bộ đếm tràn về 0, kết quả vẫn tính được cả số chu kỳ trước và sau điểm tràn. Vì vậy, vòng lặp chỉ thoát khi đã đủ số chu kỳ cần chờ. Cách này đúng khi số chu kỳ cần chờ được biểu diễn hợp lệ bằng uint32_t và thời gian thực tế từ start đến lần kiểm tra chưa đạt một vòng đếm 2^32 chu kỳ, khoảng 59,65 s ở 72 MHz.
## 5. Sửa starvation
Cách tôi chọn: Thêm vTaskDelay(1) sau mỗi vòng làm việc của cả ba task, giữ ưu tiên A=3, B=1, C=2 và bật preemption.
Vì sao nó hoạt động (dựa vào trạng thái task trong scheduler): Khi gọi vTaskDelay(1), task chuyển từ Running sang Blocked đến tick tiếp theo. Scheduler chọn task có ưu tiên cao nhất trong các task đang Ready(A Blocked thì C chạy nếu C đang Ready; khi cả A và C đều Blocked, B được chạy nếu B đang Ready.). Hết delay, task trở lại Ready; nếu có ưu tiên cao hơn task đang chạy, nó sẽ chiếm CPU và chạy ngay.
Đánh đổi:
- Giảm số vòng xử lý mỗi giây: Trước đây A làm xong 300 µs thì làm vòng tiếp theo ngay. Sau khi thêm delay, A phải chờ tick tiếp theo mới được chạy lại, nên số vòng A thực hiện mỗi giây giảm.
- Tăng thời gian phản hồi: Nếu có việc mới đến khi A đang Blocked, A phải chờ hết delay mới xử lý được.
- Thêm chi phí chuyển task: Mỗi lần A nghỉ để C hoặc B chạy, CPU cần lưu trạng thái task cũ và khôi phục task mới. Việc này cũng tốn một ít thời gian.
- Delay quá ngắn: A và C có thể trở lại Ready quá sớm, khiến B vẫn bị starvation.
- Delay quá dài: Nếu cả ba task đều đang Blocked thì làm tăng thời gian ở Idle task.

## 6. Lệch so với dự đoán
Mỗi mục lệch + nguyên nhân vật lý.

**Trả lời:**
- Ở bước 4: sai số tính được chỉ khoảng 0.002%. Lý do vật lý: có thể đến từ clock của STM32, dây của logic analyzer, độ trễ cạnh GPIO.
- Bước 2 và 3: không lệch vì A chiếm CPU trong toàn bộ thời gian nên CPU share luôn là 100%

## 7. Câu hỏi đánh đổi
Với **bộ ưu tiên của bạn**, nếu `TASK_C_WORK_US` tăng gấp 3 lần thì bảng CPU share ở
bước 2 thay đổi thế nào? Trả lời bằng số suy ra từ dữ liệu của bạn, rồi **đo lại để
kiểm chứng**. Nếu lệch, giải thích.

**Trả lời:**
- Ở bước 2, ưu tiên A=3, B=1, C=2, cả ba task không block. Tăng TASK_C_WORK_US từ 420 lên 1260 µs không làm thay đổi CPU share(A = 100%, B = 0%, C = 0%) vì A luôn Ready và có ưu tiên cao nhất, C không được chạy nên thời gian làm việc của C tăng cũng không ảnh hưởng.
- Hình ảnh đo kiểm chứng: [kiem_chung](image/ex2_5.jpg) 
