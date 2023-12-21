#include "bgi2.h"

using namespace bgi;
using namespace bgi::colors;

int main(int argc, char *argv[])
{
    Surface surface(800, 600);
    Drawer d(surface);

    d.Clear(Blue);

    d.SetWriteStyle(Brown, /*scale_x=*/1, /*scale_y=*/2);
    d.Write(10, 10, "Welcome to BGI2!");
    d.Write(10, 30, "Press any key to exit!");

    d.SetFillStyle(LightBlue);
    d.FillRect(100, 100, 100, 100);

    App app;
    Window window("Simple", 800, 600);
    window.Update(surface);
    app.WaitKeyPress();
}
