# Report — Week 2 / Ex2

`student_id`: 2510379    Commit của PREDICTION.md: 785f639

## 1. Lỗi tái hiện (UART_GUARD_MODE = 0)
Log UART dán nguyên văn, đánh dấu vị trí ký tự lẫn lộn:

**Trả lời:**
```
00:1070 AA0A1A:A1A0A70 BBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
000023:::1071 AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
0023::110072 DDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
02:1073 CCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
01:1074 BB0B0B:B1B0B74 AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
```
- Vị trí ký tự bị lẫn:
    + Dòng bắt đầu bằng `00:1070`: đoạn **`AA0A1A:A1A0A70`** có các ký tự số và dấu `:` chen giữa các chữ `A` của writer 0, sau đó xuất hiện chuỗi `B` của writer 1.
    + Dòng bắt đầu bằng `01:1074`: đoạn **`BB0B0B:B1B0B74`** có các ký tự thuộc phần nhận diện của writer 0 chen giữa các chữ `B` của writer 1, sau đó xuất hiện chuỗi `A`.
- Nguyên nhân: các writer cùng sử dụng UART nhưng không bảo vệ toàn bộ bản tin. Mỗi ký tự được truyền bằng một lần gọi `HAL_UART_Transmit()`. Trong mode 0, em thêm `taskYIELD()` sau khi truyền thành công từng ký tự để chủ động tạo cơ hội chuyển sang writer khác khi bản tin hiện tại chưa hoàn thành. Vì vậy, các ký tự của nhiều bản tin bị xen kẽ.
- Tái hiện lại bao nhiêu lần / bao nhiêu lần thấy lỗi: 10 / 10.

## 2. Bảng so sánh 3 cách
| Cách | Độ trễ in trung bình (us) | Độ trễ in max (us) | Ảnh hưởng task khác | RAM thêm (bytes) |
|---|---|---|---|---|
| Mutex | 8622.5898 | 14795.9727 | 72444 chu kỳ | 96 |
| Critical section | 8482.5654 | 20187.5654 | 285070 chu kỳ | 0 |
| Gatekeeper | 8606.8955 | 20421.291 | 72258 chu kỳ | 1392 |

Cách đo độ trễ in (mô tả ngắn, kèm ảnh logic analyzer nếu dùng pin đánh dấu):
- Độ trễ in được đo bằng DWT->CYCCNT. Sau khi uart_guard_send() trả về, mỗi writer đặt mốc yêu cầu tiếp theo sau 7 ms theo DWT và chờ bằng các lần vTaskDelay(1). Mốc bắt đầu đo là mốc yêu cầu đã định, kể cả writer phải chờ tại thời điểm đó. Mốc kết thúc là khi UART truyền xong bản tin 40 byte; riêng Gatekeeper, mốc bắt đầu được lưu theo ID writer và số thứ tự bản tin, rồi lấy mốc kết thúc trong Gatekeeper.
- Phép đo bao gồm độ trễ đánh thức, chờ CPU, chuẩn bị bản tin, chờ mutex hoặc queue và truyền UART. Thời gian được tính bằng (uint32_t)(end − start) / (SystemCoreClock / 1000000), đơn vị µs. Mỗi mode thu tổng cộng 400 bản tin truyền thành công để tính trung bình và lớn nhất. Lần đầu của mỗi writer lấy mốc khi task bắt đầu chạy. 
- Ảnh hưởng lên task khác được em đo bằng task Probe ưu tiên 4, cao hơn writer và Gatekeeper. Probe dùng vTaskDelayUntil() với chu kỳ 1 tick = 1 ms, đọc DWT mỗi lần chạy và lưu khoảng cách lớn nhất giữa hai lần liên tiếp. Em nhận xét là Critical section ảnh hưởng nhiều đến những task khác vì khi em để chu kỳ 1 tick thì nếu đúng, khoảng cách giữa 2 lần chạy liên tục khoảng 72000-73000 chu kỳ (SystemCoreClock = 72 MHz), Critical section vì có đặc điểm chặn các ngắt nên cả ngắt PendSV và Systick cũng bị chặn, dẫn đến việc không thể chuyển task. 
## 3. Vì sao critical section nguy hiểm nhất
Giải thích bằng số đo của bạn: đoạn in dài bao nhiêu us khi ở trong critical
section, và điều đó chặn ngắt nào trong hệ thống bao lâu.

**Trả lời:**
- Thời gian giữ critical: đoạn in trong critical section kéo dài trung bình khoảng 3823,34 µs.
- Các ngắt bị che: Với configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5, critical section che các ngắt có mức ưu tiên số 5–15, gồm SysTick và PendSV ở mức 15. Ngắt phát sinh ngay sau khi vào critical có thể phải chờ gần 3,84 ms; các ngắt ưu tiên 0–4 vẫn được phép chạy.
- Ảnh hưởng đến SysTick: SysTick cập nhật tick count và kiểm tra các task hết thời gian chờ. Khi bị che, task đang delay hoặc chờ timeout của queue, mutex có thể thức dậy muộn. SysTick có thể không được đếm đầy đủ vì bị chặn ngắt khiến thời gian RTOS chậm so với thời gian mong muốn.
- Ảnh hưởng đến PendSV: PendSV thực hiện chuyển ngữ cảnh. Khi bị che, CPU chưa thể chuyển sang task khác, kể cả task ưu tiên cao hơn đã Ready; task đó phải chờ đến khi thoát critical.
- Ảnh hưởng đến ngoại vi: Ngắt ngoại vi thuộc nhóm ưu tiên 5–15 cũng bị trì hoãn. Ví dụ, nếu ngắt UART RX bị che và dữ liệu tiếp tục đến, phần cứng có thể bị overrun, gây mất dữ liệu.
- Do đó Critical section nguy hiểm nhất: Critical section giữ việc che ngắt suốt quá trình truyền UART, làm giảm khả năng đáp ứng của cả hệ thống, kể cả các task không dùng UART. Mutex và gatekeeper vẫn cho phép ngắt và chuyển task trong lúc truyền, nên không gây che ngắt kéo dài như vậy.
## 4. Lệch so với dự đoán
Mỗi mục lệch + nguyên nhân vật lý.

**Trả lời:**
- Theo dự đoán của em thì thứ tự sẽ là mutex <= gatekeeper <= critical section nhưng sau khi đo lại thì thứ tự thực tế là: critical section < gatekeeper < mutex. Nguyên nhân được giải thích như sau:
    + Mutex: Em dự đoán có latency thấp nhất, nhưng đo được trung bình cao nhất (8622,59 µs). Lý do là ngoài chờ mutex và truyền UART, writer còn có thể bị ngắt hoặc chuyển task khi đang giữ mutex do time slicing, từ đó làm tăng thêm chi phí để chuyển task.
    + Critical section: Em dự đoán trễ cao nhất, nhưng trung bình lại thấp nhất (8482,57 µs). Trong đoạn truyền, SysTick và PendSV bị che nên writer không bị chuyển task, giúp hoàn thành bản tin liền mạch hơn. Tuy nhiên, các writer chưa được chạy phải chờ, nên latency lớn nhất vẫn đạt 20187,57 µs. Khoảng cách lớn nhất giữa hai lần chạy của task Probe cũng đạt 285070 chu kỳ, cao hơn rõ rệt hai cách còn lại.
    + Gatekeeper: Trung bình nằm giữa (8606,90 µs), nhưng latency lớn nhất lại cao nhất (20421,29 µs). Phép đo tính cả thời gian writer chờ CPU trước khi gửi queue, chờ Gatekeeper xử lý và truyền UART. Gatekeeper có ưu tiên cao hơn writer nên thường xử lý nhanh khi nhận bản tin, nhưng trong lúc nó truyền bằng polling, các writer ưu tiên thấp hơn phải chờ CPU. Do đó, một số bản tin có thể trễ nhiều dù trung bình không cao nhất.

## 5. Câu hỏi đánh đổi
Với `WRITER_COUNT` của **bạn**, nếu tăng gấp đôi thì cách nào bị ảnh hưởng độ trễ
nhiều nhất? Trả lời bằng số từ dữ liệu của bạn, rồi đo lại để kiểm chứng.

**Trả lời:** 
- Em dự đoán Mutex bị tăng độ trễ nhiều nhất khi `WRITER_COUNT` tăng từ 4 lên 8. Nhiều writer cùng tranh UART làm thời gian chờ mutex tăng. Ngoài ra, writer giữ mutex vẫn có thể bị chuyển lượt sang writer cùng ưu tiên, phát sinh thêm chi phí lập lịch.

- Đo kiểm chứng:

| Cách | Latency trung bình với 4 writer (µs) | Latency trung bình với 8 writer (µs) | Mức tăng (µs) |
|---|---:|---:|---:|
| Mutex | 8622.5898 | 24209.0879 | 15586.4981 |
| Critical section | 8482.5654 | 23653.1641 | 15170.5987 |
| Gatekeeper |8606.8955 | 23911.0508 | 15304.1553 |