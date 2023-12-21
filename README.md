# BGI2 (Beginners' Graphic Interface 2) library

A simple SDL2 based graphics library, which provides BGI style graphics in modern C++ (≥17).

The library is intentionally not GPU accelerated, but provides good performance for 2D graphics. (2000FPS for 800x600 is possible on a 1Ghz processor, assuming each pixel is written only once per frame).

Some of the code is based on the [SDL_bgi](https://sdl-bgi.sourceforge.io/) project, but it is written in modern C++ instead of C.

## Demo

This "Grill simulator" example was created with BGI2.

![](https://raw.githubusercontent.com/tdanyluk/bgi2/main/example/grill_example.gif)

## Hello world!

```c++
#include "bgi2.h"

using namespace bgi;

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
```

## Build

```c++
mkdir build
cd build
cmake ..
make && ./example_grill
```

## Drawing

### Colors

```c++
using Color = uint32_t;
```

Colors are 32 bit unsigned integers, where the bytes of the integer represent the Alpha, Red, Green and Blue channels, in this manner: `0xAARRGGBB`.

Each channel can have a value from 0 to 255. Currently the Alpha channel is not supported in most operations, so we should set it simply to 255 (fully opaque).

We can define colors with the `Rgb` and `Argb` functions or directly using the raw value.

```c++
Color yellow = Rgb(255,255,0);
Color red = 0xffff0000;
```

We can read components of the color using the `GetAlpha`, `GetRed`, `GetGreen` and `GetBlue` functions or directly using bit manipulation.

```c++
uint8_t red_value = GetRed(0xffaabbcc); // 0xaa
uint8_t green_value = (0xffaabbcc >> 8) & 0xff; // 0xbb
```

There are also predefined colors in the `bgi::colors` namespace.
They come in dark-light pairs that look nice together.

### Surfaces

```c++
struct Surface {
    Surface(int w, int h);
    std::vector<Color> pixels;
    int w = 0;
    int h = 0;
};
```

A surface represents an image in memory, using an array of colors (pixels) plus a width and height.
The image is stored in a row-major layout and pixels can be accessed directly if needed. All BGI2 data structures store the data in host (CPU) memory.

```c++
Surface s(800, 600);
const int x = 10;
const int y = 20;
s.pixels[y * s.w + x] = 0xffff00ff;
```

### Drawer

A drawer is a tool that can draw on a surface. It has a state, which consists of the current darwing, writing and fill style as well as the viewport.

```c++
Drawer d(surface);
d.Clear(Blue);

d.SetWriteStyle(Brown, /*scale_x=*/1, /*scale_y=*/2);
d.Write(10, 10, "Welcome to BGI2!");
d.Write(10, 30, "Use the arrow keys to move the rectangle!");

d.SetFillStyle(LightBlue);
d.FillRect(rect_x, rect_y, 50, 50);
```

We can copy a `Drawer` to save or restore the state.

The drawer has a `Viewport` method, which creates another `Drawer` which draws to the given Viewport. (Viewports are currently not clipping, it's possible to draw outside them - but of course we cannot draw outside the surface.)

## Application and window handling

We must have exactly one App instance for the whole duration of our program. It handles the loading/unloading of the SDL library and provides some functions/state which corresponds to the whole application.

Window instances correspond to OS windows.
They are opened automatically in the constructor and closed in the destructor.

```c++
App app;
Window win("My window", 800, 600);
win.Update(surface);
app.WaitKeyPress();
```

NOTE: Windows are currently scaled 2x to make thing more visible in high resolution screens. Multiple windows are supported, but it's recommended to use one window per application.

To draw a surface to a Window, we can use the `Window::Update(const Surface&)` method.

All windows are automatically closed when the program comes to an end, so we have to keep them open by waiting for something. For example, we can keep it open until a key is pressed, using `App::WaitKeyPress`.

## Input handling

We can wait for a keydown event with `App::WaitKeyPress` or we can check for an existing keydown event without blocking, using `App::PollKeyPress`.

For more advanced input handling (including mouse), we can write a regular [SDL event loop](https://lazyfoo.net/tutorials/SDL/17_mouse_events/index.php#:~:text=while%20application%20is%20running).

## Design principles

- Easy to use, but doesn't hide state
- Feels modern (no legacy things that are irrelevant now)
- Performant, but CPU-only
- Concise code that "anyone" can modify
- Easy to use concrete types, that are modifiable "manually"

## Differences from BGI

Modernizations:
- There are no graphic modes, just a window size.
- There are no palettes, just 8bit per channel ARGB colors.
  - We can emulate palettes with an `std::vector<Color>`.
- There are no visual pages.
  - We can emulate them with surfaces.

Simplifications:
- There are no relative drawing methods (linerel/lineto/etc).
- There is just one (bitmap) font.
- There are no line styles.
- There are no write modes (COPY_PUT/XOR_PUT).
- There are fewer, but more versatile methods, for example (DrawEllipse instead of circle, ellipse and arc.)

New functionality:
- 2D transformations of polygons.
- Rounded rectangles.

## Limitations

- Some missing functions.
- Currently only 8bit (ASCII) text is supported.
- Error handling is through asserts.

### License

The code is licensed under the MIT license.

Copyright 2023 Tamas Danyluk

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

