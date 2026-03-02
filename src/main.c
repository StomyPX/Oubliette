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
    InitWindow(1280, 720, GAME_NAME);
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
    monster_init(&m->monsters);

    m->fonts.text = LoadFontEx("data/fonts/CrimsonText-Regular.ttf", 24, 0, 0);
    m->fonts.textB = LoadFontEx("data/fonts/CrimsonText-Bold.ttf", 24, 0, 0);
    m->fonts.textI = LoadFontEx("data/fonts/CrimsonText-Italic.ttf", 24, 0, 0);
    m->fonts.title = LoadFontEx("data/fonts/Coelacanth.otf", 64, 0, 0);
    m->fonts.titleB = LoadFontEx("data/fonts/CoelacanthBold.otf", 64, 0, 0);
    m->fonts.titleI = LoadFontEx("data/fonts/CoelacanthItalic.otf", 64, 0, 0);

    m->encounter.klaxon = LoadSound("data/sounds/encounter_bell.wav");

    m->border = LoadTexture("data/textures/panel-border.png");
    m->marble = LoadTexture("data/textures/Marble023B.png");
    m->vellum = LoadTexture("data/textures/parchment_2.png");
    m->dead = LoadTexture("data/textures/dead.jpg");
    m->hover = LoadSound("data/sounds/button_hover.wav");
    m->click = LoadSound("data/sounds/button_click.wav");
    m->click2 = LoadSound("data/sounds/button_click2.wav");

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

    bool lastHover = false;;
    while (!WindowShouldClose() && !(m->flags & GlobalFlags_RequestQuit)) {
        /* Update */
        m->deltaTime = GetFrameTime();
        bool anyHover = false;

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
        } else if (m->flags & GlobalFlags_Encounter) {
        } else {
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

            /* TODO Move into ents.c as a function for all entity movement,
             * will have to extract camera movement though */
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

                    /* TODO Play different sounds based on if the move is allowed or not */

                    /* Tick up encounter accumulator */
                    int ticks = 20;
                    for (int i = 0; i < arrlen(m->party); i++) {
                        if (m->party[i].name[0]) {
                            if (m->party[i].health > 0) {
                                int contrib = 20;
                                contrib -= char_modifier(m->party[i].dexterity) * 2;
                                contrib -= char_modifier(m->party[i].intellect);
                                contrib += char_modifier(m->party[i].strength);
                                // TODO Silent move
                                ticks += contrib;
                            }
                        }
                    }

                    if (ticks < 50)
                        ticks = 50;
                    m->encounter.ticks += ticks;
                } else {
                    /* Tick up very slightly */
                    m->encounter.ticks += 1;
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
        }

        /* Rendering */

        { /* Determine GUI layout proportions */
            float aspect = (float)GetRenderWidth() / (float)GetRenderHeight();

            if (aspect > 2.f) {
                m->area.width = GetRenderHeight() * 2;
                m->area.height = GetRenderHeight();
                m->area.top = 0;
                m->area.left = (GetRenderWidth() - m->area.width) / 2;
            } else if (aspect < 1.f) {
                m->area.width = GetRenderWidth();
                m->area.height = GetRenderWidth();
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
        }

        Rectangle viewport;
        viewport.x = m->area.left + UI_PADDING;
        viewport.y = m->area.top + UI_PADDING;
        viewport.width = m->area.width * (1.f - UI_SIDE_PANEL_FRACTION) - UI_PADDING * 2;
        viewport.height = m->area.height * (1.f - UI_PORTRAIT_FRACTION) - UI_PADDING * 2;

        if (!(m->flags & GlobalFlags_Encounter)) {
            /* Draw map to a render texture. Incredibly, raylib doesn't do viewports */
            if (m->rtexW != viewport.width || m->rtexH != viewport.height) {
                if (IsRenderTextureValid(m->rtex))
                    UnloadRenderTexture(m->rtex);
                m->rtex = LoadRenderTexture(viewport.width, viewport.height);
                m->rtexW = viewport.width;
                m->rtexH = viewport.height;
            }

            BeginTextureMode(m->rtex);
            BeginMode3D(m->camera);
            ClearBackground(BLACK);

            if (m->map.name[0]) {
                // TODO Redo lighting to allow multiple sources with different curves
                map_draw(&m->map, /*BEIGE*/ DARKOLIVEGREEN, 4.f * TILE_SIDE_LENGTH, 2.0f);
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
                            DrawCube(position, 0.1f, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, CLIP_COLOR);
                            DrawCubeWires(position, 0.1f, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, LIGHTGRAY);
                        }

                        if (!(m->map.tiles[index] & TileFlags_AllowSouth)) {
                            Vector3 position = map_tileCenter(x, y);
                            position.z += (float)TILE_SIDE_LENGTH / 2.f;
                            DrawCube(position, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, 0.1f, CLIP_COLOR);
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
                        Vector2 anchor = (Vector2){0.5f, 0.5f}; /* TODO Make customizable */
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
                    Vector2 position;
                    Rectangle button;
                    int result;
                    bool active = !(m->flags & GlobalFlags_GameOver);

                    position.x = viewport.x + UI_PADDING;
                    position.y = viewport.y + UI_PADDING;
                    if (unit->alive == 1) {
                        snprintf(buffer, sizeof(buffer), unit->class.truename);
                    } else {
                        snprintf(buffer, sizeof(buffer), "%u %s", unit->alive, unit->class.truenamePlural);
                    }
                    ui_text(m->fonts.title, buffer, position, m->fonts.title.baseSize, 0, MINDAROGREEN);

                    button.width = 120;
                    button.height = 48;
                    button.x = viewport.x + viewport.width - UI_PADDING - button.width;
                    button.y = viewport.y + viewport.height - UI_PADDING - button.height;
                    result = ui_button(button, "FLEE", m->fonts.textB.baseSize, KEY_R, active);
                    if (result > 0) {
                        PlaySound(m->click);
                        combat_flee();
                    } else if (result < 0) {
                        anyHover = true;
                    }

                    /* FIGHT! */
                    button.x = viewport.x + UI_PADDING;
                    result = ui_button(button, "FIGHT", m->fonts.textB.baseSize, KEY_F, active);
                    if (result > 0) {
                        PlaySound(m->click);
                        combat_fight();
                    } else if (result < 0) {
                        anyHover = true;
                    }
                }

            } else {
                Rectangle button;
                int result;

                /* Map viewport. Because OpenGL's origin is in the bottom left, this has to be inverted */
                DrawTextureRec(m->rtex.texture,
                    (Rectangle){0, 0, m->rtex.texture.width, -m->rtex.texture.height},
                    (Vector2){viewport.x, viewport.y}, WHITE);

                button.width = 120;
                button.height = 48;
                button.x = viewport.x + viewport.width - UI_PADDING - button.width;
                button.y = viewport.y + viewport.height - UI_PADDING - button.height;
                result = ui_button(button, "REST", m->fonts.textB.baseSize, KEY_R, true);
                if (result > 0) {
                    ui_log(ZINNWALDITEBROWN, "Resting...");
                    m->encounter.ticks += 300;
                    m->flags |= GlobalFlags_AdvanceTurn;

                    for (int i = 0; i < arrlen(m->party); i++) {
                        Character* c = m->party + i;
                        if (!c->name[0] || c->health < 1)
                            continue;

                        if (c->stamina < char_maxStamina(*c)) {
                            int die = (c->constitution + c->willpower) / 10 + 2;
                            c->stamina += PcgRandom_roll(&m->rng, 1, die);
                            if (c->stamina > char_maxStamina(*c))
                                c->stamina = char_maxStamina(*c);
                        } else if (c->health < char_maxHealth(*c)) {
                            int die = c->constitution / 20 + 2;
                            c->health += PcgRandom_roll(&m->rng, 1, die) - 1;
                            if (c->health > char_maxHealth(*c))
                                c->health = char_maxHealth(*c);
                        }
                    }

                    PlaySound(m->click);
                } else if (result < 0) {
                    anyHover = true;
                }
            }

            if (m->flags & GlobalFlags_GameOver) {
                Vector2 position;
                Vector2 measure;

                measure = MeasureTextEx(m->fonts.titleB, "GAME OVER", m->fonts.titleB.baseSize, 0);
                position.x = viewport.x + viewport.width / 2 - measure.x / 2;
                position.y = viewport.y + viewport.height / 2 - measure.y / 2;
                ui_text(m->fonts.titleB, "GAME OVER", position, m->fonts.titleB.baseSize, 0, MAROON);
            }

            ui_border(m->border, viewport, BONE);

            if (m->map.name[0]) {
                Vector2 position;
                Vector2 dimensions = MeasureTextEx(m->fonts.title, m->map.name, m->fonts.title.baseSize, 0);
                position.x = m->area.left + m->area.width;
                position.x -= m->area.width * UI_SIDE_PANEL_FRACTION / 2.f;
                position.x -= dimensions.x / 2.f;
                position.y = m->area.top + UI_SIDE_PANEL_HEADER / 2.f - dimensions.y / 2.f;
                ui_text(m->fonts.title, m->map.name, position, m->fonts.title.baseSize, 0, BONE);
            }

            { /* Side Panel */
                Rectangle panel;
                Vector2 position;
                int count = 0;
                int scrollMax = 0;
                int visible;

                panel.x = m->area.left + m->area.width * (1.f - UI_SIDE_PANEL_FRACTION) + UI_PADDING;
                panel.y = m->area.top + UI_SIDE_PANEL_HEADER + UI_PADDING;
                panel.width = m->area.width * UI_SIDE_PANEL_FRACTION - UI_PADDING * 2;
                panel.height = m->area.height - UI_PADDING * 2 - UI_SIDE_PANEL_HEADER - UI_SIDE_PANEL_FOOTER;
                position.x = panel.x + UI_PADDING;
                position.y = panel.y + UI_PADDING;
                visible = panel.height / m->fonts.text.baseSize;

                DrawTextureRec(m->vellum, panel, (Vector2){panel.x, panel.y}, WHITE);
                BeginScissorMode(panel.x, panel.y, panel.width, panel.height);

                /* TODO Find scroll point first */
                /* TODO Rescale font to fit horizontally */
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
                }

                for (unsigned i = 0; i < UI_LOGLINE_COUNT; i++) {
                    unsigned index = (i + m->logCursor) % UI_LOGLINE_COUNT;
                    if (m->logs[index].text[0]) {
                        DrawTextEx(m->fonts.text, m->logs[index].text, position,
                                    m->fonts.text.baseSize, 0, m->logs[index].color);
                        position.y += m->fonts.text.baseSize;
                    }
                }
                EndScissorMode();
                // TODO scrollbar
                ui_border(m->border, panel, BONE);
            }

            { /* Portraits */
                Rectangle card;

                card.x = m->area.left + UI_PADDING;
                card.y = m->area.top + m->area.height * (1.f - UI_PORTRAIT_FRACTION) + UI_PADDING;
                card.width = m->area.width * (1.f - UI_SIDE_PANEL_FRACTION) / 4.f - UI_PADDING * 2;
                card.height = m->area.height * UI_PORTRAIT_FRACTION - UI_PADDING * 2;
                ui_characterHudCard(m->party[0], card);

                card.x += card.width + UI_PADDING * 2;
                ui_characterHudCard(m->party[1], card);

                card.x += card.width + UI_PADDING * 2;
                ui_characterHudCard(m->party[2], card);

                card.x += card.width + UI_PADDING * 2;
                ui_characterHudCard(m->party[3], card);
            }

            if (m->flags & GlobalFlags_PartyStats) {
                char buf[128] = {0};
                Vector2 position = (Vector2){GetRenderWidth() - 120, 20};
                snprintf(buf, sizeof(buf),
                        "Debug View: %s\n"
                        "Camera: %4.1f, %4.1f\n"
                        "Tile: %2i, %2i\n"
                        "Facing: %s\n"
                        "Enc: %i/%i\n"
                        "Seed: %llu\n",
                        m->flags & GlobalFlags_EditorMode ? "ON" : "off",
                        m->camera.position.x, m->camera.position.z,
                        m->partyX, m->partyY,
                        Facing_toString(m->partyFacing),
                        m->encounter.ticks, m->map.encounterFreq,
                        m->map.seed);
                ui_text(GetFontDefault(), buf, position, GetFontDefault().baseSize, 1, BONE);
            }

            if (m->ext.cimgui.handle && (m->flags & GlobalFlags_EditorModePermitted)) {
                ext_CImguiNewFrame(m);
                if (m->flags & GlobalFlags_ShowMap) {
                    bool show = 1;
                    char buffer[TILE_COUNT * 2 + 1] = {};

                    m->ext.cimgui.Begin("Map Overview", &show, ImGuiWindowFlags_AlwaysAutoResize);

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
                            } else if (t & TileFlags_Feature) {
                                buffer[x * 2] = '?';
                            } else if (t & TileFlags_Filled) {
                                buffer[x * 2] = 'E';
                            } else if ((t & TileFlags_AllowEntry) && !(t & TileFlags_AllowSouth)) {
                                buffer[x * 2] = '_';
                            } else if (!(t & TileFlags_AllowEntry)) {
                                buffer[x * 2] = '#';
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
            if (m->flags & GlobalFlags_AdvanceTurn) {
                if (m->encounter.ticks > m->map.encounterFreq) {
                    int chance = m->encounter.ticks / m->map.encounterFreq + 1;
                    int die = 6;
                    m->encounter.ticks %= m->map.encounterFreq;

                    if (PcgRandom_roll(&m->rng, 1, die) <= chance) {
                        monster_encounter(&m->monsters);
                    }
                }

                m->flags &= ~(GlobalFlags_AdvanceTurn);
            }

            // TODO if not in a menu or encounter
            Camera intended = map_cameraForTile(m->partyX, m->partyY, m->partyFacing);
            float lerp = Clamp(m->deltaTime * 10.0f, 0.f, 1.f);
            m->camera.position = Vector3Lerp(m->camera.position, intended.position, lerp);
            m->camera.target = Vector3Lerp(m->camera.target, intended.target, lerp);
        }

        if (!lastHover && anyHover)
            PlaySound(m->hover);
        lastHover = anyHover;
    }

    UnloadFont(m->fonts.text);
    UnloadFont(m->fonts.textB);
    UnloadFont(m->fonts.textI);
    UnloadFont(m->fonts.title);
    UnloadFont(m->fonts.titleB);
    UnloadFont(m->fonts.titleI);
    UnloadSound(m->encounter.klaxon);
    UnloadTexture(m->border);
    UnloadTexture(m->marble);
    UnloadTexture(m->vellum);
    UnloadSound(m->hover);
    UnloadSound(m->click);
    UnloadSound(m->click2);
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

