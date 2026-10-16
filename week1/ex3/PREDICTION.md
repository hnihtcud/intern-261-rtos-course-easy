# Dự đoán — Week 1 / Ex3

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379    `PERIOD_MS` = 17   `WORK_JITTER_US` = 1400    `SAMPLE_COUNT` = 500

1. Bảng dự đoán (điền trước khi đo):

| # | Delay | Tick | Chu kỳ tb (ms) | Jitter (us) | Drift tổng sau SAMPLE_COUNT (ms) |
|---|---|---|---|---|---|
| 1 | `vTaskDelay` | 1000 | ~17,2857 | ~1000 | ~142.86 |
| 2 | `xTaskDelayUntil` | 1000 | ~17 | ~0 | ~0 |
| 3 | `vTaskDelay` | 100 | ~10 | ~0 | -3500 |
| 4 | `xTaskDelayUntil` | 100 | ~10 | ~0 | -3500 |

2. Cấu hình nào có drift lớn nhất? Vì sao, viết bằng công thức có `WORK_JITTER_US`.

**Trả lời:**
- Cấu hình ở trường hợp 3 và 4 có độ lớn drift lớn nhất.
- Trường hợp 3 và 4: tick rate 100 Hz

   + Một tick dài 10 ms. Khi dùng pdMS_TO_TICKS(17) mặc định,
phép chia nguyên cho 1 tick, tương ứng 10 ms.
   + Tải tối đa: `WORK_JITTER_US / 1000 = 1,4 ms < 10 ms`
   + Nếu task bắt đầu gần đầu tick, phần tải giả hoàn thành trong cùng
tick. Cấu hình 3 chưa bị kéo dài thêm tick do tải; cấu hình 4 vẫn
kịp các mốc lịch 10 ms. Cả hai có chu kỳ dự đoán gần 10 ms.
```
   Drift ≈ SAMPLE_COUNT × (T_tick − PERIOD_MS)
       = 500 × (10 − 17)
       = −3500 ms.
```
   + Nguyên nhân chính là việc quy đổi chu kỳ yêu cầu 17 ms xuống
10 ms. Task hoàn thành 500 chu kỳ sớm hơn lịch mong muốn khoảng 3,5 giây.

- Trường hợp 1: vTaskDelay ở 1000 Hz
   + Một tick dài 1000 µs. Tải tối đa là: WORK_JITTER_US / 1000 = 1400 / 1000 = 1,4 ms
   + Khi task bắt đầu gần đầu tick: Tải dưới 1000 µs: gọi delay trong cùng tick, chu kỳ khoảng 17 ms. Tải từ 1000 đến 1400 µs: tick count đã tăng thêm 1 trước khi gọi delay, chu kỳ khoảng 18 ms.
   + Nếu ta giả sử tải phân bố đều liên tục, ta tính được tỷ lệ vòng kéo dài thêm một tick:
   ```text
   A = (WORK_JITTER_US − 1000) / WORK_JITTER_US  
     = (1400 − 1000) / 1400 
     ≈ 0,285714 = 28,57%
   ```
   + Chu kỳ trung bình ≈ 17 × (1 − A) + 18 × A ≈ 17,285714 ms
   + Jitter ≈ 18 − 17 = 1 ms = 1000 µs = 1 ms
   + Drift ≈ SAMPLE_COUNT × A x Thời gian dư mỗi vòng
          = 500 × (400 / 1400) x 1
          ≈ 142,86 ms

- Trường hợp 2: xTaskDelayUntil ở 1000 Hz

   + Hàm giữ các mốc lịch cách nhau 17 tick, tương ứng 17 ms. Tải tối đa 1,4 ms nhỏ hơn chu kỳ 17 ms nên task có thể hoàn thành trước mốc tiếp theo. Vì vậy, chu kỳ gần 17 ms, jitter gần 0 và drift gần 0.

3. `PERIOD_MS` = 17 . Với tick 100 Hz, một tick = 10 ms, thời gian delay 17 ms được quy đổi xuống 1 tick, tương ứng 10 ms. Sai số hệ thống: `(10 − 17) / 17 × 100 ≈ −41,176%`.
4. Jitter ở tick 1000 Hz so với 100 Hz: lớn hơn / nhỏ hơn / bằng? Vì sao?

**Trả lời:**
- Với vTaskDelay(), jitter ở 1000 Hz được dự đoán lớn hơn ở 100 Hz.
   + Ở 1000 Hz, tải tối đa 1,4 ms > 1 ms (1 tick), tạo các chu kỳ khoảng 17 ms và 18 ms. Vì vậy, jitter khoảng 1000 µs.
   + Ở 100 Hz, tải tối đa 1,4 ms nhỏ hơn một tick 10 ms. Khi chạy gần đầu tick, các chu kỳ đều khoảng 10 ms nên jitter gần 0.
- Với xTaskDelayUntil(), jitter ở hai tick rate đều được dự đoán gần 0 vì task được chạy ngay khi hết thời gian chờ. Tuy nhiên, pha khởi đầu có thể làm jitter đo được khác 0.
5. Nếu task này **không** phải ưu tiên cao nhất, đại lượng nào trong bảng bị ảnh hưởng
   nhiều nhất? 

**Trả lời:**
- Jitter đự đoán bị ảnh hưởng rõ nhất. Khi hết delay, task chỉ chuyển sang Ready và có thể phải chờ thêm task có ưu tiên cao hơn chạy xong mới được chạy. Với vTaskDelay(), sự trì hoãn có thể tích lũy thành drift. Với xTaskDelayUntil(), vì cách đo lấy mốc nên drift xấp xỉ 0.