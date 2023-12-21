#include "bgi2.h"

using namespace bgi;
using namespace bgi::colors;

int main(int argc, char *argv[])
{
    App app;
    Window window("Hello World", 800, 600);

    Surface surface(window.size());
    Drawer d(surface);
    d.Write(10, 10, "Hello world!");

    window.Update(surface);
    app.WaitKeyPress();
}
