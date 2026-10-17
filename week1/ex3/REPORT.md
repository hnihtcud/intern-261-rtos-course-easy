# Report — Week 1 / Ex3

`student_id`: 2510379    Commit của PREDICTION.md: ______

## 1. Bảng thống kê (tính từ CSV, không đọc tay)
| # | Delay | Tick | min (ms) | max (ms) | tb (ms) | jitter = max−min (us) | drift tổng (ms) |
|---|---|---|---|---|---|---|---|
| 1 | `vTaskDelay` | 1000 |16.999 | 18.01 | 17.35704 | 1011 | 178.52 |
| 2 | `xTaskDelayUntil` | 1000 | 16.999 | 17.011 | 17.001 | 12 | 0.5 |
| 3 | `vTaskDelay` | 100 | 9.999 | 10.011 | 10.0006 | 12 | -3499.7 |
| 4 | `xTaskDelayUntil` | 100 | 9.999 | 10.010 | 10.0006 | 11 | -3499.7 |

- Script dùng để tính: [analyze_ex3.py](./tool/analyze_ex3.py)
- CSV gốc: [CSV](./tool/) (commit kèm)
- Ảnh: [4_images_of_PulseView](./images/)
- Mỗi cấu hình lấy 501 cạnh chuyển mức liên tiếp trên D0 để tính đúng 500 chu kỳ, tính cả cạnh lên và cạnh xuống.
- Drift tổng = (thời điểm cạnh cuối − thời điểm cạnh đầu) − 500 × 17 ms.
## 2. Tại sao `vTaskDelay` bị drift
Công thức drift theo `PERIOD_MS` và `WORK_JITTER_US` của **bạn**, rồi so với số đo:
- Trường hợp 1: vTaskDelay(), 1000Hz. Công thức áp dụng:
```
Giả sử tải phân bố đều từ 0 đến 1400 µs.
Tỉ lệ tải lớn hơn 1 ms: A = (WORK_JITTER_US − 1000) / WORK_JITTER_US  
Jitter ≈ 18 − 17 = 1 ms = 1000 µs = 1 ms
Drift ≈ SAMPLE_COUNT × A x Thời gian dư mỗi vòng
```
- vTaskDelay() tính thời gian chờ tương đối từ tick tại lúc gọi hàm. Nếu phần xử lý vượt sang tick tiếp theo trước khi gọi delay, chu kỳ có thể kéo dài thêm một tick. Sự kéo dài này tích lũy qua nhiều vòng thành drift. Trong 500 chu kỳ đo được, có 178 chu kỳ gần 18 ms và 322 chu kỳ gần 17 ms, cho drift tổng +178,52 ms.

| | Tính theo công thức | Đo được | Lệch |
|---|---|---|---|
| Drift tổng (ms) | 142.86 | 178.52 | 35.66 |

- Trường hợp 3 và 4. Công thức áp dụng: 
```
Drift ≈ SAMPLE_COUNT × (T_tick − PERIOD_MS)
```

| | Tính theo công thức | Đo được | Lệch |
|---|---|---|---|
| Drift tổng (ms) | -3500 | -3499.7 | 0.3 |

## 3. Tại sao tick 100 Hz sai nhiều hơn
`PERIOD_MS` = 17 ; một tick = 10 ms. Chứng minh bằng số đo, không bằng lý thuyết:

**Trả lời:**
- PERIOD_MS = 17 ms; ở tick rate 100 Hz, một tick = 10 ms. Kết quả đo cho thấy cả hai hàm delay ở 100 Hz đều có chu kỳ trung bình 10,00060 ms, ngắn hơn mục tiêu 6,99940 ms mỗi chu kỳ, tương ứng sai lệch khoảng −41,17%. Sau 500 chu kỳ, drift là −3499,70 ms. 
- Ở 1000 Hz, chu kỳ trung bình của vTaskDelay là 17,35704 ms, drift +178,52 ms; của xTaskDelayUntil là 17,00100 ms, drift chỉ +0,50 ms. 
- Như vậy, số đo xác nhận cấu hình 100 Hz sai nhiều hơn vì chu kỳ thực tế gần 10 ms thay vì 17 ms,  thể hiện việc pdMS_TO_TICKS(17) quy đổi xuống 1 tick.
## 4. `xTaskDelayUntil` trả về gì
Giá trị trả về nghĩa là gì, và trong 4 lần chạy của bạn nó có bao giờ báo
"đã trễ deadline" không? Bao nhiêu lần / `SAMPLE_COUNT`?

**Trả lời:**

- xTaskDelayUntil() trả về kiểu BaseType_t:
    + pdTRUE: task đã được đưa vào trạng thái Blocked để chờ mốc lịch tiếp theo.
    + pdFALSE: khi gọi hàm, tick hiện tại đã bằng hoặc vượt mốc lịch cần chờ, nên task tiếp tục chạy ngay.
- Kết quả đếm ở hai cấu hình xTaskDelayUntil với tick rate 1000 Hz và 100 Hz đều là 0/500 lần trả về pdFALSE. Tải tối đa 1,4 ms nên task sẽ luôn hoàn thành trước mốc tiếp theo, cách nhau 17 ms ở 1000 Hz và 10 ms ở 100 Hz.

## 5. Lệch so với dự đoán
Mỗi dòng lệch trong bảng → lệch bao nhiêu + nguyên nhân vật lý.

| Cấu hình | Đại lượng | Dự đoán | Đo được | Độ lệch (đo − dự đoán) | Nguyên nhân có thể gây lệch |
|---|---|---:|---:|---:|---|
| `vTaskDelay`, 1000 Hz | Chu kỳ trung bình | 17,285714 ms | 17,357040 ms | **+0,071326 ms** | tỷ lệ tải trong 500 vòng khác mô hình phân bố đều lý tưởng. |
| `vTaskDelay`, 1000 Hz | Jitter | 1000 µs | 1011 µs | **+11 µs** | Độ trễ thực thi GPIO, sai số trên dây của logic analyzer. |
| `vTaskDelay`, 1000 Hz | Drift tổng | +142,86 ms | +178,52 ms | **+35,66 ms** | Sai số này có thể do bộ phân phối ngẫu nhiên không đúng với lý tưởng. |
| `xTaskDelayUntil`, 1000 Hz | Chu kỳ trung bình | 17 ms | 17,001 ms | **+0,001 ms** | Gần như chính xác, có thể do sai lệch clock giữa STM32 và logic analyzer, nhiễu. |
| `xTaskDelayUntil`, 1000 Hz | Jitter | ≈0 µs | 12 µs | **≈+12 µs** | Độ trễ không đều từ lúc hết chờ đến lúc toggle GPIO, sai số trên dây. |
| `xTaskDelayUntil`, 1000 Hz | Drift tổng | ≈0 ms | +0,50 ms | **≈+0,50 ms** | Sai lệch clock nhỏ có thể tích lũy qua 500 chu kỳ. |
| `vTaskDelay`, 100 Hz | Chu kỳ trung bình | 10 ms | 10,00060 ms | **+0,00060 ms** | Gần như chính xác với dự đoán. |
| `vTaskDelay`, 100 Hz | Jitter | ≈0 µs | 12 µs | **≈+12 µs** | Có thể do nhiễu, sai số trên dây, độ trễ ngắt và lập lịch. |
| `vTaskDelay`, 100 Hz | Drift tổng | −3500 ms | −3499,70 ms | **+0,30 ms** | Chu kỳ trung bình dài hơn dự đoán 0,00060 ms, tích lũy qua 500 chu kỳ. |
| `xTaskDelayUntil`, 100 Hz | Chu kỳ trung bình | 10 ms | 10,00060 ms | **+0,00060 ms** | Gần đúng với dự đoán, có thể do sai lệch clock giữa STM32 và logic analyzer, nhiễu. |
| `xTaskDelayUntil`, 100 Hz | Jitter | ≈0 µs | 11 µs | **≈+11 µs** | Có thể do nhiễu, sai số trên dây, độ trễ ngắt và lập lịch. |
| `xTaskDelayUntil`, 100 Hz | Drift tổng | −3500 ms | −3499,70 ms | **+0,30 ms** | Sai lệch clock nhỏ có thể tích lũy qua khoảng đo. |

## 6. Câu hỏi đánh đổi
Tick 1000 Hz cho độ chính xác tốt hơn nhưng **tốn** gì? Dựa vào dữ liệu của bạn:
đo CPU overhead của tick ISR ở 100 Hz và 1000 Hz (gợi ý: cho một task ưu tiên thấp
nhất đếm vòng lặp trong 1 giây, so sánh 2 cấu hình). Điền số:

| Tick rate | Số vòng lặp idle đếm được trong 1 s | Overhead suy ra (%) |
|---|---|---|
| 100 Hz | 337350 | 0% (mốc so sánh) |
| 1000 Hz | 329000 | 2.48% |

Với ứng dụng `PERIOD_MS` = 17 của bạn, bạn chọn tick rate nào? Bảo vệ bằng số trên.

**Trả lời:**
- Em chọn tick rate 1000 Hz cho ứng dụng có PERIOD_MS = 17 ms. Khi tăng từ 100 Hz lên 1000 Hz, số vòng lặp đếm được trong 1 giây giảm từ 337350 xuống 329000 - khoảng 2,48% so với mốc 100 Hz. CPU phải dành thêm thời gian xử lý ngắt tick và hoạt động lập lịch, nên thời gian dành cho task giảm.
- Đổi lại, với xTaskDelayUntil(), cấu hình 1000 Hz cho chu kỳ trung bình 17,001 ms, drift chỉ +0,50 ms sau 500 chu kỳ. Ở 100 Hz, chu kỳ trung bình là 10,00060 ms, drift −3499,70 ms. Vì vậy, em chọn 1000 Hz kết hợp xTaskDelayUntil(), chấp nhận chi phí xử lý tăng thêm để đạt chu kỳ gần mục tiêu 17 ms.
