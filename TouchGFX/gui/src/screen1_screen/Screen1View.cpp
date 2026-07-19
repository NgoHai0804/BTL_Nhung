#include <gui/screen1_screen/Screen1View.hpp>

// Hàng đợi nhận mã hướng từ task đọc nút
extern osMessageQueueId_t myQueue01Handle;

Screen1View::Screen1View()
{

}

/*
    Hàm khởi tạo màn hình khi được tạo
*/
void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
    //B1: Khởi tạo game - hàm game2048_init
    game2048_init(&game);

    //Trao widget  con trỏ đến trạng thái game
    board.setGame(&game);

    board.setXY(0, 0);
    //B2: Add widget Board2048
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

/*
    Hàm kiểm tra sự kiện tương tác
    Được TouchGFX gọi mỗi khung hình
    Sử dụng cơ chế hàng đợi phát hiện lệnh
*/
void Screen1View::tickEvent()
{

    uint16_t dir = 0xFFFF;
    // Gọi hàm GET, nếu không có thì thoát ngay (trả về osErrorResource)
    if (osMessageQueueGet(myQueue01Handle, &dir, NULL, 0) != osOK)
    {
        return;
    }

    //Trạng thái win/lose: cần khởi tạo lại game mới và click nút bất kỳ
    if (game.state != GAME_PLAYING)
    {
        game2048_init(&game);
        board.invalidate();
        return;
    }

    // Chỉ xử lý các hướng hợp lệ
    if (dir <= GAME2048_DIR_RIGHT)
    {
        // Thực hiện nước đi và lấy kết quả
        bool moved = game2048_move(&game, (uint8_t)dir);

        // Lưu ý: tính cả việc khi không còn hướng di chuyển -> trạng thái lose/win
        // Cần thêm điều kiện game.state để xử lý
        if (moved || game.state != GAME_PLAYING)
        {
            board.invalidate();
        }
    }
}
