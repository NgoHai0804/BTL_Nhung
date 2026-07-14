#ifndef BOARD2048_HPP
#define BOARD2048_HPP

#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/hal/Types.hpp>
#include "game2048.h"

/**
 * Widget TouchGFX tự vẽ toàn bộ bàn cờ 2048 (các ô, số, điểm và thông báo)
 * trực tiếp lên framebuffer bằng HAL::lcd().fillRect.
 *
 * Số và chữ được vẽ bằng font bitmap 5x7 nhúng sẵn, nên KHÔNG cần sinh
 * font/ảnh qua TouchGFX Designer.
 */
class Board2048 : public touchgfx::Widget
{
public:
    Board2048();
    virtual ~Board2048() {}

    /* Gắn con trỏ tới trạng thái game để widget đọc và vẽ. */
    void setGame(const Game2048* gamePtr)
    {
        game = gamePtr;
    }

    virtual void draw(const touchgfx::Rect& invalidatedArea) const;
    virtual touchgfx::Rect getSolidRect() const;

private:
    const Game2048* game;   /* trạng thái game (chỉ đọc) */

    /* Tô hình chữ nhật theo tọa độ tương đối, đã cắt theo vùng vẽ lại. */
    void fillRel(const touchgfx::Rect& area, int16_t x, int16_t y,
                 int16_t w, int16_t h, touchgfx::colortype color) const;

    /* Vẽ một ký tự bằng font 5x7 với hệ số phóng to nguyên 's'. */
    void drawChar(const touchgfx::Rect& area, int16_t x, int16_t y,
                  char c, int16_t s, touchgfx::colortype color) const;

    /* Vẽ chuỗi; trả về tổng bề rộng (px) của chuỗi đã vẽ. */
    int16_t drawText(const touchgfx::Rect& area, int16_t x, int16_t y,
                     const char* str, int16_t s, touchgfx::colortype color) const;

    static int16_t textWidth(const char* str, int16_t s);

    /* Biến thể phóng to theo phân số (scale = num/den), vd 3/2 = 1.5x. */
    void drawCharFrac(const touchgfx::Rect& area, int16_t x, int16_t y,
                      char c, int16_t num, int16_t den, touchgfx::colortype color) const;
    int16_t drawTextFrac(const touchgfx::Rect& area, int16_t x, int16_t y,
                         const char* str, int16_t num, int16_t den, touchgfx::colortype color) const;
    static int16_t textWidthFrac(const char* str, int16_t num, int16_t den);

    /* Màu nền và màu chữ của ô theo giá trị. */
    static touchgfx::colortype tileColor(uint16_t value);
    static touchgfx::colortype tileTextColor(uint16_t value);
};

#endif // BOARD2048_HPP
