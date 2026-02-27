#define CIMGUI_KEYMAP_COUNT 105

typedef struct CImgui_ {
    DLHandle        handle;
    ImGuiContext*   context;

    /* API */
    ImGuiContext*   (*CreateContext)(ImFontAtlas* /* default: NULL */ shared_font_atlas);
    void            (*DestroyContext)(ImGuiContext* ctx);

    ImGuiIO*        (*GetIO)(void);
    void            (*NewFrame)(void);
    void            (*EndFrame)(void);
    void            (*Render)(void);
    ImDrawData*     (*GetDrawData)(void);

    void            (*ShowDemoWindow)(bool* p_open);

    ImGuiMouseCursor(*GetMouseCursor)(void);

    /* Classes */
    ImFont*         (*ImFontAtlas_AddFontDefault)(ImFontAtlas* self, ImFontConfig* /* default: NULL */ font_cfg);
    void            (*ImFontAtlas_GetTexDataAsRGBA32)(ImFontAtlas* self, unsigned char** out_pixels, int* out_width, int* out_height, int* /* Default: NULL */ out_bytes_per_pixel);

    void            (*ImGuiIO_AddInputCharacter)(ImGuiIO* self, unsigned c);
    void            (*ImGuiIO_AddKeyEvent)(ImGuiIO* self, ImGuiKey key, bool down);

    /* Internal Implementation */
    Texture             fontTexture;
    ImGuiMouseCursor    cursor;
    MouseCursor         cursorMap[ImGuiMouseCursor_COUNT];
    KeyboardKey         rayKey[CIMGUI_KEYMAP_COUNT];
    ImGuiKey            imKey[CIMGUI_KEYMAP_COUNT];
    char                keyDown[CIMGUI_KEYMAP_COUNT];
} CImgui;

typedef struct External_
{
    CImgui cimgui;
} External;

static void ext_init(External* ext);
static void ext_deinit(External* ext);

static void ext_CImguiNewFrame(Memory* m);
static void ext_CImguiRender(CImgui* cimgui);
static const char* ext_CImguiGetClipboard(void* ctx);
static void ext_CImguiSetClipboard(void* ctx, const char* text);
