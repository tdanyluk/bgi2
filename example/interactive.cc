#include <algorithm>

#include "bgi2.h"

using namespace bgi;
using namespace bgi::colors;

int main(int argc, char *argv[])
{
    App app;
    Window main_win("Interactive", 800, 600);
    Surface surface(main_win.size());
    Drawer d(surface);

    int rect_x = 100;
    int rect_y = 100;
    const int step = 5;
    while (true)
    {
        // 1. Draw:
        d.Clear(Blue);

        d.SetWriteStyle(Brown, /*scale_x=*/1, /*scale_y=*/2);
        d.Write(10, 10, "Welcome to BGI2!");
        d.Write(10, 30, "Use the arrow keys to move the rectangle!");

        d.SetFillStyle(LightBlue);
        d.FillRect(rect_x, rect_y, 50, 50);

        // 2. Update window:
        main_win.Update(surface);

        // 3. Handle keys:
        switch (app.WaitKeyPress().keycode)
        {
        case SDLK_LEFT:
            rect_x -= step;
            break;
        case SDLK_RIGHT:
            rect_x += step;
            break;
        case SDLK_UP:
            rect_y -= step;
            break;
        case SDLK_DOWN:
            rect_y += step;
            break;
        }
        rect_x = std::clamp(rect_x, 0, surface.w - step);
        rect_y = std::clamp(rect_y, 0, surface.h - step);
    }

    return 0;
}
