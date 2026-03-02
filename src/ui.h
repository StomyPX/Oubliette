#define UI_SIDE_PANEL_FRACTION (3.f / 8.f)
#define UI_SIDE_PANEL_HEADER 104.f
#define UI_SIDE_PANEL_FOOTER 128.f
#define UI_PORTRAIT_FRACTION (1.f / 4.f)
#define UI_PADDING 10.f
#define UI_LOGLINE_COUNT 256
#define UI_LOGLINE_LENGTH 41

typedef struct
{
    char text[UI_LOGLINE_LENGTH];
    Color color;
} LogLine;

static void ui_text(Font font, const char* text, Vector2 position, float fontSize, int spacing, Color tint);
static void ui_border(Texture border, Rectangle rec, Color color);

/* Default color: ZINNWALDITEBROWN */
static void ui_log(Color color, char* fmt, ...);
static void ui_characterHudCard(Character c, Rectangle r);
