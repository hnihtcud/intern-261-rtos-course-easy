# Dự đoán — Week 2 / Ex2

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379    `WRITER_COUNT` = 4   `MSG_LEN` = 40

1. Ở `UART_GUARD_MODE = 0`: dự đoán lỗi lẫn lộn sẽ xuất hiện sau khoảng bao
   nhiêu dòng log? Vì sao MSG_LEN và WRITER_PERIOD_MS của bạn ảnh hưởng đến
   con số này?
- Em dự đoán lỗi lẫn ký tự có thể xuất hiện trong vài dòng log đầu tiên, giả sử bản tin được truyền qua nhiều lần gọi và không có bảo vệ toàn bộ bản tin. Mỗi bản tin 40 byte cần khoảng 3,47 ms ở baud 115200. Với 4 writer, dù không bắt đầu đồng thời, thời gian in vẫn có thể chồng nhau. 
- MSG_LEN càng lớn thì thời gian truyền càng dài, tăng cơ hội bị chen vào; WRITER_PERIOD_MS càng nhỏ thì các writer quay lại in thường xuyên hơn, tăng khả năng tranh chấp. 
2. Dự đoán thứ tự **độ trễ in** (latency) từ thấp đến cao giữa mutex / critical
   section / gatekeeper. Vì sao?
- Dự đoán độ trễ in từ thấp đến cao: mutex <= gatekeeper <= critical section.
   + Mutex: writer có thể phải chờ CPU và chờ lấy mutex, nhưng ngắt tick và việc lập lịch vẫn hoạt động, giúp các task được đánh thức bình thường.
   + Gatekeeper: latency gồm thời gian writer chờ CPU, chuẩn bị và gửi bản tin vào queue, chờ Gatekeeper và truyền UART. Gatekeeper có ưu tiên cao hơn writer nên thường xử lý ngay khi nhận bản tin; tuy nhiên, lúc Gatekeeper truyền bằng polling, các writer ưu tiên thấp hơn phải chờ.
   + Critical section: phép đo mới tính cả khoảng writer đã đến mốc yêu cầu nhưng chưa được chạy. Writer khác giữ critical sẽ che SysTick/PendSV, trì hoãn việc đánh thức và chuyển task, nên em dự đoán latency lớn nhất.
- Tóm lại, với bản tin 40 byte, UART 115200 baud, 8N1, riêng thời gian truyền đã khoảng 3,47 ms ở cả ba cách. Phần chênh lệch chủ yếu đến từ thời gian chờ và lập lịch.
3. Với critical section, task nào trong hệ thống có nguy cơ bị ảnh hưởng xấu nhất nếu đoạn in quá dài? (gợi ý: nghĩ đến ngắt, không chỉ task)
- Bị ảnh hưởng xấu nhất là các task cần phản hồi đúng hạn, đặc biệt task phải chờ sự kiện từ ngắt như task nhận dữ liệu UART hoặc xử lý mẫu cảm biến.
- Với cấu hình trong file config của em, critical section che các ngắt có mức ưu tiên số từ 5 đến 15, gồm SysTick và PendSV. Nếu giữ critical section suốt lúc in:
   + Ngắt ngoại vi bị che phải chờ: UART RX có thể bị overrun, mất dữ liệu nếu không được xử lý kịp.
   + SysTick bị trì hoãn: các task đang chờ delay có thể thức dậy muộn. Nhiều tick xảy ra trong lúc bị che có thể không được đếm đủ.
   + Chuyển task bị trì hoãn: ngay cả task có ưu tiên cao hơn cũng không được chuyển sang chạy kịp thời.
- Với thông số MSG_LEN = 40, UART 115200 baud, 8N1, thời gian truyền tối thiểu khoảng 3,47 ms, dài hơn ba chu kỳ tick 1 ms. Vì vậy, em nghĩ chỉ một lần in trong critical section đã có thể gây trễ đáng kể.
4. RAM dùng thêm của gatekeeper so với mutex: dự đoán tăng hay giảm, vì sao?
- Em dự đoán gatekeeper dùng thêm RAM nhiều hơn mutex. Mutex chỉ cần một đối tượng quản lý quyền sử dụng UART; các writer vẫn tự truyền bản tin và không cần thêm task.
- Gatekeeper cần thêm stack và TCB của task chuyên truyền UART, cùng queue lưu các bản tin đang chờ. Mỗi phần tử queue chứa một bản sao của bản tin MSG_LEN = 40 byte, nên queue dài Q phần tử cần Q × 40 byte dữ liệu. Vì vậy, gatekeeper tốn RAM hơn so với Mutex.
5. Nếu `WRITER_COUNT` tăng gấp đôi, cách nào trong 3 cách bị ảnh hưởng về độ trễ
   nhiều nhất? Vì sao?
- Em dự đoán critical section bị tăng độ trễ nhiều nhất khi WRITER_COUNT tăng từ 4 lên 8, với phép đo bao gồm thời gian writer đã đến mốc yêu cầu nhưng chưa được chạy.
- Mỗi bản tin 40 byte cần tối thiểu khoảng 3,47 ms để truyền ở 115200 baud. Khi có nhiều writer tranh UART hơn, mỗi writer có thể phải chờ nhiều lượt truyền hơn. Trong critical section, SysTick và PendSV bị che, làm trì hoãn việc đánh thức và chuyển sang các writer đang chờ.
