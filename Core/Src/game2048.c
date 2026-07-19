// Logic lõi game 2048 (tách khỏi phần hiển thị)
// Thuật toán phỏng theo demo STM32 (User/2048game/2048game.c)

#include "game2048.h"
#include "stm32f4xx_hal.h"

static uint32_t s_rng_state = 0;

// Điểm kỷ lục — để ngoài struct để giữ qua các ván chơi
static uint16_t s_best = 0;

// Gieo mầm bộ sinh số ngẫu nhiên bằng tick mili-giây
static void prng_seed(void)
{
    s_rng_state ^= (HAL_GetTick() * 2654435761u) + 0x9E3779B9u;
    if (s_rng_state == 0)
    {
        // xorshift bị kẹt nếu state = 0
        s_rng_state = 0xA5A5A5A5u;
    }
}

// Sinh một byte ngẫu nhiên trong khoảng 0..255 (xorshift32)
static uint8_t prng_byte(void)
{
    uint32_t x = s_rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_rng_state = x;
    return (uint8_t)(x & 0xFFu);
}

// Kiểm tra ô (i,j) có ô kề cùng giá trị theo hướng k không
// k: 0 = lên, 1 = xuống, 2 = trái, 3 = phải
static uint8_t test_adjacent(uint16_t g[4][4], uint8_t i, uint8_t j, uint8_t k)
{
    switch (k)
    {
        case 0: // lên
            if (i >= 1 && g[i - 1][j] == g[i][j])
                return 1;
            return 0;
        case 1: // xuống
            if (i <= 2 && g[i + 1][j] == g[i][j])
                return 1;
            return 0;
        case 2: // trái
            if (j >= 1 && g[i][j - 1] == g[i][j])
                return 1;
            return 0;
        case 3: // phải
            if (j <= 2 && g[i][j + 1] == g[i][j])
                return 1;
            return 0;
        default:
            return 0;
    }
}

// Duyệt bàn cờ xem còn cặp ô kề nhau có thể gộp được không
static uint8_t has_duplicate(uint16_t g[4][4])
{
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            for (uint8_t k = 0; k < 4; k++)
            {
                if (test_adjacent(g, i, j, k))
                    return 1;
            }
        }
    }
    return 0;
}

// Trả về 1 nếu thua: bàn đầy và không còn nước gộp nào
static uint8_t is_game_over(uint16_t g[4][4], uint16_t v[4][4])
{
    // Còn ô trống thì chưa thua
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            if (v[i][j] == 0)
                return 0;
        }
    }

    // Bàn đầy nhưng vẫn gộp được thì chưa thua
    if (has_duplicate(g))
        return 0;
    return 1;
}

// Trả về 1 nếu đã xuất hiện ô 2048
static uint8_t has_won(uint16_t g[4][4])
{
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            if (g[i][j] == 2048)
                return 1;
        }
    }
    return 0;
}

// Sinh num ô mới vào các vị trí trống ngẫu nhiên
// Xác suất ~90% ô 2, ~10% ô 4 (ngưỡng 230/255)
static void add_block(Game2048* game, uint8_t num)
{
    uint8_t counter = 0;

    while (counter < num)
    {
        uint8_t x = prng_byte() % 4; // Vị trí x ngẫu nhiên
        uint8_t y = prng_byte() % 4; // Vị trí y

        if (game->v[x][y] == 0)
        {
            uint8_t possibility = prng_byte();
            uint16_t value = (possibility <= 230) ? 2 : 4;

            game->v[x][y] = 1;
            game->g[x][y] = value;


            if (game->maxNum < value)
                game->maxNum = value;
            counter++;
        }
    }
}

// Dồn lên: xử lý theo từng cột, từ trên xuống
// Trả về 1 nếu có ô trượt hoặc gộp
static uint8_t move_up(Game2048* game)
{
    uint16_t (*g)[4] = game->g;
    uint16_t (*v)[4] = game->v;
    uint8_t moved = 0;
    uint8_t merged;

    for (uint8_t j = 0; j < 4; j++)
    {
        merged = 0; // mỗi cột chỉ gộp một lần
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
                            g[k][j] = 0; v[k][j] = 0;
                            update_score_on_merge(game, g[k - 1][j]);
                            moved = 1;
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

    return moved;
}

// Dồn xuống: xử lý theo từng cột, từ dưới lên
static uint8_t move_down(Game2048* game)
{
    uint16_t (*g)[4] = game->g;
    uint16_t (*v)[4] = game->v;
    uint8_t moved = 0;
    uint8_t merged;

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
                            g[k - 1][j] = 0; v[k - 1][j] = 0;
                            update_score_on_merge(game, g[k][j]);
                            moved = 1;
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

    return moved;
}

// Dồn trái: xử lý theo từng hàng, từ trái sang phải
static uint8_t move_left(Game2048* game)
{
    uint16_t (*g)[4] = game->g;
    uint16_t (*v)[4] = game->v;
    uint8_t moved = 0;
    uint8_t merged;

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
                            g[i][k] = 0; v[i][k] = 0;
                            update_score_on_merge(game, g[i][k - 1]);
                            moved = 1;
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

    return moved;
}

// Dồn phải: xử lý theo từng hàng, từ phải sang trái
static uint8_t move_right(Game2048* game)
{
    uint16_t (*g)[4] = game->g;
    uint16_t (*v)[4] = game->v;
    uint8_t moved = 0;
    uint8_t merged;

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
                            g[i][k - 1] = 0; v[i][k - 1] = 0;
                            update_score_on_merge(game, g[i][k]);
                            moved = 1;
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

    return moved;
}

// Sau nước đi: sinh ô mới (nếu có đổi), cập nhật thắng/thua và kỷ lục
static void update_after_move(Game2048* game, uint8_t moved)
{
    if (moved)
        add_block(game, 1);

    // Ưu tiên kiểm tra thắng trước, rồi mới kiểm tra thua
    if (has_won(game->g))
        game->state = GAME_WIN;
    else if (is_game_over(game->g, game->v))
        game->state = GAME_OVER;

    if (game->score > s_best)
        s_best = game->score;
}

// Khởi tạo ván mới: xóa bàn, reset điểm, sinh 2 ô đầu
void game2048_init(Game2048* game)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            game->g[i][j] = 0;
            game->v[i][j] = 0;
        }
    }

    game->maxNum = 0;
    game->score = 0;
    game->state = GAME_PLAYING;

    prng_seed();
    add_block(game, 2);
}

// Xử lý một nước đi theo hướng dir
// Trả về 1 nếu bàn đổi, 0 nếu không đổi (hoặc ván đã kết thúc)
uint8_t game2048_move(Game2048* game, uint8_t dir)
{
    uint8_t moved = 0;

    if (game->state != GAME_PLAYING)
        return 0;

    if (dir == GAME2048_DIR_UP)
        moved = move_up(game);
    else if (dir == GAME2048_DIR_DOWN)
        moved = move_down(game);
    else if (dir == GAME2048_DIR_LEFT)
        moved = move_left(game);
    else if (dir == GAME2048_DIR_RIGHT)
        moved = move_right(game);
    else
        return 0;

    update_after_move(game, moved);
    return moved;
}

// Lấy điểm kỷ lục hiện tại
uint16_t game2048_get_best(void)
{
    return s_best;
}
