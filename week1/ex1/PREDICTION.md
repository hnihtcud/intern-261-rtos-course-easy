# Dự đoán — Week 1 / Ex1

> **Commit file này TRƯỚC khi nạp chương trình lần đầu.** Report sẽ phải giải thích
> mọi chỗ dự đoán lệch so đo. Dự đoán sai không bị trừ điểm; dự đoán sau khi đo thì bị.

`student_id`: 2510379    Ngày/giờ commit: 23h12p ngày 11/10/2026

1. Chu kỳ LED tôi sẽ đo được: 148 ms (`BLINK_PERIOD_MS` = 149)
2. Sai số giữa chu kỳ đo được và `BLINK_PERIOD_MS`: 1000 us. Vì Chu kỳ LED là số lẻ, nên thời gian giữa 2 lần toggle
   sẽ là 149/2 = 74.5 nên sẽ được làm tròn xuống thành 74 vì mỗi tick là 1ms. Do đó, chu kỳ LED đo được sẽ khoảng 148.
   Bên cạnh đó, em nghĩ thời điểm gọi delay cũng sẽ ảnh hưởng đến chu kỳ đo được nhưng không đáng kể.
3. Nếu để HAL timebase ở lại SysTick, điều gì sẽ xảy ra khi `vTaskDelay()` được gọi?
   (chọn và giải thích: chạy bình thường / treo / tick sai nhịp / hard fault)
   Task bị treo vì khi gọi `vTaskDelay()`, task vào trạng thái Blocked, bộ đếm HAL tăng 
   nhưng tick RTOS không tăng do chưa nối SysTick với handler FreeRTOS (chỉ gọi HAL_IncTick()). Do đó, kernel 
   không đưa task trở lại Ready nên task sẽ bị treo ở đó mãi.
4. `BRINGUP_STACK_WORDS` = 96 words = 384 bytes. Task này có đủ không? Vì sao?
   Task này em dự đoán là đủ. Stack dùng để lưu biến cục bộ, dữ liệu việc gọi hàm và quay về cũng như ngữ cảnh thanh ghi khi bị tạm dùng để chuyển task khác. Vì task chỉ toggle GPIO và gọi vTaskDelay(), không dùng biến cục bộ lớn hoặc đệ quy nên em dự đoán 384 bytes là đủ.
5. Trước khi map handler, linker sẽ báo lỗi gì (đoạn nguyên văn thông báo):
C:/PROGRA~2/ARMGNU~1/14EFD8~1.2RE/bin/../lib/gcc/arm-none-eabi/14.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1_ex1.dir/FreeRTOS/portable/GCC/ARM_CM3/port.c.obj: in function `SVC_Handler':
D:/RTOS/Week1/Week1_ex1/FreeRTOS/portable/GCC/ARM_CM3/port.c:230: multiple definition of `SVC_Handler'; CMakeFiles/Week1_ex1.dir/Core/Src/stm32f1xx_it.c.obj:D:/RTOS/Week1/Week1_ex1/Core/Src/stm32f1xx_it.c:146: first defined here
C:/PROGRA~2/ARMGNU~1/14EFD8~1.2RE/bin/../lib/gcc/arm-none-eabi/14.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1_ex1.dir/FreeRTOS/portable/GCC/ARM_CM3/port.c.obj: in function `PendSV_Handler':
D:/RTOS/Week1/Week1_ex1/FreeRTOS/portable/GCC/ARM_CM3/port.c:459: multiple definition of `PendSV_Handler'; CMakeFiles/Week1_ex1.dir/Core/Src/stm32f1xx_it.c.obj:D:/RTOS/Week1/Week1_ex1/Core/Src/stm32f1xx_it.c:172: first defined here
C:/PROGRA~2/ARMGNU~1/14EFD8~1.2RE/bin/../lib/gcc/arm-none-eabi/14.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1_ex1.dir/FreeRTOS/portable/GCC/ARM_CM3/port.c.obj: in function `SysTick_Handler':
D:/RTOS/Week1/Week1_ex1/FreeRTOS/portable/GCC/ARM_CM3/port.c:492: multiple definition of `SysTick_Handler'; CMakeFiles/Week1_ex1.dir/Core/Src/stm32f1xx_it.c.obj:D:/RTOS/Week1/Week1_ex1/Core/Src/stm32f1xx_it.c:185: first defined here