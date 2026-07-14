/**
  ******************************************************************************
  * @file    game2048.c
  * @brief   Logic lõi của game 2048 (tách rời khỏi phần hiển thị).
  *          Thuật toán phỏng theo demo STM32 (User/2048game/2048game.c),
  *          đã bỏ toàn bộ code LCD/SPI/cảm biến/printf và thay nguồn ngẫu
  *          nhiên từ ADC bằng PRNG nhẹ, gieo mầm bằng HAL_GetTick().
  ******************************************************************************
  */

#include "game2048.h"
#include "stm32f4xx_hal.h"

/* -------------------------------------------------------------------------- */
/* Bộ sinh số giả ngẫu nhiên (PRNG)                                           */
/* -------------------------------------------------------------------------- */

static uint32_t s_rng_state = 0;

/* Điểm kỷ lục: đặt ngoài Game2048 để giữ lại qua các lần chơi lại. */
static uint16_t s_best = 0;

/* Gieo mầm PRNG dựa trên tick mili-giây để mỗi ván sinh ô khác nhau. */
static void prng_seed(void)
{
    s_rng_state ^= (HAL_GetTick() * 2654435761u) + 0x9E3779B9u;
    if (s_rng_state == 0)
    {
        s_rng_state = 0xA5A5A5A5u;   /* tránh trạng thái 0 (xorshift sẽ kẹt) */
    }
}

/* Trả về một byte trong [0, 255] bằng thuật toán xorshift32. */
static uint8_t prng_byte(void)
{
    uint32_t x = s_rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_rng_state = x;
    return (uint8_t)(x & 0xFFu);
}

/* -------------------------------------------------------------------------- */
/* Các hàm phụ trợ                                                            */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Kiểm tra ô (i,j) có ô kề bằng giá trị theo hướng k hay không
  *         (0:lên, 1:xuống, 2:trái, 3:phải).
  */
static uint8_t test_adjacent(uint16_t g[4][4], uint8_t i, uint8_t j, uint8_t k)
{
    switch (k)
    {
        case 0: /* lên   */
            return (i >= 1 && g[i - 1][j] == g[i][j]) ? 1 : 0;
        case 1: /* xuống */
            return (i <= 2 && g[i + 1][j] == g[i][j]) ? 1 : 0;
        case 2: /* trái  */
            return (j >= 1 && g[i][j - 1] == g[i][j]) ? 1 : 0;
        case 3: /* phải  */
            return (j <= 2 && g[i][j + 1] == g[i][j]) ? 1 : 0;
        default:
            return 0;
    }
}

/* Trả về 1 nếu vẫn còn cặp ô kề nhau có thể gộp được. */
static uint8_t has_duplicate(uint16_t g[4][4])
{
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            for (uint8_t k = 0; k < 4; k++)
            {
                if (test_adjacent(g, i, j, k))
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Trả về 1 nếu bàn cờ đã đầy và không còn nước gộp nào (thua). */
static uint8_t is_game_over(uint16_t g[4][4], uint16_t v[4][4])
{
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            if (v[i][j] == 0)
            {
                return 0;
            }
        }
    }
    return has_duplicate(g) ? 0 : 1;
}

/* Trả về 1 nếu đã xuất hiện ô 2048 (thắng). */
static uint8_t has_won(uint16_t g[4][4])
{
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            if (g[i][j] == 2048)
            {
                return 1;
            }
        }
    }
    return 0;
}

/**
  * @brief  Sinh 'num' ô mới vào các vị trí trống ngẫu nhiên.
  *         Xác suất ~90% là ô 2, ~10% là ô 4 (ngưỡng 230/255).
  */
static void add_block(Game2048* gm, uint8_t num)
{
    uint8_t counter = 0;

    do
    {
        uint8_t x = prng_byte() % 4;
        uint8_t y = prng_byte() % 4;

        if (gm->v[x][y] == 0)   /* chỉ đặt vào ô đang trống */
        {
            uint8_t possibility = prng_byte();
            uint16_t value = (possibility <= 230) ? 2 : 4;

            gm->v[x][y] = 1;
            gm->g[x][y] = value;
            gm->maxNum  = (gm->maxNum >= value) ? gm->maxNum : value;
            counter++;
        }
    } while (counter < num);
}

/* -------------------------------------------------------------------------- */
/* API công khai                                                              */
/* -------------------------------------------------------------------------- */

/* Khởi tạo ván mới: xóa sạch bàn cờ, đặt lại điểm/trạng thái, sinh 2 ô đầu. */
void game2048_init(Game2048* gm)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            gm->g[i][j] = 0;
            gm->v[i][j] = 0;
        }
    }

    gm->maxNum = 0;
    gm->score  = 0;
    gm->state  = GAME_PLAYING;

    prng_seed();
    add_block(gm, 2);
}

/**
  * @brief  Xử lý một nước đi.
  *
  * Với mỗi hướng, ta duyệt các ô theo chiều dồn rồi:
  *   - nếu ô kề trống thì trượt ô hiện tại qua (đổi chỗ),
  *   - nếu ô kề cùng giá trị và chưa gộp trong lượt này thì gộp (cộng dồn,
  *     cộng điểm), cờ 'merged' đảm bảo mỗi ô chỉ gộp một lần mỗi nước đi.
  * Sau khi di chuyển, sinh thêm 1 ô mới rồi cập nhật trạng thái thắng/thua
  * và điểm kỷ lục.
  *
  * @retval 1 nếu bàn cờ có thay đổi, 0 nếu không.
  */
uint8_t game2048_move(Game2048* gm, uint8_t dir)
{
    uint16_t (*g)[4] = gm->g;   /* giá trị các ô */
    uint16_t (*v)[4] = gm->v;   /* cờ ô có giá trị / đã gộp */
    uint8_t  moved   = 0;       /* có ô nào di chuyển/gộp không */
    uint8_t  merged  = 0;       /* đã gộp trong cột/hàng hiện tại chưa */

    /* Đã kết thúc ván thì không nhận nước đi nữa. */
    if (gm->state != GAME_PLAYING)
    {
        return 0;
    }

    if (dir == GAME2048_DIR_UP)         /* dồn lên trên, xử lý theo từng cột */
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            merged = 0;
            for (uint8_t i = 0; i < 4; i++)
            {
                if (v[i][j] == 1)
                {
                    for (uint8_t k = i; k >= 1; k--)
                    {
                        if (v[k - 1][j] == 1)
                        {
                            if (g[k - 1][j] == g[k][j] && !merged)
                            {
                                g[k - 1][j] += g[k - 1][j];
                                g[k][j] = 0;
                                v[k][j] = 0;
                                gm->maxNum = (gm->maxNum >= g[k - 1][j]) ? gm->maxNum : g[k - 1][j];
                                gm->score += g[k - 1][j];
                                moved  = 1;
                                merged = 1;
                            }
                            break;
                        }
                        else
                        {
                            uint16_t temp = g[k - 1][j];
                            g[k - 1][j] = g[k][j];
                            g[k][j] = temp;
                            v[k][j] = 0;
                            v[k - 1][j] = 1;
                            moved = 1;
                        }
                    }
                }
            }
        }
    }
    else if (dir == GAME2048_DIR_DOWN) /* dồn xuống dưới, xử lý theo từng cột */
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            merged = 0;
            for (uint8_t i = 4; i >= 1; i--)
            {
                if (v[i - 1][j] == 1)
                {
                    for (uint8_t k = i; k < 4; k++)
                    {
                        if (v[k][j] == 1)
                        {
                            if (g[k][j] == g[k - 1][j] && !merged)
                            {
                                g[k][j] += g[k - 1][j];
                                g[k - 1][j] = 0;
                                v[k - 1][j] = 0;
                                gm->maxNum = (gm->maxNum >= g[k][j]) ? gm->maxNum : g[k][j];
                                gm->score += g[k][j];
                                moved  = 1;
                                merged = 1;
                            }
                            break;
                        }
                        else
                        {
                            uint16_t temp = g[k][j];
                            g[k][j] = g[k - 1][j];
                            g[k - 1][j] = temp;
                            v[k - 1][j] = 0;
                            v[k][j] = 1;
                            moved = 1;
                        }
                    }
                }
            }
        }
    }
    else if (dir == GAME2048_DIR_LEFT) /* dồn sang trái, xử lý theo từng hàng */
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            merged = 0;
            for (uint8_t j = 0; j < 4; j++)
            {
                if (v[i][j] == 1)
                {
                    for (uint8_t k = j; k >= 1; k--)
                    {
                        if (v[i][k - 1] == 1)
                        {
                            if (g[i][k - 1] == g[i][k] && !merged)
                            {
                                g[i][k - 1] += g[i][k];
                                g[i][k] = 0;
                                v[i][k] = 0;
                                gm->maxNum = (gm->maxNum >= g[i][k - 1]) ? gm->maxNum : g[i][k - 1];
                                gm->score += g[i][k - 1];
                                moved  = 1;
                                merged = 1;
                            }
                            break;
                        }
                        else
                        {
                            uint16_t temp = g[i][k - 1];
                            g[i][k - 1] = g[i][k];
                            g[i][k] = temp;
                            v[i][k] = 0;
                            v[i][k - 1] = 1;
                            moved = 1;
                        }
                    }
                }
            }
        }
    }
    else if (dir == GAME2048_DIR_RIGHT) /* dồn sang phải, xử lý theo từng hàng */
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            merged = 0;
            for (uint8_t j = 4; j >= 1; j--)
            {
                if (v[i][j - 1] == 1)
                {
                    for (uint8_t k = j; k < 4; k++)
                    {
                        if (v[i][k] == 1)
                        {
                            if (g[i][k] == g[i][k - 1] && !merged)
                            {
                                g[i][k] += g[i][k - 1];
                                g[i][k - 1] = 0;
                                v[i][k - 1] = 0;
                                gm->maxNum = (gm->maxNum >= g[i][k]) ? gm->maxNum : g[i][k];
                                gm->score += g[i][k];
                                moved  = 1;
                                merged = 1;
                            }
                            break;
                        }
                        else
                        {
                            uint16_t temp = g[i][k];
                            g[i][k] = g[i][k - 1];
                            g[i][k - 1] = temp;
                            v[i][k - 1] = 0;
                            v[i][k] = 1;
                            moved = 1;
                        }
                    }
                }
            }
        }
    }
    else
    {
        return 0;
    }

    /* Có thay đổi thì sinh thêm một ô mới. */
    if (moved)
    {
        add_block(gm, 1);
    }

    /* Cập nhật trạng thái: ưu tiên kiểm tra thắng trước, rồi mới thua. */
    if (has_won(g))
    {
        gm->state = GAME_WIN;
    }
    else if (is_game_over(g, v))
    {
        gm->state = GAME_OVER;
    }

    /* Cập nhật kỷ lục nếu vượt qua. */
    if (gm->score > s_best)
    {
        s_best = gm->score;
    }

    return moved;
}

/* Trả về điểm kỷ lục hiện tại. */
uint16_t game2048_get_best(void)
{
    return s_best;
}
