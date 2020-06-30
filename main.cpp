#include "vgfw.h"

#include <algorithm>

#include "vga9.h"

class GliNes : public Vgfw
{
public:
    static constexpr int DisplayWidth = 256;
    static constexpr int DisplayHeight = 240;

    static constexpr int InspectorWidth = 80 * 9;
    static constexpr int InspectorHeight = 50 * 16;

    static constexpr int WindowWidth = 16 + (DisplayWidth * 2) + 16 + InspectorWidth + 16;
    static constexpr int WindowHeight = 16 + std::max(DisplayHeight * 2, InspectorHeight) + 16;

    bool on_create() override
    {
        return true;
    }

    void on_destroy() override
    {

    }

    bool on_update(float delta) override
    {
        draw_string(16, 16, "Hello, World! Derek IS here...", (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 7, 0);
        return true;
    }
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    GliNes nes;

    if (!nes.initialize(L"GLI NES", GliNes::WindowWidth, GliNes::WindowHeight, 1))
    {
        return 1;
    }

    nes.run();

    return 0;
}