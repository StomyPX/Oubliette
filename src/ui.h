#define UI_PADDING 10.f
#define UI_SIDE_PANEL_FRACTION (3.f / 8.f)
#define UI_SIDE_PANEL_HEADER (48 + UI_PADDING)
#define UI_PORTRAIT_FRACTION (1.f / 4.f)
#define UI_LOGLINE_COUNT 256
#define UI_LOGLINE_LENGTH 80

typedef enum
{
    GuiLayout_Widescreen = 0,
    GuiLayout_Square,
    GuiLayout_Ultrawide,
    //GuiLayout_Tall, TODO 2x2 cards along the bottom
} GuiLayout;

typedef enum
{
    GuiScreen_None = 0,
    GuiScreen_Options,
    GuiScreen_Intro,
    GuiScreen_NewGame,
    GuiScreen_Outro,
    GuiScreen_Credits,
} GuiScreen;

typedef enum
{
    GuiPopup_Actions,
} GuiPopup;

typedef struct
{
    char text[UI_LOGLINE_LENGTH];
    Color color;
} LogLine;

typedef struct
{
    unsigned image; /* Index into m->scene plus one, zero indicates no image */
    float prefade;
    float fadein;
    float hold;
    float fadeout;
    char text[620];
} StorySlide;

/* Elements */

static void ui_text(Font font, const char* text, Vector2 position, float fontSize, int spacing, Color tint);
static void ui_border(Texture border, Rectangle rec, Color color);

/* Default color: ZINNWALDITEBROWN */
static void ui_log(Color color, char* fmt, ...);
/* -1 on hover, 1 on LMB down */
static int ui_button(Rectangle rect, char* text, char* tooltip, int hotkey, bool enabled);
static Rectangle ui_area(int* out_aspect);

static int ui_characterHudCard(Character* c, Rectangle r, int portraitSize, int keycode);
static void ui_tooltipPane(void);

/* Screens */

static void ui_mainMenu(void);
static void ui_newGame(void);
static bool ui_slideshow(unsigned last); /* Returns true once completed */
static void ui_intro(void);
static void ui_outro(void);
static void ui_dungeon(void); /* Primary gameplay screen */
static void ui_options(void);
static void ui_credits(void);

