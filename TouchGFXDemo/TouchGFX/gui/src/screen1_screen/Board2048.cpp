#include <gui/screen1_screen/Board2048.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/lcd/LCD.hpp>

using namespace touchgfx;

/* -------------------------------------------------------------------------- */
/* Hằng số bố cục (màn hình 240 x 320, hướng dọc)                              */
/* -------------------------------------------------------------------------- */
#define SCREEN_W        240
#define SCREEN_H        320
#define TILE            50                     /* cạnh mỗi ô (px)              */
#define GAP             6                      /* khe hở giữa các ô (px)       */
#define BOARD_X0        5                      /* lề trái lưới = (240 - 4*TILE - 5*GAP)/2 */
#define BOARD_Y0        76                     /* đẩy lưới xuống để không đè title */
#define BOARD_PX        (4 * TILE + 5 * GAP)    /* cạnh khối lưới = 230         */
#define TITLE_Y         10                     /* tọa độ y của chữ "2048"      */
#define SCORE_Y         58                     /* tọa độ y của hàng SCORE/BEST */
#define TILE_X(col)     (BOARD_X0 + GAP + (col) * (TILE + GAP))  /* x của ô theo cột */
#define TILE_Y(row)     (BOARD_Y0 + GAP + (row) * (TILE + GAP))  /* y của ô theo hàng */
#define CONTENT_BUDGET  (TILE - 4)             /* bề rộng/cao tối đa chữ trong ô */

/* -------------------------------------------------------------------------- */
/* Font bitmap 5x7 (lưu theo cột, bit0 = hàng trên cùng)                      */
/* -------------------------------------------------------------------------- */
typedef struct
{
    char    c;
    uint8_t col[5];
} Glyph;

static const Glyph FONT[] = {
    { ' ', { 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { '!', { 0x00, 0x00, 0x5F, 0x00, 0x00 } },
    { ':', { 0x00, 0x36, 0x36, 0x00, 0x00 } },
    { '0', { 0x3E, 0x51, 0x49, 0x45, 0x3E } },
    { '1', { 0x00, 0x42, 0x7F, 0x40, 0x00 } },
    { '2', { 0x42, 0x61, 0x51, 0x49, 0x46 } },
    { '3', { 0x21, 0x41, 0x45, 0x4B, 0x31 } },
    { '4', { 0x18, 0x14, 0x12, 0x7F, 0x10 } },
    { '5', { 0x27, 0x45, 0x45, 0x45, 0x39 } },
    { '6', { 0x3C, 0x4A, 0x49, 0x49, 0x30 } },
    { '7', { 0x01, 0x71, 0x09, 0x05, 0x03 } },
    { '8', { 0x36, 0x49, 0x49, 0x49, 0x36 } },
    { '9', { 0x06, 0x49, 0x49, 0x29, 0x1E } },
    { 'A', { 0x7E, 0x11, 0x11, 0x11, 0x7E } },
    { 'B', { 0x7F, 0x49, 0x49, 0x49, 0x36 } },
    { 'C', { 0x3E, 0x41, 0x41, 0x41, 0x22 } },
    { 'E', { 0x7F, 0x49, 0x49, 0x49, 0x41 } },
    { 'G', { 0x3E, 0x41, 0x49, 0x49, 0x7A } },
    { 'I', { 0x00, 0x41, 0x7F, 0x41, 0x00 } },
    { 'K', { 0x7F, 0x08, 0x14, 0x22, 0x41 } },
    { 'M', { 0x7F, 0x02, 0x0C, 0x02, 0x7F } },
    { 'N', { 0x7F, 0x04, 0x08, 0x10, 0x7F } },
    { 'O', { 0x3E, 0x41, 0x41, 0x41, 0x3E } },
    { 'P', { 0x7F, 0x09, 0x09, 0x09, 0x06 } },
    { 'R', { 0x7F, 0x09, 0x19, 0x29, 0x46 } },
    { 'S', { 0x46, 0x49, 0x49, 0x49, 0x31 } },
    { 'T', { 0x01, 0x01, 0x7F, 0x01, 0x01 } },
    { 'V', { 0x1F, 0x20, 0x40, 0x20, 0x1F } },
    { 'W', { 0x3F, 0x40, 0x38, 0x40, 0x3F } },
    { 'Y', { 0x07, 0x08, 0x70, 0x08, 0x07 } },
};

/* Tra cứu dữ liệu glyph của một ký tự; không tìm thấy thì trả về ô trống. */
static const uint8_t* getGlyph(char c)
{
    for (unsigned i = 0; i < sizeof(FONT) / sizeof(FONT[0]); i++)
    {
        if (FONT[i].c == c)
        {
            return FONT[i].col;
        }
    }
    return FONT[0].col; /* ký tự trắng */
}

/* Đổi số nguyên không dấu sang chuỗi (không có số 0 ở đầu). */
static void uintToStr(uint16_t value, char* out)
{
    char tmp[6];
    int  n = 0;

    if (value == 0)
    {
        out[0] = '0';
        out[1] = '\0';
        return;
    }
    while (value > 0 && n < 5)
    {
        tmp[n++] = (char)('0' + (value % 10));
        value /= 10;
    }
    int k = 0;
    while (n > 0)
    {
        out[k++] = tmp[--n];
    }
    out[k] = '\0';
}

/* -------------------------------------------------------------------------- */
/* Bảng màu các ô (theo phong cách 2048 cổ điển)                              */
/* -------------------------------------------------------------------------- */
/* Màu nền của ô tương ứng với giá trị. */
colortype Board2048::tileColor(uint16_t value)
{
    switch (value)
    {
        case 0:    return Color::getColorFromRGB(205, 193, 180);
        case 2:    return Color::getColorFromRGB(238, 228, 218);
        case 4:    return Color::getColorFromRGB(237, 224, 200);
        case 8:    return Color::getColorFromRGB(242, 177, 121);
        case 16:   return Color::getColorFromRGB(245, 149,  99);
        case 32:   return Color::getColorFromRGB(246, 124,  95);
        case 64:   return Color::getColorFromRGB(246,  94,  59);
        case 128:  return Color::getColorFromRGB(237, 207, 114);
        case 256:  return Color::getColorFromRGB(237, 204,  97);
        case 512:  return Color::getColorFromRGB(237, 200,  80);
        case 1024: return Color::getColorFromRGB(237, 197,  63);
        case 2048: return Color::getColorFromRGB(237, 194,  46);
        default:   return Color::getColorFromRGB(60,  58,  50);
    }
}

/* Màu chữ trong ô: ô 2 và 4 dùng chữ sẫm, các ô lớn hơn dùng chữ sáng. */
colortype Board2048::tileTextColor(uint16_t value)
{
    if (value == 2 || value == 4)
    {
        return Color::getColorFromRGB(119, 110, 101);
    }
    return Color::getColorFromRGB(249, 246, 242);
}

/* -------------------------------------------------------------------------- */
/* Khởi tạo widget                                                            */
/* -------------------------------------------------------------------------- */
Board2048::Board2048()
    : game(0)
{
    /* Widget phủ toàn màn hình. */
    setWidth(SCREEN_W);
    setHeight(SCREEN_H);
}

Rect Board2048::getSolidRect() const
{
    /* Toàn bộ widget là vùng đặc (luôn vẽ kín nền) -> tối ưu vẽ lại. */
    return Rect(0, 0, getWidth(), getHeight());
}

/* -------------------------------------------------------------------------- */
/* Các hàm vẽ mức thấp                                                        */
/* -------------------------------------------------------------------------- */
/* Tô một hình chữ nhật theo tọa độ tương đối của widget:
   cắt theo vùng cần vẽ lại (area), đổi sang tọa độ tuyệt đối rồi tô. */
void Board2048::fillRel(const Rect& area, int16_t x, int16_t y,
                        int16_t w, int16_t h, colortype color) const
{
    Rect r = Rect(x, y, w, h) & area;   /* giao với vùng invalidated */
    if (!r.isEmpty())
    {
        translateRectToAbsolute(r);
        HAL::lcd().fillRect(r, color, 255);
    }
}

/* Vẽ một ký tự với hệ số phóng to nguyên s: mỗi điểm bật của font là ô s x s. */
void Board2048::drawChar(const Rect& area, int16_t x, int16_t y,
                         char c, int16_t s, colortype color) const
{
    const uint8_t* glyph = getGlyph(c);
    for (int16_t col = 0; col < 5; col++)
    {
        uint8_t bits = glyph[col];
        for (int16_t row = 0; row < 7; row++)
        {
            if (bits & (1 << row))    /* điểm ảnh bật trong font */
            {
                fillRel(area, x + col * s, y + row * s, s, s, color);
            }
        }
    }
}

/* Tính bề rộng chuỗi ở hệ số s: mỗi ký tự rộng 5 cột + 1 khe, ký tự cuối bỏ khe. */
int16_t Board2048::textWidth(const char* str, int16_t s)
{
    int16_t len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    if (len == 0)
    {
        return 0;
    }
    return (int16_t)(len * 6 * s - s);
}

int16_t Board2048::drawText(const Rect& area, int16_t x, int16_t y,
                            const char* str, int16_t s, colortype color) const
{
    int16_t cx = x;
    for (int16_t i = 0; str[i] != '\0'; i++)
    {
        drawChar(area, cx, y, str[i], s, color);
        cx += 6 * s;
    }
    return textWidth(str, s);
}

/* Vẽ ký tự với hệ số phân số num/den (vd 3/2 = 1.5x) để có cỡ chữ "nửa bước".
   Mỗi điểm font ánh xạ thành ô có kích thước làm tròn để raster sắc nét. */
void Board2048::drawCharFrac(const Rect& area, int16_t x, int16_t y,
                             char c, int16_t num, int16_t den, colortype color) const
{
    const uint8_t* glyph = getGlyph(c);
    for (int16_t col = 0; col < 5; col++)
    {
        int16_t dx0 = (int16_t)(col * num / den);
        int16_t dw  = (int16_t)((col + 1) * num / den) - dx0;
        if (dw < 1) dw = 1;

        uint8_t bits = glyph[col];
        for (int16_t row = 0; row < 7; row++)
        {
            if (bits & (1 << row))
            {
                int16_t dy0 = (int16_t)(row * num / den);
                int16_t dh  = (int16_t)((row + 1) * num / den) - dy0;
                if (dh < 1) dh = 1;
                fillRel(area, x + dx0, y + dy0, dw, dh, color);
            }
        }
    }
}

int16_t Board2048::textWidthFrac(const char* str, int16_t num, int16_t den)
{
    int16_t len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    if (len == 0)
    {
        return 0;
    }
    int16_t adv   = (int16_t)(6 * num / den);
    int16_t lastw = (int16_t)(5 * num / den);
    return (int16_t)((len - 1) * adv + lastw);
}

int16_t Board2048::drawTextFrac(const Rect& area, int16_t x, int16_t y,
                                const char* str, int16_t num, int16_t den, colortype color) const
{
    int16_t cx  = x;
    int16_t adv = (int16_t)(6 * num / den);
    for (int16_t i = 0; str[i] != '\0'; i++)
    {
        drawCharFrac(area, cx, y, str[i], num, den, color);
        cx += adv;
    }
    return textWidthFrac(str, num, den);
}

/* -------------------------------------------------------------------------- */
/* Hàm vẽ chính: được TouchGFX gọi mỗi khi widget cần vẽ lại                   */
/* -------------------------------------------------------------------------- */
void Board2048::draw(const Rect& invalidatedArea) const
{
    const colortype pageBg   = Color::getColorFromRGB(250, 248, 239);
    const colortype boardBg  = Color::getColorFromRGB(187, 173, 160);
    const colortype darkText = Color::getColorFromRGB(119, 110, 101);

    /* Nền cả trang. */
    fillRel(invalidatedArea, 0, 0, SCREEN_W, SCREEN_H, pageBg);

    /* Tiêu đề "2048" căn giữa. */
    {
        const char* title = "2048";
        int16_t s  = 6;
        int16_t tw = textWidth(title, s);
        drawText(invalidatedArea, (SCREEN_W - tw) / 2, TITLE_Y, title, s, darkText);
    }

    /* Hàng điểm: SCORE bên trái, BEST bên phải, cỡ 1.5x (= 3/2). */
    if (game != 0)
    {
        char num[6];
        const int16_t mn = 3, dn = 2;       /* fractional scale 3/2 */
        const int16_t gapW = 4 * mn / dn;   /* spacing between label and value */

        /* SCORE: căn trái. */
        uintToStr(game->score, num);
        int16_t lx = BOARD_X0 + GAP;
        drawTextFrac(invalidatedArea, lx, SCORE_Y, "SCORE", mn, dn, darkText);
        drawTextFrac(invalidatedArea, lx + textWidthFrac("SCORE", mn, dn) + gapW, SCORE_Y, num, mn, dn, darkText);

        /* BEST: căn phải (tính bề rộng để lùi vào từ mép phải lưới). */
        uintToStr(game2048_get_best(), num);
        int16_t bw = textWidthFrac("BEST", mn, dn) + gapW + textWidthFrac(num, mn, dn);
        int16_t bx = (BOARD_X0 + BOARD_PX - GAP) - bw;
        drawTextFrac(invalidatedArea, bx, SCORE_Y, "BEST", mn, dn, darkText);
        drawTextFrac(invalidatedArea, bx + textWidthFrac("BEST", mn, dn) + gapW, SCORE_Y, num, mn, dn, darkText);
    }

    /* Khối nền của lưới (màu lộ ra ở các khe giữa các ô). */
    fillRel(invalidatedArea, BOARD_X0, BOARD_Y0, BOARD_PX, BOARD_PX, boardBg);

    /* Vẽ 16 ô của lưới 4x4. */
    for (int16_t row = 0; row < 4; row++)
    {
        for (int16_t col = 0; col < 4; col++)
        {
            int16_t  tx    = TILE_X(col);
            int16_t  ty    = TILE_Y(row);
            uint16_t value = (game != 0) ? game->g[row][col] : 0;

            fillRel(invalidatedArea, tx, ty, TILE, TILE, tileColor(value));

            if (value != 0)
            {
                char num[6];
                uintToStr(value, num);

                /* Chọn cỡ chữ lớn nhất vẫn vừa trong ô (số càng nhiều chữ số càng nhỏ). */
                int16_t s = 1;
                for (int16_t cand = 6; cand >= 1; cand--)
                {
                    if (textWidth(num, cand) <= CONTENT_BUDGET && (7 * cand) <= CONTENT_BUDGET)
                    {
                        s = cand;
                        break;
                    }
                }

                /* Căn giữa số trong ô. */
                int16_t tw = textWidth(num, s);
                int16_t th = 7 * s;
                int16_t nx = tx + (TILE - tw) / 2;
                int16_t ny = ty + (TILE - th) / 2;
                drawText(invalidatedArea, nx, ny, num, s, tileTextColor(value));
            }
        }
    }

    /* Lớp phủ kết thúc ván (thắng / thua): hiện điểm và kỷ lục. */
    if (game != 0 && game->state != GAME_PLAYING)
    {
        const colortype panelBg    = Color::getColorFromRGB(119, 110, 101);
        const colortype panelText  = Color::getColorFromRGB(249, 246, 242);
        const colortype titleColor = (game->state == GAME_WIN)
                                          ? Color::getColorFromRGB(237, 194, 46)
                                          : Color::getColorFromRGB(246, 124, 95);

        /* Bảng thông báo đặt giữa màn hình. */
        const int16_t panelX = 18;
        const int16_t panelW = SCREEN_W - 2 * panelX;   /* 204 */
        const int16_t panelY = 86;
        const int16_t panelH = 148;

        fillRel(invalidatedArea, panelX, panelY, panelW, panelH, panelBg);

        /* Tiêu đề: WIN! nếu thắng, GAME OVER nếu thua. */
        const char* msg = (game->state == GAME_WIN) ? "WIN!" : "GAME OVER";
        int16_t ts = 3;
        drawText(invalidatedArea, (SCREEN_W - textWidth(msg, ts)) / 2, panelY + 16, msg, ts, titleColor);

        /* Hai dòng "NHÃN  giá trị"; giá trị căn cùng cột để thẳng hàng. */
        char num[6];
        int16_t ls = 2;
        int16_t lineX = panelX + 16;

        uintToStr(game->score, num);
        drawText(invalidatedArea, lineX, panelY + 56, "SCORE", ls, panelText);
        drawText(invalidatedArea, lineX + textWidth("SCORE", ls) + 6 * ls, panelY + 56, num, ls, panelText);

        uintToStr(game2048_get_best(), num);
        drawText(invalidatedArea, lineX, panelY + 80, "BEST", ls, panelText);
        drawText(invalidatedArea, lineX + textWidth("SCORE", ls) + 6 * ls, panelY + 80, num, ls, panelText);

        /* Gợi ý: bấm nút bất kỳ để chơi lại. */
        const char* hint = "PRESS KEY";
        int16_t hs = 2;
        drawText(invalidatedArea, (SCREEN_W - textWidth(hint, hs)) / 2, panelY + 116, hint, hs, panelText);
    }
}
