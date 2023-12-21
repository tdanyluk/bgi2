#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "bgi2.h"

using namespace bgi;
using namespace bgi::colors;
using namespace bgi::fill_patterns;

namespace
{
    enum class DoorAnim
    {
        None,
        Open,
        Close,
    };

    enum class MoveAnim
    {
        None,
        Left,
        Right,
    };

    struct GrillState
    {
        int x_pos = 0;
        int door_open_pct = 0;
        int left_gas_knob_angle = 0;
        int right_gas_knob_angle = 0;
        float lid_heat_c = 20;
        bool open = false;
        DoorAnim door_anim = DoorAnim::None;
        MoveAnim move_anim = MoveAnim::None;
        bool paused = false;
    };

    // TODO: create similar in BGI2.
    std::string GetKeyNameByScancode(SDL_Scancode scancode)
    {
        return SDL_GetKeyName(SDL_GetKeyFromScancode(scancode));
    }

    void DrawGrill(Drawer global_drawer, const GrillState &state)
    {
        static Color BgLightGray = 0xffbbbbbb;
        static Color GrillBlack = 0xff161616;
        static Polygon grill_matte_left_poly = MakePolygon(310, 100, 305, 110, 300, 185, 310, 200, 315, 200, 320, 195, 330, 195, 330, 185, 320, 185, 315, 175, 315, 110, 320, 100);
        static Polygon grill_matte_right_poly = MirrorHoriz(grill_matte_left_poly, 400);
        static Polygon clock_hand = MakePolygon(-2, 2, 2, 2, 0, -9);
        static TransformType grill_cupboard_tr = {0, 0.85, 0.85, 400, 200};
        static Polygon grill_cupboard_poly = Transform(MakePolygon(-100, 0, -100, 300, 100, 300, 100, 0), grill_cupboard_tr);
        static Polygon grill_tray_left_poly = Transform(MakePolygon(-223, 2, -101, 2, -101, 33, -233, 33, -233, 10), grill_cupboard_tr);
        static Polygon grill_tray_left_border = Transform(MakePolygon(-101, 33, -233, 33, -233, 10, -223, 2, -101, 2, -223, 2, -233, 10, -101, 10), grill_cupboard_tr);
        static Polygon grill_tray_right_poly = MirrorHoriz(grill_tray_left_poly, 400);
        static Polygon grill_tray_right_border = MirrorHoriz(grill_tray_left_border, 400);
        static Polygon grill_dashboard_poly = Transform(MakePolygon(-95, 2, -95, 62, 95, 62, 95, 2), grill_cupboard_tr);
        static Polygon grill_dashboard_top_dark_poly = Transform(MakePolygon(-95, 2, -95, 10, 95, 10, 95, 2), grill_cupboard_tr);
        static Polygon grill_dashboard_top_dark_poly2 = Transform(MakePolygon(-95, 13, -95, 13, 95, 13, 95, 13), grill_cupboard_tr);
        static Polygon grill_dashboard_bottom_dark_poly = Transform(MakePolygon(-95, 57, -95, 62, 95, 62, 95, 57), grill_cupboard_tr);
        static Polygon grill_dashboard_top_highlight_poly = Transform(MakePolygon(-95, 12, 95, 12), grill_cupboard_tr);
        static Polygon grill_dashboard_bottom_highlight_poly = Transform(MakePolygon(-95, 56, 95, 56), grill_cupboard_tr);
        static Polygon grill_dashboard_matte_left_poly = Transform(MakePolygon(-102, 0, -102, 62, -100, 64, -90, 64, -95, 0), grill_cupboard_tr);
        static Polygon grill_dashboard_matte_right_poly = MirrorHoriz(grill_dashboard_matte_left_poly, 400);
        static Polygon grill_knob_sign = MakePolygon(-2, -1, 2, -1, 0, -7);
        static constexpr FillPattern GrillPattern = MakeFillPattern(
            0b11001100,
            0b11111111,
            0b11001100,
            0b11001100,
            0b11001100,
            0b11001100,
            0b11001100,
            0b11001100);

        // global_drawer.SetFillStyle(CloseDot, BgLightGray, LightGray);
        // global_drawer.FillRect(0, 0, global_drawer.width(), global_drawer.height());
        global_drawer.Clear(BgLightGray);
        Drawer d = global_drawer.Viewport(state.x_pos, 0, 0, 0);
        if (state.open)
        {
            // Grill
            d.SetFillStyle(GrillBlack);
            d.FillPoly(314, 200, 330, 160, 470, 160, 487, 200);
            d.SetFillStyle(GrillPattern, Black, DarkGray);
            d.FillPoly(320, 197, 334, 164, 466, 164, 480, 197);
            d.SetFillStyle(CrossHatch, Black, DarkGray);
            d.FillEllipse(367, 181, 30, 10);
            d.SetDrawStyle(DarkGray);
            d.DrawEllipse(367, 181, 29, 9);
            d.SetDrawStyle(GrillBlack);
            d.DrawEllipse(367, 181, 30, 10);

            // Handle
            d.SetFillStyle(Line, LightGray, DarkGray);
            d.FillPoly(331, 84, 469, 84, 469, 90, 331, 90);

            // Lid
            d.SetFillStyle(GrillBlack);
            d.FillRect(328, 96, 144, 66);
            d.FillRect(328, 82, 6, 15);
            d.FillRect(466, 82, 6, 15);
            {
                static Polygon p = MakePolygon(328, 82, 321, 98, 328, 161);
                static Polygon q = MirrorHoriz(p, 400);
                d.FillPoly(p);
                d.FillPoly(q);
            }
            {
                static Polygon p = MakePolygon(333, 82, 340, 82, 340, 90, 333, 90);
                static Polygon q = MirrorHoriz(p, 400);
                d.FillPoly(p);
                d.FillPoly(q);
            }
            d.SetFillStyle(WideDot, DarkGray, GrillBlack);
            {
                static Polygon p = MirrorHorizConcat(MakePolygon(334, 102,
                                                                 337, 112,
                                                                 337, 124,
                                                                 334, 156),
                                                     400);
                d.FillPoly(p);
            }
        }
        else
        {
            // Grill
            d.SetFillStyle(GrillBlack);
            d.FillPoly(310, 100, 305, 110, 300, 185, 310, 200, 490, 200, 500, 185, 495, 110, 490, 100, 480, 100, 480, 102, 320, 102, 320, 100);

            // Grill highlight
            d.SetFillStyle(CloseDot, DarkGray, GrillBlack);
            d.FillPoly(300 + 18, 100 + 7, 500 - 18, 100 + 7, 500 - 21, 100 + 3, 300 + 21, 100 + 3);
            d.SetFillStyle(CloseDot, GrillBlack, DarkGray);
            d.FillPoly(300 + 18, 100 + 9, 500 - 18, 100 + 9, 500 - 18, 100 + 15, 300 + 18, 100 + 15);

            // Branding
            d.SetFillStyle(LightGray);
            d.SetDrawStyle(Black);
            d.SetWriteStyle(Black, 1, 2);
            d.WriteEx(330, 163, "weeburn", Padding{3, 2, 0, -1}, Margin{1, 1, 1, 1}, 3, 3);

            // Matte part of grill
            d.SetFillStyle(CloseDot, DarkGray, GrillBlack);
            d.FillPoly(grill_matte_left_poly);
            d.FillPoly(grill_matte_right_poly);

            // Handle
            d.SetFillStyle(Line, LightGray, DarkGray);
            d.FillPoly(331, 187, 469, 187, 469, 193, 331, 193);

            // Lid thermometer
            d.SetFillStyle(LightGray);
            d.FillEllipse(400, 136, 12, 12);
            d.SetFillStyle(White);
            d.FillEllipse(400, 136, 9, 9);

            // Lid thermometer hand
            {
                float heat = state.lid_heat_c;
                if (heat < 40)
                {
                    heat = 40;
                }
                if (heat > 360)
                {
                    heat = 360;
                }
                float deg = 180 + heat * 360 / 400;

                d.SetFillStyle(GrillBlack);
                d.FillPoly(Transform(clock_hand, deg, 1, 1, 400, 136));
                d.SetPixel(400, 136, Yellow);
            }
        }

        // Wheels
        d.SetFillStyle(DarkGray);
        d.FillEllipse(324, 465, 10, 10);
        d.FillEllipse(476, 465, 10, 10);
        d.SetDrawStyle(GrillBlack);
        int wheel_angle = -state.x_pos * 360 / 100;
        d.DrawEllipse(324, 465, 10, 10, wheel_angle, wheel_angle + 90);
        d.DrawEllipse(324, 465, 9, 9, wheel_angle, wheel_angle + 90);
        d.DrawEllipse(476, 465, 10, 10, wheel_angle, wheel_angle + 90);
        d.DrawEllipse(476, 465, 9, 9, wheel_angle, wheel_angle + 90);
        d.SetFillStyle(Black);
        d.FillRoundedRect(305, 422, 190, 38, 10, 20);

        // Grill cupboard
        d.SetFillStyle(GrillBlack);
        d.FillPoly(grill_cupboard_poly);

        d.SetFillStyle(LightGray);
        d.FillPoly(grill_tray_left_poly);
        d.FillPoly(grill_tray_right_poly);

        d.SetDrawStyle(DarkGray);
        d.DrawOpenPoly(grill_tray_left_border);
        d.DrawOpenPoly(grill_tray_right_border);

        d.SetFillStyle(LightGray);
        d.FillPoly(grill_dashboard_poly);

        d.SetFillStyle(DarkGray);
        d.FillPoly(grill_dashboard_top_dark_poly);
        d.FillPoly(grill_dashboard_top_dark_poly2);
        d.FillPoly(grill_dashboard_bottom_dark_poly);

        d.SetFillStyle(CloseDot, DarkGray, GrillBlack);
        d.FillPoly(grill_dashboard_matte_left_poly);
        d.FillPoly(grill_dashboard_matte_right_poly);

        // Dashboard Branding
        d.SetWriteStyle(GrillBlack);
        d.Write(436, 215, "SPORT");

        // Knob strength markings
        d.SetDrawStyle(GrillBlack);
        d.SetFillStyle(GrillBlack);
        // left
        // min
        d.DrawEllipse(362 + 15, 236, 2, 2);
        d.FillEllipse(362 + 16, 236, 1, 1, 0, 90);
        // med
        d.DrawEllipse(362 + 10, 236 + 9, 2, 2);
        d.FillEllipse(362 + 11, 236 + 9, 1, 1, -90, 90);
        // max
        d.FillEllipse(362 - 15, 236, 2, 2);
        // right
        // min
        d.DrawEllipse(438 + 15, 236, 2, 2);
        d.FillEllipse(438 + 16, 236, 1, 1, 0, 90);
        // med
        d.DrawEllipse(438 + 10, 236 + 9, 2, 2);
        d.FillEllipse(438 + 11, 236 + 9, 1, 1, -90, 90);
        // max
        d.FillEllipse(438 - 15, 236, 2, 2);

        //  Knobs
        FillPattern left_knob_fill_pattern = RotateRight(VerticalLine, state.left_gas_knob_angle / 10);
        d.SetFillStyle(left_knob_fill_pattern, LightGray, DarkGray);
        d.FillEllipse(362, 236, 10, 10);
        FillPattern right_knob_fill_pattern = RotateRight(VerticalLine, state.right_gas_knob_angle / 10);
        d.SetFillStyle(right_knob_fill_pattern, LightGray, DarkGray);
        d.FillEllipse(438, 236, 10, 10);
        d.SetDrawStyle(DarkGray);
        d.DrawEllipse(362, 236, 10, 10);
        d.DrawEllipse(438, 236, 10, 10);
        d.SetFillStyle(LightGray);
        d.FillEllipse(362, 236 - 4, 10, 9);
        d.FillEllipse(438, 236 - 4, 10, 9);
        d.SetDrawStyle(DarkGray);
        d.DrawEllipse(362, 236 - 4, 10, 9);
        d.DrawEllipse(438, 236 - 4, 10, 9);
        d.DrawPoly(Transform(grill_knob_sign, -state.left_gas_knob_angle, 1, 1, 362, 236 - 4));
        d.DrawPoly(Transform(grill_knob_sign, -state.right_gas_knob_angle, 1, 1, 438, 236 - 4));

        // Inside
        d.SetFillStyle(Black);
        d.FillRect(333, 272, 134, 168);
        // Gas
        if (state.door_open_pct > 0)
        {
            d.SetFillStyle(Yellow);
            d.FillRect(444, 274, 7, 52);
            d.SetFillStyle(Yellow);
            d.FillRect(416, 322, 32, 7);
            d.FillEllipse(447, 325, 3, 3);
            d.SetFillStyle(LightCyan);
            d.FillRoundedRect(386, 317, 31, 23, 2, 2);
            d.FillRoundedRect(387 - 2, 400 - 2, 35, 29, 2, 2);
            d.SetFillStyle(Cyan);
            d.FillRoundedRect(377 - 5, 334, 60, 89, 10, 10);
        }
        // Door
        d.SetFillStyle(GrillBlack);
        int door_left_x = 335;
        int door_w = 10 + 120 * (100 - state.door_open_pct) / 100;
        int door_right_x = door_left_x + door_w - 1;
        d.FillRect(door_left_x, 274, door_w, 164);

        // Door handle
        d.SetFillStyle(DarkGray);
        int door_handle_x_tr = 15 + 11 * (100 - state.door_open_pct) / 100;
        d.FillRect(door_right_x - door_handle_x_tr, 316, 8, 65);
        d.SetFillStyle(LightGray);
        d.FillRect(door_right_x + 3 - door_handle_x_tr, 318, 5, 61);

        global_drawer.SetWriteStyle(Brown);
        int line = 1;
        // Thermometer number
        {
            std::string number;
            if (state.lid_heat_c < 70)
            {
                number = "LO";
            }
            else if (state.lid_heat_c > 330)
            {
                number = "HI";
            }
            else
            {
                number = std::to_string(Round(state.lid_heat_c)) + "C";
            }
            global_drawer.Write(10, 10 * line++, number);
        }

        // Usage
        line++;
        global_drawer.Write(10, 10 * line++, state.paused ? "Paused!" : "");
        global_drawer.Write(10, 10 * line++, ToString("Pause: ", GetKeyNameByScancode(SDL_SCANCODE_P)));
        global_drawer.Write(10, 10 * line++, ToString("Open/Close lid: ", GetKeyNameByScancode(SDL_SCANCODE_O)));
        global_drawer.Write(10, 10 * line++,
                            ToString("Rotate left gas knob: ", GetKeyNameByScancode(SDL_SCANCODE_LEFT), "/", GetKeyNameByScancode(SDL_SCANCODE_RIGHT)));
        global_drawer.Write(10, 10 * line++,
                            ToString("Rotate right gas knob: Shift+", GetKeyNameByScancode(SDL_SCANCODE_LEFT), "/", GetKeyNameByScancode(SDL_SCANCODE_RIGHT)));
        global_drawer.Write(10, 10 * line++, ToString("Roll left: ", GetKeyNameByScancode(SDL_SCANCODE_A)));
        global_drawer.Write(10, 10 * line++, ToString("Roll right: ", GetKeyNameByScancode(SDL_SCANCODE_D)));
        global_drawer.Write(10, 10 * line++, ToString("Roll left anim: ", GetKeyNameByScancode(SDL_SCANCODE_2)));
        global_drawer.Write(10, 10 * line++, ToString("Roll right anim: ", GetKeyNameByScancode(SDL_SCANCODE_4)));
        global_drawer.Write(10, 10 * line++, ToString("Open door: ", GetKeyNameByScancode(SDL_SCANCODE_Q)));
        global_drawer.Write(10, 10 * line++, ToString("Close door: ", GetKeyNameByScancode(SDL_SCANCODE_E)));
        global_drawer.Write(10, 10 * line++, ToString("Open door anim: ", GetKeyNameByScancode(SDL_SCANCODE_1)));
        global_drawer.Write(10, 10 * line++, ToString("Close door anim: ", GetKeyNameByScancode(SDL_SCANCODE_3)));
        global_drawer.Write(10, 10 * line++, ToString("Reset: ", GetKeyNameByScancode(SDL_SCANCODE_R)));
    }

    void HandleKeyPress(GrillState &state, int scancode, bool shift)
    {
        if (scancode == SDL_SCANCODE_P)
        {
            state.paused = !state.paused;
        }
        if (state.paused)
        {
            return;
        }

        if (scancode == SDL_SCANCODE_LEFT)
        {
            if (shift)
            {
                if (state.right_gas_knob_angle == 0)
                {
                    state.right_gas_knob_angle = 90;
                }
                else
                {
                    state.right_gas_knob_angle = std::min(270, state.right_gas_knob_angle + 10);
                }
            }
            else
            {
                if (state.left_gas_knob_angle == 0)
                {
                    state.left_gas_knob_angle = 90;
                }
                else
                {
                    state.left_gas_knob_angle = std::min(270, state.left_gas_knob_angle + 10);
                }
            }
            state.lid_heat_c += 5;
        }
        else if (scancode == SDL_SCANCODE_RIGHT)
        {
            if (shift)
            {
                if (state.right_gas_knob_angle == 90)
                {
                    state.right_gas_knob_angle = 0;
                }
                else
                {
                    state.right_gas_knob_angle = std::max(0, state.right_gas_knob_angle - 10);
                }
            }
            else
            {
                if (state.left_gas_knob_angle == 90)
                {
                    state.left_gas_knob_angle = 0;
                }
                else
                {
                    state.left_gas_knob_angle = std::max(0, state.left_gas_knob_angle - 10);
                }
            }
            state.lid_heat_c -= 5;
        }
        else if (scancode == SDL_SCANCODE_A)
        {
            if (state.x_pos > -100)
            {
                state.x_pos -= 1;
            }
        }
        else if (scancode == SDL_SCANCODE_D)
        {
            if (state.x_pos < 100)
            {
                state.x_pos = std::min(state.x_pos + 1, 100);
            }
        }
        else if (scancode == SDL_SCANCODE_R)
        {
            state = {};
            state.door_anim = DoorAnim::None;
        }
        else if (scancode == SDL_SCANCODE_Q)
        {
            if (state.door_open_pct < 100)
            {
                state.door_open_pct += 1;
            }
        }
        else if (scancode == SDL_SCANCODE_E)
        {
            if (state.door_open_pct > 0)
            {
                state.door_open_pct -= 1;
            }
        }
        else if (scancode == SDL_SCANCODE_1)
        {
            state.door_anim = DoorAnim::Open;
        }
        else if (scancode == SDL_SCANCODE_3)
        {
            state.door_anim = DoorAnim::Close;
        }
        else if (scancode == SDL_SCANCODE_2)
        {
            state.move_anim = MoveAnim::Left;
        }
        else if (scancode == SDL_SCANCODE_4)
        {
            state.move_anim = MoveAnim::Right;
        }
        else if (scancode == SDL_SCANCODE_O)
        {
            state.open = !state.open;
        }
    }

    void Animate(GrillState &state)
    {
        if (state.paused)
        {
            return;
        }

        if (state.door_anim == DoorAnim::Open)
        {
            if (state.door_open_pct < 100)
            {
                state.door_open_pct += 1;
            }
            else
            {
                state.door_anim = DoorAnim::None;
            }
        }
        else if (state.door_anim == DoorAnim::Close)
        {
            if (state.door_open_pct > 0)
            {
                state.door_open_pct -= 1;
            }
            else
            {
                state.door_anim = DoorAnim::None;
            }
        }
        if (state.move_anim == MoveAnim::Left)
        {
            if (state.x_pos > -100)
            {
                state.x_pos -= 1;
                if (state.x_pos == 0)
                {
                    state.move_anim = MoveAnim::None;
                }
            }
            else
            {
                state.move_anim = MoveAnim::None;
            }
        }
        else if (state.move_anim == MoveAnim::Right)
        {
            if (state.x_pos < 100)
            {
                state.x_pos += 1;
                if (state.x_pos == 0)
                {
                    state.move_anim = MoveAnim::None;
                }
            }
            else
            {
                state.move_anim = MoveAnim::None;
            }
        }
    }
} // namespace

int main(int argc, char *argv[])
{
    App app;
    Window main_win("Grill", 800, 600);
    Surface surface(main_win.size());
    Drawer d(surface);

    GrillState state;
    int last_mouse_x = 0;
    int last_mouse_y = 0;
    for (;;)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                return 0;

            if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.scancode == SDL_SCANCODE_F)
                    main_win.set_fullscreen(!main_win.fullscreen());
                else
                    HandleKeyPress(state, e.key.keysym.scancode, e.key.keysym.mod & KMOD_SHIFT);
            }

            else if (e.type == SDL_MOUSEMOTION)
            {
                last_mouse_x = Int(e.motion.x);
                last_mouse_y = Int(e.motion.y);
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                printf("Mouse pressed: %d %d\n", Int(e.button.x), Int(e.button.y));
            }
        }

        Animate(state);
        DrawGrill(d, state);

        d.SetWriteStyle(Brown);
        d.Write(10, 580, std::to_string(last_mouse_x) + " " + std::to_string(last_mouse_y));

        main_win.Update(surface);
        // Don't overheat the CPU.
        SDL_Delay(10);
    }

    return 0;
}
