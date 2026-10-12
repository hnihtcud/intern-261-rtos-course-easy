# Report — Week 1 / Ex1

`student_id`: 2510379    Commit của PREDICTION.md: ______

## 1. Số đo
| Đại lượng | Giá trị |
|---|---|
| `BLINK_PERIOD_MS` (từ params.h) | 149 ms|
| Chu kỳ đo được — min | 147.45 ms|
| Chu kỳ đo được — max | 147.75 ms|
| Chu kỳ đo được — trung bình | 147.53 ms|
| Sai số trung bình vs `BLINK_PERIOD_MS` | -1.47 ms|
| `uxTaskGetStackHighWaterMark()` của bringup_task | 68 words |

Ảnh PulseView (>= 10 chu kỳ, đọc được giá trị): `![](do_ex1.png)`

## 2. Ba handler của kernel
| Tên trong port ARM_CM3 | Tên trong vector table | Nhiệm vụ |
|---|---|---|
| `vPortSVCHandler` | SVC_Handler | Khôi phục ngữ cảnh để bắt đầu task đầu tiên |
| `xPortPendSVHandler` | PendSV_Handler | Lưu/khôi phục ngữ cảnh khi chuyển task |
| `xPortSysTickHandler` | SysTick_Handler | Cập nhật tick kernel, xử lý task hết thời gian chờ và yêu cầu chuyển task khi cần |

Log lỗi link trước khi sửa (dán nguyên văn):

```text
C:/PROGRA~2/ARMGNU~1/14EFD8~1.2RE/bin/../lib/gcc/arm-none-eabi/14.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1_ex1.dir/FreeRTOS/portable/GCC/ARM_CM3/port.c.obj: in function `SVC_Handler':
D:/RTOS/Week1/Week1_ex1/FreeRTOS/portable/GCC/ARM_CM3/port.c:230: multiple definition of `SVC_Handler'; CMakeFiles/Week1_ex1.dir/Core/Src/stm32f1xx_it.c.obj:D:/RTOS/Week1/Week1_ex1/Core/Src/stm32f1xx_it.c:146: first defined here
C:/PROGRA~2/ARMGNU~1/14EFD8~1.2RE/bin/../lib/gcc/arm-none-eabi/14.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1_ex1.dir/FreeRTOS/portable/GCC/ARM_CM3/port.c.obj: in function `PendSV_Handler':
D:/RTOS/Week1/Week1_ex1/FreeRTOS/portable/GCC/ARM_CM3/port.c:459: multiple definition of `PendSV_Handler'; CMakeFiles/Week1_ex1.dir/Core/Src/stm32f1xx_it.c.obj:D:/RTOS/Week1/Week1_ex1/Core/Src/stm32f1xx_it.c:172: first defined here
C:/PROGRA~2/ARMGNU~1/14EFD8~1.2RE/bin/../lib/gcc/arm-none-eabi/14.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1_ex1.dir/FreeRTOS/portable/GCC/ARM_CM3/port.c.obj: in function `SysTick_Handler':
D:/RTOS/Week1/Week1_ex1/FreeRTOS/portable/GCC/ARM_CM3/port.c:492: multiple definition of `SysTick_Handler'; CMakeFiles/Week1_ex1.dir/Core/Src/stm32f1xx_it.c.obj:D:/RTOS/Week1/Week1_ex1/Core/Src/stm32f1xx_it.c:185: first defined here
```
Lỗi xuất hiện sau khi map tên handler FreeRTOS sang tên trong vector table, nhưng chưa xử lý các định nghĩa cùng tên do CubeMX tạo.

Cách sửa:
- Lỗi xảy ra vì port.c và stm32f1xx_it.c cùng định nghĩa SVC_Handler, PendSV_Handler, SysTick_Handler. Trong CMake, áp dụng macro đổi tên riêng khi biên dịch stm32f1xx_it.c, chuyển ba handler của CubeMX sang tên khác(CubeMX_SVC_Handler, CubeMX_PendSV_Handler, CubeMX_SysTick_Handler). Giữ mapping trong FreeRTOSConfig.h để các hàm của FreeRTOS mang đúng tên mà bảng vector tham chiếu:
```
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
```

## 3. Tại sao HAL timebase phải sang TIM4
Phải trả lời được: `HAL_IncTick()` chạy ở đâu, `configKERNEL_INTERRUPT_PRIORITY`
ảnh hưởng gì đến nó, và `HAL_Delay()` / `vTaskDelay()` khác nhau chỗ nào.
**Trả lời:**
- Trước tiên, HAL có biến đếm thời gian là uwTick, khi HAL vẫn dùng SysTick làm Timebase, `HAL_IncTick()` được gọi trong hàm xử lý ngắt `SysTick_Handler()`. Với cấu hình timebase HAL 1 ms, mỗi lần gọi `HAL_IncTick()` sẽ tăng uwTick thêm 1. Nó có tác dụng theo dõi Delay và Timeout HAL. 
- RTOS dùng SysTick làm xung nhịp thời gian cho Kernel. Mỗi lần xử lý ngắt SysTick, kernel sẽ tăng bộ đếm tick của RTOS. Nếu HAL và RTOS cùng dùng SysTick có thể xảy ra các tình huống:
    + Handler chỉ gọi `HAL_IncTick()`: Task sẽ bị treo vì khi gọi vTaskDelay() với thời gian chờ khác 0 thì task sẽ ở trạng thái Blocked và không được đánh thức theo thời gian, vì tick kernel không tăng.
    + Handler chỉ xử lý tick của RTOS: `HAL_Delay()` có thể bị treo mãi, timeout HAL sẽ hoạt động sai.
    + Handler xử lý cả 2: bộ đếm của cả 2 vẫn tăng nhưng có thể có vấn đề về độ ưu tiên
- Nói về vấn đề độ ưu tiên, `configKERNEL_INTERRUPT_PRIORITY` ảnh hưởng gián tiếp đến `HAL_IncTick()` do ưu tiên của ngắt SysTick. Trong Port dùng cho STM32F103, RTOS dùng thông số này đặt SysTick ở mức ưu tiên thấp nhất. Khi một ISR ưu tiên cao hơn đang chạy, SysTick phải chờ và `HAL_IncTick()` chưa được thực thi, khiến uwTick chưa tăng. Nếu ISR đó gọi `HAL_Delay()`, nó sẽ chờ mãi vì SysTick không thể chen vào để cập nhật thời gian HAL.
- Do đó, việc chuyển HAL timebase sang TIM4 cho phép `HAL_IncTick()` chạy theo ưu tiên riêng của ngắt TIM4, không còn phụ thuộc vào ưu tiên SysTick do `configKERNEL_INTERRUPT_PRIORITY` thiết lập. Nhờ vậy, HAL và RTOS có Timebase riêng sẽ đảm bảo hoạt động bình thường.
- So sánh `HAL_Delay()` / `vTaskDelay()`:
     + `HAL_Delay()` chờ bằng cách liên tục kiểm tra biến đếm thời gian uwTick của HAL. Trong lúc chờ, task dùng CPU để kiểm tra thời gian mỗi khi được chạy. Task đó vẫn có thể bị ngắt hoặc bị task ưu tiên cao hơn chiếm CPU. Khi đủ thời gian, hàm kết thúc và chương trình chạy tiếp.
     + `vTaskDelay()` nhờ kernel FreeRTOS đưa task đang gọi vào trạng thái Blocked trong số tick yêu cầu. Task không chạy trong thời gian này, nên CPU có thể thực hiện task khác hoặc Idle. Khi hết thời gian chờ theo tick FreeRTOS, kernel đưa task về Ready; task chỉ tiếp tục khi được scheduler chọn, không nhất thiết ngay lập tức.
## 4. Lệch so với dự đoán
Với mỗi mục trong `PREDICTION.md` lệch so đo: lệch bao nhiêu, và **nguyên nhân vật lý**
(không được trả lời "do sai số").
**Trả lời:**
- `BLINK_PERIOD_MS` (từ params.h): 149 ms
- Chu kỳ dự đoán: 148 ms
- Chu kỳ đo được trung bình: 147.53 ms
- Số đo lệch với dự đoán: -0.47 ms
- Nguyên nhân vật lý:
    + Clock HSI thực tế: nếu tần số dao động cao hơn giá trị danh định, mỗi tick sẽ ngắn hơn, làm chu kỳ LED ngắn hơn.
    + Sai số của logic Analyzer: thiết bị cũng dùng một bộ dao động để xác định thời gian. Nếu clock thực tế khác giá trị danh định, thang thời gian đo sẽ lệch theo. Bên cạnh đó, độ delay trên dây của logic analyzer cũng sẽ có ảnh hưởng.
- Ngoài ra, có một nguyên nhân khác là thời điểm thực thi task so với tick: Nếu thời điểm gọi vTaskDelay() nằm gần lúc chuẩn bị sang 1 tick khác, sai lệch có thể xảy ra và nằm ở khoảng -1 ms đến 0 ms.
## 5. Câu hỏi đánh đổi
`configTICK_RATE_HZ` = 1000 trong bài này. Dựa vào số đo của **bạn**, nếu hạ xuống
100 Hz thì chu kỳ LED và sai số thay đổi thế nào? Trả lời bằng con số, không bằng lý thuyết.
**Trả lời:**
- `BLINK_PERIOD_MS` (từ params.h): 149 ms
- Thời gian delay: 149 / 2 = 74 ms
- Chu kỳ dự đoán: 148 ms
- 1000 Hz thì chu kỳ sẽ là 148 ms tương đương 148 tick (đo được: 147.53 ms, sai số: -1.47ms)
- 100 Hz:
    + Mỗi lần delay: floor(74 × 100 / 1000) = 7 tick
    + Chu kỳ đầy đủ: 2 × 7 × 10 ms = 140 ms
- Sai lệch so với mong muốn (149 ms): 140 - 149 = -9 ms
- Đánh đổi là: số ngắt tick giảm từ 1000 xuống 100 lần/giây nên độ phân giải thời gian từ 1 lên 10 ms sẽ gây cảm giác bị thô hơn nên phép đổi thời gian sang tick làm chu kỳ lệch nhiều hơn.

