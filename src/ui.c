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

    DrawTexturePro(c.portrait, (Rectangle){0, 0, c.portrait.width, c.portrait.height},
        portrait, zero, 0.f, WHITE);
    ui_border(m->border, portrait, BONE);
}

