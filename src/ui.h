static void
ui_text(Font font, const char* text, Vector2 position, Color tint)
{
    Vector2 pos = position;
    pos.x += 1.f;
    pos.x += 1.f;
    DrawTextEx(font, text, pos, font.baseSize, 0, BLACK);
    pos.x -= 1.f;
    pos.x -= 1.f;
    DrawTextEx(font, text, pos, font.baseSize, 0, tint);
}
