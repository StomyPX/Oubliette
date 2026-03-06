#define UI_PADDING 10.f
#define UI_SIDE_PANEL_FRACTION (3.f / 8.f)
#define UI_SIDE_PANEL_HEADER 104.f
#define UI_SIDE_PANEL_FOOTER 48
#define UI_PORTRAIT_FRACTION (1.f / 4.f)
#define UI_LOGLINE_COUNT 256
#define UI_LOGLINE_LENGTH 80

typedef enum
{
    HudLayout_Widescreen = 0,
    HudLayout_Square,
    HudLayout_Ultrawide,
    //HudLayout_Tall, TODO 2x2 cards along the bottom
} HudLayout;

typedef struct
{
    char text[UI_LOGLINE_LENGTH];
    Color color;
} LogLine;

static void ui_text(Font font, const char* text, Vector2 position, float fontSize, int spacing, Color tint);
static void ui_border(Texture border, Rectangle rec, Color color);

/* Default color: ZINNWALDITEBROWN */
static void ui_log(Color color, char* fmt, ...);
static int ui_characterHudCard(Character* c, Rectangle r, int portraitSize, int keycode);
/* -1 on hover, 1 on LMB down, 2 on RMB down */
static int ui_button(Rectangle rect, char* text, int hotkey, bool enabled);
