static void
ext_init(External* ext)
{
    ext->cimgui.handle = platform_dlopen("cimgui");
    if (ext->cimgui.handle) {
        /* API */
        ext->cimgui.CreateContext   = platform_dlsym(ext->cimgui.handle, "igCreateContext");
        ext->cimgui.DestroyContext  = platform_dlsym(ext->cimgui.handle, "igDestroyContext");

        ext->cimgui.GetIO           = platform_dlsym(ext->cimgui.handle, "igGetIO");
        ext->cimgui.NewFrame        = platform_dlsym(ext->cimgui.handle, "igNewFrame");
        ext->cimgui.EndFrame        = platform_dlsym(ext->cimgui.handle, "igEndFrame");
        ext->cimgui.Render          = platform_dlsym(ext->cimgui.handle, "igRender");
        ext->cimgui.GetDrawData     = platform_dlsym(ext->cimgui.handle, "igGetDrawData");

        ext->cimgui.ShowDemoWindow  = platform_dlsym(ext->cimgui.handle, "igShowDemoWindow");

        ext->cimgui.GetMouseCursor  = platform_dlsym(ext->cimgui.handle, "igGetMouseCursor");

        /* Classes */
        ext->cimgui.ImFontAtlas_AddFontDefault      = platform_dlsym(ext->cimgui.handle, "ImFontAtlas_AddFontDefault");
        ext->cimgui.ImFontAtlas_GetTexDataAsRGBA32  = platform_dlsym(ext->cimgui.handle, "ImFontAtlas_GetTexDataAsRGBA32");

        ext->cimgui.ImGuiIO_AddInputCharacter       = platform_dlsym(ext->cimgui.handle, "ImGuiIO_AddInputCharacter");
        ext->cimgui.ImGuiIO_AddKeyEvent             = platform_dlsym(ext->cimgui.handle, "ImGuiIO_AddKeyEvent");

        /* Initialization */
        ext->cimgui.context = ext->cimgui.CreateContext(0);
        if (ext->cimgui.context) {
            ImGuiIO* io = ext->cimgui.GetIO();

            io->BackendPlatformName = "custom_impl_raylib_cimgui";
            io->BackendRendererName = "custom_impl_raylib_cimgui";
            io->BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
            io->SetClipboardTextFn = ext_CImguiSetClipboard;
            io->GetClipboardTextFn = ext_CImguiGetClipboard;
            io->ClipboardUserData = 0;

            /* Cursor mapping */

            ext->cimgui.cursorMap[ImGuiMouseCursor_Arrow]       = MOUSE_CURSOR_ARROW;
            ext->cimgui.cursorMap[ImGuiMouseCursor_TextInput]   = MOUSE_CURSOR_IBEAM;
            ext->cimgui.cursorMap[ImGuiMouseCursor_Hand]        = MOUSE_CURSOR_POINTING_HAND;
            ext->cimgui.cursorMap[ImGuiMouseCursor_ResizeAll]   = MOUSE_CURSOR_RESIZE_ALL;
            ext->cimgui.cursorMap[ImGuiMouseCursor_ResizeEW]    = MOUSE_CURSOR_RESIZE_EW;
            ext->cimgui.cursorMap[ImGuiMouseCursor_ResizeNESW]  = MOUSE_CURSOR_RESIZE_NESW;
            ext->cimgui.cursorMap[ImGuiMouseCursor_ResizeNS]    = MOUSE_CURSOR_RESIZE_NS;
            ext->cimgui.cursorMap[ImGuiMouseCursor_ResizeNWSE]  = MOUSE_CURSOR_RESIZE_NWSE;
            ext->cimgui.cursorMap[ImGuiMouseCursor_NotAllowed]  = MOUSE_CURSOR_NOT_ALLOWED;

            /* Key mapping */
            ext->cimgui.rayKey[  0] = KEY_APOSTROPHE;
            ext->cimgui.rayKey[  1] = KEY_COMMA;
            ext->cimgui.rayKey[  2] = KEY_MINUS;
            ext->cimgui.rayKey[  3] = KEY_PERIOD;
            ext->cimgui.rayKey[  4] = KEY_SLASH;
            ext->cimgui.rayKey[  5] = KEY_ZERO;
            ext->cimgui.rayKey[  6] = KEY_ONE;
            ext->cimgui.rayKey[  7] = KEY_TWO;
            ext->cimgui.rayKey[  8] = KEY_THREE;
            ext->cimgui.rayKey[  9] = KEY_FOUR;
            ext->cimgui.rayKey[ 10] = KEY_FIVE;
            ext->cimgui.rayKey[ 11] = KEY_SIX;
            ext->cimgui.rayKey[ 12] = KEY_SEVEN;
            ext->cimgui.rayKey[ 13] = KEY_EIGHT;
            ext->cimgui.rayKey[ 14] = KEY_NINE;
            ext->cimgui.rayKey[ 15] = KEY_SEMICOLON;
            ext->cimgui.rayKey[ 16] = KEY_EQUAL;
            ext->cimgui.rayKey[ 17] = KEY_A;
            ext->cimgui.rayKey[ 18] = KEY_B;
            ext->cimgui.rayKey[ 19] = KEY_C;
            ext->cimgui.rayKey[ 20] = KEY_D;
            ext->cimgui.rayKey[ 21] = KEY_E;
            ext->cimgui.rayKey[ 22] = KEY_F;
            ext->cimgui.rayKey[ 23] = KEY_G;
            ext->cimgui.rayKey[ 24] = KEY_H;
            ext->cimgui.rayKey[ 25] = KEY_I;
            ext->cimgui.rayKey[ 26] = KEY_J;
            ext->cimgui.rayKey[ 27] = KEY_K;
            ext->cimgui.rayKey[ 28] = KEY_L;
            ext->cimgui.rayKey[ 29] = KEY_M;
            ext->cimgui.rayKey[ 30] = KEY_N;
            ext->cimgui.rayKey[ 31] = KEY_O;
            ext->cimgui.rayKey[ 32] = KEY_P;
            ext->cimgui.rayKey[ 33] = KEY_Q;
            ext->cimgui.rayKey[ 34] = KEY_R;
            ext->cimgui.rayKey[ 35] = KEY_S;
            ext->cimgui.rayKey[ 36] = KEY_T;
            ext->cimgui.rayKey[ 37] = KEY_U;
            ext->cimgui.rayKey[ 38] = KEY_V;
            ext->cimgui.rayKey[ 39] = KEY_W;
            ext->cimgui.rayKey[ 40] = KEY_X;
            ext->cimgui.rayKey[ 41] = KEY_Y;
            ext->cimgui.rayKey[ 42] = KEY_Z;
            ext->cimgui.rayKey[ 43] = KEY_LEFT_BRACKET;
            ext->cimgui.rayKey[ 44] = KEY_BACKSLASH;
            ext->cimgui.rayKey[ 45] = KEY_RIGHT_BRACKET;
            ext->cimgui.rayKey[ 46] = KEY_GRAVE;
            ext->cimgui.rayKey[ 47] = KEY_SPACE;
            ext->cimgui.rayKey[ 48] = KEY_ESCAPE;
            ext->cimgui.rayKey[ 49] = KEY_ENTER;
            ext->cimgui.rayKey[ 50] = KEY_TAB;
            ext->cimgui.rayKey[ 51] = KEY_BACKSPACE;
            ext->cimgui.rayKey[ 52] = KEY_INSERT;
            ext->cimgui.rayKey[ 53] = KEY_DELETE;
            ext->cimgui.rayKey[ 54] = KEY_RIGHT;
            ext->cimgui.rayKey[ 55] = KEY_LEFT;
            ext->cimgui.rayKey[ 56] = KEY_DOWN;
            ext->cimgui.rayKey[ 57] = KEY_UP;
            ext->cimgui.rayKey[ 58] = KEY_PAGE_UP;
            ext->cimgui.rayKey[ 59] = KEY_PAGE_DOWN;
            ext->cimgui.rayKey[ 60] = KEY_HOME;
            ext->cimgui.rayKey[ 61] = KEY_END;
            ext->cimgui.rayKey[ 62] = KEY_CAPS_LOCK;
            ext->cimgui.rayKey[ 63] = KEY_SCROLL_LOCK;
            ext->cimgui.rayKey[ 64] = KEY_NUM_LOCK;
            ext->cimgui.rayKey[ 65] = KEY_PRINT_SCREEN;
            ext->cimgui.rayKey[ 66] = KEY_PAUSE;
            ext->cimgui.rayKey[ 67] = KEY_F1;
            ext->cimgui.rayKey[ 68] = KEY_F2;
            ext->cimgui.rayKey[ 69] = KEY_F3;
            ext->cimgui.rayKey[ 70] = KEY_F4;
            ext->cimgui.rayKey[ 71] = KEY_F5;
            ext->cimgui.rayKey[ 72] = KEY_F6;
            ext->cimgui.rayKey[ 73] = KEY_F7;
            ext->cimgui.rayKey[ 74] = KEY_F8;
            ext->cimgui.rayKey[ 75] = KEY_F9;
            ext->cimgui.rayKey[ 76] = KEY_F10;
            ext->cimgui.rayKey[ 77] = KEY_F11;
            ext->cimgui.rayKey[ 78] = KEY_F12;
            ext->cimgui.rayKey[ 79] = KEY_LEFT_SHIFT;
            ext->cimgui.rayKey[ 80] = KEY_LEFT_CONTROL;
            ext->cimgui.rayKey[ 81] = KEY_LEFT_ALT;
            ext->cimgui.rayKey[ 82] = KEY_LEFT_SUPER;
            ext->cimgui.rayKey[ 83] = KEY_RIGHT_SHIFT;
            ext->cimgui.rayKey[ 84] = KEY_RIGHT_CONTROL;
            ext->cimgui.rayKey[ 85] = KEY_RIGHT_ALT;
            ext->cimgui.rayKey[ 86] = KEY_RIGHT_SUPER;
            ext->cimgui.rayKey[ 87] = KEY_KB_MENU;
            ext->cimgui.rayKey[ 88] = KEY_KP_0;
            ext->cimgui.rayKey[ 89] = KEY_KP_1;
            ext->cimgui.rayKey[ 90] = KEY_KP_2;
            ext->cimgui.rayKey[ 91] = KEY_KP_3;
            ext->cimgui.rayKey[ 92] = KEY_KP_4;
            ext->cimgui.rayKey[ 93] = KEY_KP_5;
            ext->cimgui.rayKey[ 94] = KEY_KP_6;
            ext->cimgui.rayKey[ 95] = KEY_KP_7;
            ext->cimgui.rayKey[ 96] = KEY_KP_8;
            ext->cimgui.rayKey[ 97] = KEY_KP_9;
            ext->cimgui.rayKey[ 98] = KEY_KP_DECIMAL;
            ext->cimgui.rayKey[ 99] = KEY_KP_DIVIDE;
            ext->cimgui.rayKey[100] = KEY_KP_MULTIPLY;
            ext->cimgui.rayKey[101] = KEY_KP_SUBTRACT;
            ext->cimgui.rayKey[102] = KEY_KP_ADD;
            ext->cimgui.rayKey[103] = KEY_KP_ENTER;
            ext->cimgui.rayKey[104] = KEY_KP_EQUAL;

            ext->cimgui.imKey[  0] = ImGuiKey_Apostrophe;
            ext->cimgui.imKey[  1] = ImGuiKey_Comma;
            ext->cimgui.imKey[  2] = ImGuiKey_Minus;
            ext->cimgui.imKey[  3] = ImGuiKey_Period;
            ext->cimgui.imKey[  4] = ImGuiKey_Slash;
            ext->cimgui.imKey[  5] = ImGuiKey_0;
            ext->cimgui.imKey[  6] = ImGuiKey_1;
            ext->cimgui.imKey[  7] = ImGuiKey_2;
            ext->cimgui.imKey[  8] = ImGuiKey_3;
            ext->cimgui.imKey[  9] = ImGuiKey_4;
            ext->cimgui.imKey[ 10] = ImGuiKey_5;
            ext->cimgui.imKey[ 11] = ImGuiKey_6;
            ext->cimgui.imKey[ 12] = ImGuiKey_7;
            ext->cimgui.imKey[ 13] = ImGuiKey_8;
            ext->cimgui.imKey[ 14] = ImGuiKey_9;
            ext->cimgui.imKey[ 15] = ImGuiKey_Semicolon;
            ext->cimgui.imKey[ 16] = ImGuiKey_Equal;
            ext->cimgui.imKey[ 17] = ImGuiKey_A;
            ext->cimgui.imKey[ 18] = ImGuiKey_B;
            ext->cimgui.imKey[ 19] = ImGuiKey_C;
            ext->cimgui.imKey[ 20] = ImGuiKey_D;
            ext->cimgui.imKey[ 21] = ImGuiKey_E;
            ext->cimgui.imKey[ 22] = ImGuiKey_F;
            ext->cimgui.imKey[ 23] = ImGuiKey_G;
            ext->cimgui.imKey[ 24] = ImGuiKey_H;
            ext->cimgui.imKey[ 25] = ImGuiKey_I;
            ext->cimgui.imKey[ 26] = ImGuiKey_J;
            ext->cimgui.imKey[ 27] = ImGuiKey_K;
            ext->cimgui.imKey[ 28] = ImGuiKey_L;
            ext->cimgui.imKey[ 29] = ImGuiKey_M;
            ext->cimgui.imKey[ 30] = ImGuiKey_N;
            ext->cimgui.imKey[ 31] = ImGuiKey_O;
            ext->cimgui.imKey[ 32] = ImGuiKey_P;
            ext->cimgui.imKey[ 33] = ImGuiKey_Q;
            ext->cimgui.imKey[ 34] = ImGuiKey_R;
            ext->cimgui.imKey[ 35] = ImGuiKey_S;
            ext->cimgui.imKey[ 36] = ImGuiKey_T;
            ext->cimgui.imKey[ 37] = ImGuiKey_U;
            ext->cimgui.imKey[ 38] = ImGuiKey_V;
            ext->cimgui.imKey[ 39] = ImGuiKey_W;
            ext->cimgui.imKey[ 40] = ImGuiKey_X;
            ext->cimgui.imKey[ 41] = ImGuiKey_Y;
            ext->cimgui.imKey[ 42] = ImGuiKey_Z;
            ext->cimgui.imKey[ 43] = ImGuiKey_LeftBracket;
            ext->cimgui.imKey[ 44] = ImGuiKey_Backslash;
            ext->cimgui.imKey[ 45] = ImGuiKey_RightBracket;
            ext->cimgui.imKey[ 46] = ImGuiKey_GraveAccent;
            ext->cimgui.imKey[ 47] = ImGuiKey_Space;
            ext->cimgui.imKey[ 48] = ImGuiKey_Escape;
            ext->cimgui.imKey[ 49] = ImGuiKey_Enter;
            ext->cimgui.imKey[ 50] = ImGuiKey_Tab;
            ext->cimgui.imKey[ 51] = ImGuiKey_Backspace;
            ext->cimgui.imKey[ 52] = ImGuiKey_Insert;
            ext->cimgui.imKey[ 53] = ImGuiKey_Delete;
            ext->cimgui.imKey[ 54] = ImGuiKey_RightArrow;
            ext->cimgui.imKey[ 55] = ImGuiKey_LeftArrow;
            ext->cimgui.imKey[ 56] = ImGuiKey_DownArrow;
            ext->cimgui.imKey[ 57] = ImGuiKey_UpArrow;
            ext->cimgui.imKey[ 58] = ImGuiKey_PageUp;
            ext->cimgui.imKey[ 59] = ImGuiKey_PageDown;
            ext->cimgui.imKey[ 60] = ImGuiKey_Home;
            ext->cimgui.imKey[ 61] = ImGuiKey_End;
            ext->cimgui.imKey[ 62] = ImGuiKey_CapsLock;
            ext->cimgui.imKey[ 63] = ImGuiKey_ScrollLock;
            ext->cimgui.imKey[ 64] = ImGuiKey_NumLock;
            ext->cimgui.imKey[ 65] = ImGuiKey_PrintScreen;
            ext->cimgui.imKey[ 66] = ImGuiKey_Pause;
            ext->cimgui.imKey[ 67] = ImGuiKey_F1;
            ext->cimgui.imKey[ 68] = ImGuiKey_F2;
            ext->cimgui.imKey[ 69] = ImGuiKey_F3;
            ext->cimgui.imKey[ 70] = ImGuiKey_F4;
            ext->cimgui.imKey[ 71] = ImGuiKey_F5;
            ext->cimgui.imKey[ 72] = ImGuiKey_F6;
            ext->cimgui.imKey[ 73] = ImGuiKey_F7;
            ext->cimgui.imKey[ 74] = ImGuiKey_F8;
            ext->cimgui.imKey[ 75] = ImGuiKey_F9;
            ext->cimgui.imKey[ 76] = ImGuiKey_F10;
            ext->cimgui.imKey[ 77] = ImGuiKey_F11;
            ext->cimgui.imKey[ 78] = ImGuiKey_F12;
            ext->cimgui.imKey[ 79] = ImGuiKey_LeftShift;
            ext->cimgui.imKey[ 80] = ImGuiKey_LeftCtrl;
            ext->cimgui.imKey[ 81] = ImGuiKey_LeftAlt;
            ext->cimgui.imKey[ 82] = ImGuiKey_LeftSuper;
            ext->cimgui.imKey[ 83] = ImGuiKey_RightShift;
            ext->cimgui.imKey[ 84] = ImGuiKey_RightCtrl;
            ext->cimgui.imKey[ 85] = ImGuiKey_RightAlt;
            ext->cimgui.imKey[ 86] = ImGuiKey_RightSuper;
            ext->cimgui.imKey[ 87] = ImGuiKey_Menu;
            ext->cimgui.imKey[ 88] = ImGuiKey_Keypad0;
            ext->cimgui.imKey[ 89] = ImGuiKey_Keypad1;
            ext->cimgui.imKey[ 90] = ImGuiKey_Keypad2;
            ext->cimgui.imKey[ 91] = ImGuiKey_Keypad3;
            ext->cimgui.imKey[ 92] = ImGuiKey_Keypad4;
            ext->cimgui.imKey[ 93] = ImGuiKey_Keypad5;
            ext->cimgui.imKey[ 94] = ImGuiKey_Keypad6;
            ext->cimgui.imKey[ 95] = ImGuiKey_Keypad7;
            ext->cimgui.imKey[ 96] = ImGuiKey_Keypad8;
            ext->cimgui.imKey[ 97] = ImGuiKey_Keypad9;
            ext->cimgui.imKey[ 98] = ImGuiKey_KeypadDecimal;
            ext->cimgui.imKey[ 99] = ImGuiKey_KeypadDivide;
            ext->cimgui.imKey[100] = ImGuiKey_KeypadMultiply;
            ext->cimgui.imKey[101] = ImGuiKey_KeypadSubtract;
            ext->cimgui.imKey[102] = ImGuiKey_KeypadAdd;
            ext->cimgui.imKey[103] = ImGuiKey_KeypadEnter;
            ext->cimgui.imKey[104] = ImGuiKey_KeypadEqual;

            /* Fonts */
            unsigned char* pixels = 0;
            int width, height;
            ext->cimgui.ImFontAtlas_AddFontDefault(io->Fonts, 0);
            ext->cimgui.ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &pixels, &width, &height, 0);
            Image image = GenImageColor(width, height, BLANK);
            memcpy(image.data, pixels, width * height * 4);
            ext->cimgui.fontTexture = LoadTextureFromImage(image);
            UnloadImage(image);
            io->Fonts->TexID = &ext->cimgui.fontTexture;

            TraceLog(LOG_INFO, "CIMGUI successfully loaded. Full editor functionality available");
        } else {
            TraceLog(LOG_WARNING, "CIMGUI couldn't load successfull. Editor will have limited functionality");
        }
    }
}

static void
ext_deinit(External* ext)
{
    if (ext->cimgui.handle) {
        UnloadTexture(ext->cimgui.fontTexture);
        ext->cimgui.DestroyContext(ext->cimgui.context);
        platform_dlclose(ext->cimgui.handle);
    }
    /* TODO Clear ext data to zero if successful */
}

static void
ext_CImguiNewFrame(Memory* m)
{
    ImGuiIO* io = m->ext.cimgui.GetIO();

    if (IsWindowFullscreen()) {
        /* TODO Double check this after implementing Borderless Fullscreen */
        int monitor = GetCurrentMonitor();
        io->DisplaySize.x = GetMonitorWidth(monitor);
        io->DisplaySize.y = GetMonitorHeight(monitor);
    } else {
        io->DisplaySize.x = GetScreenWidth();
        io->DisplaySize.y = GetScreenHeight();
    }

    io->DisplayFramebufferScale = (ImVec2){1.f, 1.f};
    io->DeltaTime = m->deltaTime;

    /* Mouse */

    if (io->WantSetMousePos) {
        SetMousePosition(io->MousePos.x, io->MousePos.y);
    } else {
        io->MousePos.x = GetMouseX();
        io->MousePos.y = GetMouseY();
    }

    io->MouseDown[0] = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    io->MouseDown[1] = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
    io->MouseDown[2] = IsMouseButtonDown(MOUSE_MIDDLE_BUTTON);
    io->MouseWheel += GetMouseWheelMove();

    /* Mouse Cursor Changes */
    if (!(io->ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)) {
        ImGuiMouseCursor cursor = m->ext.cimgui.GetMouseCursor();
        if (cursor != m->ext.cimgui.cursor || io->MouseDrawCursor) {
            m->ext.cimgui.cursor = cursor;
            if (io->MouseDrawCursor || cursor == ImGuiMouseCursor_None) {
                HideCursor();
            } else {
                ShowCursor();
                if (!(io->ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)) {
                    SetMouseCursor(cursor > -1 && cursor < ImGuiMouseCursor_COUNT
                                   ? m->ext.cimgui.cursorMap[cursor]
                                   : MOUSE_CURSOR_DEFAULT);
                }
            }
        }
    }

    /* Keyboard */

    io->KeyCtrl     = IsKeyDown(KEY_RIGHT_CONTROL)  || IsKeyDown(KEY_LEFT_CONTROL);
    io->KeyShift    = IsKeyDown(KEY_RIGHT_SHIFT)    || IsKeyDown(KEY_LEFT_SHIFT);
    io->KeyAlt      = IsKeyDown(KEY_RIGHT_ALT)      || IsKeyDown(KEY_LEFT_ALT);
    io->KeySuper    = IsKeyDown(KEY_RIGHT_SUPER)    || IsKeyDown(KEY_LEFT_SUPER);

    /* Keymap Events */
    int keyid;
    while (keyid = GetKeyPressed()) {
        for (unsigned i = 0; i < CIMGUI_KEYMAP_COUNT; i++) {
            if (m->ext.cimgui.rayKey[i] == keyid) {
                m->ext.cimgui.ImGuiIO_AddKeyEvent(io, m->ext.cimgui.imKey[i], 1);
                m->ext.cimgui.keyDown[i] = 1;
                break;
            }
        }
    }

    for (unsigned i = 0; i < CIMGUI_KEYMAP_COUNT; i++) {
        if (m->ext.cimgui.keyDown[i]) {
            if (!IsKeyDown(m->ext.cimgui.rayKey[i])) {
                m->ext.cimgui.ImGuiIO_AddKeyEvent(io, m->ext.cimgui.imKey[i], 0);
                m->ext.cimgui.keyDown[i] = 0;
            }
        }
    }

    unsigned pressed;
    while (pressed = GetCharPressed())
        m->ext.cimgui.ImGuiIO_AddInputCharacter(io, pressed);

    m->ext.cimgui.NewFrame();
}

static void ext_CImguiRender_triangles(unsigned count, int start, ImVector_ImDrawIdx* idxBuffer,
                                       ImVector_ImDrawVert* vtxBuffer, Texture* texture);
static void ext_CImguiRender_vertex(ImDrawVert vertex);

static void
ext_CImguiRender(CImgui* cimgui)
{
    cimgui->Render();
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    ImGuiIO* io = cimgui->GetIO();
    ImDrawData* dd = cimgui->GetDrawData();
    for (int i = 0; i < dd->CmdListsCount; i++) {
        ImDrawList* cmdlist = dd->CmdLists[i];
        for (int j = 0; j < cmdlist->CmdBuffer.Size; j++) {
            ImDrawCmd* cmd = cmdlist->CmdBuffer.Data + j;
            Rectangle scissor = (Rectangle){
                cmd->ClipRect.x - dd->DisplayPos.x,
                cmd->ClipRect.y - dd->DisplayPos.y,
                cmd->ClipRect.z - (cmd->ClipRect.x - dd->DisplayPos.x),
                cmd->ClipRect.w - (cmd->ClipRect.y - dd->DisplayPos.y)
            };
            rlEnableScissorTest();
            rlScissor(scissor.x * io->DisplayFramebufferScale.x,
                      (GetScreenHeight() - (int)(scissor.y + scissor.height)) * io->DisplayFramebufferScale.y,
                      scissor.width * io->DisplayFramebufferScale.x,
                      scissor.height * io->DisplayFramebufferScale.y);
            if (cmd->UserCallback) {
                cmd->UserCallback(cmdlist, cmd);
            }

            ext_CImguiRender_triangles(cmd->ElemCount, cmd->IdxOffset, &cmdlist->IdxBuffer, &cmdlist->VtxBuffer, cmd->TextureId);
            rlDrawRenderBatchActive();
        }
    }

    rlSetTexture(0);
    rlDisableScissorTest();
    rlEnableBackfaceCulling();
}

static void
ext_CImguiRender_triangles(unsigned count, int start, ImVector_ImDrawIdx* idxBuffer,
                           ImVector_ImDrawVert* vtxBuffer, Texture* texture)
{
    unsigned texID = (texture) ? texture->id : 0;
    rlBegin(RL_TRIANGLES);
    rlSetTexture(texID);
    for (unsigned i = 0; i <= (count - 3); i += 3) {
        if (rlCheckRenderBatchLimit(3)) {
            rlBegin(RL_TRIANGLES);
            rlSetTexture(texID);
        }

        ImDrawIdx i0 = idxBuffer->Data[start + i + 0];
        ImDrawIdx i1 = idxBuffer->Data[start + i + 1];
        ImDrawIdx i2 = idxBuffer->Data[start + i + 2];

        ImDrawVert v0 = vtxBuffer->Data[i0];
        ImDrawVert v1 = vtxBuffer->Data[i1];
        ImDrawVert v2 = vtxBuffer->Data[i2];

        ext_CImguiRender_vertex(v0);
        ext_CImguiRender_vertex(v1);
        ext_CImguiRender_vertex(v2);
    }
    rlEnd();
}

static void
ext_CImguiRender_vertex(ImDrawVert vertex)
{
    Color* c = (Color*) &vertex.col;
    rlColor4ub(c->r, c->g, c->b, c->a);
    rlTexCoord2f(vertex.uv.x, vertex.uv.y);
    rlVertex2f(vertex.pos.x, vertex.pos.y);
}

static const char*
ext_CImguiGetClipboard(void* ctx)
{
    return GetClipboardText();
}

static void
ext_CImguiSetClipboard(void* ctx, const char* text)
{
    SetClipboardText(text);
}
