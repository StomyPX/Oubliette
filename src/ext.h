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

    // 2676
    bool            (*Begin)(const char* name, bool* p_open, ImGuiWindowFlags flags);
    void            (*End)(void);

    // 2741
    ImU32           (*GetColorU32_Vec4)(const ImVec4 col);
    void            (*SameLine)(float offset_from_start_x,float spacing);
    void            (*NewLine)(void);
    void            (*Text)(const char* fmt,...);

    // 2907
    bool            (*BeginTable)(const char* id, int column, ImGuiTableFlags flags, const ImVec2 outer_size, float inner_width);
    void            (*EndTable)(void);
    void            (*TableNextRow)(ImGuiTableRowFlags row_flags,float min_row_height);
    bool            (*TableNextColumn)(void);
    void            (*TableSetBgColor)(ImGuiTableBgTarget target,ImU32 color,int column_n);

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
