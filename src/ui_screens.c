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
    DrawTexturePro(m->vellum, window, window, (Vector2){0,0}, 0.f, WHITE);

    { /* Decorative Image */
        Rectangle dest, source;
        float scale;

        source = (Rectangle){0, 0, m->options.width, m->options.height};
        dest.x = window.x + UI_PADDING;
        dest.y = window.y + UI_PADDING;
        dest.height = window.height - UI_PADDING * 2 - button.height;
        scale = (float)dest.height / (float)source.height;
        dest.width = source.width * scale;
        DrawTexturePro(m->options, source, dest, (Vector2){0,0}, 0.f, ZINNWALDITEBROWN);
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
            StopMusicStream(m->music.all[m->music.track]);
            PlaySound(m->click);
        }
    } else {
        result = ui_button(button, "(off)", "Enable background music", KEY_NULL, true);
        if (result > 0) {
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
            StopMusicStream(m->music.ambient);
            PlaySound(m->click);
        }
    } else {
        result = ui_button(button, "(off)", "Enable ambient sound loop", KEY_NULL, true);
        if (result > 0) {
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

    button.x = window.x + UI_PADDING;
    button.y = window.y + window.height - button.height - UI_PADDING;
    result = ui_button(button, "RESUME", "Return to the game [Escape]", KEY_ESCAPE, true);
    if (result > 0) {
        m->screen = GuiScreen_None;
        PlaySound(m->click);
    }

    button.x += button.width + UI_PADDING;
    ui_button(button, "EXIT", "TODO: Exit current game and return to main menu", KEY_NULL, true);

    button.x += button.width + UI_PADDING;
    if (m->flags & GlobalFlags_ConfirmQuit) {
        result = ui_button(button, "CONFIRM?", "Are you sure you want to quit?", KEY_NULL, true);
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

    ui_border(m->border, window, BONE);

    if (m->flags & GlobalFlags_ShowTooltips) {
        ui_tooltipPane();
    }
}
