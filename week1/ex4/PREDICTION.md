# Dự đoán — Week 1 / Ex4

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510379    `HEAP_SIZE_BYTES` = 9216    `DYN_TASK_STACK_WORDS` = 160
`STATIC_TASK_STACK_WORDS` = 96    `HOG_DEPTH_BYTES` = 640    `TARGET_FREE_HEAP_BYTES` = 4096

1. Ngân sách RAM dự đoán (tổng phải = 20480 bytes):

| Mục | Bytes dự đoán |
|---|---|
| `.data` | 100 |
| `.bss` (không kể heap FreeRTOS) | 2500 |
| Heap FreeRTOS (`ucHeap`) | 9216 |
| Stack của main (MSP) |1024 |
| Còn lại / chưa dùng | 7640 |
| **Tổng** | **20480** |

2. `xPortGetFreeHeapSize()` ngay sau khi scheduler chạy: 6296 bytes.
   `HEAP_SIZE_BYTES` − số này = 2920 . Số đó là của những gì?
- Phần chênh lệch gồm:
   + 640 byte của stack `dyn_task`
   + 2048 byte của stack `ram_report_task`
   + 92*2 = 184 byte là tổng TCB của 2 task.
   + 8 byte cuối dành cho mốc cuối heap.
   + 40 byte dành cho header và căn chỉnh của các khối cấp phát.
3. High water mark dự đoán (words): dyn 128 , stat 64 , idle 96 , timer 80 .
4. `dyn_task` có stack 160 words = 640 bytes. Mảng `HOG_DEPTH_BYTES` = 640 bytes.
   Hook tràn stack **có** chạy không? Vì sao?
- Ngoài mảng, task còn cần stack cho ngữ cảnh CPU, biến cục bộ khác và thông tin gọi hàm. Vì vậy, khi tạo và ghi vào mảng, tổng nhu cầu vượt 640 byte, gây tràn stack. Do đó, em dự đoán hook vApplicationStackOverflowHook() sẽ được gọi khi phát hiện stack bị tràn. Tuy nhiên, bộ nhớ có thể bị hỏng trước khi kernel phát hiện khiến không gọi được hook.
5. Heap trong `.map` và `xPortGetFreeHeapSize()` sẽ lệch nhau. Lệch vì:
- Heap trong .map là toàn bộ vùng nhớ dành cho heap FreeRTOS, còn xPortGetFreeHeapSize() chỉ phản ánh phần còn trống lúc chạy. Vì vậy, dự đoán số từ API sẽ nhỏ hơn số trong .map; phần chênh lệch là stack dùng để cấp phát cho các task qua hàm xTaskCreate().
6. Stack nhỏ nhất an toàn của `dyn_task` dự đoán: 224 words. Dựa vào đâu?
- Mảng HOG_DEPTH_BYTES chiếm 640 bytes = 160 words, em dự trù thêm 32 words = 128 bytes cho ngữ cảnh CPU, các biến cục bộ khác và thông tin gọi hàm, rồi dành 32 words làm biên an toàn. Tổng là 160 + 32 + 32 = 224 words. Trong 128 byte dự trù cho dữ liệu thực thi, khoảng 64 byte dành cho ngữ cảnh thanh ghi cơ bản khi chuyển task trên Cortex‑M3; phần còn lại dự trù cho biến cục bộ, thông tin gọi hàm và căn chỉnh. 