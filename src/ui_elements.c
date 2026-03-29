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

    rec = RectangleFloor(rec);
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
    m->logScrollSmooth += m->fonts.text.baseSize;
}

static int
ui_button(Rectangle rect, char* text, char* tooltip, int hotkey, bool enabled)
{
    int result = 0;
    Vector2 measure;
    Vector2 position;

    m->elementCount++;
    { /* Input Checks */
        Vector2 mouse;

        if (!enabled)
            goto draw;

        if (IsKeyPressed(hotkey)) {
            result = 1;
            goto draw;
        } else if (IsKeyDown(hotkey)) {
            result = -1;
        }

        if (util_mouseInRect(rect)) {
            if (tooltip)
                snprintf(m->tooltip, sizeof(m->tooltip), tooltip);
        } else {
            goto draw;
        }

        result = -1;

        if (IsMouseButtonPressed(0)) {
            result = 1;
        }
    }

draw:
    if (result > 0 && (m->flags & GlobalFlags_IgnoreInput)) {
        result = 0;
    }

    rect = RectangleFloor(rect);
    measure = MeasureTextEx(m->fonts.heading, text, m->fonts.heading.baseSize, 0);
    position.x = floorf(rect.x + rect.width / 2 - measure.x / 2);
    position.y = floorf(rect.y + rect.height / 2 - measure.y / 2);
    if (result && (IsMouseButtonDown(0) || IsKeyDown(hotkey))) { // Down
        Color fade = ColorLerp(MAROON, BLACK, 0.6f);
        position.y += 1;
        DrawTexturePro(m->textures.marble, rect, rect, (Vector2){0, 0}, 0.f, fade);
        BeginScissorMode(rect.x, rect.y, rect.width, rect.height);
        ui_text(m->fonts.heading, text, position, m->fonts.heading.baseSize, 0, MOSSGREEN);
        EndScissorMode();
        ui_border(m->textures.border, rect, MINDAROGREEN);
    } else if (result < 0) { // Hover
        DrawTexturePro(m->textures.marble, rect, rect, (Vector2){0, 0}, 0.f, MAROON);
        BeginScissorMode(rect.x, rect.y, rect.width, rect.height);
        ui_text(m->fonts.heading, text, position, m->fonts.heading.baseSize, 0, MINDAROGREEN);
        EndScissorMode();
        ui_border(m->textures.border, rect, MINDAROGREEN);
    } else { // Normal
        DrawTexturePro(m->textures.marble, rect, rect, (Vector2){0, 0}, 0.f, MAROON);
        BeginScissorMode(rect.x, rect.y, rect.width, rect.height);
        ui_text(m->fonts.heading, text, position, m->fonts.heading.baseSize, 0, BONE);
        EndScissorMode();
        ui_border(m->textures.border, rect, BONE);
    }

    if (result != 0)
        m->elementHover = m->elementCount;

    return result;
}

static void
ui_dumpCredits(void)
{
    ui_log(ZINNWALDITEBROWN, " ");
    ui_log(ZINNWALDITEBROWN, "CREDITS:");
    ui_log(ZINNWALDITEBROWN, " ");
    ui_log(ZINNWALDITEBROWN, "Programming, Design, 3D Models: Stomy (stomygame.itch.io)");
    ui_log(ZINNWALDITEBROWN, " ");
    ui_log(ZINNWALDITEBROWN, "Featuring public domain artwork by:");
    ui_log(ZINNWALDITEBROWN, " Sidney Sime, Harry Clarke, Alfred Kubin, Henry Justice Ford, and others");
    ui_log(ZINNWALDITEBROWN, " ");
    ui_log(ZINNWALDITEBROWN, "Music:");
    ui_log(ZINNWALDITEBROWN, " \"Specters of the Enclave\" by Eliot Corley from ChaosIsHarmony (CC-BY 3.0, opengameart.org)");
    ui_log(ZINNWALDITEBROWN, " \"Welt Herrscherer Theme\" by yd (CC0, opengameart.org)");
    ui_log(ZINNWALDITEBROWN, " \"Apologies to JSB\" by Yubatake (CC-BY 3.0, opengameart.org)");
    ui_log(ZINNWALDITEBROWN, " \"Lament for a Warrior's Soul\" by RandomMind (CC0, opengameart.org)");
    ui_log(ZINNWALDITEBROWN, " ");
    ui_log(ZINNWALDITEBROWN, "Ambient Track: \"Dark Ambient\" by Alexandr Zhelanov (CC-BY 3.0, opengameart.org)");
    ui_log(ZINNWALDITEBROWN, "   https://soundcloud.com/alexandr-zhelanov");
    ui_log(ZINNWALDITEBROWN, "Sound Effects from freesound.org. Artists:");
    ui_log(ZINNWALDITEBROWN, "   nomiqbomi, el_boss, tom_a73, kyles, Aerny");
    ui_log(ZINNWALDITEBROWN, " ");
    ui_log(ZINNWALDITEBROWN, "Delven Textures by Bradley D. (https://strideh.itch.io)");
    ui_log(ZINNWALDITEBROWN, "Torment Textures by Bradley D. (https://strideh.itch.io)");
    ui_log(ZINNWALDITEBROWN, "Additional CC0 textures by sean10m, Kenney");
    ui_log(ZINNWALDITEBROWN, "Stone Coffin model by Yughues");
}

static int
ui_characterHudCard(Character* ch, Rectangle card, int portraitSize, int hotkey)
{
    Rectangle portrait;
    Texture ptex;
    Vector2 zero = {};
    Vector2 position;
    Color color;
    int result;

    ptex = ch->health >= 0 ? ch->portrait : m->textures.dead;
    portrait.x = card.x + UI_PADDING;
    portrait.y = card.y + UI_PADDING;
    portrait.width = portraitSize;
    portrait.height = portrait.width;
    position.x = portrait.x + portrait.width + UI_PADDING;
    position.y = portrait.y;
    portrait.y -= (float)UI_PADDING * ch->effects.bumpSmooth;

    card = RectangleFloor(card);
    portrait = RectangleFloor(portrait);
    DrawTextureRec(m->textures.vellum, card, (Vector2){card.x, card.y}, WHITE);

    { /* Information */
        char buffer[64];

        BeginScissorMode(card.x, card.y, card.width, card.height);
        ui_text(m->fonts.heading, ch->name, Vector2Floor(position), m->fonts.heading.baseSize, 0, MINDAROGREEN);
        position.y += m->fonts.heading.baseSize + 2;

        snprintf(buffer, sizeof(buffer), "%s %u", CharacterClass_toString(ch->class), ch->level);
        DrawTextEx(m->fonts.text, buffer, Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
        position.y += m->fonts.text.baseSize + UI_PADDING;

        snprintf(buffer, sizeof(buffer), "HP: %i/%i", ch->health, char_maxHealth(*ch));
        DrawTextEx(m->fonts.text, buffer, Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
        position.y += m->fonts.text.baseSize;

        snprintf(buffer, sizeof(buffer), "SP: %i/%i", util_intmax(0, ch->stamina), char_maxStamina(*ch));
        DrawTextEx(m->fonts.text, buffer, Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
        position.y += m->fonts.text.baseSize;

        /* Characteristic display */
        if (ch->level < UINT8_MAX) {
            snprintf(buffer, sizeof(buffer), "XP: %llu/%llu", ch->experience,
                    char_levelRequirement(ch->class, ch->level + 1));
        } else {
            snprintf(buffer, sizeof(buffer), "XP: %llu", ch->experience);
        }
        DrawTextEx(m->fonts.text, buffer, Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
        position.y += m->fonts.text.baseSize;

        snprintf(buffer, sizeof(buffer), "STR: %i DEX: %i", ch->strength, ch->dexterity);
        DrawTextEx(m->fonts.text, buffer, Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);

        position.y += m->fonts.text.baseSize;
        snprintf(buffer, sizeof(buffer), "CON: %i INT: %i", ch->constitution, ch->intellect);
        DrawTextEx(m->fonts.text, buffer, Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);

        position.y += m->fonts.text.baseSize;
        snprintf(buffer, sizeof(buffer), "WIL: %i CHA: %i", ch->willpower, ch->charisma);
        DrawTextEx(m->fonts.text, buffer, Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
        position.y += m->fonts.text.baseSize;

        EndScissorMode();
        ui_border(m->textures.border, card, BONE);
    }

    { /* Action selection button */
        Rectangle button;
        Vector2 measure;
        Vector2 position;
        char buffer[16];
        char tooltip[48];

        button.x = portrait.x;
        button.y = portrait.y + portrait.height + UI_PADDING;
        button.height = 48;
        button.width = util_intmin(120, portrait.width);
        snprintf(buffer, sizeof(buffer), "ACTION");

        measure = MeasureTextEx(m->fonts.heading, buffer, m->fonts.heading.baseSize, 0);
        if (measure.x > button.width - UI_PADDING) {
            snprintf(buffer, sizeof(buffer), "ACT");
        }

        if (m->flags & GlobalFlags_Encounter) {
            snprintf(tooltip, sizeof(tooltip), "Cycle through combat actions");
        } else {
            snprintf(tooltip, sizeof(tooltip), "Cycle through wait activities");
        }

        if (hotkey != KEY_NULL) {
            int len = strlen(tooltip);
            snprintf(tooltip + len, sizeof(tooltip) - len, " [%i]", hotkey - KEY_ONE + 1);
        }

        result = ui_button(button, buffer, tooltip, hotkey, hotkey >= 0 && ch->health > 0);

        if (result > 0) {
            PlaySound(m->click);
            if (m->flags & GlobalFlags_Encounter) {
                switch (ch->class) {
                    default: break;
                    case CharacterClass_Warrior: {
                        if (ch->action == CombatAction_Attack) {
                            ch->action = CombatAction_DefendSelf;
                        } else if (ch->action == CombatAction_DefendSelf) {
                            ch->action = CombatAction_GuardOthers;
                        } else if (ch->action == CombatAction_GuardOthers) {
                            ch->action = CombatAction_MultiAttack;
                        } else {
                            ch->action = CombatAction_Attack;
                        }
                    } break;
                    case CharacterClass_Thief: {
                        if (ch->action == CombatAction_Attack) {
                            ch->action = CombatAction_DefendSelf;
                        } else if (ch->action == CombatAction_DefendSelf) {
                            ch->action = CombatAction_Hide;
                        } else {
                            ch->action = CombatAction_Attack;
                        }
                    } break;
                    case CharacterClass_Mage: {
                        if (ch->action == CombatAction_Attack) {
                            ch->action = CombatAction_DefendSelf;
                        } else if (ch->action == CombatAction_DefendSelf) {
                            ch->action = CombatAction_CastSpell;
                        } else {
                            ch->action = CombatAction_Attack;
                        }
                    } break;
                }

            /* Rest Activity */
            } else {
                ch->activity += 1;
                if (ch->activity == WaitActivity_TendWounds && ch->class != CharacterClass_Mage)
                    ch->activity += 1;
                ch->activity %= WaitActivity_Count;
            }
        }

        if (ch->health > 0) { /* Current Action */
            char buffer[64];
            position.x = button.x;
            position.y = card.y + card.height - UI_PADDING - m->fonts.text.baseSize;
            bool tooltip = util_mouseInRect((Rectangle){position.x, position.y, card.width - UI_PADDING * 2, m->fonts.text.baseSize});

            if (m->flags & GlobalFlags_Encounter) {
                switch (ch->action) {
                    case CombatAction_Attack: {
                        DrawTextEx(m->fonts.text, CombatAction_toStringFancy(ch->action), Vector2Floor(position),
                                    m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "Attack a random enemy");
                    } break;

                    case CombatAction_MultiAttack: {
                        DrawTextEx(m->fonts.text, CombatAction_toStringFancy(ch->action), Vector2Floor(position),
                                    m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "Will attack multiple times against enemies of lower level");
                    } break;

                    case CombatAction_DefendSelf: {
                        DrawTextEx(m->fonts.text, CombatAction_toStringFancy(ch->action), Vector2Floor(position),
                                    m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "Increases defense and reduces chance of being targeted");
                    } break;

                    case CombatAction_GuardOthers: {
                        DrawTextEx(m->fonts.text, CombatAction_toStringFancy(ch->action), Vector2Floor(position),
                                    m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "Increases defense slightly and increases chance of being targeted (scales with CHA)");
                    } break;

                    case CombatAction_Hide: {
                        snprintf(buffer, sizeof(buffer), "%s (%i%%)",
                                    CombatAction_toStringFancy(ch->action), char_hideChance(ch));
                        DrawTextEx(m->fonts.text, buffer, Vector2Floor(position),
                                    m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "Attempt to hide in shadows (base chance of success)");
                    } break;

                    case CombatAction_CastSpell: {
                        DrawTextEx(m->fonts.text, CombatAction_toStringFancy(ch->action), Vector2Floor(position),
                                    m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "Strike multiple enemies with lighting (scales with WIL & CHA, costs a lot of SP)");
                    } break;

                    default: {
                        DrawTextEx(m->fonts.text, CombatAction_toStringFancy(ch->action), Vector2Floor(position),
                                    m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "TODO: This action has no tooltip yet");
                    } break;
                }

            } else {
                switch (ch->activity) {
                    case WaitActivity_Rest: {
                        DrawTextEx(m->fonts.text, WaitActivity_toStringFancy(ch->activity),
                                    Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "Regain additional SP and (if SP is full) HP by resting");
                    } break;

                    case WaitActivity_Guard: {
                        DrawTextEx(m->fonts.text, WaitActivity_toStringFancy(ch->activity),
                                    Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "Stand guard while waiting (less likely to be taken by surprise)");
                    } break;

                    case WaitActivity_Hide: {
                        int chance = char_hideChance(ch);
                        if (ch->class == CharacterClass_Thief)
                            chance += ch->level;
                        snprintf(buffer, sizeof(buffer), "%s (%i%%)",
                                    WaitActivity_toStringFancy(ch->activity), util_intmax(0, chance));
                        DrawTextEx(m->fonts.text, buffer, Vector2Floor(position),
                                    m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "Hide in ambush while waiting (base chance of success)");
                    } break;

                    case WaitActivity_TendWounds: {
                        DrawTextEx(m->fonts.text, WaitActivity_toStringFancy(ch->activity),
                                    Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "Restore health points by tending to wounds");
                    } break;

                    default: {
                        DrawTextEx(m->fonts.text, WaitActivity_toStringFancy(ch->activity),
                                    Vector2Floor(position), m->fonts.text.baseSize, 0, ZINNWALDITEBROWN);
                        if (tooltip)
                            snprintf(m->tooltip, sizeof(m->tooltip), "TODO: This activity has no tooltip yet");
                    } break;
                }
            }
        }
    }

    { /* Portrait */
        Vector2 position;
        color = WHITE;
        Rectangle tooltip;

        /* Color lerping  */
        if (ch->health > 0) {
            if ((m->flags & GlobalFlags_Encounter) && (ch->flags & CharacterFlags_Surprised))
                color = ColorLerp(color, PINK, 0.5f);
            if ((m->flags & GlobalFlags_Encounter) && (ch->flags & CharacterFlags_Hidden))
                color = ColorLerp(color, DARKBLUE, 0.5f);
            if (ch->stamina <= 0)
                color = ColorLerp(color, MOSSGREEN, 0.5f);
        }

        color = ColorLerp(YELLOW, color, Clamp((ch->health * 1.1f) / (float)char_maxHealth(*ch), 0.f, 1.f));
        color = ColorLerp(MAROON, color, Clamp((ch->health * 2.f) / (float)char_maxHealth(*ch), 0.f, 1.f));

        /* TODO Shake/Zoom using scaled sinwave */

        DrawTexturePro(ptex, (Rectangle){0, 0, ptex.width, ptex.height}, portrait, zero, 0.f, color);

        /* Write status effects over the portrait */
        BeginScissorMode(portrait.x, portrait.y, portrait.width, portrait.height);
        position.x = portrait.x + UI_PADDING;
        position.y = portrait.y + UI_PADDING;
        tooltip.x = position.x;
        tooltip.y = position.y;
        tooltip.width = portrait.width - UI_PADDING * 2;
        tooltip.height = m->fonts.text.baseSize;
        if (ch->health == 0) {
            ui_text(m->fonts.text, "Unconscious", Vector2Floor(position), m->fonts.text.baseSize, 0, MAROON);
            if (util_mouseInRect(tooltip))
                snprintf(m->tooltip, sizeof(m->tooltip), "Can be revived with healing or if SP restores to full");
            position.y += m->fonts.text.baseSize;
            tooltip.y += m->fonts.text.baseSize;
        } else if (ch->health < 0) {
            ui_text(m->fonts.text, "DEAD", Vector2Floor(position), m->fonts.text.baseSize, 0, MAROON);
            if (util_mouseInRect(tooltip))
                snprintf(m->tooltip, sizeof(m->tooltip), "Rest In Peace");
            position.y += m->fonts.text.baseSize;
            tooltip.y += m->fonts.text.baseSize;
        } else {
            if ((m->flags & GlobalFlags_Encounter) && (ch->flags & CharacterFlags_Surprised)) {
                ui_text(m->fonts.text, "Surprised", Vector2Floor(position), m->fonts.text.baseSize, 0, PINK);
                if (util_mouseInRect(tooltip))
                    snprintf(m->tooltip, sizeof(m->tooltip), "Cannot act this round. Reduced chance to flee unharmed");
                position.y += m->fonts.text.baseSize;
                tooltip.y += m->fonts.text.baseSize;
            }

            if ((m->flags & GlobalFlags_Encounter) && (ch->flags & CharacterFlags_Hidden)) {
                ui_text(m->fonts.text, "Hidden", Vector2Floor(position), m->fonts.text.baseSize, 0, DARKBLUE);
                if (util_mouseInRect(tooltip)) {
                    if (ch->class == CharacterClass_Thief) {
                        snprintf(m->tooltip, sizeof(m->tooltip), "Cannot be attacked. Attacks are more likely to hit and always deal critical damage");
                    } else {
                        snprintf(m->tooltip, sizeof(m->tooltip), "Cannot be attacked. Attacks are more likely to hit");
                    }
                }
                position.y += m->fonts.text.baseSize;
                tooltip.y += m->fonts.text.baseSize;
            }

            if (ch->stamina <= 0) {
                ui_text(m->fonts.text, "Exhausted", Vector2Floor(position), m->fonts.text.baseSize, 0, MOSSGREEN);
                if (util_mouseInRect(tooltip))
                    snprintf(m->tooltip, sizeof(m->tooltip), "All actions have reduced chance to succeed. Further SP expenditure may cause damage");
                position.y += m->fonts.text.baseSize;
                tooltip.y += m->fonts.text.baseSize;
            }
        }
        EndScissorMode();

        ui_border(m->textures.border, portrait, BONE);
    }

    if (ch->effects.flash > 0.f) {
        Color fcolor = ColorAlpha(ch->effects.color, ch->effects.flash);
        DrawTexturePro(m->textures.flash, (Rectangle){0, 0, m->textures.flash.width, m->textures.flash.height},
                        card, zero, 0.f, fcolor);
    }

    PortraitEffects_update(&ch->effects, GetFrameTime());

    return result;
}

static void
ui_tooltipPane(void)
{
    Rectangle pane;
    Vector2 position;
    const int EXTRA_PADDING = 16;

    pane.width = GetRenderWidth() + EXTRA_PADDING * 2;
    pane.height = m->fonts.text.baseSize + UI_PADDING * 2 + EXTRA_PADDING;
    pane.x = -EXTRA_PADDING;
    pane.y = GetRenderHeight() - (m->fonts.text.baseSize + UI_PADDING * 2);
    DrawTexturePro(m->textures.marble, pane, pane, (Vector2){0,0}, 0.f, DARKBROWN);

    position.x = UI_PADDING * 2;
    position.y = GetRenderHeight() - m->fonts.text.baseSize - UI_PADDING;
    if (m->tooltip[0])
        ui_text(m->fonts.text, m->tooltip, position, m->fonts.text.baseSize, 0, BONE);

    ui_border(m->textures.border, pane, BONE);
}
