static void
ui_mainMenu(void)
{
    BeginDrawing();
    {
        Vector2 position;
        Rectangle button;
        int result;

        DrawTextureRec(m->textures.marble, (Rectangle){0, 0, GetRenderWidth(), GetRenderHeight()},
                        (Vector2){0, 0}, EERIEBLACK);
        button.width = 256;
        button.height = 48;

        { /* Roland's descent */
            Rectangle source, dest;
            float proportion;

            source.x = 0;
            source.y = 0;
            source.width = m->textures.descent.width;
            source.height = m->textures.descent.height;

            dest.y = UI_PADDING * 2;
            dest.height = GetRenderHeight() - UI_PADDING * 4;
            proportion = dest.height / m->textures.descent.height;
            dest.width = m->textures.descent.width * proportion;
            dest.x = m->area.left + m->area.width / 2 - dest.width - UI_PADDING;

            DrawTexturePro(m->textures.descent, source, dest, (Vector2){0, 0}, 0.f, WHITE);
            ui_border(m->textures.border, dest, BONE);
        }

        { /* Title */
            Vector2 dims;
            Font* f;

            dims = MeasureTextEx(m->fonts.big, "Oubliette", m->fonts.big.baseSize, 0);
            position.x = m->area.left + m->area.width / 2 + 100;

            if (position.x + dims.x + UI_PADDING < GetRenderWidth()) {
                f = &m->fonts.big;
            } else {
                f = &m->fonts.title;
            }

            position.y = m->area.top + m->area.height / 2 - UI_PADDING * 2;
            position.y -= f->baseSize;
            if (position.y + button.height * 4 + UI_PADDING * 7 + m->fonts.text.baseSize + f->baseSize > m->area.bottom) {
                position.y = m->area.top + UI_PADDING * 2;
            }

            ui_text(*f, "Oubliette", position, f->baseSize, 0, BONE);
            position.y += f->baseSize + UI_PADDING * 2;
        }

        button.x = position.x;
        button.y = position.y;

        button.y += button.height + UI_PADDING;
        result = ui_button(button, "START GAME", "Start a new game", KEY_NULL, m->screen == GuiScreen_None);
        if (result > 0) {
            PlaySound(m->click);
            m->flags |= GlobalFlags_IgnoreInput;

            for (int i = 0; i < arrlen(m->party); i++) {
                m->party[i] = char_random();

                /* Reroll duplicates */
                for (int j = 0; j < i; j++) {
                    for (int k = 0; k < 10 && util_stricmp(m->party[j].name, m->party[i].name) == 0; k++) {
                        TraceLog(LOG_DEBUG, "CHARACTER: Duplicate random character %s, rerolling", m->party[j].name);
                        char_free(&m->party[i]);
                        m->party[i] = char_random();
                    }
                }
            }

            m->screen = GuiScreen_NewGame;
        }

        button.y += button.height + UI_PADDING;
        result = ui_button(button, "OPTIONS", "Adjust settings", KEY_NULL, m->screen == GuiScreen_None);
        if (result > 0) {
            m->screen = GuiScreen_Options;
            m->flags |= GlobalFlags_IgnoreInput;
            PlaySound(m->click);
        }

        button.y += button.height + UI_PADDING;
        result = ui_button(button, "CREDITS", "View game credits", KEY_NULL, m->screen == GuiScreen_None);
        if (result > 0) {
            m->screen = GuiScreen_Credits;
            m->flags |= GlobalFlags_IgnoreInput;
            PlaySound(m->click);
        }

        button.y += button.height + UI_PADDING;
        if (m->flags & GlobalFlags_ConfirmQuit) {
            result = ui_button(button, "CONFIRM?", "Are you sure you want to quit?", KEY_NULL, m->screen == GuiScreen_None);
            if (result > 0) {
                m->flags |= GlobalFlags_RequestQuit;
            } else if (result == 0) {
                m->flags &= ~(GlobalFlags_ConfirmQuit);
            }
        } else {
            result = ui_button(button, "QUIT", "Quit the game and return to desktop [Alt+F4]", KEY_NULL, m->screen == GuiScreen_None);
            if (result > 0) {
                m->flags |= GlobalFlags_ConfirmQuit;
                PlaySound(m->click2);
            }
        }

        if (m->screen == GuiScreen_Options) {
            ui_options();
        } else if (m->flags & GlobalFlags_ShowTooltips && m->tooltip[0]) {
            position.y = m->area.bottom - UI_PADDING * 3 - m->fonts.text.baseSize;
            ui_text(m->fonts.text, m->tooltip, position, m->fonts.text.baseSize, 0, BONE);
        }

        if (m->fadein < 0.5f) {
            Color color = ColorAlpha(BLACK, 1.f - m->fadein * 2.f);
            DrawRectangle(0, 0, GetRenderWidth(), GetRenderHeight(), color);
        }

        util_drawLog();
    }
    EndDrawing();
    m->fadein += m->deltaTime / 2.f;
    m->fadein = Clamp(m->fadein, 0.f, 1.f);
}

static void
ui_newGame(void)
{
    BeginDrawing();
    {
        Vector2 position, dims;
        Rectangle button, canvas, portrait;
        int availableHeight;
        int maxHeight;
        int maxWidth;
        char buffer[128];

        button.width = 256;
        button.height = 48;
        portrait.width = 145;
        portrait.height = 145;

        DrawTextureRec(m->textures.marble, (Rectangle){0, 0, GetRenderWidth(), GetRenderHeight()},
                        (Vector2){0, 0}, EERIEBLACK);

        canvas.x = m->area.left + UI_PADDING * 2;
        canvas.y = m->area.top + UI_PADDING * 2;
        canvas.width = m->area.width - UI_PADDING * 4;
        canvas.height = m->area.height - UI_PADDING * 6 - m->fonts.text.baseSize;

        maxHeight = (portrait.height + UI_PADDING) * arrlen(m->party)
                    + UI_PADDING * 2 + m->fonts.title.baseSize;
        if (canvas.height > maxHeight) {
            canvas.height = maxHeight;;
            canvas.y = m->area.top + m->area.height / 2 - canvas.height / 2;
        }

        maxWidth = portrait.width + button.width * 3 + UI_PADDING * 5;
        if (canvas.width > maxWidth) {
            canvas.width = maxWidth;
            canvas.x = m->area.left + m->area.width / 2 - canvas.width / 2;
        }

        DrawTexturePro(m->textures.vellum, canvas, canvas, (Vector2){0, 0}, 0.f, WHITE);
        BeginScissorMode(canvas.x, canvas.y, canvas.width, canvas.height);

        portrait.x = canvas.x + UI_PADDING;
        portrait.y = canvas.y + UI_PADDING;
        availableHeight = canvas.height - UI_PADDING * 2;

        if ((portrait.height + UI_PADDING) * arrlen(m->party) + m->fonts.title.baseSize <= availableHeight) {
            position.x = canvas.x + UI_PADDING;
            position.y = canvas.y + UI_PADDING;
            ui_text(m->fonts.title, "Party Details", Vector2Floor(position), m->fonts.title.baseSize, 0, BONE);
            portrait.y += m->fonts.title.baseSize + UI_PADDING;
        } else if ((portrait.height + UI_PADDING) * arrlen(m->party) > availableHeight) {
            portrait.height = (availableHeight - UI_PADDING * 4) / 4;
            portrait.width = portrait.height;
        }

        for (unsigned i = 0; i < arrlen(m->party); i++) {
            Character* ch = m->party + i;
            Rectangle p2 = portrait;
            Color border = BONE;
            Color inner = WHITE;

            m->elementCount++;
            if (util_mouseInRect(portrait)) {
                m->elementHover = m->elementCount;
                border = MINDAROGREEN;
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    inner = ColorLerp(WHITE, BLACK, 0.6f);
                }

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    PlaySound(m->click);
                    char_free(m->party + i);
                    m->party[i] = char_random();
                }
                snprintf(m->tooltip, sizeof(m->tooltip), "Click to generate a different character");
            }

            DrawTexturePro(ch->portrait, (Rectangle){0, 0, ch->portrait.width, ch->portrait.height},
                            p2, (Vector2){0, 0}, 0.f, inner);
            ui_border(m->textures.border, portrait, border);

            position.x = portrait.x + portrait.width + UI_PADDING;
            position.y = portrait.y;
            ui_text(m->fonts.heading, ch->name, Vector2Floor(position), m->fonts.heading.baseSize, 0 , MINDAROGREEN);

            position.y += m->fonts.heading.baseSize;
            snprintf(buffer, sizeof(buffer), "Class: %s\t Hit Points: %i\t Stamina Points: %i",
                    CharacterClass_toString(ch->class), char_maxHealth(*ch), char_maxStamina(*ch));
            DrawTextEx(m->fonts.text, buffer, Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);

            position.y += m->fonts.text.baseSize;
            snprintf(buffer, sizeof(buffer), "Strength: %i\t Dexterity: %i\t Constitution: %i",
                    ch->strength, ch->dexterity, ch->constitution);
            DrawTextEx(m->fonts.text, buffer, Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);

            position.y += m->fonts.text.baseSize;
            snprintf(buffer, sizeof(buffer), "Intellect: %i\t Willpower: %i\t Charisma: %i",
                    ch->intellect, ch->willpower, ch->charisma);
            DrawTextEx(m->fonts.text, buffer, Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);

            portrait.y += portrait.height + UI_PADDING;
        }

        dims = MeasureTextEx(m->fonts.title, "Difficulty Options", m->fonts.title.baseSize, 0);
        position.x = canvas.x + canvas.width - UI_PADDING - dims.x;
        position.y = canvas.y + UI_PADDING;
        ui_text(m->fonts.title, "Difficulty Options", Vector2Floor(position), m->fonts.title.baseSize, 0, BONE);

        button.x = canvas.x + canvas.width - UI_PADDING - button.width;
        button.y = position.y + m->fonts.title.baseSize + UI_PADDING;
        if (m->flags & GlobalFlags_HugeMap) {
            if (ui_button(button, "GIGANTIC", "(Hard) Giant map size that requires a longer play session. "
                "Be sure to grab some graph paper.", KEY_NULL, true) > 0)
            {
                m->flags &= ~(GlobalFlags_HugeMap);
                PlaySound(m->click);
            }
        } else {
            if (ui_button(button, "NORMAL", "(Easy) Smaller map more manageable for a short playthrough",
                KEY_NULL, true) > 0)
            {
                m->flags |= GlobalFlags_HugeMap;
                PlaySound(m->click);
            }
        }
        dims = MeasureTextEx(m->fonts.heading, "Map Size", m->fonts.heading.baseSize, 0);
        position.x = button.x - UI_PADDING * 3 - dims.x;
        position.y = button.y + button.height / 2 - dims.y / 2;
        DrawTextEx(m->fonts.heading, "Map Size", position, m->fonts.heading.baseSize, 0, ZINNWALDITEBROWN);

        button.y += button.height + UI_PADDING;
        if (m->flags & GlobalFlags_SlowXP) {
            if (ui_button(button, "SLOW", "(Hard) Characters require a lot of experience to level up",
                KEY_NULL, true) > 0)
            {
                m->flags &= ~(GlobalFlags_SlowXP);
                PlaySound(m->click);
            }
        } else {
            if (ui_button(button, "FAST", "(Easy) Characters will advance in level relatively quickly",
                KEY_NULL, true) > 0)
            {
                m->flags |= GlobalFlags_SlowXP;
                PlaySound(m->click);
            }
        }
        dims = MeasureTextEx(m->fonts.heading, "XP Gain", m->fonts.heading.baseSize, 0);
        position.x = button.x - UI_PADDING * 3 - dims.x;
        position.y = button.y + button.height / 2 - m->fonts.heading.baseSize / 2;
        DrawTextEx(m->fonts.heading, "XP Gain", position, m->fonts.heading.baseSize, 0, ZINNWALDITEBROWN);

        button.x = canvas.x + canvas.width - UI_PADDING - button.width;
        button.y = canvas.y + canvas.height - UI_PADDING - button.height;
        if (ui_button(button, "BACK", "Return to the main menu", KEY_NULL, true) > 0) {
            m->screen = GuiScreen_None;
            m->flags |= GlobalFlags_IgnoreInput;
            for (unsigned i = 0; i < arrlen(m->party); i++)
                char_free(&m->party[i]);
            PlaySound(m->click);
        }

        button.y -= button.height + UI_PADDING;
        if (ui_button(button, "BEGIN", "Accept settings and begin the game", KEY_NULL, true) > 0) {
            if (m->flags & GlobalFlags_HugeMap) {
                map_generate(&m->map, util_rdtsc(), TILE_COUNT_MAX, TILE_COUNT_MAX);
            } else {
                map_generate(&m->map, util_rdtsc(), 48, 48);
            }

            m->partyFacing = 2; // Party always enters facing South
            m->partyX = m->map.entryX;
            m->partyY = m->map.entryY + 1;
            m->camera = map_cameraForTile(&m->map, m->partyX, m->partyY, m->partyFacing);
            m->flags &= ~(GlobalFlags_GameOver | GlobalFlags_TheEnd);

            m->page = 0;
            m->fadein = 0.f;
            m->screen = GuiScreen_Intro;
            PlaySound(m->click2);
        }

        EndScissorMode();
        ui_border(m->textures.border, canvas, BONE);
        ui_tooltipPane();
        util_drawLog();
    }
    EndDrawing();
}

static bool
ui_slideshow(unsigned end)
{
    float total = m->slides[m->page].prefade;
    StorySlide* s = m->slides + m->page;
    Color tint = WHITE;
    Vector2 position;
    Font* f = &m->fonts.text;
    bool result = false;

    BeginDrawing();

    DrawRectangle(0, 0, GetRenderWidth(), GetRenderHeight(), BLACK);

    /* Esc to skip everything, space to advance to next start frame */
    if (IsKeyPressed(KEY_ESCAPE)) {
        m->page = end + 1;
        goto page;
    } else if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (m->fadein < s->prefade + s->fadein) {
            m->fadein = s->prefade + s->fadein;
        } else {
            m->page += 1;
            if (m->page > end) {
                goto page;
            } else {
                s++;
                m->fadein = s->prefade + s->fadein * 0.9f;
            }
        }
    }

    if (m->fadein < total)
        goto skip;

    total += s->fadein;
    position.x = GetRenderWidth() * 6.f / 10.f + UI_PADDING;
    position.y = GetRenderHeight() / 10.f;
    if (m->fadein < total) {
        float alpha = (m->fadein - s->prefade) / s->fadein;
        tint = ColorAlpha(tint, util_floatclamp(alpha, 0.f, 1.f));
        position.x -= (1.f - alpha) * UI_PADDING; // TODO Ease in
        goto draw;
    }

    total += s->hold;
    if (m->fadein < total) {
        goto draw;
    }

    total += s->fadeout;
    if (m->fadein < total) {
        float alpha = (m->fadein - s->prefade - s->fadein - s->hold) / s->fadeout;
        tint = ColorAlpha(tint, util_floatclamp(1.f - alpha, 0.f, 1.f));
        position.x += alpha * UI_PADDING; // TODO Ease out
        goto draw;
    } else {
        tint = BLACK;
    }

    m->page++;
    m->fadein = 0.f;
page:
    if (m->page > end) {
        m->screen = GuiScreen_None;
        result = true;
        goto skip;
    }
draw:
    DrawTextPro(*f, s->text, position, (Vector2){0, 0}, 0.f, f->baseSize, 0.f, tint);
    if (s->image > 0) {
        Texture* t = m->textures.scene + s->image - 1;
        float scale = (float)GetRenderHeight() / (float)t->height;
        position.x -= t->width * scale + UI_PADDING;
        position.y = 0.f;
        DrawTextureEx(*t, position, 0.f, scale, tint);
    }

skip:
    EndDrawing();
    m->fadein += m->deltaTime;

    return result;
}

static void
ui_intro(void)
{
    if (ui_slideshow(5)) {
        main_changeSong(&m->music.general - &m->music.ambient);
        m->screen = GuiScreen_None;
        m->fadein = 0.f;
    }
}

static void
ui_outro(void)
{
    if (ui_slideshow(7)) {
        m->screen = GuiScreen_Credits;
        m->fadein = 0.f;
    }
}

static void
ui_dungeon(void)
{
    if (m->flags & GlobalFlags_EditorMode) {
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            m->dragStart = GetMousePosition();
            DisableCursor();
        }

        if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            UpdateCamera(&m->camera, CAMERA_FIRST_PERSON);
            m->camera.up = (Vector3){ 0.f, 1.f, 0.f };
            if (IsKeyDown(KEY_SPACE)) {
                m->camera.position.y += 10.f * m->deltaTime;
                m->camera.target.y += 10.f * m->deltaTime;
            }
            if (IsKeyDown(KEY_C)) {
                m->camera.position.y -= 10.f * m->deltaTime;
                m->camera.target.y -= 10.f * m->deltaTime;
            }
        }

        if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
            EnableCursor();
            SetMousePosition(m->dragStart.x, m->dragStart.y);
        }
    }

    /* Rendering */

    /* Determine GUI layout proportions */
    Rectangle viewport, card, panel;
    GuiLayout layout;
    int portraitSize = 145;
    {
        /* We proceed by slicing out of the main viewport */
        viewport.x = m->area.left + UI_PADDING;
        viewport.y = m->area.top + UI_PADDING;
        viewport.width = m->area.width - UI_PADDING * 2;
        viewport.height = m->area.height - UI_PADDING * 2;

        /* Minimum dimensions */
        card.height = 254;
        card.width = 245;
        panel.width = 480;

        /* Portrait has a fixed square size based on the vertical play area (145px @ 1080p)
            * This means the card rectangle will need at least that plus 30 for padding and 115 for text
            * on width. For height it should always be 260 or else all the stats won't fit beside the action.
            * Panel width maxes out at 700px */

        /* Ultrawide, cards on left side */
        if (m->aspect > 180 && viewport.height >= card.height * 4 + UI_PADDING * 3) {
            layout = GuiLayout_Ultrawide;

            portraitSize = 145;
            card.x = viewport.x;
            card.y = viewport.y;
            card.width = 300;
            viewport.x += card.width + UI_PADDING;
            viewport.width -= card.width + UI_PADDING;

            panel.width = 700;
            panel.x = viewport.x + viewport.width - panel.width;
            panel.y = viewport.y;
            panel.height = viewport.height;
            viewport.width -= panel.width + UI_PADDING;

        /* TODO Square, 2x2 cards along bottom, for aspects as small as 70-80 */
        #if 0
        } else if (m->aspect < 120) {
            layout = GuiLayout_Tall;
        #endif

        /* Non-Widescreen, cards along bottom */
        } else if (viewport.width < card.width * 4 + panel.width + UI_PADDING * 3) {
            layout = GuiLayout_Square;

            card.x = viewport.x;
            card.y = viewport.y + viewport.height - card.height;
            card.width = (viewport.width - UI_PADDING * 3) / 4;
            portraitSize = util_intclamp(card.width - 145, 64, 145);
            viewport.height -= card.height + UI_PADDING;

            panel.width = util_intclamp(viewport.width * UI_SIDE_PANEL_FRACTION, 480, 700);
            panel.height = viewport.height;
            panel.y = viewport.y;
            panel.x = viewport.x + viewport.width - panel.width;
            viewport.width -= panel.width + UI_PADDING;

        } else { /* Regular Widescreen, cards below viewport */
            layout = GuiLayout_Widescreen;

            panel.width = util_intclamp(viewport.width * UI_SIDE_PANEL_FRACTION, 480, 700);
            panel.height = viewport.height;
            panel.y = viewport.y;
            panel.x = viewport.x + viewport.width - panel.width;
            viewport.width -= panel.width + UI_PADDING;

            card.x = viewport.x;
            card.y = viewport.y + viewport.height - card.height;
            card.width = (viewport.width - UI_PADDING * 3) / 4;
            portraitSize = util_intclamp(card.width - 145, 64, 145);
            viewport.height -= card.height + UI_PADDING;
        }

        viewport = RectangleFloor(viewport);
        card = RectangleFloor(card);
        panel = RectangleFloor(panel);
    }

    if (!(m->flags & GlobalFlags_Encounter)) {
        /* Draw map to a render texture. Incredibly, raylib doesn't do viewports */
        if (m->rtexW != viewport.width || m->rtexH != viewport.height) {
            if (IsRenderTextureValid(m->rtex))
                UnloadRenderTexture(m->rtex);
            m->rtex = LoadRenderTexture(viewport.width, viewport.height);
            TraceLog(LOG_DEBUG, "Viewport Resize W: %4i->%4i H: %4i->%4i", m->rtexW, (int)viewport.width, m->rtexH, (int)viewport.height);
            m->rtexW = viewport.width;
            m->rtexH = viewport.height;
        }

        BeginTextureMode(m->rtex);
        BeginMode3D(m->camera);
        ClearBackground(BLACK);

        if (m->map.name[0]) {
            // TODO Redo lighting to allow multiple sources with different curves
            map_draw(&m->map, /*BEIGE*/ LIGHTOLIVEGREEN, 4.f * TILE_SIDE_LENGTH, 0.9f);
        }

        rlDisableDepthMask();
        if (m->flags & GlobalFlags_EditorMode)
            DrawGrid(util_intmax(m->map.width, m->map.height) + 1, TILE_SIDE_LENGTH);

        if (m->flags & GlobalFlags_ShowCollision) {
            // TODO Draw inward in a spiral pattern
            float size = TILE_SIDE_LENGTH + 0.05f;
            for (int x = 0; x < m->map.width; x++) {
                for (int y = 0; y < m->map.height; y++) {
                    if (abs(x - m->partyX) > 4 || abs(y - m->partyY) > 4)
                        continue;

                    int index = x + y * m->map.width;
                    if (!(m->map.tiles[index] & TileFlags_AllowEntry)) {
                        Vector3 position = map_tileCenter(&m->map, x, y);
                        DrawCube(position, size, size, size, CLIP_COLOR);
                        DrawCubeWires(position, size, size, size, LIGHTGRAY);
                    }

                    if (!(m->map.tiles[index] & TileFlags_AllowEast)) {
                        Vector3 position = map_tileCenter(&m->map, x, y);
                        position.x += (float)TILE_SIDE_LENGTH / 2.f;
                        DrawCube(position, 0.1f, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, WALLCLIP_COLOR);
                        DrawCubeWires(position, 0.1f, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, LIGHTGRAY);
                    }

                    if (!(m->map.tiles[index] & TileFlags_AllowSouth)) {
                        Vector3 position = map_tileCenter(&m->map, x, y);
                        position.z += (float)TILE_SIDE_LENGTH / 2.f;
                        DrawCube(position, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, 0.1f, WALLCLIP_COLOR);
                        DrawCubeWires(position, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, 0.1f, LIGHTGRAY);
                    }
                }
            }
        }

        if (m->flags & GlobalFlags_EditorMode) { /* Party location indicator */
            Camera3D c;
            Vector3 start, end, center;
            c = map_cameraForTile(&m->map, m->partyX, m->partyY, m->partyFacing);

            start = map_tileCenter(&m->map, m->partyX, m->partyY);
            end = c.position;
            start.y = end.y;
            DrawCylinderWiresEx(start, end, 0.001f, 0.2f, 6, SKYBLUE);
            center = c.position;
            center.y -= CAMERA_HEIGHT / 2.f;
            DrawCube(center, 0.5f, CAMERA_HEIGHT + 0.15f, 0.5f, BEIGE);
            DrawCubeWires(center, 0.5f, CAMERA_HEIGHT + 0.15f, 0.5f, SKYBLUE);
        }
        rlEnableDepthMask();

        EndMode3D();
        EndTextureMode();
    }

    BeginDrawing();
    {
        DrawTextureRec(m->textures.marble, (Rectangle){0, 0, GetRenderWidth(), GetRenderHeight()}, (Vector2){0, 0}, DARKBROWN);
        bool active = true;

        if (m->flags & (GlobalFlags_GameOver | GlobalFlags_TheEnd))
            active = false;

        if (m->screen != GuiScreen_None)
            active = false;

        if (m->flags & GlobalFlags_Encounter) {
            Texture* tex;
            Rectangle portrait = {};
            Rectangle viewport2;
            char buffer[128];
            MonsterStack* stack;

            stack = &m->encounter.stack;
            tex = &m->encounter.stack.class.texture;

            portrait.width = tex->width;
            portrait.height = tex->height;
            viewport.y += (float)UI_PADDING * stack->effects.bumpSmooth;
            DrawRectangleRec(viewport, BLACK);

            { /* Aspect Correction */
                float vpAspect, rtAspect;
                vpAspect = (float)viewport.width / (float)viewport.height;
                rtAspect = (float)tex->width / (float)tex->height;

                if (fabsf(vpAspect - rtAspect) > 0.001f) {
                    float diff = rtAspect - vpAspect;
                    Vector2 anchor = stack->class.anchor;
                    if (diff > 0.f) {
                        int cut = (float)tex->width * diff / 2.f;
                        portrait.x = cut * anchor.x;
                        portrait.width -= cut;
                    } else {
                        int cut = (float)tex->height * -diff / 2.f;
                        portrait.y = cut * anchor.y;
                        portrait.height -= cut;
                    }
                }
            }

            DrawTexturePro(*tex, portrait, viewport, (Vector2){0,0}, 0.f, WHITE);

            if (stack->effects.flash > 0.f) {
                Color fcolor = ColorAlpha(stack->effects.color, stack->effects.flash);
                DrawTexturePro(m->textures.flash,
                                (Rectangle){0, 0, m->textures.flash.width, m->textures.flash.height},
                                viewport, (Vector2){0,0}, 0.f, fcolor);
            }

            PortraitEffects_update(&stack->effects, GetFrameTime());

            { /* Combat UI */
                Vector2 position, dims;
                Rectangle button, frame;
                int result;

                if (m->encounter.state != CombatState_Menu) {
                    active = false;
                    if (m->encounter.timer <= 0.f) {
                        switch (m->encounter.state) {
                            default:
                            case CombatState_Fighting: {
                                combat_resolveFight();
                            } break;
                            case CombatState_Fleeing: {
                                combat_resolveFlee();
                            } break;
                        }
                        m->encounter.timer = CombatSpeed_time(m->encounter.speed);
                    } else {
                        float dt = m->deltaTime;

                        /* Allow skipping ahead */
                        if (IsMouseButtonPressed(0) || IsKeyPressed(KEY_SPACE))
                            m->encounter.timer = 0;
                        if (IsMouseButtonDown(0) || IsKeyDown(KEY_SPACE))
                            dt *= 3.f;

                        m->encounter.timer -= dt;
                    }
                }

                if (stack->alive > 0) {
                    if (stack->alive == 1) {
                        snprintf(buffer, sizeof(buffer), stack->class.truename);
                    } else {
                        snprintf(buffer, sizeof(buffer), "%u %s", stack->alive, stack->class.truenamePlural);
                    }

                    dims = MeasureTextEx(m->fonts.title, buffer, m->fonts.title.baseSize, 0);
                    frame.x = viewport.x + UI_PADDING;
                    frame.y = viewport.y + UI_PADDING;
                    frame.width = dims.x + UI_PADDING * 4;
                    frame.height = dims.y + UI_PADDING * 2;
                    DrawRectangleRec(frame, ColorAlpha(BLACK, 0.5f));
                    ui_border(m->textures.border, frame, BONE);

                    position.x = frame.x + UI_PADDING * 2;
                    position.y = frame.y + UI_PADDING;
                    ui_text(m->fonts.title, buffer, position, m->fonts.title.baseSize, 0, MINDAROGREEN);
                }

                /* FIGHT! */
                button.width = 120;
                button.height = 48;
                button.x = viewport.x + UI_PADDING;
                button.y = viewport.y + viewport.height - UI_PADDING - button.height;
                result = ui_button(button, "FIGHT", "Fight the next round of combat [F]", KEY_F, active);
                if (result > 0) {
                    PlaySound(m->click);
                    combat_startFight();
                }

                button.x += button.width + UI_PADDING;
                result = ui_button(button, "FLEE", "Attempt to escape. Will drain SP and everyone must "
                                    "save DEX to avoid being hit [R]", KEY_R, active);
                if (result > 0) {
                    PlaySound(m->click);
                    combat_startFlee();
                }
            }

        } else {
            Rectangle button;
            int result;
            Color color;

            /* Map viewport. Because OpenGL's origin is in the bottom left, this has to be inverted */
            color = ColorLerp(BLACK, WHITE, m->fadein);
            DrawTextureRec(m->rtex.texture,
                (Rectangle){0, 0, m->rtex.texture.width, -m->rtex.texture.height},
                (Vector2){viewport.x, viewport.y}, color);

            { /* Compass */
                Vector2 pos;
                Vector2 dim = MeasureTextEx(m->fonts.heading, Facing_toString(m->partyFacing),
                                            m->fonts.heading.baseSize, 0);
                pos.x = viewport.x + viewport.width / 2 - dim.x / 2;
                pos.y = viewport.y + UI_PADDING;
                ui_text(m->fonts.heading, Facing_toString(m->partyFacing), Vector2Floor(pos),
                        m->fonts.heading.baseSize, 0, BONE);
            }

            button.width = 120;
            button.height = 48;
            button.x = viewport.x + viewport.width - UI_PADDING - button.width;
            button.y = viewport.y + viewport.height - UI_PADDING - button.height;
            result = ui_button(button, "WAIT", "Pass time and engage in wait activities. SP always "
                                "restores over time [R]", KEY_R, active);
            if (result > 0) {
                ui_log(ZINNWALDITEBROWN, "Resting...");
                m->encounter.ticks += 300;
                m->flags |= GlobalFlags_AdvanceTurn | GlobalFlags_Resting;

                for (int i = 0; i < arrlen(m->party); i++) {
                    Character* ch = m->party + i;
                    if (!ch->name[0] || ch->health < 1)
                        continue;

                    if (ch->activity == WaitActivity_Rest) {
                        if (ch->stamina < char_maxStamina(*ch)) {
                            int die = (ch->constitution + ch->willpower) / 10 + 2;
                            ch->stamina += PcgRandom_roll(&m->rng, 1, util_intmax(2, die));
                            if (ch->stamina > char_maxStamina(*ch))
                                ch->stamina = char_maxStamina(*ch);
                        } else if (ch->health < char_maxHealth(*ch)) {
                            int die = ch->constitution / 20 + 2;
                            ch->health += PcgRandom_roll(&m->rng, 1, util_intmax(2, die)) - 1;
                            if (ch->health > char_maxHealth(*ch))
                                ch->health = char_maxHealth(*ch);
                        }
                    } else if (ch->activity == WaitActivity_TendWounds) {
                        if (ch->stamina < 1) {
                            ui_log(ZINNWALDITEBROWN, "%s is too tired to tend wounds and so "
                                    "rests for a while instead", ch->name);
                            int die = (ch->constitution + ch->willpower) / 10 + 2;
                            ch->stamina += PcgRandom_roll(&m->rng, 1, die);
                            if (ch->stamina > char_maxStamina(*ch))
                                ch->stamina = char_maxStamina(*ch);
                        } else {
                            Character* patient = 0;
                            int32_t lowest = INT32_MAX;
                            for (int j = 0; j < arrlen(m->party); j++) {
                                if (m->party[j].health < char_maxHealth(m->party[j])
                                    && m->party[j].health >= 0
                                    && m->party[j].health < lowest)
                                {
                                    patient = m->party + j;
                                    lowest = m->party[j].health;
                                }
                            }

                            if (patient) {
                                /* TODO Requires bandages!, without them it only heals 0-CharismaMod.
                                    * And if Charisma mod isn't positive, it does nothing. */
                                int healing = PcgRandom_roll(&m->rng, 1, 6);
                                healing += char_modifier(ch->intellect);
                                healing = util_intclamp(healing, 1, char_maxHealth(*patient) - patient->health);
                                if (patient == ch) {
                                    ui_log(ZINNWALDITEBROWN, "%s tends to %s wounds, "
                                            "healing %i points of damage", ch->name,
                                            ch->flags & CharacterFlags_Female ? "her" : "his", healing);
                                } else {
                                    ui_log(ZINNWALDITEBROWN, "%s tends to %s's wounds, "
                                            "healing %i points of damage",
                                            ch->name, patient->name, healing);
                                }
                                patient->health += healing;
                                patient->effects.color = MINDAROGREEN;
                                patient->effects.flash = 1.f;
                                ch->stamina -= 1;
                            } else {
                                /* TODO Need to make this rest Stamina regain into a function */
                                int die = (ch->constitution + ch->willpower) / 10 + 2;
                                ch->stamina += PcgRandom_roll(&m->rng, 1, die);
                                if (ch->stamina > char_maxStamina(*ch))
                                    ch->stamina = char_maxStamina(*ch);
                            }
                        }
                    }
                }

                PlaySound(m->click);
            }

            if (!(m->flags & GlobalFlags_EditorMode)) { /* Movement Keys
                * TODO Buttons need to distinguish between pressed and down. Probably with a 2 */
                int direction = -1;

                if (IsKeyPressed(KEY_W)) {
                    direction = m->partyFacing + 0;
                    m->partyMoveTimer = m->partyMoveFreq;
                } else if (IsKeyPressed(KEY_A)) {
                    direction = m->partyFacing + 3;
                    m->partyMoveTimer = m->partyMoveFreq;
                } else if (IsKeyPressed(KEY_S)) {
                    direction = m->partyFacing + 2;
                    m->partyMoveTimer = m->partyMoveFreq;
                } else if (IsKeyPressed(KEY_D)) {
                    direction = m->partyFacing + 1;
                    m->partyMoveTimer = m->partyMoveFreq;

                } else if (IsKeyDown(KEY_W)) {
                    m->partyMoveTimer -= m->deltaTime;
                    if (m->partyMoveTimer <= 0.f) {
                        direction = m->partyFacing + 0;
                        m->partyMoveTimer = m->partyMoveFreq;
                    }
                } else if (IsKeyDown(KEY_A)) {
                    m->partyMoveTimer -= m->deltaTime;
                    if (m->partyMoveTimer <= 0.f) {
                        direction = m->partyFacing + 3;
                        m->partyMoveTimer = m->partyMoveFreq;
                    }
                } else if (IsKeyDown(KEY_S)) {
                    m->partyMoveTimer -= m->deltaTime;
                    if (m->partyMoveTimer <= 0.f) {
                        direction = m->partyFacing + 2;
                        m->partyMoveTimer = m->partyMoveFreq;
                    }
                } else if (IsKeyDown(KEY_D)) {
                    m->partyMoveTimer -= m->deltaTime;
                    if (m->partyMoveTimer <= 0.f) {
                        direction = m->partyFacing + 1;
                        m->partyMoveTimer = m->partyMoveFreq;
                    }
                }

                if (direction >= 0) {
                    unsigned targetTile, currentTile;
                    bool successful = false;

                    currentTile = m->partyX + m->partyY * m->map.width;
                    switch (direction % 4) {
                        case 0: { /* North */
                            targetTile = m->partyX + (m->partyY - 1) * m->map.width;
                            if (targetTile < m->map.width * m->map.height
                                && m->map.tiles[targetTile] & TileFlags_AllowEntry
                                && m->map.tiles[targetTile] & TileFlags_AllowSouth
                                && m->partyY > 0)
                            {
                                m->partyY -= 1;
                                successful = true;
                            } else {
                                m->camera.position.z -= 0.1f;
                            }
                        } break;

                        case 1: { /* East */
                            targetTile = m->partyX + 1 + m->partyY * m->map.width;
                            if (targetTile < m->map.width * m->map.height
                                && m->map.tiles[targetTile] & TileFlags_AllowEntry
                                && m->map.tiles[currentTile] & TileFlags_AllowEast
                                && m->partyX + 1 < m->map.width)
                            {
                                m->partyX += 1;
                                successful = true;
                            } else {
                                m->camera.position.x += 0.1f;
                            }
                        } break;

                        case 2: { /* South */
                            targetTile = m->partyX + (m->partyY + 1) * m->map.width;
                            if (targetTile < m->map.width * m->map.height
                                && m->map.tiles[targetTile] & TileFlags_AllowEntry
                                && m->map.tiles[currentTile] & TileFlags_AllowSouth
                                && m->partyY + 1 < m->map.height)
                            {
                                m->partyY += 1;
                                successful = true;
                            } else {
                                m->camera.position.z += 0.1f;
                            }
                        } break;

                        case 3: { /* West */
                            targetTile = m->partyX - 1 + m->partyY * m->map.width;
                            if (targetTile < m->map.width * m->map.height
                                && m->map.tiles[targetTile] & TileFlags_AllowEntry
                                && m->map.tiles[targetTile] & TileFlags_AllowEast
                                && m->partyX > 0)
                            {
                                m->partyX -= 1;
                                successful = true;
                            } else {
                                m->camera.position.x -= 0.1f;
                            }
                        } break;
                    }

                    if (successful) {
                        m->flags |= GlobalFlags_AdvanceTurn;

                        /* Footsteps */
                        int advance = PcgRandom_roll(&m->rng2, 1, 4);
                        int first = PcgRandom_randomu(&m->rng2) % arrlen(m->footstep);
                        for (int i = 0; i < arrlen(m->party); i++) {
                            if (m->party[i].health > 0) {
                                int index = (first + i * advance) % arrlen(m->footstep);
                                PlaySound(m->footstep[index]);
                            }
                        }

                        /* Tick up encounter accumulator */
                        int ticks = 20;
                        for (int i = 0; i < arrlen(m->party); i++) {
                            if (m->party[i].name[0]) {
                                if (m->party[i].health > 0) {
                                    int contrib = 20;
                                    contrib -= char_modifier(m->party[i].dexterity) * 2;
                                    contrib -= char_modifier(m->party[i].intellect);
                                    contrib += char_modifier(m->party[i].strength);
                                    contrib -= m->party[i].movesilent / 10;
                                    ticks += contrib;
                                }
                            }
                        }

                        if (ticks < 50)
                            ticks = 50;
                        m->encounter.ticks += ticks;

                        /* TODO Check for interactables, for now just the entry and exit */
                        if (!(m->flags & GlobalFlags_MissionAccomplished)
                            && m->partyX == m->map.goalX && m->partyY == m->map.goalY)
                        {
                            m->flags |= GlobalFlags_MissionAccomplished;
                            ui_log(MINDAROGREEN, "Dread fills your heart as you open the tomb of the Last King");
                            ui_log(ZINNWALDITEBROWN, "The deed is done, return to the entrance");
                            PlaySound(m->objective);
                        } else if ((m->flags & GlobalFlags_MissionAccomplished)
                                    && m->partyX == m->map.entryX && m->partyY == m->map.entryY)
                        {
                            m->flags |= GlobalFlags_TheEnd;
                            m->encounter.ticks = 0;
                            ui_log(MINDAROGREEN, "You climb out of the Oubliette, to a new and uncertain future...");
                            PlaySound(m->victory);
                            main_changeSong(&m->music.victory - &m->music.ambient);
                            m->music.delay = 3.f;
                        }
                    } else {
                        /* Tick up very slightly */
                        m->encounter.ticks += 1;

                        /* TODO Play a different sound for a failed move */
                    }
                }

                if (IsKeyPressed(KEY_Q)) {
                    m->partyFacing += 3;
                    m->partyFacing %= 4;
                }

                if (IsKeyPressed(KEY_E)) {
                    m->partyFacing += 1;
                    m->partyFacing %= 4;
                }

                int index = map_tileIndex(&m->map, m->partyX, m->partyY);
                if (index >= 0) {
                    m->map.tiles[index] |= TileFlags_Visited;
                }
            }
        }

        ui_border(m->textures.border, viewport, BONE);

        { /* Header */
            Rectangle button;
            int result;

            button.width = 140;
            button.height = 48;
            button.y = panel.y;
            button.x = panel.x + panel.width - button.width;

            result = ui_button(button, "OPTIONS", "Adjust settings or return to main menu [Escape]",
                                KEY_ESCAPE, active);
            if (result > 0) {
                m->screen = GuiScreen_Options;
                m->flags |= GlobalFlags_IgnoreInput;
                PlaySound(m->click);
            }

            Vector2 position;
            Vector2 dimensions = MeasureTextEx(m->fonts.title, m->map.name, m->fonts.title.baseSize, 0);
            position.x = panel.x + (panel.width - button.width - UI_PADDING) / 2 - dimensions.x / 2;
            position.y = panel.y + UI_SIDE_PANEL_HEADER / 2.f - dimensions.y / 2.f;
            ui_text(m->fonts.title, m->map.name, Vector2Floor(position), m->fonts.title.baseSize, 0, BONE);

            panel.height -= UI_SIDE_PANEL_HEADER;
            panel.y += UI_SIDE_PANEL_HEADER;
        }

        { /* Character Cards */
            bool active = m->screen == GuiScreen_None && !(m->flags & (GlobalFlags_GameOver | GlobalFlags_TheEnd));
            ui_characterHudCard(m->party + 0, card, portraitSize, active ? KEY_ONE : -1);

            if (layout == GuiLayout_Ultrawide) {
                card.y += card.height + UI_PADDING;
            } else {
                card.x += card.width + UI_PADDING;
            }
            ui_characterHudCard(m->party + 1, card, portraitSize, active ? KEY_TWO : -1);

            if (layout == GuiLayout_Ultrawide) {
                card.y += card.height + UI_PADDING;
            } else {
                card.x += card.width + UI_PADDING;
            }
            ui_characterHudCard(m->party + 2, card, portraitSize, active ? KEY_THREE : -1);

            if (layout == GuiLayout_Ultrawide) {
                card.y += card.height + UI_PADDING;
            } else {
                card.x += card.width + UI_PADDING;
            }
            ui_characterHudCard(m->party + 3, card, portraitSize, active ? KEY_FOUR : -1);
        }

        if (m->flags & GlobalFlags_ShowTooltips) { /* Tooltips */
            Vector2 position;
            position.x = panel.x;
            position.y = panel.y + panel.height - m->fonts.text.baseSize;
            panel.height -= UI_PADDING + m->fonts.text.baseSize;
            if (m->tooltip[0])
                ui_text(m->fonts.text, m->tooltip, position, m->fonts.text.baseSize, 0, BONE);
        }

        { /* Side Panel */
            Vector2 position;
            int count = 0;
            int scrollMax = 0;
            int visible;

            position.x = panel.x + UI_PADDING;
            position.y = panel.y + UI_PADDING;
            visible = panel.height / m->fonts.text.baseSize;
            if ((int)panel.height % (int)m->fonts.text.baseSize)
                visible += 1;

            if (util_mouseInRect(panel))
                m->logScroll += (int)GetMouseWheelMoveV().y * 3;
            DrawTextureRec(m->textures.vellum, panel, (Vector2){panel.x, panel.y}, WHITE);
            BeginScissorMode(panel.x, panel.y, panel.width, panel.height);

            { /* Decorative Flourish */
                Vector2 position;
                float scale;

                if (panel.width / 2 - UI_PADDING * 2 < m->textures.panel.width) {
                    scale = (float)(panel.width / 2 - UI_PADDING * 2) / m->textures.panel.width;
                } else {
                    scale = 1.f;
                }

                position.x = panel.x + panel.width - UI_PADDING - m->textures.panel.width * scale;
                position.y = panel.y + UI_PADDING;
                DrawTextureEx(m->textures.panel, position, 0.f, scale, ZINNWALDITEBROWN);
            }

            /* Find scroll point first */
            for (unsigned i = 0; i < UI_LOGLINE_COUNT; i++) {
                unsigned index = (i + m->logCursor) % UI_LOGLINE_COUNT;
                if (m->logs[index].text[0]) {
                    count++;
                }
            }

            scrollMax = count - visible;
            if (scrollMax < 0) {
                scrollMax = 0;
            } else {
                position.y -= (scrollMax + 1) * m->fonts.text.baseSize;
                position.y -= (int)panel.height % m->fonts.text.baseSize;
                position.y += m->logScrollSmooth;
            }

            m->logScroll = util_intclamp(m->logScroll, 0, scrollMax + 1);
            float factor = util_floatclamp(GetFrameTime() * 10.f, 0.f, 1.f);
            m->logScrollSmooth = ((float)m->logScroll * m->fonts.text.baseSize)
                                * factor + m->logScrollSmooth * (1.f - factor);

            for (unsigned i = 0; i < UI_LOGLINE_COUNT; i++) {
                unsigned index = (i + m->logCursor) % UI_LOGLINE_COUNT;
                if (m->logs[index].text[0]) {
                    Color color = m->logs[index].color;
                    if (color.r > 100 || color.g > 100 | color.b > 150) {
                        ui_text(m->fonts.text, m->logs[index].text, position,
                                    m->fonts.text.baseSize, 0, color);
                    } else {
                        DrawTextEx(m->fonts.text, m->logs[index].text, position,
                                    m->fonts.text.baseSize, 0, color);
                    }
                    position.y += m->fonts.text.baseSize;
                }
            }
            EndScissorMode();
            // TODO scrollbar
            ui_border(m->textures.border, panel, BONE);
        }

        if (m->flags & GlobalFlags_PartyStats) {
            char buf[256] = {0};
            Vector2 position = (Vector2){GetRenderWidth() - 160, UI_SIDE_PANEL_HEADER + UI_PADDING * 2};
            int danger = -1;
            if (m->flags & GlobalFlags_MissionAccomplished) {
                danger = 1;
            }

            snprintf(buf, sizeof(buf),
                    "FPS: %3i\n"
                    //"Debug View: %s\n"
                    "Camera: %4.1f, %4.1f\n"
                    "Tile: %2i, %2i [%i]\n"
                    "Enc. Ticks: %i/%i %s\n"
                    "Enc. State: %i %05.2f\n"
                    "Dungeon Lv%i Size %i\n"
                    "Seed: %llu\n",
                    GetFPS(),
                    //m->flags & GlobalFlags_EditorMode ? "ON" : "off",
                    m->camera.position.x, m->camera.position.z,
                    m->partyX, m->partyY, map_tileIndex(&m->map, m->partyX, m->partyY),
                    m->encounter.ticks, m->map.encounterFreq,
                    m->flags & GlobalFlags_IgnoreEncounters ? "OFF" : "",
                    m->encounter.speed, m->encounter.timer,
                    danger + 2, m->map.chamberCount,
                    m->map.seed);
            ui_text(GetFontDefault(), buf, position, GetFontDefault().baseSize, 1, BONE);
        }

        if (m->ext.cimgui.handle && (m->flags & GlobalFlags_EditorModePermitted)) {
            ext_CImguiNewFrame(m);
            if (m->flags & GlobalFlags_ShowMap) {
                bool show = 1;
                char buffer[TILE_COUNT_MAX * 2 + 1] = {};

                {
                    ImGuiWindowFlags flags = 0;
                    flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
                    flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
                    m->ext.cimgui.Begin("Map Overview", &show, flags);
                }

                for (int y = 0; y < m->map.height; y++) {
                    for (int x = 0; x < m->map.width; x++) {
                        TileFlags t = m->map.tiles[x + y * m->map.width];

                        if (m->partyX == x && m->partyY == y) {
                            switch (m->partyFacing) {
                                default: buffer[x * 2] = '^'; break;
                                case Facing_East: buffer[x * 2] = '>'; break;
                                case Facing_South: buffer[x * 2] = 'v'; break;
                                case Facing_West: buffer[x * 2] = '<'; break;
                            }
                        } else if (m->map.goalX == x && m->map.goalY == y) {
                            buffer[x * 2] = 'X';
                        } else if (t & TileFlags_Failure) {
                            buffer[x * 2] = 'f';
                        } else if (t & TileFlags_Feature) {
                            buffer[x * 2] = '?';
                        } else if (t & TileFlags_Filled) {
                            buffer[x * 2] = 'E';
                        } else if ((t & TileFlags_AllowEntry) && !(t & TileFlags_AllowSouth)) {
                            buffer[x * 2] = '_';
                        } else if (t & TileFlags_Visited) {
                            buffer[x * 2] = '.';
                        } else if (!(t & TileFlags_AllowEntry)) {
                            buffer[x * 2] = '#';
                        } else if (!(t & TileFlags_Flooded)) {
                            buffer[x * 2] = 'u';
                        } else {
                            buffer[x * 2] = ' ';
                        }

                        if ((t & TileFlags_AllowEntry) && !(t & TileFlags_AllowEast)) {
                            buffer[x * 2 + 1] = '|';
                        } else if (!(t & TileFlags_AllowEntry)) {
                            buffer[x * 2 + 1] = '#';
                        } else {
                            buffer[x * 2 + 1] = ' ';
                        }
                    }
                    m->ext.cimgui.Text(buffer);

                }
                m->ext.cimgui.End();
                if (!show)
                    m->flags &= ~(GlobalFlags_ShowMap);
            }
            ext_CImguiRender(&m->ext.cimgui);
        }
    }

    if (m->fadein < 0.5f) {
        Color color = ColorAlpha(BLACK, 1.f - m->fadein * 2.f);
        DrawRectangle(0, 0, GetRenderWidth(), GetRenderHeight(), color);
    }


    if (m->flags & GlobalFlags_GameOver) {
        Vector2 position;
        Vector2 measure;
        Rectangle frame;
        Rectangle button;

        button.height = 48;
        button.width = 256;

        DrawRectangle(0, 0, GetRenderWidth(), GetRenderHeight(), ColorAlpha(BLACK, 0.8f));
        measure = MeasureTextEx(m->fonts.big, "GAME OVER", m->fonts.big.baseSize, 0);
        frame.x = GetRenderWidth() / 2 - measure.x / 2 - UI_PADDING * 2;
        frame.y = GetRenderHeight() / 2 - measure.y / 2 - UI_PADDING;
        frame.width = measure.x + UI_PADDING * 4;
        frame.height = measure.y + UI_PADDING * 3 + button.height;
        frame = RectangleFloor(frame);
        DrawRectangleRec(frame, ColorAlpha(BLACK, 0.8f));
        ui_border(m->textures.border, frame, BONE);
        position.x = frame.x + UI_PADDING * 2;
        position.y = frame.y + UI_PADDING;
        ui_text(m->fonts.big, "GAME OVER", Vector2Floor(position), m->fonts.big.baseSize, 0, MAROON);

        button.x = frame.x + frame.width / 2 - button.width / 2;
        button.y = frame.y + frame.height - UI_PADDING - button.height;
        if (ui_button(button, "CONTINUE", "", KEY_ENTER, true) > 0) {
            m->screen = GuiScreen_Credits;
            m->flags |= GlobalFlags_IgnoreInput;
            PlaySound(m->click);
            map_unload(&m->map);
        }

    } else if (m->flags & GlobalFlags_TheEnd) {
        Vector2 position;
        Vector2 measure;
        Rectangle frame;
        Rectangle button;

        button.height = 48;
        button.width = 256;

        DrawRectangle(0, 0, GetRenderWidth(), GetRenderHeight(), ColorAlpha(BLACK, 0.8f));
        measure = MeasureTextEx(m->fonts.big, "VICTORY", m->fonts.big.baseSize, 0);
        frame.x = GetRenderWidth() / 2 - measure.x / 2 - UI_PADDING * 2;
        frame.y = GetRenderHeight() / 2 - measure.y / 2 - UI_PADDING;
        frame.width = measure.x + UI_PADDING * 4;
        frame.height = measure.y + UI_PADDING * 3 + button.height;
        frame = RectangleFloor(frame);
        DrawRectangleRec(frame, ColorAlpha(BLACK, 0.8f));
        ui_border(m->textures.border, frame, BONE);
        position.x = frame.x + UI_PADDING * 2;
        position.y = frame.y + UI_PADDING;
        ui_text(m->fonts.big, "VICTORY", position, m->fonts.big.baseSize, 0, MINDAROGREEN);

        button.x = frame.x + frame.width / 2 - button.width / 2;
        button.y = frame.y + frame.height - UI_PADDING - button.height;
        if (ui_button(button, "CONTINUE", "", KEY_ENTER, true) > 0) {
            m->screen = GuiScreen_Outro;
            m->page = 6;
            m->fadein = 0.f;
            m->flags |= GlobalFlags_IgnoreInput;
            PlaySound(m->click);
            map_unload(&m->map);
        }

    } else if (m->screen == GuiScreen_Options) {
        ui_options();
    }

    util_drawLog();

    EndDrawing();
    m->fadein += m->deltaTime / 2.f;
    m->fadein = Clamp(m->fadein, 0.f, 1.f);

    /* Encounter Checks */
    if (!(m->flags & GlobalFlags_Encounter)) {
        if ((m->flags & GlobalFlags_AdvanceTurn)) {
            if (m->encounter.ticks > m->map.encounterFreq) {
                int chance = m->encounter.ticks / m->map.encounterFreq + 1;
                int die = 6;
                m->encounter.ticks %= m->map.encounterFreq;

                /* Per-check events TODO Use a separate, fixed accumulator */
                for (int i = 0; i < arrlen(m->party); i++) {
                    Character* ch = m->party + i;

                    /* Incapacitated Characters have a percentage chance to wake */
                    if (ch->health == 0 && PcgRandom_roll(&m->rng, 1, 100) < ch->stamina) {
                        ch->health += 1;
                        ui_log(WHITE, "%s recovers consciousness", ch->name);
                    }

                    /* Passive stamina recovery */
                    if (ch->health >= 0 && ch->stamina < char_maxStamina(*ch)) {
                        int die = (ch->constitution + ch->willpower) / 20 + 1;
                        ch->stamina += PcgRandom_roll(&m->rng, 1, util_intmax(2, die)) - 1;
                        if (ch->stamina > char_maxStamina(*ch))
                            ch->stamina = char_maxStamina(*ch);
                    }

                    /* Dead characters decompose, losing more HP */
                    if (ch->health < 0 && PcgRandom_roll(&m->rng, 1, 6) == 1) {
                        ch->health -= 1;
                        if (PcgRandom_roll(&m->rng, 1, 6) == 1) {
                            switch (PcgRandom_roll(&m->rng, 1, 6)) {
                                default:
                                case 1: ch->strength -= 1;
                                case 2: ch->dexterity -= 1;
                                case 3: ch->constitution -= 1;
                                case 4: ch->intellect -= 1;
                                case 5: ch->willpower -= 1;
                                case 6: ch->charisma -= 1;
                            }
                            /* TODO Having a stat reduced to zero renders a character invalid */
                        }
                    }
                }

                if (PcgRandom_roll(&m->rng, 1, die) <= chance && !(m->flags & GlobalFlags_IgnoreEncounters)) {
                    combat_randomEncounter(&m->monsters);
                }
            }

            m->flags &= ~(GlobalFlags_AdvanceTurn | GlobalFlags_Resting);
        }

        // TODO if not in a menu or encounter
        if (!(m->flags & GlobalFlags_EditorMode)) {
            Camera intended = map_cameraForTile(&m->map, m->partyX, m->partyY, m->partyFacing);
            float lerp = Clamp(m->deltaTime * 10.0f, 0.f, 1.f);
            m->camera.position = Vector3Lerp(m->camera.position, intended.position, lerp);
            m->camera.target = Vector3Lerp(m->camera.target, intended.target, lerp);
        }
    }
}

static void
ui_options(void)
{
    Rectangle window, button;
    Vector2 position, dims;
    int result;
    char buffer[32];

    DrawRectangle(0, 0, GetRenderWidth(), GetRenderHeight(), ColorAlpha(BLACK, 0.8f));

    dims = MeasureTextEx(m->fonts.title, "Options", m->fonts.title.baseSize, 0.f);
    button.width = 140;
    button.height = 48;
    window.width = button.width * 3 + UI_PADDING * 4;
    window.height = button.height * 7 + UI_PADDING * 7 + dims.y;
    window.x = GetRenderWidth() / 2 - window.width / 2;
    window.y = GetRenderHeight() / 2 - window.height / 2;
    DrawTexturePro(m->textures.vellum, window, window, (Vector2){0,0}, 0.f, WHITE);

    { /* Decorative Image */
        Rectangle dest, source;
        float scale;

        source = (Rectangle){0, 0, m->textures.options.width, m->textures.options.height};
        dest.x = window.x + UI_PADDING;
        dest.y = window.y + UI_PADDING;
        dest.height = window.height - UI_PADDING * 2 - button.height;
        scale = (float)dest.height / (float)source.height;
        dest.width = source.width * scale;
        DrawTexturePro(m->textures.options, source, dest, (Vector2){0,0}, 0.f, ZINNWALDITEBROWN);
    }

    position.x = window.x + window.width / 2;
    position.y = window.y;
    ui_text(m->fonts.title, "Options", position, m->fonts.title.baseSize, 0.f, ZINNWALDITEBROWN);

    button.x = window.x + window.width - UI_PADDING - button.width;
    button.y = position.y + dims.y;
    dims = MeasureTextEx(m->fonts.heading, "Music", m->fonts.heading.baseSize, 0.f);
    position.x = button.x - UI_PADDING * 2 - dims.x;
    position.y = button.y + button.height / 2 - dims.y / 2;
    DrawTextEx(m->fonts.heading, "Music", position, m->fonts.heading.baseSize, 0.f, ZINNWALDITEBROWN);
    if (IsMusicStreamPlaying(m->music.all[m->music.track])) {
        result = ui_button(button, "ON", "Disable background music", KEY_NULL, true);
        if (result > 0) {
            m->flags |= GlobalFlags_MuteMusic;
            StopMusicStream(m->music.all[m->music.track]);
            PlaySound(m->click);
        }
    } else {
        result = ui_button(button, "(off)", "Enable background music", KEY_NULL, true);
        if (result > 0) {
            m->flags &= ~(GlobalFlags_MuteMusic);
            PlayMusicStream(m->music.all[m->music.track]);
            PlaySound(m->click);
        }
    }

    button.y += button.height + UI_PADDING;
    dims = MeasureTextEx(m->fonts.heading, "Ambient Loop", m->fonts.heading.baseSize, 0.f);
    position.x = button.x - UI_PADDING * 2 - dims.x;
    position.y = button.y + button.height / 2 - dims.y / 2;
    DrawTextEx(m->fonts.heading, "Ambient Loop", position, m->fonts.heading.baseSize, 0.f, ZINNWALDITEBROWN);
    if (IsMusicStreamPlaying(m->music.ambient)) {
        result = ui_button(button, "ON", "Disable ambient sound loop", KEY_NULL, true);
        if (result > 0) {
            m->flags |= GlobalFlags_MuteAmbience;
            StopMusicStream(m->music.ambient);
            PlaySound(m->click);
        }
    } else {
        result = ui_button(button, "(off)", "Enable ambient sound loop", KEY_NULL, true);
        if (result > 0) {
            m->flags &= ~(GlobalFlags_MuteAmbience);
            PlayMusicStream(m->music.ambient);
            PlaySound(m->click);
        }
    }

    button.y += button.height + UI_PADDING;
    dims = MeasureTextEx(m->fonts.heading, "Sound Effects", m->fonts.heading.baseSize, 0.f);
    position.x = button.x - UI_PADDING * 2 - dims.x;
    position.y = button.y + button.height / 2 - dims.y / 2;
    DrawTextEx(m->fonts.heading, "Sound Effects", position, m->fonts.heading.baseSize, 0.f, ZINNWALDITEBROWN);
    if (m->flags & GlobalFlags_MuteSFX) {
        result = ui_button(button, "(off)", "Enable sound effects", KEY_NULL, true);
        if (result > 0) {
            m->flags &= ~(GlobalFlags_MuteSFX);
            for (int i = 0; i < arrlen(m->sfx); i++)
                SetSoundVolume(m->sfx[i], 1.f);
            PlaySound(m->click);
        }
    } else {
        result = ui_button(button, "ON", "Disable sound effects", KEY_NULL, true);
        if (result > 0) {
            m->flags |= GlobalFlags_MuteSFX;
            for (int i = 0; i < arrlen(m->sfx); i++)
                SetSoundVolume(m->sfx[i], 0.f);
        }
    }

    button.y += button.height + UI_PADDING;
    dims = MeasureTextEx(m->fonts.heading, "Fullscreen", m->fonts.heading.baseSize, 0.f);
    position.x = button.x - UI_PADDING * 2 - dims.x;
    position.y = button.y + button.height / 2 - dims.y / 2;
    DrawTextEx(m->fonts.heading, "Fullscreen", position, m->fonts.heading.baseSize, 0.f, ZINNWALDITEBROWN);
    if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) {
        result = ui_button(button, "ON", "Game will display in borderless fullscreen. Click to change to resizable window. [Alt+Enter]", KEY_NULL, true);
        if (result > 0) {
            ToggleBorderlessWindowed();
            PlaySound(m->click);
        }
    } else {
        result = ui_button(button, "(off)", "Game displays as a resizable window. Click to change to borderless fullscreen. [Alt+Enter]", KEY_NULL, true);
        if (result > 0) {
            ToggleBorderlessWindowed();
            PlaySound(m->click);
        }
    }

    button.y += button.height + UI_PADDING;
    dims = MeasureTextEx(m->fonts.heading, "Combat Speed", m->fonts.heading.baseSize, 0.f);
    position.x = button.x - UI_PADDING * 2 - dims.x;
    position.y = button.y + button.height / 2 - dims.y / 2;
    DrawTextEx(m->fonts.heading, "Combat Speed", position, m->fonts.heading.baseSize, 0.f, ZINNWALDITEBROWN);
    memset(buffer, 0, sizeof(buffer));
    switch (m->encounter.speed) {
        default:
        case CombatSpeed_Instant: { snprintf(buffer, sizeof(buffer), "INSTANT"); } break;
        case CombatSpeed_Fast:    { snprintf(buffer, sizeof(buffer), "FAST"); } break;
        case CombatSpeed_Slow:    { snprintf(buffer, sizeof(buffer), "SLOW"); } break;
    }
    result = ui_button(button, buffer, "Change how fast combat rounds resolve. "
                        "Can always press Space to skip ahead", KEY_NULL, true);
    if (result > 0) {
        m->encounter.speed = (m->encounter.speed + 1) % CombatSpeed_Count;
        PlaySound(m->click);
    }

    button.y += button.height + UI_PADDING;
    dims = MeasureTextEx(m->fonts.heading, "Show Tooltips", m->fonts.heading.baseSize, 0.f);
    position.x = button.x - UI_PADDING * 2 - dims.x;
    position.y = button.y + button.height / 2 - dims.y / 2;
    DrawTextEx(m->fonts.heading, "Show Tooltips", position, m->fonts.heading.baseSize, 0.f, ZINNWALDITEBROWN);
    if (m->flags & GlobalFlags_ShowTooltips) {
        result = ui_button(button, "ON", "Disable tooltips", KEY_NULL, true);
        if (result > 0) {
            m->flags &= ~(GlobalFlags_ShowTooltips);
            PlaySound(m->click);
        }
    } else {
        result = ui_button(button, "(off)", "Enable tooltips", KEY_NULL, true);
        if (result > 0) {
            m->flags |= GlobalFlags_ShowTooltips;
            PlaySound(m->click);
        }
    }

    if (m->map.name[0]) {
        button.x = window.x + UI_PADDING;
        button.y = window.y + window.height - button.height - UI_PADDING;
        result = ui_button(button, "RESUME", "Return to the game [Escape]", KEY_ESCAPE, true);
        if (result > 0) {
            m->screen = GuiScreen_None;
            PlaySound(m->click);
            m->flags |= GlobalFlags_IgnoreInput;
        }

        button.x += button.width + UI_PADDING;
        if (m->flags & GlobalFlags_ConfirmExit) {
            result = ui_button(button, "CONFIRM?", "Are you sure you want to stop? Progress will not be saved.", KEY_NULL, true);
            if (result > 0) {
                m->flags |= GlobalFlags_IgnoreInput;
                m->screen = GuiScreen_None;
                map_unload(&m->map);
                m->fadein = 0.f;
                main_changeSong(&m->music.intro - &m->music.ambient);
            } else if (result == 0) {
                m->flags &= ~(GlobalFlags_ConfirmExit);
            }
        } else {
            result = ui_button(button, "EXIT", "Exit current game and return to main menu", KEY_NULL, true);
            if (result > 0) {
                PlaySound(m->click);
                m->flags |= GlobalFlags_ConfirmExit;
            }
        }

        button.x += button.width + UI_PADDING;
        if (m->flags & GlobalFlags_ConfirmQuit) {
            result = ui_button(button, "CONFIRM?", "Are you sure you want to quit? Progress will not be saved.", KEY_NULL, true);
            if (result > 0) {
                m->flags |= GlobalFlags_RequestQuit;
            } else if (result == 0) {
                m->flags &= ~(GlobalFlags_ConfirmQuit);
            }
        } else {
            result = ui_button(button, "QUIT", "Quit the game and return to desktop [Alt+F4]", KEY_NULL, true);
            if (result > 0) {
                m->flags |= GlobalFlags_ConfirmQuit;
                PlaySound(m->click2);
            }
        }
    } else {
        button.y += button.height + UI_PADDING;
        result = ui_button(button, "RESUME", "Return to the main menu", KEY_ESCAPE, true);
        if (result > 0) {
            m->screen = GuiScreen_None;
            PlaySound(m->click);
            m->flags |= GlobalFlags_IgnoreInput;
        }
    }

    ui_border(m->textures.border, window, BONE);

    if (m->flags & GlobalFlags_ShowTooltips) {
        ui_tooltipPane();
    }
}

static void
ui_credits(void)
{
    BeginDrawing();
    {
        Vector2 position;
        Rectangle button, canvas;
        char* credits;

        DrawTextureRec(m->textures.marble, (Rectangle){0, 0, GetRenderWidth(), GetRenderHeight()}, (Vector2){0, 0}, DARKBROWN);

        canvas.x = m->area.left + UI_PADDING * 2;
        canvas.y = m->area.top + UI_PADDING * 2;
        canvas.width = m->area.width - UI_PADDING * 4;
        canvas.height = m->area.height - UI_PADDING * 4;
        DrawTexturePro(m->textures.vellum, canvas, canvas, (Vector2){0, 0}, 0.f, WHITE);
        BeginScissorMode(canvas.x, canvas.y, canvas.width, canvas.height);

        { /* Decorative Flourish */
            Vector2 position;
            float scale;

            if (canvas.width / 2 - UI_PADDING * 2 < m->textures.stationery.width) {
                scale = (float)(canvas.width / 2 - UI_PADDING * 2) / m->textures.stationery.width;
            } else {
                scale = 1.f;
            }

            position.x = canvas.x + canvas.width - UI_PADDING - m->textures.stationery.width * scale;
            position.y = canvas.y + UI_PADDING;
            DrawTextureEx(m->textures.stationery, position, 0.f, scale, ZINNWALDITEBROWN);
        }

        position.x = canvas.x + UI_PADDING * 2;
        position.y = canvas.y + UI_PADDING * 2;
        DrawTextEx(m->fonts.title, "CREDITS", position, m->fonts.title.baseSize, 0, ZINNWALDITEBROWN);
        position.y += UI_PADDING + m->fonts.title.baseSize;
        credits =
        "Programming, Design, 3D Models: Stomy (stomygame.itch.io)\n"
        " \n"
        "Featuring public domain artwork by:\n"
        " Gustave Dore, Sidney Sime, Harry Clarke, Alfred Kubin, Henry Justice Ford, and others\n"
        " \n"
        "Music:\n"
        " \"Specters of the Enclave\" by Eliot Corley from ChaosIsHarmony (CC-BY 3.0, opengameart.org)\n"
        " \"Welt Herrscherer Theme\" by yd (CC0, opengameart.org)\n"
        " \"Apologies to JSB\" by Yubatake (CC-BY 3.0, opengameart.org)\n"
        " \"Lament for a Warrior's Soul\" by RandomMind (CC0, opengameart.org)\n"
        " \n"
        "Ambient Track: \"Dark Ambient\" by Alexandr Zhelanov (CC-BY 3.0, opengameart.org)\n"
        "   https://soundcloud.com/alexandr-zhelanov\n"
        "Sound Effects from freesound.org. Artists:\n"
        "   nomiqbomi, el_boss, tom_a73, kyles, Aerny\n"
        " \n"
        "Delven Textures by Bradley D. (https://strideh.itch.io)\n"
        "Torment Textures by Bradley D. (https://strideh.itch.io)\n"
        "Additional CC0 textures by sean10m, Kenney\n"
        "Stone Coffin model by Yughues";
        DrawTextEx(m->fonts.text, credits, position, m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);

        button.width = 120;
        button.height = 48;
        button.x = canvas.x + canvas.width - UI_PADDING * 2 - button.width;
        button.y = canvas.y + canvas.height - UI_PADDING * 2 - button.height;
        if (ui_button(button, "BACK", "", KEY_NULL, true) > 0) {
            m->screen = GuiScreen_None;
            m->flags |= GlobalFlags_IgnoreInput;
            main_changeSong(&m->music.intro - &m->music.ambient);
            PlaySound(m->click);
        }

        EndScissorMode();
        ui_border(m->textures.border, canvas, BONE);
        util_drawLog();
    }
    EndDrawing();
}
