#include "bgi2.h"

#include <algorithm>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <ctime>

// TODO: better error handling.
#define BGI_DIE(...)                      \
    do                                    \
    {                                     \
        fprintf(stderr, "Fatal error: "); \
        fprintf(stderr, __VA_ARGS__);     \
        fprintf(stderr, "\n");            \
        fflush(stderr);                   \
        std::exit(1);                     \
    } while (false)

#define BGI_WARN(...)                 \
    do                                \
    {                                 \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
        fflush(stderr);               \
    } while (false)

#define BGI_SDL_CHECK_ZERO(expr) \
    if ((expr) != 0)             \
    BGI_DIE("%s: %s", #expr, SDL_GetError())

#define BGI_SDL_CHECK_PTR(expr) \
    if ((expr) == nullptr)      \
    BGI_DIE("%s: %s", #expr, SDL_GetError())

#define BGI_WARN_FALSE(expr) \
    if (!(expr))             \
    BGI_WARN("Warning: %s failed", #expr)

namespace bgi
{
    namespace
    {
        void Crop(int &x, int &y, int &w, int &h, int sw, int sh)
        {
            if (x < 0)
            {
                w += x;
                x = 0;
            }
            if (y < 0)
            {
                h += y;
                y = 0;
            }
            if (x + w > sw)
            {
                w = sw - x;
            }
            if (y + h > sh)
            {
                h = sh - y;
            }
            if (w <= 0 || h <= 0)
            {
                w = 0;
                h = 0;
            }
        }

        // void draw_pixel(int x, int y, int i, int counter);
        template <typename F>
        void DrawLineTempl(int x1, int y1, int x2, int y2, int surface_w, int surface_h, F draw_pixel)
        {
            int counter = 0;
            int dx = std::abs(x2 - x1);
            int sx = x1 < x2 ? 1 : -1;
            int dy = std::abs(y2 - y1);
            int sy = y1 < y2 ? 1 : -1;
            int err = (dx > dy ? dx : -dy) / 2;

            for (;;)
            {
                if (x1 >= 0 && x1 < surface_w && y1 >= 0 && y1 < surface_h)
                    draw_pixel(x1, y1, y1 * surface_w + x1, counter++);

                if (x1 == x2 && y1 == y2)
                {
                    break;
                }

                int last_err = err;
                if (last_err > -dx)
                {
                    err -= dy;
                    x1 += sx;
                }
                if (last_err < dy)
                {
                    err += dx;
                    y1 += sy;
                }
            }
        }

        // void draw_pixel(int x, int y, int i);
        template <typename F>
        void DrawHorizLineTempl(int x1, int x2, int y, int surface_w, int surface_h, F draw_pixel)
        {
            if (y < 0 || y >= surface_h)
                return;

            if (x2 < x1)
                std::swap(x1, x2);

            if (x2 < 0 || x1 >= surface_w)
                return;

            if (x1 < 0)
                x1 = 0;

            if (x2 >= surface_w)
                x2 = surface_w - 1;

            for (int x = x1; x < x2; x++)
                draw_pixel(x, y, y * surface_w + x);
        }

        // void draw_pixel(int x, int y, int i);
        template <typename F>
        void FillRectTempl(int x, int y, int w, int h, int surface_w, int surface_h, F draw_pixel)
        {
            Crop(x, y, w, h, surface_w, surface_h);
            for (int row = y; row < y + h; row++)
            {
                int i = row * surface_w + x;
                for (int col = x; col < x + w; col++, i++)
                {
                    draw_pixel(col, row, i);
                }
            }
        }

        // void draw_pixel(int x, int y, int i);
        template <typename F>
        void FillPolygonTempl(const Polygon &polygon, int surface_w, int surface_h, F draw_pixel)
        {
            if (polygon.size() < 3)
            {
                BGI_WARN("Warning: Polygon size: %d < 3", Int(polygon.size()));
                return;
            }

            // find Y maxima
            int ymin = polygon.back().y;
            int ymax = polygon.back().y;
            for (const Point &p : polygon)
            {
                if (p.y < ymin)
                    ymin = p.y;
                else if (p.y > ymax)
                    ymax = p.y;
            }

            std::vector<int> nodeX;
            nodeX.reserve(polygon.size());
            for (int pixelY = ymin; pixelY <= ymax; pixelY++)
            {
                //  Build a list of nodes.
                nodeX.clear();
                Point p = polygon.back();
                for (Point q : polygon)
                {
                    float y = Float(pixelY);
                    float x1 = Float(p.x);
                    float y1 = Float(p.y);
                    float x2 = Float(q.x);
                    float y2 = Float(q.y);

                    if ((y1 < y && y2 >= y) || (y2 < y && y1 >= y))
                    {
                        nodeX.push_back(Int(x1 + (y - y1) / (y2 - y1) * (x2 - x1)));
                    }
                    else if ((y1 == y && y2 > y) || (y2 == y && y1 > y))
                    {
                        float x = (x1 + (y - y1) / (y2 - y1) * (x2 - x1));
                        if (x >= 0 && x < surface_w && y >= 0 && y < surface_h)
                        {
                            draw_pixel(x, y, y * surface_w + x);
                        }
                    }
                    else if (y1 == y && y2 == y)
                    {
                        DrawHorizLineTempl(Int(x1), Int(x2), Int(y), surface_w, surface_h, draw_pixel);
                    }
                    p = q;
                }
                std::sort(nodeX.begin(), nodeX.end());

                // fill the pixels between node pairs.
                if (nodeX.size() % 2 != 0)
                {
                    BGI_DIE("Nodes not even: %d", Int(nodeX.size()));
                }
                for (int i = 0; i < Int(nodeX.size()); i += 2)
                {
                    DrawHorizLineTempl(nodeX.at(i), nodeX.at(i + 1), pixelY, surface_w, surface_h, draw_pixel);
                }
            }
        }

        // Fills an ellipse centered at (cx, cy), with axes given by
        // xradius and yradius.
        //
        // From "A Fast Bresenham Type Algorithm For Drawing Ellipses"
        // by John Kennedy.
        //
        // void draw_pixel(int x, int y, int i);
        template <typename F>
        void FillEllipseTempl(int cx, int cy, int xradius, int yradius, int surface_w, int surface_h, F draw_pixel)
        {
            if (xradius == 0 && yradius == 0)
                return;

            const int TwoASquare = 2 * xradius * xradius;
            const int TwoBSquare = 2 * yradius * yradius;

            int x = xradius;
            int y = 0;
            int xchange = yradius * yradius * (1 - 2 * xradius);
            int ychange = xradius * xradius;
            int ellipseerror = 0;
            int StoppingX = TwoBSquare * xradius;
            int StoppingY = 0;

            while (StoppingX >= StoppingY)
            {
                // 1st set of points, y' > -1
                DrawHorizLineTempl(cx - x, cx + x, cy - y, surface_w, surface_h, draw_pixel);
                DrawHorizLineTempl(cx - x, cx + x, cy + y, surface_w, surface_h, draw_pixel);

                y++;
                StoppingY += TwoASquare;
                ellipseerror += ychange;
                ychange += TwoASquare;

                if ((2 * ellipseerror + xchange) > 0)
                {
                    x--;
                    StoppingX -= TwoBSquare;
                    ellipseerror += xchange;
                    xchange += TwoBSquare;
                }
            }

            // 1st point set is done; start the 2nd set of points
            x = 0;
            y = yradius;
            xchange = yradius * yradius;
            ychange = xradius * xradius * (1 - 2 * yradius);
            ellipseerror = 0;
            StoppingX = 0;
            StoppingY = TwoASquare * yradius;

            while (StoppingX <= StoppingY)
            {
                // 2nd set of points, y' < -1
                DrawHorizLineTempl(cx - x, cx + x, cy - y, surface_w, surface_h, draw_pixel);
                DrawHorizLineTempl(cx - x, cx + x, cy + y, surface_w, surface_h, draw_pixel);

                x++;
                StoppingX += TwoBSquare;
                ellipseerror += xchange;
                xchange += TwoBSquare;

                if ((2 * ellipseerror + ychange) > 0)
                {
                    y--;
                    StoppingY -= TwoASquare;
                    ellipseerror += ychange;
                    ychange += TwoASquare;
                }
            }
        }

        // Draws an ellipse centered at (cx, cy), with axes given by
        // xradius and yradius.
        //
        // From "A Fast Bresenham Type Algorithm For Drawing Ellipses"
        // by John Kennedy.
        //
        // void draw_pixel(int x, int y, int i, int counter);
        template <typename F>
        void DrawEllipseTempl(int cx, int cy, int xradius, int yradius, int surface_w, int surface_h, F draw_pixel)
        {
            auto draw_pixel_if_needed = [&](int x, int y)
            {
                if (x >= 0 && x < surface_w && y >= 0 && y < surface_h)
                    draw_pixel(x, y, y * surface_w + x, 0);
            };

            int x,
                y,
                xchange, ychange,
                ellipseerror,
                TwoASquare, TwoBSquare,
                StoppingX, StoppingY;

            if (0 == xradius && 0 == yradius)
                return;

            TwoASquare = 2 * xradius * xradius;
            TwoBSquare = 2 * yradius * yradius;
            x = xradius;
            y = 0;
            xchange = yradius * yradius * (1 - 2 * xradius);
            ychange = xradius * xradius;
            ellipseerror = 0;
            StoppingX = TwoBSquare * xradius;
            StoppingY = 0;

            while (StoppingX >= StoppingY)
            {
                // 1st set of points, y' > -1
                draw_pixel_if_needed(cx + x, cy - y);
                draw_pixel_if_needed(cx - x, cy - y);
                draw_pixel_if_needed(cx - x, cy + y);
                draw_pixel_if_needed(cx + x, cy + y);

                y++;
                StoppingY += TwoASquare;
                ellipseerror += ychange;
                ychange += TwoASquare;

                if ((2 * ellipseerror + xchange) > 0)
                {
                    x--;
                    StoppingX -= TwoBSquare;
                    ellipseerror += xchange;
                    xchange += TwoBSquare;
                }
            }

            // 1st point set is done; start the 2nd set of points
            x = 0;
            y = yradius;
            xchange = yradius * yradius;
            ychange = xradius * xradius * (1 - 2 * yradius);
            ellipseerror = 0;
            StoppingX = 0;
            StoppingY = TwoASquare * yradius;

            while (StoppingX <= StoppingY)
            {
                // 2nd set of points, y' < -1
                draw_pixel_if_needed(cx + x, cy - y);
                draw_pixel_if_needed(cx - x, cy - y);
                draw_pixel_if_needed(cx - x, cy + y);
                draw_pixel_if_needed(cx + x, cy + y);
                x++;
                StoppingX += TwoBSquare;
                ellipseerror += xchange;
                xchange += TwoBSquare;
                if ((2 * ellipseerror + ychange) > 0)
                {
                    y--,
                        StoppingY -= TwoASquare;
                    ellipseerror += ychange;
                    ychange += TwoASquare;
                }
            }
        }

    } // namespace

    App::App() : App(time(nullptr))
    {
    }

    App::App(int random_seed)
    {
        BGI_SDL_CHECK_ZERO(SDL_Init(SDL_INIT_VIDEO));
        BGI_WARN_FALSE(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest"));
        std::srand(random_seed);
    }

    int App::Random(int exclusive_upper_limit)
    {
        // TODO: better random. SDL random?
        return std::rand() % exclusive_upper_limit;
    }

    Color App::RandomRgbColor()
    {
        return Rgb(Random(256), Random(256), Random(256));
    }

    Color App::RandomColor()
    {
        return colors::AllColors[Random(colors::AllColors.size())];
    }

    App::~App()
    {
        SDL_Quit();
    }

    KeyPress App::WaitKeyPress(bool auto_quit)
    {
        SDL_Event e;
        while (!(SDL_WaitEvent(&e) && (e.type == SDL_QUIT || e.type == SDL_KEYDOWN)))
        {
            // wait for next event
        };
        if (e.type == SDL_QUIT)
        {
            if (auto_quit)
            {
                SDL_Quit();
                std::exit(1);
            }
            return {/*should_quit=*/true};
        }
        return {/*should_quit=*/false, e.key.keysym.sym, e.key.keysym.scancode};
    }

    bool App::PollKeyPress(KeyPress &key_press, bool auto_quit)
    {
        key_press = {};
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_QUIT:
                if (auto_quit)
                {
                    SDL_Quit();
                    std::exit(1);
                }
                key_press.should_quit = true;
                return true;
            case SDL_KEYDOWN:
                key_press.keycode = e.key.keysym.sym;
                key_press.scancode = e.key.keysym.scancode;
                return true;
            default:;
            }
        };
        return false;
    }

    Window::Window(std::string_view title, int w, int h)
    {
        Size physical_size(2 * w, 2 * h);
        BGI_SDL_CHECK_PTR(window_ = SDL_CreateWindow(std::string(title).c_str(),
                                                     SDL_WINDOWPOS_UNDEFINED,
                                                     SDL_WINDOWPOS_UNDEFINED,
                                                     physical_size.w,
                                                     physical_size.h,
                                                     SDL_WINDOW_SHOWN));
        BGI_SDL_CHECK_PTR(renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_PRESENTVSYNC));
        BGI_SDL_CHECK_ZERO(SDL_RenderSetLogicalSize(renderer_, w, h));
        BGI_SDL_CHECK_PTR(texture_ = SDL_CreateTexture(renderer_,
                                                       SDL_PIXELFORMAT_ARGB8888,
                                                       SDL_TEXTUREACCESS_STREAMING,
                                                       w, h));
    }

    Window::~Window()
    {
        SDL_DestroyWindow(window_);
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyTexture(texture_);
    }

    void Window::Update(const Surface &surface)
    {
        BGI_SDL_CHECK_ZERO(SDL_UpdateTexture(texture_, NULL, surface.pixels.data(), surface.w * sizeof(Color)));
        BGI_SDL_CHECK_ZERO(SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255));
        BGI_SDL_CHECK_ZERO(SDL_RenderClear(renderer_));
        BGI_SDL_CHECK_ZERO(SDL_RenderCopy(renderer_, texture_, NULL, NULL));
        SDL_RenderPresent(renderer_);
    }

    void Window::set_fullscreen(bool full_screen)
    {
        BGI_SDL_CHECK_ZERO(SDL_SetWindowFullscreen(window_, full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
    }

    Drawer::Drawer(Surface &surface)
        : Drawer(surface, Rect(0, 0, surface.w, surface.h))
    {
    }

    Drawer::Drawer(Surface &surface, const Rect &viewport)
        : surface_(&surface), viewport_(viewport)
    {
    }

    Drawer::~Drawer() = default;

    Drawer Drawer::Viewport(int x, int y, int w, int h)
    {
        Drawer d = *this;
        d.viewport_.x += x;
        d.viewport_.y += y;
        d.viewport_.w = w;
        d.viewport_.h = h;
        return d;
    }

    Color *Drawer::GetPixelPtr(int x, int y) const
    {
        x += viewport_.x;
        y += viewport_.y;

        if (x < 0 || y < 0 || x >= surface_->w || y >= surface_->h)
        {
            return nullptr;
        }

        return &surface_->pixels[y * surface_->w + x];
    }

    Color Drawer::GetPixel(int x, int y) const
    {
        if (Color *pixel = GetPixelPtr(x, y))
        {
            return *pixel;
        }
        return basic_colors::Black;
    }

    void Drawer::SetPixel(int x, int y, Color c)
    {
        if (Color *pixel = GetPixelPtr(x, y))
        {
            *pixel = c;
        }
    }

    void Drawer::Clear(Color c)
    {
        Drawer d = *this;
        d.SetFillStyle(c);
        d.FillRect(0, 0, viewport_.w, viewport_.h);
    }

    void Drawer::DrawRect(int x, int y, int w, int h)
    {
        DrawPoly(x, y,
                 x + w - 1, y,
                 x, y + h - 1,
                 x + w - 1, y + h - 1);
    }

    void Drawer::FillRect(int x, int y, int w, int h)
    {
        x += viewport_.x;
        y += viewport_.y;

        if (fill_pattern_ == basic_fill_patterns::SolidBg && x == 0 && y == 0 && w == surface_->w && h == surface_->h)
        {
            std::fill(surface_->pixels.begin(), surface_->pixels.end(), fill_bg_color_);
            return;
        }

        if (fill_pattern_ == basic_fill_patterns::SolidBg)
        {
            Crop(x, y, w, h, surface_->w, surface_->h);
            for (int row = y; row < y + h; ++row)
            {
                std::fill(surface_->pixels.begin() + row * surface_->w + x,
                          surface_->pixels.begin() + row * surface_->w + x + w,
                          fill_bg_color_);
            }
        }
        else
        {
            FillRectTempl(x, y, w, h, surface_->w, surface_->h,
                          [pixels = surface_->pixels.data(),
                           pattern = fill_pattern_,
                           fg = fill_fg_color_,
                           bg = fill_bg_color_,
                           vpx = viewport_.x,
                           vpy = viewport_.y](int x, int y, int i)
                          { pixels[i] = IsFg(pattern, x - vpx, y - vpy) ? fg : bg; });
        }
    }

    void Drawer::FillRect(const Rect &rect)
    {
        FillRect(rect.x, rect.y, rect.w, rect.h);
    }

    void Drawer::DrawRoundedRect(int x, int y, int w, int h, int rx, int ry)
    {
        int m = std::min(w, h) / 2;
        rx = std::min(rx, m);
        ry = std::min(ry, m);

        int x2 = x + w - 1;
        int y2 = y + h - 1;
        DrawEllipse(x + rx, y + ry, rx, ry, 90, 180);
        DrawEllipse(x + rx, y2 - ry, rx, ry, 180, 270);
        DrawEllipse(x2 - rx, y + ry, rx, ry, 0, 90);
        DrawEllipse(x2 - rx, y2 - ry, rx, ry, 270, 360);
        DrawLine(x + rx, y, x2 - rx, y);
        DrawLine(x + rx, y2, x2 - rx, y2);
        DrawLine(x, y + ry, x, y2 - ry);
        DrawLine(x2, y + ry, x2, y2 - ry);
    }

    void Drawer::FillRoundedRect(int x, int y, int w, int h, int rx, int ry)
    {
        int m = std::min(w, h) / 2;
        rx = std::min(rx, m);
        ry = std::min(ry, m);

        int x2 = x + w - 1;
        int y2 = y + h - 1;
        FillEllipse(x + rx, y + ry, rx, ry);
        FillEllipse(x + rx, y2 - ry, rx, ry);
        FillEllipse(x2 - rx, y + ry, rx, ry);
        FillEllipse(x2 - rx, y2 - ry, rx, ry);
        FillRect(x + rx, y, w - 2 * rx, ry);
        FillRect(x, y + ry, w, h - 2 * ry);
        FillRect(x + rx, y2 - ry + 1, w - 2 * rx, ry);
    }

    // PointPair GetEllipticalArcEndpoints(int x, int y, int w, int h, int angle1 = 0, int angle2 = 360);
    void Drawer::DrawEllipse(int x, int y, int rx, int ry, int angle1, int angle2)
    {
        if (angle1 == 0 && angle2 == 360)
        {
            x += viewport_.x;
            y += viewport_.y;

            DrawEllipseTempl(
                x, y, rx, ry, surface_->w, surface_->h,
                [pixels = surface_->pixels.data(),
                 color = draw_color_](int, int, int i, int)
                { pixels[i] = color; });
            return;
        }

        DrawOpenPoly(MakeEllipticalArc(x, y, rx, ry, angle1, angle2));
    }

    void Drawer::FillEllipse(int x, int y, int rx, int ry, int angle1, int angle2)
    {
        if (angle1 == 0 && angle2 == 360)
        {
            x += viewport_.x;
            y += viewport_.y;

            if (fill_pattern_ == basic_fill_patterns::SolidBg)
            {
                FillEllipseTempl(x, y, rx, ry, surface_->w, surface_->h,
                                 [pixels = surface_->pixels.data(), bg = fill_bg_color_](int, int, int i)
                                 { pixels[i] = bg; });
            }
            else
            {
                FillEllipseTempl(x, y, rx, ry, surface_->w, surface_->h,
                                 [pixels = surface_->pixels.data(),
                                  pattern = fill_pattern_,
                                  fg = fill_fg_color_,
                                  bg = fill_bg_color_,
                                  vpx = viewport_.x,
                                  vpy = viewport_.y](int x, int y, int i)
                                 { pixels[i] = IsFg(pattern, x - vpx, y - vpy) ? fg : bg; });
            }
            return;
        }

        Polygon p = MakeEllipticalArc(x, y, rx, ry, angle1, angle2);
        p.push_back(Point(x, y));
        FillPoly(p);
    }

    void Drawer::DrawLine(int x1, int y1, int x2, int y2)
    {
        x1 += viewport_.x;
        y1 += viewport_.y;
        x2 += viewport_.x;
        y2 += viewport_.y;
        DrawLineTempl(x1, y1, x2, y2, surface_->w, surface_->h,
                      [pixels = surface_->pixels.data(),
                       color = draw_color_](int, int, int i, int)
                      { pixels[i] = color; });
    }

    void Drawer::SetPixelWithFillPattern(int x, int y)
    {
        SetPixel(x, y, IsFg(fill_pattern_, x, y) ? fill_fg_color_ : fill_bg_color_);
    }

    void Drawer::DrawOpenPoly(const Polygon &polygon)
    {
        if (polygon.size() == 0)
        {
            return;
        }
        if (polygon.size() == 1)
        {
            Point p = polygon.back();
            DrawLine(p.x, p.y, p.x, p.y);
            return;
        }
        for (int i = 0; i < Int(polygon.size()) - 1; i++)
        {
            Point p = polygon.at(i);
            Point q = polygon.at(i + 1);
            DrawLine(p.x, p.y, q.x, q.y);
        }
    }

    void Drawer::DrawPoly(const Polygon &polygon)
    {
        if (polygon.size() == 0)
        {
            BGI_WARN("Warning: Polygon size = 0");
            return;
        }
        Point p = polygon.back();
        for (Point q : polygon)
        {
            DrawLine(p.x, p.y, q.x, q.y);
            p = q;
        }
    }

    void Drawer::FillPoly(const Polygon &polygon)
    {
        Polygon p = Transform(polygon, 0, 1, 1, viewport_.x, viewport_.y);
        if (fill_pattern_ == basic_fill_patterns::SolidBg)
        {
            FillPolygonTempl(p, surface_->w, surface_->h,
                             [pixels = surface_->pixels.data(),
                              bg = fill_bg_color_](int, int, int i)
                             { pixels[i] = bg; });
        }
        else
        {
            FillPolygonTempl(p, surface_->w, surface_->h,
                             [pixels = surface_->pixels.data(),
                              pattern = fill_pattern_,
                              fg = fill_fg_color_,
                              bg = fill_bg_color_,
                              vpx = viewport_.x,
                              vpy = viewport_.y](int x, int y, int i)
                             { pixels[i] = IsFg(pattern, x - vpx, y - vpy) ? fg : bg; });
        }
    }

    Rect Drawer::GetTextRect(int x, int y, std::string_view text)
    {
        return Rect(x, y, write_scale_x_ * 8 * text.size(), write_scale_y_ * 8);
    }

    void Drawer::Write(int x, int y, std::string_view text)
    {
        for (size_t i = 0; i < text.size(); i++)
        {
            DrawBitmapChar(x + i * write_scale_x_ * 8, y, text[i]);
        }
    }

    void Drawer::WriteEx(int x, int y, std::string_view text, const Padding &padding, const Margin &margin, int rx, int ry)
    {
        Rect r = GetTextRect(x, y, text);
        FillRoundedRect(r.x - padding.left - margin.left - 1,
                        r.y - padding.top - margin.top - 1,
                        r.w + padding.left + padding.right + margin.left + margin.right + 2,
                        r.h + padding.top + padding.bottom + margin.top + margin.bottom + 2,
                        rx, ry);
        DrawRoundedRect(r.x - padding.left - 1,
                        r.y - padding.top - 1,
                        r.w + padding.left + padding.right + 2,
                        r.h + padding.top + padding.bottom + 2,
                        rx, ry);
        Write(x, y, text);
    }

    void Drawer::SetDrawStyle(Color c)
    {
        draw_color_ = c;
    }

    void Drawer::SetFillStyle(Color c)
    {
        fill_bg_color_ = c;
        fill_fg_color_ = c;
        fill_pattern_ = basic_fill_patterns::SolidBg;
    }

    void Drawer::SetFillStyle(FillPattern pattern, Color bg, Color fg)
    {
        fill_bg_color_ = bg;
        fill_fg_color_ = fg;
        fill_pattern_ = pattern;
    }

    void Drawer::SetWriteStyle(Color c, int scale_x, int scale_y)
    {
        write_color_ = c;
        write_scale_x_ = scale_x;
        write_scale_y_ = scale_y;
    }

    void Drawer::DrawBitmapChar(int x, int y, char c)
    {
        uint8_t uc = static_cast<uint8_t>(c);
        FillPattern pattern = bitmap_font_[uc];

        if (write_scale_x_ == 1 && write_scale_y_ == 1)
        {
            x += viewport_.x;
            y += viewport_.y;

            FillRectTempl(x, y, 8, 8, surface_->w, surface_->h,
                          [pixels = surface_->pixels.data(),
                           pattern,
                           fg = write_color_,
                           x0 = x,
                           y0 = y](int x, int y, int i)
                          {
                              if (IsFg(pattern, x - x0, y - y0))
                                  pixels[i] = fg;
                          });
            return;
        }

        Drawer d = *this;
        d.SetFillStyle(write_color_);
        for (int row = 0; row < 8; row++)
        {
            for (int col = 0; col < 8; col++)
            {
                if (IsFg(pattern, col, row))
                {
                    d.FillRect(x + col * write_scale_x_, y + row * write_scale_y_, write_scale_x_, write_scale_y_);
                }
            }
        }
    }

    Polygon Transform(const Polygon &polygon, float cw_rot_deg, float scale_x, float scale_y, int translate_x, int translate_y)
    {
        Polygon result = polygon;
        if (cw_rot_deg != 0.0f)
        {
            float rad = 0.01745329252f * cw_rot_deg;
            float cosine = std::cos(rad);
            float sine = std::sin(rad);
            for (Point &p : result)
            {
                p = {Round((p.x * cosine - p.y * sine) * scale_x) + translate_x,
                     Round((p.x * sine + p.y * cosine) * scale_y) + translate_y};
            }
            return result;
        }

        if (scale_x != 1.0f || scale_y != 1.0f)
        {
            for (Point &p : result)
            {
                p = {Round(p.x * scale_x) + translate_x,
                     Round(p.y * scale_y) + translate_y};
            }
            return result;
        }

        if (translate_x != 0 || translate_y != 0)
        {
            for (Point &p : result)
            {
                p = {p.x + translate_x,
                     p.y + translate_y};
            }
            return result;
        }

        return result;
    }

    Polygon Transform(const Polygon &polygon, const TransformType &transform)
    {
        return Transform(polygon, transform.cw_rot_deg, transform.scale_x, transform.scale_y, transform.translate_x, transform.translate_y);
    }

    Polygon MirrorHoriz(const Polygon &polygon, int mirror_x)
    {
        return Transform(polygon, 0, -1, 1, 2 * mirror_x, 0);
    }
    Polygon MirrorVert(const Polygon &polygon, int mirror_y)
    {
        return Transform(polygon, 0, 1, -1, 0, 2 * mirror_y);
    }
    Polygon MirrorHorizConcat(const Polygon &polygon, int mirror_x)
    {
        Polygon mirrored = MirrorHoriz(polygon, mirror_x);
        Polygon result = polygon;
        std::reverse_copy(mirrored.begin(), mirrored.end(), std::back_inserter(result));
        return result;
    }

    Polygon MakeEllipticalArc(int x, int y, int rx, int ry, int angle1, int angle2)
    {
        constexpr float pi_div_180 = 3.1415926 / 180.0;
        constexpr float pi_mul_2 = 3.1415926 * 2.0;
        constexpr float eps = 0.01;

        int steps = std::max(1, Round(std::max(rx, ry) * pi_mul_2 * (angle2 - angle1 + 1) / 360.f));
        float step = Float(angle2 - angle1) / steps;

        Polygon poly;
        poly.push_back(Point(x + Round(rx * std::cos(angle1 * pi_div_180)),
                             y - Round(ry * std::sin(angle1 * pi_div_180))));
        for (float angle = angle1 + step; angle < angle2 + eps; angle += step)
        {
            Point p(x + Round(rx * std::cos(angle * pi_div_180)),
                    y - Round(ry * std::sin(angle * pi_div_180)));
            if (poly.back().x != p.x || poly.back().y != p.y)
            {
                poly.push_back(p);
            }
        }
        return poly;
    }

    // 8x8 font array, dumped from DOSBox
    std::array<FillPattern, 256> Drawer::bitmap_font_{
        0x0000000000000000ull, //  0 0x00
        0x7e81a581bd99817eull, //  1 0x01
        0x7effdbffc3e7ff7eull, //  2 0x02
        0x6cfefefe7c381000ull, //  3 0x03
        0x10387cfe7c381000ull, //  4 0x04
        0x387c38fefe7c387cull, //  5 0x05
        0x1010387cfe7c387cull, //  6 0x06
        0x0000183c3c180000ull, //  7 0x07
        0xffffe7c3c3e7ffffull, //  8 0x08
        0x003c664242663c00ull, //  9 0x09
        0xffc399bdbd99c3ffull, // 10 0x0a
        0x0f070f7dcccccc78ull, // 11 0x0b
        0x3c6666663c187e18ull, // 12 0x0c
        0x3f333f303070f0e0ull, // 13 0x0d
        0x7f637f636367e6c0ull, // 14 0x0e
        0x995a3ce7e73c5a99ull, // 15 0x0f
        0x80e0f8fef8e08000ull, // 16 0x10
        0x020e3efe3e0e0200ull, // 17 0x11
        0x183c7e18187e3c18ull, // 18 0x12
        0x6666666666006600ull, // 19 0x13
        0x7fdbdb7b1b1b1b00ull, // 20 0x14
        0x3e63386c6c38cc78ull, // 21 0x15
        0x000000007e7e7e00ull, // 22 0x16
        0x183c7e187e3c18ffull, // 23 0x17
        0x183c7e1818181800ull, // 24 0x18
        0x181818187e3c1800ull, // 25 0x19
        0x00180cfe0c180000ull, // 26 0x1a
        0x003060fe60300000ull, // 27 0x1b
        0x0000c0c0c0fe0000ull, // 28 0x1c
        0x002466ff66240000ull, // 29 0x1d
        0x00183c7effff0000ull, // 30 0x1e
        0x00ffff7e3c180000ull, // 31 0x1f
        0x0000000000000000ull, // 32 0x20 ' '
        0x3078783030003000ull, // 33 0x21 '!'
        0x6c6c6c0000000000ull, // 34 0x22 '"'
        0x6c6cfe6cfe6c6c00ull, // 35 0x23 '#'
        0x307cc0780cf83000ull, // 36 0x24 '$'
        0x00c6cc183066c600ull, // 37 0x25 '%'
        0x386c3876dccc7600ull, // 38 0x26 '&'
        0x6060c00000000000ull, // 39 0x27 '''
        0x1830606060301800ull, // 40 0x28 '('
        0x6030181818306000ull, // 41 0x29 ')'
        0x00663cff3c660000ull, // 42 0x2a '*'
        0x003030fc30300000ull, // 43 0x2b '+'
        0x0000000000303060ull, // 44 0x2c ','
        0x000000fc00000000ull, // 45 0x2d '-'
        0x0000000000303000ull, // 46 0x2e '.'
        0x060c183060c08000ull, // 47 0x2f '/'
        0x7cc6c6c6c6c67c00ull, // 48 0x30 '0'
        0x307030303030fc00ull, // 49 0x31 '1'
        0x78cc0c3860ccfc00ull, // 50 0x32 '2'
        0x78cc0c380ccc7800ull, // 51 0x33 '3'
        0x1c3c6cccfe0c1e00ull, // 52 0x34 '4'
        0xfcc0f80c0ccc7800ull, // 53 0x35 '5'
        0x3860c0f8cccc7800ull, // 54 0x36 '6'
        0xfccc0c1830303000ull, // 55 0x37 '7'
        0x78cccc78cccc7800ull, // 56 0x38 '8'
        0x78cccc7c0c187000ull, // 57 0x39 '9'
        0x0030300000303000ull, // 58 0x3a ':'
        0x0030300000303060ull, // 59 0x3b ';'
        0x183060c060301800ull, // 60 0x3c '<'
        0x0000fc0000fc0000ull, // 61 0x3d '='
        0x6030180c18306000ull, // 62 0x3e '>'
        0x78cc0c1830003000ull, // 63 0x3f '?'
        0x7cc6dededec07800ull, // 64 0x40 '@'
        0x386cc6c6fec6c600ull, // 65 0x41 'A'
        0xfcc6c6fcc6c6fc00ull, // 66 0x42 'B'
        0x7cc6c6c0c0c67c00ull, // 67 0x43 'C'
        0xf8ccc6c6c6ccf800ull, // 68 0x44 'D'
        0xfec0c0fcc0c0fe00ull, // 69 0x45 'E'
        0xfec0c0fcc0c0c000ull, // 70 0x46 'F'
        0x7cc6c0cec6c67e00ull, // 71 0x47 'G'
        0xc6c6c6fec6c6c600ull, // 72 0x48 'H'
        0x7830303030307800ull, // 73 0x49 'I'
        0x1e060606c6c67c00ull, // 74 0x4a 'J'
        0xc6ccd8f0d8ccc600ull, // 75 0x4b 'K'
        0xc0c0c0c0c0c0fe00ull, // 76 0x4c 'L'
        0xc6eefed6c6c6c600ull, // 77 0x4d 'M'
        0xc6e6f6decec6c600ull, // 78 0x4e 'N'
        0x7cc6c6c6c6c67c00ull, // 79 0x4f 'O'
        0xfcc6c6fcc0c0c000ull, // 80 0x50 'P'
        0x7cc6c6c6c6c67c06ull, // 81 0x51 'Q'
        0xfcc6c6fcc6c6c600ull, // 82 0x52 'R'
        0x78cc603018cc7800ull, // 83 0x53 'S'
        0xfc30303030303000ull, // 84 0x54 'T'
        0xc6c6c6c6c6c67c00ull, // 85 0x55 'U'
        0xc6c6c6c6c66c3800ull, // 86 0x56 'V'
        0xc6c6c6d6feeec600ull, // 87 0x57 'W'
        0xc6c66c386cc6c600ull, // 88 0x58 'X'
        0xc3c3663c18181800ull, // 89 0x59 'Y'
        0xfe0c183060c0fe00ull, // 90 0x5a 'Z'
        0x3c30303030303c00ull, // 91 0x5b '['
        0xc06030180c060300ull, // 92 0x5c '\'
        0x3c0c0c0c0c0c3c00ull, // 93 0x5d ']'
        0x00386cc600000000ull, // 94 0x5e '^'
        0x00000000000000ffull, // 95 0x5f '_'
        0x3030180000000000ull, // 96 0x60 '`'
        0x00007c067ec67e00ull, // 97 0x61 'a'
        0xc0c0fcc6c6e6dc00ull, // 98 0x62 'b'
        0x00007cc6c0c07e00ull, // 99 0x63 'c'
        0x06067ec6c6ce7600ull, // 100 0x64 'd'
        0x00007cc6fec07e00ull, // 101 0x65 'e'
        0x1e307c3030303000ull, // 102 0x66 'f'
        0x00007ec6ce76067cull, // 103 0x67 'g'
        0xc0c0fcc6c6c6c600ull, // 104 0x68 'h'
        0x1800381818183c00ull, // 105 0x69 'i'
        0x18003818181818f0ull, // 106 0x6a 'j'
        0xc0c0ccd8f0d8cc00ull, // 107 0x6b 'k'
        0x3818181818183c00ull, // 108 0x6c 'l'
        0x0000ccfed6c6c600ull, // 109 0x6d 'm'
        0x0000fcc6c6c6c600ull, // 110 0x6e 'n'
        0x00007cc6c6c67c00ull, // 111 0x6f 'o'
        0x0000fcc6c6e6dcc0ull, // 112 0x70 'p'
        0x00007ec6c6ce7606ull, // 113 0x71 'q'
        0x00006e7060606000ull, // 114 0x72 'r'
        0x00007cc07c06fc00ull, // 115 0x73 's'
        0x30307c3030301c00ull, // 116 0x74 't'
        0x0000c6c6c6c67e00ull, // 117 0x75 'u'
        0x0000c6c6c66c3800ull, // 118 0x76 'v'
        0x0000c6c6d6fe6c00ull, // 119 0x77 'w'
        0x0000c66c386cc600ull, // 120 0x78 'x'
        0x0000c6c6ce76067cull, // 121 0x79 'y'
        0x0000fc183060fc00ull, // 122 0x7a 'z'
        0x1c3030e030301c00ull, // 123 0x7b '{'
        0x1818180018181800ull, // 124 0x7c '|'
        0xe030301c3030e000ull, // 125 0x7d '}'
        0x76dc000000000000ull, // 126 0x7e '~'
        0x0010386cc6c6fe00ull, // 127 0x7f
        0x3c66c0c0663c1870ull, // 128 0x80
        0x00cc00cccccccc76ull, // 129 0x81
        0x0c18007cc6fec07cull, // 130 0x82
        0x386c00780c7ccc76ull, // 131 0x83
        0x00cc00780c7ccc76ull, // 132 0x84
        0x603000780c7ccc76ull, // 133 0x85
        0x386c38780c7ccc76ull, // 134 0x86
        0x007cc6c0c67c1870ull, // 135 0x87
        0x386c007cc6fec07cull, // 136 0x88
        0x00c6007cc6fec07cull, // 137 0x89
        0x3018007cc6fec07cull, // 138 0x8a
        0x006600381818183cull, // 139 0x8b
        0x386c00381818183cull, // 140 0x8c
        0x301800381818183cull, // 141 0x8d
        0xc610386cc6fec6c6ull, // 142 0x8e
        0x386c387cc6fec6c6ull, // 143 0x8f
        0x0c18fec0f8c0c0feull, // 144 0x90
        0x000000ec367ed86eull, // 145 0x91
        0x003e6cccfeccccceull, // 146 0x92
        0x386c007cc6c6c67cull, // 147 0x93
        0x00c6007cc6c6c67cull, // 148 0x94
        0x3018007cc6c6c67cull, // 149 0x95
        0x78cc00cccccccc76ull, // 150 0x96
        0x603000cccccccc76ull, // 151 0x97
        0x00c600c6c67e06fcull, // 152 0x98
        0xc600386cc6c66c38ull, // 153 0x99
        0xc600c6c6c6c6c67cull, // 154 0x9a
        0x00027cced6e67c80ull, // 155 0x9b
        0x386c64f0606066fcull, // 156 0x9c
        0x003a6cced6e66cb8ull, // 157 0x9d
        0x0000c66c386cc600ull, // 158 0x9e
        0x0e1b183c1818d870ull, // 159 0x9f
        0x183000780c7ccc76ull, // 160 0xa0
        0x0c1800381818183cull, // 161 0xa1
        0x0c18007cc6c6c67cull, // 162 0xa2
        0x183000cccccccc76ull, // 163 0xa3
        0x76dc00dc66666666ull, // 164 0xa4
        0x76dc00e6f6decec6ull, // 165 0xa5
        0x003c6c6c36007e00ull, // 166 0xa6
        0x00386c6c38007c00ull, // 167 0xa7
        0x003000303060c67cull, // 168 0xa8
        0x7c82b2aab2aa827cull, // 169 0xa9
        0x000000fe06060000ull, // 170 0xaa
        0x63e66c7e3366cc0full, // 171 0xab
        0x63e66c7a366adf06ull, // 172 0xac
        0x00180018183c3c18ull, // 173 0xad
        0x0000003366cc6633ull, // 174 0xae
        0x000000cc663366ccull, // 175 0xaf
        0x2288228822882288ull, // 176 0xb0
        0x55aa55aa55aa55aaull, // 177 0xb1
        0x77dd77dd77dd77ddull, // 178 0xb2
        0x1818181818181818ull, // 179 0xb3
        0x18181818f8181818ull, // 180 0xb4
        0x0c18386cc6fec6c6ull, // 181 0xb5
        0x7cc6386cc6fec6c6ull, // 182 0xb6
        0x6030386cc6fec6c6ull, // 183 0xb7
        0x7c829aa2a29a827cull, // 184 0xb8
        0x3636f606f6363636ull, // 185 0xb9
        0x3636363636363636ull, // 186 0xba
        0x0000fe06f6363636ull, // 187 0xbb
        0x3636f606fe000000ull, // 188 0xbc
        0x18187ec0c07e1818ull, // 189 0xbd
        0x66663c7e187e1818ull, // 190 0xbe
        0x00000000f8181818ull, // 191 0xbf
        0x181818181f000000ull, // 192 0xc0
        0x18181818ff000000ull, // 193 0xc1
        0x00000000ff181818ull, // 194 0xc2
        0x181818181f181818ull, // 195 0xc3
        0x00000000ff000000ull, // 196 0xc4
        0x18181818ff181818ull, // 197 0xc5
        0x76dc00780c7ccc76ull, // 198 0xc6
        0x76dc386cc6fec6c6ull, // 199 0xc7
        0x363637303f000000ull, // 200 0xc8
        0x00003f3037363636ull, // 201 0xc9
        0x3636f700ff000000ull, // 202 0xca
        0x0000ff00f7363636ull, // 203 0xcb
        0x3636373037363636ull, // 204 0xcc
        0x0000ff00ff000000ull, // 205 0xcd
        0x3636f700f7363636ull, // 206 0xce
        0x0000c67cc6c67cc6ull, // 207 0xcf
        0x76186c063e66663cull, // 208 0xd0
        0x00f86c66f6666cf8ull, // 209 0xd1
        0x386c00fec0f8c0feull, // 210 0xd2
        0xc600fec0f8c0c0feull, // 211 0xd3
        0x3018fec0f8c0c0feull, // 212 0xd4
        0x7cc2f8c0f0c0c27cull, // 213 0xd5
        0x0c183c181818183cull, // 214 0xd6
        0x3c66003c1818183cull, // 215 0xd7
        0x66003c181818183cull, // 216 0xd8
        0x18181818f8000000ull, // 217 0xd9
        0x000000001f181818ull, // 218 0xda
        0xffffffffffffffffull, // 219 0xdb
        0x00000000ffffffffull, // 220 0xdc
        0x1818180000181818ull, // 221 0xdd
        0x30183c181818183cull, // 222 0xde
        0xffffffff00000000ull, // 223 0xdf
        0x0c18386cc6c66c38ull, // 224 0xe0
        0x0078ccd8ccc6c6ccull, // 225 0xe1
        0x7cc6386cc6c66c38ull, // 226 0xe2
        0x6030386cc6c66c38ull, // 227 0xe3
        0x76dc007cc6c6c67cull, // 228 0xe4
        0x76dc386cc6c66c38ull, // 229 0xe5
        0x0000006666667cc0ull, // 230 0xe6
        0xe0607c66667c60f0ull, // 231 0xe7
        0x00f0607c667c60f0ull, // 232 0xe8
        0x0c18c6c6c6c6c67cull, // 233 0xe9
        0x386c00c6c6c6c67cull, // 234 0xea
        0x3018c6c6c6c6c67cull, // 235 0xeb
        0x0c1800c6c67e06fcull, // 236 0xec
        0x0c1866663c18183cull, // 237 0xed
        0xff00000000000000ull, // 238 0xee
        0x0c18000000000000ull, // 239 0xef
        0x000000007e000000ull, // 240 0xf0
        0x0018187e1818007eull, // 241 0xf1
        0x0000000000ff00ffull, // 242 0xf2
        0xe132e43af62a5f86ull, // 243 0xf3
        0x007fdbdb7b1b1b1bull, // 244 0xf4
        0x3e613c66663c867cull, // 245 0xf5
        0x000018007e001800ull, // 246 0xf6
        0x0000000000001870ull, // 247 0xf7
        0x00386c6c38000000ull, // 248 0xf8
        0xc600000000000000ull, // 249 0xf9
        0x0000000018000000ull, // 250 0xfa
        0x00183818183c0000ull, // 251 0xfb
        0x00780c380c780000ull, // 252 0xfc
        0x00780c18307c0000ull, // 253 0xfd
        0x00003c3c3c3c0000ull, // 254 0xfe
        0x0000000000000000ull, // 255 0xff

    }; // bitmap_font[]

} // namespace bgi
