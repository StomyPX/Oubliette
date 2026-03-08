/* Standard Library */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* External Libraries */
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS 1
#define CIMGUI_NO_EXPORT 1
#include "cimgui.h"

#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"
#include "rlgl.h"

#define MST_RANDOM_IMPL 1
#define MST_RANDOM_API static
#include "mst_random.h"

#if PLATFORM_LINUX
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
    #include <alloca.h>
#define PHYSFS_IMPL
#define PHYSFS_PLATFORM_IMPL
#define PHYSFS_SUPPORTS_ONLY_ZIP
#include "miniphysfs.h"
#pragma GCC diagnostic pop
#else
#include "miniphysfs.h"
#endif

#define JSON_PARSE_FLAGS ( json_parse_flags_allow_trailing_comma \
                         | json_parse_flags_allow_unquoted_keys \
                         | json_parse_flags_allow_global_object \
                         | json_parse_flags_allow_equals_in_object \
                         | json_parse_flags_allow_c_style_comments \
                         | json_parse_flags_allow_hexadecimal_numbers \
                         | json_parse_flags_allow_leading_plus_sign \
                         | json_parse_flags_allow_leading_or_trailing_decimal_point \
                         | json_parse_flags_allow_inf_and_nan \
                         | json_parse_flags_allow_multi_line_strings )
#include "json.h"

#define INI_IMPLEMENTATION 1
#include <ini.h>

/* Headers (order matters) */
#include "global.h"
#include "util.h"
#include "platform.h"
#include "ext.h"
#include "map.h"
#include "char.h"
#include "ui.h"
#include "monster.h"
#include "combat.h"
#include "main.h"

/* Sources (order doesn't matter) */
#include "char.c"
#include "combat.c"
#include "ext.c"
#include "map.c"
#include "monster.c"
#include "platform.c"
#include "ui.c"
#include "util.c"

int
main(int argc, char* argv[])
{
    PHYSFS_init(argv[0]);
    PHYSFS_permitSymbolicLinks(1);
    PHYSFS_setSaneConfig("StomyGame", GAME_NAME, "pk3", 0, 0);

    util_logInit();

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    SetTraceLogCallback(util_trace);
    /* TODO Set Raylib file callbacks */
    SetLoadFileDataCallback(util_readFileData);
    SetSaveFileDataCallback(util_writeFileData);
    SetLoadFileTextCallback(util_readFileText);
    SetSaveFileTextCallback(util_writeFileText);
    #if DEBUG_MODE
    InitWindow(1024, 768, GAME_NAME);
    #else
    InitWindow(1280, 720, GAME_NAME);
    #endif
    InitAudioDevice();
    SetExitKey(KEY_NULL);
    SetTraceLogLevel(LOG_ALL);

    m = RL_MALLOC(sizeof(Memory));
    memset(m, 0, sizeof(Memory));

    /* TODO Always on in debug mode, requires cmdline switch "--editor" in release mode */
    #if DEBUG_MODE
        //m->flags |= GlobalFlags_PartyStats;
        m->flags |= GlobalFlags_EditorModePermitted;
        //m->flags |= GlobalFlags_ShowCollision;
    #endif

    ext_init(&m->ext);
    PcgRandom_init(&m->rng, util_rdtsc());
    PcgRandom_init(&m->rng2, util_rdtsc());
    monster_init(&m->monsters);

    for (unsigned i = 0; i < arrlen(m->footstep); i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "data/sounds/footstep_%02u.wav", i);
        m->footstep[i] = LoadSound(buf);
    }

    m->fonts.text = LoadFontEx("data/fonts/CrimsonText-Bold.ttf", 24, 0, 0);
    m->fonts.heading = LoadFontEx("data/fonts/CoelacanthBold.otf", 30, 0, 0);
    m->fonts.title = LoadFontEx("data/fonts/Coelacanth.otf", 64, 0, 0);
    m->fonts.big = LoadFontEx("data/fonts/Coelacanth.otf", 200, 0, 0);

    m->encounter.klaxon = LoadSound("data/sounds/encounter_bell.wav");

    m->border = LoadTexture("data/textures/panel-border.png");
    m->marble = LoadTexture("data/textures/Marble023B.png");
    m->vellum = LoadTexture("data/textures/parchment_2.png");
    m->dead = LoadTexture("data/textures/dead.jpg");
    m->hover = LoadSound("data/sounds/button_hover.wav");
    m->click = LoadSound("data/sounds/button_click.wav");
    m->click2 = LoadSound("data/sounds/button_click2.wav");

    m->ambient = LoadMusicStream("darkambient.mp3");
    SetMusicVolume(m->ambient, 0.3f);
    PlayMusicStream(m->ambient);
    m->music = LoadMusicStream("weltherrscherer.ogg");
    PlayMusicStream(m->music);

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

    /* TODO Auto-load all zip files found in data/ */

    Vector2 dragStart;

    map_generate(&m->map, util_rdtsc());

    m->partyFacing = 2; // Party always enters facing South
    m->partyX = m->map.entryX;
    m->partyY = m->map.entryY + 1;
    m->partyMoveFreq = 0.2f;

    m->camera = map_cameraForTile(m->partyX, m->partyY, m->partyFacing);

    SetTargetFPS(200); /* TODO Make configurable, prefer VSync */

    /* Clear out all of Raylib's noise */
    for (unsigned i = 0; i < UTIL_LOGLINE_COUNT; i++) {
        if (g_util_logLines[i].seconds > 0.f && g_util_logLines[i].channel >= 0)
            g_util_logLines[i].seconds = 0.f;
    }

    ui_log(ZINNWALDITEBROWN, "Oubliette, Version 1.1");

    int lastHover = 0;
    while (!WindowShouldClose() && !(m->flags & GlobalFlags_RequestQuit)) {
        /* Update */
        m->deltaTime = GetFrameTime();
        m->second += m->deltaTime;
        int anyHover = 0;

        UpdateMusicStream(m->ambient);
        UpdateMusicStream(m->music);

        if (IsKeyPressed(KEY_F4) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
            m->flags |= GlobalFlags_RequestQuit;

        if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))) {
            ToggleBorderlessWindowed();
            TraceLog(LOG_TRACE, "Toggled Borderless Fullscreen");
        }

        if (IsKeyPressed(KEY_KP_ENTER))
            util_clearLog();

        if (m->flags & GlobalFlags_EditorModePermitted) {
            if (IsKeyPressed(KEY_F1))
                m->flags ^= GlobalFlags_EditorMode;
            if (IsKeyPressed(KEY_F2))
                m->flags ^= GlobalFlags_PartyStats;
            if (IsKeyPressed(KEY_F3))
                m->flags ^= GlobalFlags_ShowCollision;
            if (IsKeyPressed(KEY_F4))
                m->flags ^= GlobalFlags_ShowMap;
            if (IsKeyPressed(KEY_F5)) {
                for (int i = 0; i < arrlen(m->party); i++) {
                    m->party[i].health = char_maxHealth(m->party[i]);
                    m->party[i].stamina = char_maxStamina(m->party[i]);
                }
            }
            if (IsKeyPressed(KEY_F6)) {
                for (int i = 0; i < arrlen(m->party); i++) {
                    char_exp(m->party + i, 1000);
                }
            }
            if (IsKeyPressed(KEY_F7))
                m->flags ^= GlobalFlags_IgnoreEncounters;
            if (IsKeyPressed(KEY_F8))
                m->flags ^= GlobalFlags_ShowTileFlags;
        }

        if (m->flags & GlobalFlags_EditorMode) {
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                dragStart = GetMousePosition();
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
                SetMousePosition(dragStart.x, dragStart.y);
            }
        }

        /* Rendering */

        /* Determine GUI layout proportions */
        Rectangle viewport, card, panel;
        GuiLayout layout;
        int portraitSize = 145;
        int aspect = (float)GetRenderWidth() / (float)GetRenderHeight() * 100;
        {
            if (aspect > 240) {
                m->area.width = GetRenderHeight() * 2.4f;
                m->area.height = GetRenderHeight();
                m->area.top = 0;
                m->area.left = (GetRenderWidth() - m->area.width) / 2;
            } else if (aspect < 100) {
                m->area.width = GetRenderWidth();
                m->area.height = GetRenderHeight();
                m->area.top = (GetRenderHeight() - m->area.height) / 2;
                m->area.left = 0;
            } else {
                m->area.width = GetRenderWidth();
                m->area.height = GetRenderHeight();
                m->area.top = 0;
                m->area.left = 0;
            }
            m->area.bottom = m->area.top + m->area.height;
            m->area.right = m->area.left + m->area.width;

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
            if (aspect > 180 && viewport.height >= card.height * 4 + UI_PADDING * 3) {
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
            } else if (aspect < 120) {
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
                DrawGrid(TILE_COUNT + 1, TILE_SIDE_LENGTH);

            if (m->flags & GlobalFlags_ShowCollision) {
                // TODO Draw inward in a spiral pattern
                float size = TILE_SIDE_LENGTH + 0.05f;
                for (int x = 0; x < TILE_COUNT; x++) {
                    for (int y = 0; y < TILE_COUNT; y++) {
                        if (abs(x - m->partyX) > 4 || abs(y - m->partyY) > 4)
                            continue;

                        int index = x + y * TILE_COUNT;
                        if (!(m->map.tiles[index] & TileFlags_AllowEntry)) {
                            Vector3 position = map_tileCenter(x, y);
                            DrawCube(position, size, size, size, CLIP_COLOR);
                            DrawCubeWires(position, size, size, size, LIGHTGRAY);
                        }

                        if (!(m->map.tiles[index] & TileFlags_AllowEast)) {
                            Vector3 position = map_tileCenter(x, y);
                            position.x += (float)TILE_SIDE_LENGTH / 2.f;
                            DrawCube(position, 0.1f, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, WALLCLIP_COLOR);
                            DrawCubeWires(position, 0.1f, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, LIGHTGRAY);
                        }

                        if (!(m->map.tiles[index] & TileFlags_AllowSouth)) {
                            Vector3 position = map_tileCenter(x, y);
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
                c = map_cameraForTile(m->partyX, m->partyY, m->partyFacing);

                start = map_tileCenter(m->partyX, m->partyY);
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
            DrawTextureRec(m->marble, (Rectangle){0, 0, GetRenderWidth(), GetRenderHeight()}, (Vector2){0, 0}, DARKBROWN);

            if (m->flags & GlobalFlags_Encounter) {
                Texture* tex;
                Rectangle portrait = {};
                char buffer[128];
                Unit* unit;

                unit = &m->encounter.unit;
                tex = &m->encounter.unit.class.texture;

                portrait.width = tex->width;
                portrait.height = tex->height;
                DrawRectangleRec(viewport, BLACK);

                { /* Aspect Correction */
                    float vpAspect, rtAspect;
                    vpAspect = (float)viewport.width / (float)viewport.height;
                    rtAspect = (float)tex->width / (float)tex->height;

                    if (fabsf(vpAspect - rtAspect) > 0.001f) {
                        float diff = rtAspect - vpAspect;
                        Vector2 anchor = unit->class.anchor;
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

                { /* Combat UI */
                    Vector2 position, dims;
                    Rectangle button, frame;
                    int result;
                    bool active = !(m->flags & GlobalFlags_GameOver);

                    if (unit->alive == 1) {
                        snprintf(buffer, sizeof(buffer), unit->class.truename);
                    } else {
                        snprintf(buffer, sizeof(buffer), "%u %s", unit->alive, unit->class.truenamePlural);
                    }
                    dims = MeasureTextEx(m->fonts.title, buffer, m->fonts.title.baseSize, 0);
                    frame.x = viewport.x + UI_PADDING;
                    frame.y = viewport.y + UI_PADDING;
                    frame.width = dims.x + UI_PADDING * 4;
                    frame.height = dims.y + UI_PADDING * 2;
                    DrawRectangleRec(frame, ColorAlpha(BLACK, 0.5f));
                    ui_border(m->border, frame, BONE);

                    position.x = frame.x + UI_PADDING * 2;
                    position.y = frame.y + UI_PADDING;
                    ui_text(m->fonts.title, buffer, position, m->fonts.title.baseSize, 0, MINDAROGREEN);

                    /* FIGHT! */
                    button.width = 120;
                    button.height = 48;
                    button.x = viewport.x + UI_PADDING;
                    button.y = viewport.y + viewport.height - UI_PADDING - button.height;
                    result = ui_button(button, "FIGHT", KEY_F, active);
                    if (result > 0) {
                        PlaySound(m->click);
                        combat_fight();
                    } else if (result < 0) {
                        anyHover = 2;
                    }

                    button.x += button.width + UI_PADDING;
                    result = ui_button(button, "FLEE", KEY_R, active);
                    if (result > 0) {
                        PlaySound(m->click);
                        combat_flee();
                    } else if (result < 0) {
                        anyHover = 1;
                    }
                }

            } else {
                Rectangle button;
                int result;

                /* Map viewport. Because OpenGL's origin is in the bottom left, this has to be inverted */
                DrawTextureRec(m->rtex.texture,
                    (Rectangle){0, 0, m->rtex.texture.width, -m->rtex.texture.height},
                    (Vector2){viewport.x, viewport.y}, WHITE);

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
                result = ui_button(button, "REST", KEY_R, !(m->flags & GlobalFlags_TheEnd));
                if (result > 0) {
                    ui_log(ZINNWALDITEBROWN, "Resting...");
                    m->encounter.ticks += 300;
                    m->flags |= GlobalFlags_AdvanceTurn | GlobalFlags_Resting;

                    for (int i = 0; i < arrlen(m->party); i++) {
                        Character* ch = m->party + i;
                        if (!ch->name[0] || ch->health < 1)
                            continue;

                        if (ch->activity == RestActivity_Rest) {
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
                        } else if (ch->activity == RestActivity_TendWounds) {
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
                } else if (result < 0) {
                    anyHover = 3;
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

                    /* TODO switch to using util_traverse */
                    if (direction >= 0) {
                        unsigned targetTile, currentTile;
                        bool successful = false;

                        currentTile = m->partyX + m->partyY * TILE_COUNT;
                        switch (direction % 4) {
                            case 0: { /* North */
                                targetTile = m->partyX + (m->partyY - 1) * TILE_COUNT;
                                if (targetTile < TILE_COUNT * TILE_COUNT
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
                                targetTile = m->partyX + 1 + m->partyY * TILE_COUNT;
                                if (targetTile < TILE_COUNT * TILE_COUNT
                                    && m->map.tiles[targetTile] & TileFlags_AllowEntry
                                    && m->map.tiles[currentTile] & TileFlags_AllowEast
                                    && m->partyX + 1 < TILE_COUNT)
                                {
                                    m->partyX += 1;
                                    successful = true;
                                } else {
                                    m->camera.position.x += 0.1f;
                                }
                            } break;

                            case 2: { /* South */
                                targetTile = m->partyX + (m->partyY + 1) * TILE_COUNT;
                                if (targetTile < TILE_COUNT * TILE_COUNT
                                    && m->map.tiles[targetTile] & TileFlags_AllowEntry
                                    && m->map.tiles[currentTile] & TileFlags_AllowSouth
                                    && m->partyY + 1 < TILE_COUNT)
                                {
                                    m->partyY += 1;
                                    successful = true;
                                } else {
                                    m->camera.position.z += 0.1f;
                                }
                            } break;

                            case 3: { /* West */
                                targetTile = m->partyX - 1 + m->partyY * TILE_COUNT;
                                if (targetTile < TILE_COUNT * TILE_COUNT
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
                            } else if ((m->flags & GlobalFlags_MissionAccomplished)
                                        && m->partyX == m->map.entryX && m->partyY == m->map.entryY)
                            {
                                m->flags |= GlobalFlags_TheEnd;
                                m->encounter.ticks = 0;
                                ui_log(MINDAROGREEN, "You climb out of the Oubliette, to a new and uncertain future...");
                                ui_dumpCredits();
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

                    int index = map_tileIndex(m->partyX, m->partyY);
                    if (index >= 0) {
                        m->map.tiles[index] |= TileFlags_Visited;
                    }
                }
            }

            ui_border(m->border, viewport, BONE);

            { /* Options */
                Rectangle button;
                button = panel;
                button.width = 140;
                button.height = 48;
                int result;

                if (IsMusicStreamPlaying(m->music)) {
                    result = ui_button(button, "MUSIC", KEY_NULL, true);
                    if (result > 0) {
                        StopMusicStream(m->music);
                        PlaySound(m->click);
                    } else if (result < 0) {
                        anyHover = 8;
                    }
                } else {
                    result = ui_button(button, "(music)", KEY_NULL, true);
                    if (result > 0) {
                        PlayMusicStream(m->music);
                        PlaySound(m->click);
                    } else if (result < 0) {
                        anyHover = 8;
                    }
                }

                button.y += button.height + UI_PADDING;

                if (IsMusicStreamPlaying(m->ambient)) {
                    result = ui_button(button, "AMBIENCE", KEY_NULL, true);
                    if (result > 0) {
                        StopMusicStream(m->ambient);
                        PlaySound(m->click);
                    } else if (result < 0) {
                        anyHover = 9;
                    }
                } else {
                    result = ui_button(button, "(ambience)", KEY_NULL, true);
                    if (result > 0) {
                        PlayMusicStream(m->ambient);
                        PlaySound(m->click);
                    } else if (result < 0) {
                        anyHover = 9;
                    }
                }

                button.y = panel.y;
                button.x = panel.x + panel.width - button.width;

                if (m->flags & GlobalFlags_ConfirmExit) {
                    result = ui_button(button, "CONFIRM?", KEY_NULL, true);
                    if (result > 0) {
                        m->flags |= GlobalFlags_RequestQuit;
                    } else  if (result < 0) {
                        anyHover = 10;
                    } else {
                        m->flags &= ~(GlobalFlags_ConfirmExit);
                    }
                } else {
                    result = ui_button(button, "EXIT", KEY_NULL, true);
                    if (result > 0) {
                        m->flags |= GlobalFlags_ConfirmExit;
                        PlaySound(m->click2);
                    } else  if (result < 0) {
                        anyHover = 10;
                    }
                }

                button.y += button.height + UI_PADDING;

                if (m->flags & GlobalFlags_MuteSFX) {
                    result = ui_button(button, "(sfx)", KEY_NULL, true);
                    if (result > 0) {
                        m->flags &= ~(GlobalFlags_MuteSFX);
                        /* I realize there is probably a better way */
                        for (int i = 0; i < arrlen(m->footstep); i++)
                            SetSoundVolume(m->footstep[i], 1.f);
                        SetSoundVolume(m->encounter.klaxon, 1.f);
                        SetSoundVolume(m->hover, 1.f);
                        SetSoundVolume(m->click, 1.f);
                        SetSoundVolume(m->click2, 1.f);
                        PlaySound(m->click);
                    } else  if (result < 0) {
                        anyHover = 11;
                    }
                } else {
                    result = ui_button(button, "SFX", KEY_NULL, true);
                    if (result > 0) {
                        m->flags |= GlobalFlags_MuteSFX;
                        for (int i = 0; i < arrlen(m->footstep); i++)
                            SetSoundVolume(m->footstep[i], 0.f);
                        SetSoundVolume(m->encounter.klaxon, 0.f);
                        SetSoundVolume(m->hover, 0.f);
                        SetSoundVolume(m->click, 0.f);
                        SetSoundVolume(m->click2, 0.f);
                    } else  if (result < 0) {
                        anyHover = 11;
                    }
                }

                if (m->map.name[0]) {
                    Vector2 position;
                    Vector2 dimensions = MeasureTextEx(m->fonts.title, m->map.name, m->fonts.title.baseSize, 0);
                    position.x = panel.x + panel.width / 2 - dimensions.x / 2;
                    position.y = panel.y + UI_SIDE_PANEL_HEADER / 2.f - dimensions.y / 2.f;
                    ui_text(m->fonts.title, m->map.name, Vector2Floor(position), m->fonts.title.baseSize, 0, BONE);
                }

                panel.height -= UI_SIDE_PANEL_HEADER;
                panel.y += UI_SIDE_PANEL_HEADER;
            }

            { /* Side Panel */
                Vector2 position;
                int count = 0;
                int scrollMax = 0;
                int visible;

                position.x = panel.x + UI_PADDING;
                position.y = panel.y + UI_PADDING;
                visible = panel.height / m->fonts.text.baseSize;

                DrawTextureRec(m->vellum, panel, (Vector2){panel.x, panel.y}, WHITE);
                BeginScissorMode(panel.x, panel.y, panel.width, panel.height);

                /* Find scroll point first */
                for (unsigned i = 0; i < UI_LOGLINE_COUNT; i++) {
                    unsigned index = (i + m->logCursor + 1) % UI_LOGLINE_COUNT;
                    if (m->logs[index].text[0]) {
                        count++;
                    }
                }

                scrollMax = count - visible;
                if (scrollMax < 0) {
                    scrollMax = 0;
                } else {
                    position.y -= scrollMax * m->fonts.text.baseSize;
                    position.y -= (int)panel.height % m->fonts.text.baseSize;
                }

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
                ui_border(m->border, panel, BONE);
            }

            { /* Character Cards */
                bool active = !(m->flags & (GlobalFlags_GameOver | GlobalFlags_TheEnd));
                if (ui_characterHudCard(m->party + 0, card, portraitSize, active ? KEY_ONE : -1))
                    anyHover = 4;

                if (layout == GuiLayout_Ultrawide) {
                    card.y += card.height + UI_PADDING;
                } else {
                    card.x += card.width + UI_PADDING;
                }
                if (ui_characterHudCard(m->party + 1, card, portraitSize, active ? KEY_TWO : -1))
                    anyHover = 5;

                if (layout == GuiLayout_Ultrawide) {
                    card.y += card.height + UI_PADDING;
                } else {
                    card.x += card.width + UI_PADDING;
                }
                if (ui_characterHudCard(m->party + 2, card, portraitSize, active ? KEY_THREE : -1))
                    anyHover = 6;

                if (layout == GuiLayout_Ultrawide) {
                    card.y += card.height + UI_PADDING;
                } else {
                    card.x += card.width + UI_PADDING;
                }
                if (ui_characterHudCard(m->party + 3, card, portraitSize, active ? KEY_FOUR : -1))
                    anyHover = 7;
            }

            if (m->flags & GlobalFlags_GameOver) {
                Vector2 position;
                Vector2 measure;
                Rectangle frame;

                measure = MeasureTextEx(m->fonts.big, "GAME OVER", m->fonts.big.baseSize, 0);
                frame.x = GetRenderWidth() / 2 - measure.x / 2 - UI_PADDING * 2;
                frame.y = GetRenderHeight() / 2 - measure.y / 2 - UI_PADDING;
                frame.width = measure.x + UI_PADDING * 4;
                frame.height = measure.y + UI_PADDING * 2;
                frame = RectangleFloor(frame);
                DrawRectangleRec(frame, ColorAlpha(BLACK, 0.8f));
                ui_border(m->border, frame, BONE);
                position.x = frame.x + UI_PADDING * 2;
                position.y = frame.y + UI_PADDING;
                ui_text(m->fonts.big, "GAME OVER", Vector2Floor(position), m->fonts.big.baseSize, 0, MAROON);
            } else if (m->flags & GlobalFlags_TheEnd) {
                Vector2 position;
                Vector2 measure;
                Rectangle frame;

                measure = MeasureTextEx(m->fonts.big, "THE END", m->fonts.big.baseSize, 0);
                frame.x = GetRenderWidth() / 2 - measure.x / 2 - UI_PADDING * 2;
                frame.y = GetRenderHeight() / 2 - measure.y / 2 - UI_PADDING;
                frame.width = measure.x + UI_PADDING * 4;
                frame.height = measure.y + UI_PADDING * 2;
                frame = RectangleFloor(frame);
                DrawRectangleRec(frame, ColorAlpha(BLACK, 0.8f));
                ui_border(m->border, frame, BONE);
                position.x = frame.x + UI_PADDING * 2;
                position.y = frame.y + UI_PADDING;
                ui_text(m->fonts.big, "THE END", position, m->fonts.big.baseSize, 0, MINDAROGREEN);
            }

            if (m->flags & GlobalFlags_PartyStats) {
                char buf[160] = {0};
                Vector2 position = (Vector2){GetRenderWidth() - 160, UI_SIDE_PANEL_HEADER + UI_PADDING * 2};
                int danger = -1;
                if (m->flags & GlobalFlags_MissionAccomplished) {
                    danger = 1;
                //} else if (abs(m->partyX - m->map.entryX) + abs(m->partyY - m->map.entryY) > TILE_COUNT / 3) {
                    //danger = 0;
                }

                snprintf(buf, sizeof(buf),
                        "FPS: %3i\n"
                        //"Debug View: %s\n"
                        "Camera: %4.1f, %4.1f\n"
                        "Tile: %2i, %2i [%i]\n"
                        "Enc. Ticks: %i/%i %s\n"
                        "Dungeon Lv%i Size %i\n"
                        "Seed: %llu\n",
                        GetFPS(),
                        //m->flags & GlobalFlags_EditorMode ? "ON" : "off",
                        m->camera.position.x, m->camera.position.z,
                        m->partyX, m->partyY, map_tileIndex(m->partyX, m->partyY),
                        m->encounter.ticks, m->map.encounterFreq,
                        m->flags & GlobalFlags_IgnoreEncounters ? "OFF" : "",
                        danger + 2, m->map.chamberCount,
                        m->map.seed);
                ui_text(GetFontDefault(), buf, position, GetFontDefault().baseSize, 1, BONE);
            }

            if (m->ext.cimgui.handle && (m->flags & GlobalFlags_EditorModePermitted)) {
                ext_CImguiNewFrame(m);
                if (m->flags & GlobalFlags_ShowMap) {
                    bool show = 1;
                    char buffer[TILE_COUNT * 2 + 1] = {};

                    {
                        ImGuiWindowFlags flags = 0;
                        flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
                        flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
                        m->ext.cimgui.Begin("Map Overview", &show, flags);
                    }

                    for (int y = 0; y < TILE_COUNT; y++) {
                        for (int x = 0; x < TILE_COUNT; x++) {
                            TileFlags t = m->map.tiles[x + y * TILE_COUNT];

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

            util_drawLog();
        }
        EndDrawing();

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
                            ch->health -= 1;
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
                Camera intended = map_cameraForTile(m->partyX, m->partyY, m->partyFacing);
                float lerp = Clamp(m->deltaTime * 10.0f, 0.f, 1.f);
                m->camera.position = Vector3Lerp(m->camera.position, intended.position, lerp);
                m->camera.target = Vector3Lerp(m->camera.target, intended.target, lerp);
            }
        }

        if (anyHover && lastHover != anyHover)
            PlaySound(m->hover);
        lastHover = anyHover;
    }

    for (unsigned i = 0; i < arrlen(m->footstep); i++) {
        UnloadSound(m->footstep[i]);
    }
    UnloadFont(m->fonts.text);
    UnloadFont(m->fonts.heading);
    UnloadFont(m->fonts.title);
    UnloadFont(m->fonts.big);
    UnloadSound(m->encounter.klaxon);
    UnloadTexture(m->border);
    UnloadTexture(m->marble);
    UnloadTexture(m->vellum);
    UnloadSound(m->hover);
    UnloadSound(m->click);
    UnloadSound(m->click2);

    UnloadMusicStream(m->ambient);
    UnloadMusicStream(m->music);

    for (int i = 0; i < arrlen(m->party); i++)
        char_free(m->party + i);
    UnloadRenderTexture(m->rtex);
    monster_cleanup(&m->monsters);

    ext_deinit(&m->ext);
    RL_FREE(m);
    CloseAudioDevice();
    CloseWindow();

    util_logCleanup();
    PHYSFS_deinit();
    return 0;
}

