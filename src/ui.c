static void
ui_text(Font font, const char* text, Vector2 position, Color tint, int spacing)
{
    Vector2 pos = position;
    Vector4 backColor = ColorNormalize(tint);
    backColor.x = 0.5f / 255.f;
    backColor.y = 0.5f / 255.f;
    backColor.z = 0.5f / 255.f;
    pos.x += 1.f;
    pos.y += 1.f;
    DrawTextEx(font, text, pos, font.baseSize, spacing, ColorFromNormalized(backColor));
    pos.x -= 1.f;
    pos.y -= 1.f;
    DrawTextEx(font, text, pos, font.baseSize, spacing, tint);
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
