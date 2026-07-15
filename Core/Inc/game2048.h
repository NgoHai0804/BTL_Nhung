// Logic lõi game 2048 (tách khỏi phần hiển thị)
// Phỏng theo demo STM32 (User/2048game/2048game.c)

#ifndef GAME2048_H
#define GAME2048_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GAME2048_SIZE   4

// Hướng di chuyển (giữ đúng quy ước bản gốc)
#define GAME2048_DIR_UP     0
#define GAME2048_DIR_DOWN   1
#define GAME2048_DIR_LEFT   2
#define GAME2048_DIR_RIGHT  3

typedef enum
{
    GAME_PLAYING = 0,
    GAME_WIN,    // đạt ô 2048
    GAME_OVER    // hết nước đi
} GameState;

typedef struct
{
    uint16_t  g[GAME2048_SIZE][GAME2048_SIZE]; // giá trị ô
    uint16_t  v[GAME2048_SIZE][GAME2048_SIZE]; // 0 = trống, 1 = có ô
    uint16_t  maxNum;
    uint16_t  score;
    GameState state;
} Game2048;

// Khởi tạo ván mới: xóa bàn, reset điểm, sinh 2 ô đầu
void game2048_init(Game2048* game);

// Thực hiện một nước đi (dir = GAME2048_DIR_*).
// Trả về 1 nếu bàn thay đổi, 0 nếu không.
uint8_t game2048_move(Game2048* game, uint8_t dir);

// Lấy điểm kỷ lục (giữ qua các lần chơi lại trong cùng lần cấp nguồn)
uint16_t game2048_get_best(void);

#ifdef __cplusplus
}
#endif

#endif // GAME2048_H
