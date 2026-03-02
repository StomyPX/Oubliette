static void
ui_text(Font font, const char* text, Vector2 position, float fontSize, int spacing, Color tint)
{
    Vector2 pos = position;
    Vector4 backColor = ColorNormalize(tint);
    backColor.x = 0.5f / 255.f;
    backColor.y = 0.5f / 255.f;
    backColor.z = 0.5f / 255.f;
    pos.x += 1.f;
    pos.y += 1.f;
    DrawTextEx(font, text, pos, fontSize, spacing, ColorFromNormalized(backColor));
    pos.x -= 1.f;
    pos.y -= 1.f;
    DrawTextEx(font, text, pos, fontSize, spacing, tint);
}

static void
ui_border(Texture border, Rectangle rec, Color color)
{
    Rectangle src, dst;
    Vector2 origin = {};
    int offset = 9;
    int inside = 1;
    int corner;

    corner = border.width / 2 - inside;

    /* Top Left */
    src.x = 0;
    src.y = 0;
    src.width = corner;
    src.height = corner;
    dst.x = rec.x - offset;
    dst.y = rec.y - offset;
    dst.width = src.width;
    dst.height = src.height;
    DrawTexturePro(border, src, dst, origin, 0.f, color);

    /* Top */
    src.x += src.width;
    src.width = inside * 2;
    dst.x += dst.width;
    dst.width = rec.width + offset * 2 - corner * 2;
    DrawTexturePro(border, src, dst, origin, 0.f, color);

    /* Top Right */
    src.x += src.width;
    src.width = corner;
    dst.x += dst.width;
    dst.width = src.width;
    DrawTexturePro(border, src, dst, origin, 0.f, color);

    /* Right */
    src.y += src.height;
    src.height = inside * 2;
    dst.y += dst.height;
    dst.height = rec.height + offset * 2 - corner * 2;
    DrawTexturePro(border, src, dst, origin, 0.f, color);

    /* Bottom Right */
    src.y += src.height;
    src.height = corner;
    dst.y += dst.height;
    dst.height = src.height;
    DrawTexturePro(border, src, dst, origin, 0.f, color);

    /* Bottom */
    dst.width = rec.width + offset * 2 - corner * 2;
    dst.x -= dst.width;
    src.width = inside * 2;
    src.x -= src.width;
    DrawTexturePro(border, src, dst, origin, 0.f, color);

    /* Bottom Left */
    src.width = corner;
    src.x -= src.width;
    dst.x -= src.width;
    dst.width = src.width;
    DrawTexturePro(border, src, dst, origin, 0.f, color);

    /* Left */
    dst.height = rec.height + offset * 2 - corner * 2;
    dst.y -= dst.height;
    src.height = inside * 2;
    src.y -= src.height;
    DrawTexturePro(border, src, dst, origin, 0.f, color);
}

static void
ui_log(Color color, char* fmt, ...)
{
    va_list args;
    uint32_t i = m->logCursor;

    m->logs[i].color = color;
    va_start(args, fmt);
    vsnprintf(m->logs[i].text, sizeof(m->logs[i].text), fmt, args);
    va_end(args);

    TraceLog(LOG_TRACE, "LOG: %s", m->logs[i].text);

    m->logCursor += 1;
    m->logCursor %= UI_LOGLINE_COUNT;
    m->logScroll = 0;
}

static void
ui_characterHudCard(Character c, Rectangle card)
{
    Rectangle portrait;
    Texture ptex;
    Vector2 zero = {};
    Vector2 position;
    Color color;

    ptex = c.health > 0 ? c.portrait : m->dead;
    portrait.x = card.x + UI_PADDING;
    portrait.y = card.y + UI_PADDING;
    portrait.width = card.width / 2.f;
    portrait.height = portrait.width;
    position.x = portrait.x + portrait.width + UI_PADDING;
    position.y = portrait.y;

    DrawTextureRec(m->vellum, card, (Vector2){card.x, card.y}, WHITE);

    { /* Information */
        char buffer[64];

        BeginScissorMode(card.x, card.y, card.width, card.height);
        ui_text(m->fonts.textB, c.name, position, m->fonts.textB.baseSize, 0, MINDAROGREEN);
        position.y += m->fonts.textB.baseSize + 2;
        DrawTextEx(m->fonts.text, CharacterClass_toString(c.class), position, m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);

        position.y += m->fonts.text.baseSize + UI_PADDING;
        snprintf(buffer, sizeof(buffer), "HP: %i/%i", c.health, char_maxHealth(c));
        DrawTextEx(m->fonts.text, buffer, position, m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);

        position.y += m->fonts.text.baseSize;
        snprintf(buffer, sizeof(buffer), "SP: %i/%i", c.stamina, char_maxStamina(c));
        DrawTextEx(m->fonts.text, buffer, position, m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);

        /* Temporary characteristic display TODO This will be replaced by orders and status symbols */
        position.y += m->fonts.text.baseSize;
        position.x = card.x + UI_PADDING;
        if (position.y < portrait.y + portrait.height + UI_PADDING)
            position.y = portrait.y + portrait.height + UI_PADDING;
        snprintf(buffer, sizeof(buffer), "STR: %i DEX: %i CON: %i", c.strength, c.dexterity, c.constitution);
        DrawTextEx(m->fonts.text, buffer, position, m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
        position.y += m->fonts.text.baseSize;
        snprintf(buffer, sizeof(buffer), "INT: %i WIL: %i CHA: %i", c.intellect, c.willpower, c.charisma);
        DrawTextEx(m->fonts.text, buffer, position, m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);

        EndScissorMode();
        ui_border(m->border, card, BONE);
    }

    /* Color lerping */
    color = ColorLerp(YELLOW, WHITE, Clamp((c.health * 1.3f) / (float)char_maxHealth(c), 0.f, 1.f));
    color = ColorLerp(MAROON, color, Clamp((c.health * 2.f) / (float)char_maxHealth(c), 0.f, 1.f));

    DrawTexturePro(ptex, (Rectangle){0, 0, ptex.width, ptex.height}, portrait, zero, 0.f, color);
    ui_border(m->border, portrait, BONE);
}

static int
ui_button(Rectangle rect, char* text, float fontSize, int hotkey, bool enabled)
{
    int result = 0;
    Vector2 measure;
    Vector2 position;

    { /* Input Checks */
        Vector2 mouse;

        if (!enabled)
            goto draw;

        if (m->ext.cimgui.handle && m->ext.cimgui.IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
            goto draw;

        mouse = GetMousePosition();
        if (mouse.x < 0 || mouse.y < 0 || mouse.x > GetScreenWidth() || mouse.y > GetScreenHeight())
            goto draw;

        if (IsKeyPressed(hotkey)) {
            result = 1;
            goto draw;
        } else if (IsKeyDown(hotkey)) {
            result = -1;
        }

        if (mouse.x < rect.x || mouse.y < rect.y || mouse.x > rect.x + rect.width || mouse.y > rect.y + rect.height)
            goto draw;

        result = -1;

        if (IsMouseButtonPressed(0)) {
            result = 1;
        }
    }

draw:
    measure = MeasureTextEx(m->fonts.textB, text, fontSize, 0);
    position.x = rect.x + rect.width / 2 - measure.x / 2;
    position.y = rect.y + rect.height / 2 - measure.y / 2;
    if (result && (IsMouseButtonDown(0) || IsKeyDown(hotkey))) { // Down
        Color fade = ColorLerp(MAROON, BLACK, 0.6f);
        position.y += 1;
        DrawTexturePro(m->marble, rect, rect, (Vector2){0, 0}, 0.f, fade);
        BeginScissorMode(rect.x, rect.y, rect.width, rect.height);
        ui_text(m->fonts.textB, text, position, fontSize, 0, MOSSGREEN);
        EndScissorMode();
        ui_border(m->border, rect, MINDAROGREEN);
    } else if (result < 0) { // Hover
        DrawTexturePro(m->marble, rect, rect, (Vector2){0, 0}, 0.f, MAROON);
        BeginScissorMode(rect.x, rect.y, rect.width, rect.height);
        ui_text(m->fonts.textB, text, position, fontSize, 0, MINDAROGREEN);
        EndScissorMode();
        ui_border(m->border, rect, MINDAROGREEN);
    } else { // Normal
        DrawTexturePro(m->marble, rect, rect, (Vector2){0, 0}, 0.f, MAROON);
        BeginScissorMode(rect.x, rect.y, rect.width, rect.height);
        ui_text(m->fonts.textB, text, position, fontSize, 0, BONE);
        EndScissorMode();
        ui_border(m->border, rect, BONE);
    }

    return result;
}
