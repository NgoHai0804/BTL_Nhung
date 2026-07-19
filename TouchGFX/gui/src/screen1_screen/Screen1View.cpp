#include <gui/screen1_screen/Screen1View.hpp>

// queue hướng từ StartDefaultTask (main.c)
extern osMessageQueueId_t myQueue01Handle;

Screen1View::Screen1View()
{

}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    // bỏ widget demo của template
    remove(btnLed);
    remove(circle1);

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

// mỗi frame: đọc nút từ queue → move / restart
void Screen1View::tickEvent()
{
    if (osMessageQueueGetCount(myQueue01Handle) == 0)
        return;

    uint16_t dir = 0xFFFF;
    if (osMessageQueueGet(myQueue01Handle, &dir, NULL, 0) != osOK)
        return;

    // thắng/thua rồi thì nút nào cũng = chơi lại
    if (game.state != GAME_PLAYING)
    {
        game2048_init(&game);
        board.invalidate();
        return;
    }

    if (dir <= GAME2048_DIR_RIGHT)
    {
        if (game2048_move(&game, (uint8_t)dir))
            board.invalidate();
        else if (game.state != GAME_PLAYING)
            board.invalidate(); // hết nước / đạt 2048 nhưng bàn không dịch
    }
}
