#pragma once

#include <SDL.h>

#include <array>
#include <cstdint>
#include <cmath>
#include <string_view>
#include <vector>
#include <string>
#include <sstream>

namespace bgi
{
    // 0xAARRGGBB
    using Color = uint32_t;
    // 8x8 bits (1: fill fg color, 0: fill bg color)
    using FillPattern = uint64_t;

    namespace basic_colors
    {
        inline constexpr Color Black = 0xff000000;
        inline constexpr Color White = 0xffffffff;
    }
    namespace basic_fill_patterns
    {
        inline constexpr FillPattern SolidBg = 0;
    }

    struct Size
    {
        Size() {}
        Size(int w, int h) : w(w), h(h) {}
        int w = 0;
        int h = 0;
    };

    struct Point
    {
        Point() {}
        Point(int x, int y) : x(x), y(y) {}
        explicit Point(const SDL_Point &sdl) : Point(sdl.x, sdl.y) {}
        SDL_Point sdl() { return SDL_Point{x, y}; }

        int x = 0;
        int y = 0;
    };

    struct Rect
    {
        Rect() {}
        Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
        explicit Rect(const SDL_Rect &sdl) : Rect(sdl.x, sdl.y, sdl.w, sdl.h) {}
        SDL_Rect sdl() { return SDL_Rect{x, y, w, h}; }

        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
    };

    // TODO: maybe a fast allocator?
    using Polygon = std::vector<Point>;

    template <typename... Int>
    constexpr Polygon MakePolygon(Int... ints)
    {
        constexpr size_t num_ints = sizeof...(ints);
        static_assert(num_ints % 2 == 0,
                      "The number of parameters must be even (x0, y0, x1, y1, x2, y2, ...).");
        std::array<int, num_ints> int_array{ints...};
        Polygon polygon;
        polygon.reserve(num_ints / 2);
        for (size_t i = 0; i < num_ints / 2; i++)
        {
            polygon.emplace_back(int_array[i * 2], int_array[i * 2 + 1]);
        }
        return polygon;
    }

    struct TransformType
    {
        float cw_rot_deg = 0;
        float scale_x = 1;
        float scale_y = 1;
        int translate_x = 0;
        int translate_y = 0;
    };

    struct Padding
    {
        int left;
        int right;
        int top;
        int bottom;
    };

    struct Margin
    {
        int left;
        int right;
        int top;
        int bottom;
    };

    struct Surface
    {
        Surface()
        {
        }

        Surface(int w, int h)
            : pixels(w * h), w(w), h(h)
        {
        }

        explicit Surface(const Size &size)
            : Surface(size.w, size.h)
        {
        }

        std::vector<Color> pixels;
        int w = 0;
        int h = 0;
    };

    class Drawer final
    {
    public:
        explicit Drawer(Surface &surface);
        Drawer(Surface &surface, const Rect &viewport);
        ~Drawer();

        Drawer Viewport(int x, int y, int w, int h);

        Color GetPixel(int x, int y) const;
        void SetPixel(int x, int y, Color c);
        void Clear(Color c);
        void DrawRect(int x, int y, int w, int h);
        void FillRect(int x, int y, int w, int h);
        void FillRect(const Rect &rect);
        void DrawRoundedRect(int x, int y, int w, int h, int rx, int ry);
        void FillRoundedRect(int x, int y, int w, int h, int rx, int ry);
        // TODO:
        // PointPair GetEllipticalArcEndpoints(int x, int y, int rx, int ry, int angle1 = 0, int angle2 = 360);
        void DrawEllipse(int x, int y, int rx, int ry, int angle1 = 0, int angle2 = 360);
        void FillEllipse(int x, int y, int rx, int ry, int angle1 = 0, int angle2 = 360);
        void DrawLine(int x1, int y1, int x2, int y2);
        void DrawOpenPoly(const Polygon &polygon);
        void DrawPoly(const Polygon &polygon);
        void FillPoly(const Polygon &polygon);

        template <typename... Int>
        void FillPoly(Int... ints)
        {
            FillPoly(MakePolygon(ints...));
        }
        template <typename... Int>
        void DrawPoly(Int... ints)
        {
            DrawPoly(MakePolygon(ints...));
        }

        Rect GetTextRect(int x, int y, std::string_view text);
        void Write(int x, int y, std::string_view text);
        void WriteEx(int x, int y, std::string_view text, const Padding &padding, const Margin &margin, int rx, int ry);

        void SetDrawStyle(Color c);
        void SetFillStyle(Color c);
        void SetFillStyle(FillPattern pattern, Color bg, Color fg);
        void SetWriteStyle(Color c, int scale_x = 1, int scale_y = 1);

        int width() const { return viewport_.w; }
        int height() const { return viewport_.h; }
        Size size() const { return {viewport_.w, viewport_.h}; }

    private:
        // Draws an 8x8 bitmap character.
        void DrawBitmapChar(int x, int y, char c);
        Color *GetPixelPtr(int x, int y) const;
        void SetPixelWithFillPattern(int x, int y);

        static std::array<FillPattern, 256> bitmap_font_;

        Surface *surface_ = nullptr;
        Rect viewport_ = {};
        Rect clip_ = {};

        // Drawing state.
        Color draw_color_ = basic_colors::White;
        Color fill_bg_color_ = basic_colors::White;
        Color fill_fg_color_ = basic_colors::White;
        FillPattern fill_pattern_ = basic_fill_patterns::SolidBg;
        Color write_color_ = basic_colors::White;
        int write_scale_x_ = 1;
        int write_scale_y_ = 1;
    };

    // Helper functions:

    inline int Round(float f)
    {
        return static_cast<int>(std::round(f));
    }

    inline float Float(int i)
    {
        return static_cast<float>(i);
    }

    template <typename T>
    inline int Int(T t)
    {
        return static_cast<int>(t);
    }

    template <typename... Args>
    std::string ToString(const Args... args)
    {
        std::stringstream ss;
        (ss << ... << args);
        return ss.str();
    }

    // Color handling:

    inline Color Argb(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
    {
        return alpha << 24 | red << 16 | green << 8 | blue;
    }
    inline Color Rgb(uint8_t red, uint8_t green, uint8_t blue)
    {
        return Argb(0xff, red, green, blue);
    }
    inline uint8_t GetAlpha(Color color)
    {
        return color >> 24;
    }
    inline uint8_t GetRed(Color color)
    {
        return (color >> 16) & 0xff;
    }
    inline uint8_t GetGreen(Color color)
    {
        return (color >> 8) & 0xff;
    }
    inline uint8_t GetBlue(Color color)
    {
        return color & 0xff;
    }

    // FillPattern handling:
    inline constexpr FillPattern MakeFillPattern(uint8_t line0,
                                                 uint8_t line1,
                                                 uint8_t line2,
                                                 uint8_t line3,
                                                 uint8_t line4,
                                                 uint8_t line5,
                                                 uint8_t line6,
                                                 uint8_t line7)
    {
        FillPattern result = 0;
        for (uint8_t line : {line0, line1, line2, line3, line4, line5, line6, line7})
        {
            result <<= 8;
            result |= line;
        }
        return result;
    }

    inline constexpr bool IsFg(FillPattern pattern, int x, int y)
    {
        constexpr uint64_t a = static_cast<uint64_t>(1) << 63;
        return pattern & (a >> ((y & 7) << 3) >> (x & 7));
    }

    inline constexpr FillPattern RotateLeft(FillPattern pattern, int rotation)
    {
        rotation %= 8;
        FillPattern result = 0;
        for (int i = 0; i < 8; i++)
        {
            uint8_t line = (pattern >> (8 * (7 - i))) & 0xff;
            line = ((line << rotation) | (line >> (8 - rotation))) & 0xff;

            result <<= 8;
            result |= line;
        }
        return result;
    }

    inline constexpr FillPattern RotateRight(FillPattern pattern, int rotation)
    {
        rotation %= 8;
        FillPattern result = 0;
        for (int i = 0; i < 8; i++)
        {
            uint8_t line = (pattern >> (8 * (7 - i))) & 0xff;
            line = ((line >> rotation) | (line << (8 - rotation))) & 0xff;

            result <<= 8;
            result |= line;
        }
        return result;
    }

    // Polygon handling:

    Polygon Transform(const Polygon &polygon, float cw_rot_deg = 0, float scale_x = 1, float scale_y = 1, int translate_x = 0, int translate_y = 0);
    Polygon Transform(const Polygon &polygon, const TransformType &transform);
    Polygon MirrorHoriz(const Polygon &polygon, int mirror_x);
    Polygon MirrorHorizConcat(const Polygon &polygon, int mirror_x);
    Polygon MirrorVert(const Polygon &polygon, int mirror_y);
    Polygon MakeEllipticalArc(int x, int y, int rx, int ry, int angle1 = 0, int angle2 = 360);

    // Classes:

    class NonCopyable
    {
    public:
        NonCopyable() = default;
        virtual ~NonCopyable() = default;
        NonCopyable(const NonCopyable &) = delete;
        NonCopyable &operator=(const NonCopyable &) = delete;
    };

    struct KeyPress
    {
        bool should_quit = false;
        // Use this if the key label matters
        SDL_Keycode keycode = SDLK_UNKNOWN;
        // Use this if the key location matters
        // For example, you want they keys at the location of the WASD keys
        // on the US keyboard, and the label doesn't matter.
        SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
    };

    class App : private NonCopyable
    {
    public:
        // Please define your main function like this:
        // int main(int argc, char *argv[])
        App();
        explicit App(int random_seed);
        ~App() override;

        int Random(int exclusive_upper_limit);
        // TODO: maybe remove
        Color RandomRgbColor();
        Color RandomColor();

        // TODO: better
        KeyPress WaitKeyPress(bool auto_quit = true);
        bool PollKeyPress(KeyPress &key_press, bool auto_quit = true);
    };

    class Window : private NonCopyable
    {
    public:
        Window(std::string_view title, int w = 800, int h = 600);
        ~Window() override;

        // TODO: show only after update?
        void Update(const Surface &surface);

        int width() const { return size().w; }
        int height() const { return size().h; }
        Size size() const
        {
            Size size;
            SDL_RenderGetLogicalSize(renderer_, &size.w, &size.h);
            return size;
        }
        bool fullscreen() const
        {
            return SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN_DESKTOP;
        }
        void set_fullscreen(bool full_screen);

    private:
        // TODO: Try unique_ptr with custom deleter?
        SDL_Window *window_ = nullptr;
        SDL_Renderer *renderer_ = nullptr;
        SDL_Texture *texture_ = nullptr;
    };

    // Original BGI colors.
    namespace colors
    {
        // Dark palette:

        inline constexpr Color Black = 0xff000000;
        inline constexpr Color Blue = 0xff0000aa;
        inline constexpr Color Green = 0xff00aa00;
        inline constexpr Color Cyan = 0xff00aaaa;
        inline constexpr Color Red = 0xffaa0000;
        inline constexpr Color Magenta = 0xffaa00aa;
        inline constexpr Color Brown = 0xffaa5500;
        inline constexpr Color LightGray = 0xffaaaaaa;

        // Light palette:

        inline constexpr Color DarkGray = 0xff555555;
        inline constexpr Color LightBlue = 0xff5555ff;
        inline constexpr Color LightGreen = 0xff55ff55;
        inline constexpr Color LightCyan = 0xff55ffff;
        inline constexpr Color LightRed = 0xffff5555;
        inline constexpr Color LightMagenta = 0xffdb7093;
        inline constexpr Color Yellow = 0xffffff55;
        inline constexpr Color White = 0xffffffff;

        inline constexpr std::array<Color, 16> AllColors = {
            Black,
            Blue,
            Green,
            Cyan,
            Red,
            Magenta,
            Brown,
            LightGray,
            DarkGray,
            LightBlue,
            LightGreen,
            LightCyan,
            LightRed,
            LightMagenta,
            Yellow,
            White,
        };
    } // namespace colors

    namespace fill_patterns
    {
        // Fills the area in background color.
        inline constexpr FillPattern SolidBg = 0;
        // Fills the area in solid foreground color.
        inline constexpr FillPattern SolidFg = ~SolidBg;
        // --- fill
        inline constexpr FillPattern Line = MakeFillPattern(
            0b11111111,
            0b11111111,
            0b00000000,
            0b00000000,
            0b11111111,
            0b11111111,
            0b00000000,
            0b00000000);
        // /// fill
        inline constexpr FillPattern LightSlash = MakeFillPattern(
            0b00000001,
            0b00000010,
            0b00000100,
            0b00001000,
            0b00010000,
            0b00100000,
            0b01000000,
            0b10000000);
        // /// fill with thick lines
        inline constexpr FillPattern Slash = MakeFillPattern(
            0b11100000,
            0b11000001,
            0b10000011,
            0b00000111,
            0b00001110,
            0b00011100,
            0b00111000,
            0b01110000);
        // \\\ fill with thick lines
        inline constexpr FillPattern Backslash = MakeFillPattern(
            0b11110000,
            0b01111000,
            0b00111100,
            0b00011110,
            0b00001111,
            0b10000111,
            0b11000011,
            0b11100001);
        // \\\ fill
        inline constexpr FillPattern LightBackslash = MakeFillPattern(
            0b11010010,
            0b01101001,
            0b10110100,
            0b01011010,
            0b00101101,
            0b10010110,
            0b01001011,
            0b10100101);
        // hatch fill
        inline constexpr FillPattern Hatch = MakeFillPattern(
            0b11111111,
            0b10001000,
            0b10001000,
            0b10001000,
            0b11111111,
            0b10001000,
            0b10001000,
            0b10001000);
        // cross hatch fill
        inline constexpr FillPattern CrossHatch = MakeFillPattern(
            0b10000001,
            0b10000001,
            0b01000010,
            0b00100100,
            0b00011000,
            0b00011000,
            0b00100100,
            0b01000010);
        // interleaving line fill (brick wall pattern)
        inline constexpr FillPattern Interleave = MakeFillPattern(
            0b11001100,
            0b00110011,
            0b11001100,
            0b00110011,
            0b11001100,
            0b00110011,
            0b11001100,
            0b00110011);
        // widely spaced dot fill
        inline constexpr FillPattern WideDot = MakeFillPattern(
            0b00000001,
            0b00000000,
            0b00010000,
            0b00000000,
            0b00000001,
            0b00000000,
            0b00010000,
            0b00000000);
        // closely spaced dot fill
        inline constexpr FillPattern CloseDot = MakeFillPattern(
            0b10001000,
            0b00000000,
            0b00100010,
            0b00000000,
            0b10001000,
            0b00000000,
            0b00100010,
            0b00000000);

        // Extras:
        // || fill
        inline constexpr FillPattern VerticalLine = MakeFillPattern(
            0b11001100,
            0b11001100,
            0b11001100,
            0b11001100,
            0b11001100,
            0b11001100,
            0b11001100,
            0b11001100);
    } // namespace fill_patterns

} // namespace bgi
