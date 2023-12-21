#include "bgi2.h"

#include <gtest/gtest.h>

TEST(Bgi2Test, FirstTest)
{
    bgi::App app;
    bgi::Window main_win("FirstTest", 800, 600);
    bgi::Surface surface(main_win.size());
    bgi::Drawer d(surface);
    d.Clear(bgi::colors::Blue);
    d.SetDrawStyle(bgi::colors::Red);
    d.DrawEllipse(100, 100, 20, 20);
    main_win.Update(surface);
}

// TODO more tests.
