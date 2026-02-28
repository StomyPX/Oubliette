#define UI_SIDE_PANEL_FRACTION (3.f / 8.f)
#define UI_SIDE_PANEL_HEADER 104.f
#define UI_SIDE_PANEL_FOOTER 128.f
#define UI_PORTRAIT_FRACTION (1.f / 4.f)
#define UI_PADDING 10.f

static void ui_text(Font font, const char* text, Vector2 position, Color tint, int spacing);
static void ui_border(Texture border, Rectangle rec, Color color);
