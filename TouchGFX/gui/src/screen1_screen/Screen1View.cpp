#include <gui/screen1_screen/Screen1View.hpp>

/* Hàng đợi nhận mã hướng từ task đọc nút (định nghĩa trong main.c). */
extern osMessageQueueId_t myQueue01Handle;

Screen1View::Screen1View()
{

}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    /* Bỏ các widget demo còn sót (nút LED + hình tròn chuyển động). */
    remove(btnLed);
    remove(circle1);

    /* Bắt đầu ván mới và hiển thị bàn cờ. */
    game2048_init(&game);
    board.setGame(&game);
    board.setXY(0, 0);
    add(board);
    board.invalidate();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::buttonClicked()
{

}

/* Gọi mỗi khung hình: lấy lệnh hướng từ hàng đợi và cập nhật game. */
void Screen1View::tickEvent()
{
    /* Không có lệnh nút -> không làm gì. */
    if (osMessageQueueGetCount(myQueue01Handle) == 0)
    {
        return;
    }

    uint16_t dir = 0xFFFF;
    if (osMessageQueueGet(myQueue01Handle, &dir, NULL, 0) != osOK)
    {
        return;
    }

    /* Khi đã thắng/thua: bấm nút bất kỳ để chơi lại ván mới. */
    if (game.state != GAME_PLAYING)
    {
        game2048_init(&game);
        board.invalidate();
        return;
    }

    /* Đang chơi: thực hiện nước đi theo hướng. Chỉ vẽ lại khi có thay đổi
       (bàn cờ dịch chuyển hoặc trạng thái chuyển sang thắng/thua). */
    if (dir <= GAME2048_DIR_RIGHT)
    {
        if (game2048_move(&game, (uint8_t)dir))
        {
            board.invalidate();
        }
        else if (game.state != GAME_PLAYING)
        {
            board.invalidate();
        }
    }
}
