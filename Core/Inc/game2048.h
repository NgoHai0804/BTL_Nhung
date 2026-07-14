/**
  ******************************************************************************
  * @file    game2048.h
  * @brief   Logic lõi của game 2048 (tách rời khỏi phần hiển thị).
  *          Phỏng theo demo STM32 (User/2048game/2048game.c).
  ******************************************************************************
  */

#ifndef GAME2048_H
#define GAME2048_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Kích thước bàn cờ (4x4) */
#define GAME2048_SIZE   4

/* Mã hướng di chuyển (giữ đúng quy ước của bản gốc) */
#define GAME2048_DIR_UP     0
#define GAME2048_DIR_DOWN   1
#define GAME2048_DIR_LEFT   2
#define GAME2048_DIR_RIGHT  3

/* Trạng thái ván chơi */
typedef enum
{
    GAME_PLAYING = 0,   /* đang chơi          */
    GAME_WIN,           /* đã đạt ô 2048      */
    GAME_OVER           /* hết nước đi (thua) */
} GameState;

/* Toàn bộ trạng thái của một ván 2048 */
typedef struct
{
    uint16_t  g[GAME2048_SIZE][GAME2048_SIZE]; /* giá trị ô (0 = trống)       */
    uint16_t  v[GAME2048_SIZE][GAME2048_SIZE]; /* cờ "đã gộp" trong 1 nước đi */
    uint16_t  maxNum;                          /* ô lớn nhất hiện tại         */
    uint16_t  score;                           /* điểm hiện tại               */
    GameState state;                           /* PLAYING / WIN / OVER        */
} Game2048;

/**
  * @brief  Khởi tạo ván mới: xóa bàn cờ và sinh 2 ô ban đầu.
  */
void game2048_init(Game2048* gm);

/**
  * @brief  Thực hiện một nước đi theo hướng cho trước.
  * @param  dir  một trong các GAME2048_DIR_*
  * @retval 1 nếu bàn cờ thay đổi (có sinh ô mới), 0 nếu không.
  */
uint8_t game2048_move(Game2048* gm, uint8_t dir);

/**
  * @brief  Lấy điểm kỷ lục cao nhất từ đầu (giữ qua các lần chơi lại
  *         trong cùng một phiên cấp nguồn).
  */
uint16_t game2048_get_best(void);

#ifdef __cplusplus
}
#endif

#endif /* GAME2048_H */
