#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <gui/screen1_screen/Board2048.hpp>
#include "cmsis_os2.h"
#include "game2048.h"

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void buttonClicked();
    void tickEvent();

protected:
    Game2048  game;    /* trạng thái logic của ván 2048 */
    Board2048 board;   /* widget hiển thị bàn cờ        */
};


#endif // SCREEN1VIEW_HPP
