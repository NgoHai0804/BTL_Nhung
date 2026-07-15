# Game 2048 — STM32F429I-DISC1

Bản 2048 chạy trên kit **STM32F429I Discovery**, hiển thị bằng **TouchGFX** (màn 320×240), điều khiển bằng 4 nút GPIO.

## Cách chơi

- Vuốt/dồn các ô theo bốn hướng: lên, xuống, trái, phải.
- Hai ô cùng số chạm nhau sẽ gộp thành ô gấp đôi; điểm cộng bằng giá trị ô mới.
- Mỗi nước đi hợp lệ sinh thêm một ô mới (thường là `2`, đôi khi `4`).
- **Thắng** khi xuất hiện ô `2048`.
- **Thua** khi bàn đầy và không còn nước gộp nào.
- Đã thắng/thua: nhấn nút bất kỳ để chơi lại. Điểm kỷ lục (`BEST`) giữ trong phiên cấp nguồn.

## Phần cứng & điều khiển

| Nút   | GPIO   | Mã hướng (`GAME2048_DIR_*`) |
|-------|--------|-----------------------------|
| Lên   | PC13   | `0`                         |
| Xuống | PC11   | `1`                         |
| Trái  | PA5    | `2`                         |
| Phải  | PA7    | `3`                         |

- Nút mức tích cực thấp (kéo lên), phát hiện cạnh nhấn xuống.
- Màn hình: **320×240**, 16 bpp (cấu hình TBS TouchGFX).

## Kiến trúc

```
Nút GPIO  →  FreeRTOS (queue)  →  Screen1View  →  game2048_*  →  Game2048
                                        ↓
                                   Board2048 (vẽ)
```

- **Logic** (`game2048`) độc lập UI: chỉ cập nhật struct `Game2048`.
- **UI** (`Board2048`) chỉ đọc `Game2048` để vẽ bàn cờ, điểm, overlay thắng/thua.
- **Cầu nối** (`Screen1View`): lấy hướng từ hàng đợi, gọi `game2048_move`, `invalidate` khi bàn đổi.

## File chính

| File | Vai trò |
|------|---------|
| `Core/Inc/game2048.h` | API và cấu trúc trạng thái ván |
| `Core/Src/game2048.c` | Luật chơi (di chuyển, gộp, thắng/thua, điểm) |
| `TouchGFX/gui/.../Screen1View.cpp` | Khởi tạo ván, xử lý input mỗi frame |
| `TouchGFX/gui/.../Board2048.cpp` | Widget vẽ bàn 2048 |
| `Core/Src/main.c` | Init MCU, FreeRTOS, task đọc nút |

### API logic

```c
void     game2048_init(Game2048* game);           // ván mới + 2 ô đầu
uint8_t  game2048_move(Game2048* game, uint8_t dir); // 1 = bàn đổi
uint16_t game2048_get_best(void);                 // điểm kỷ lục
```

## Build & chạy

1. Mở project bằng **STM32CubeIDE** (hoặc mở `STM32F429I_DISCO_REV_D01.ioc` bằng CubeMX nếu đổi IDE).
2. Build và nạp lên board STM32F429I-DISC1.
3. Có thể flash từ **TouchGFX Designer** (GCC + STM32CubeProgrammer).

Yêu cầu: [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html).

## Ghi chú kỹ thuật

- Dựa trên TBS TouchGFX cho STM32F429I Discovery (REV D01).
- Logic tham khảo demo STM32 `User/2048game/2048game.c`.
- Pin đo hiệu năng (tùy chọn): `VSYNC_FREQ` PE2, `RENDER_TIME` PE3, `FRAME_RATE` PE4, `MCU_ACTIVE` PE5.
