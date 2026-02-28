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
#include "effect.h"
#include "item.h"
#include "ent.h"
#include "map.h"
#include "party.h"
#include "ui.h"
#include "editor.h"
#include "main.h"

/* Sources (order doesn't matter) */
#include "ext.c"
#include "map.c"
#include "platform.c"
#include "util.c"

int
main(int argc, char* argv[])
{
    PHYSFS_init(argv[0]);
    PHYSFS_permitSymbolicLinks(1);
    PHYSFS_mount(PHYSFS_getBaseDir(), 0, 0);

    /* TODO Fill in and enable:
    char* prefdir = PHYSFS_setPrefDir("DevName", "GameName");
    PHYSFS_setWriteDir(prefdir);
    PHYSFS_mount(prefdir, 0, 0);
    */

    /* TODO Auto-load all zip files found in res/ */

    InitWindow(1280, 720, GAME_NAME);
    SetExitKey(KEY_NULL);

    m = RL_MALLOC(sizeof(Memory));
    memset(m, 0, sizeof(Memory));

    /* TODO Always on in debug mode, requires cmdline switch "--editor" in release mode */
    #if DEBUG_MODE
        m->flags |= GlobalFlags_PartyStats;
        m->flags |= GlobalFlags_EditorModePermitted;
        //m->flags |= GlobalFlags_ShowCollision;
    #endif

    ext_init(&m->ext);

    m->fonts.text = LoadFontEx("data/fonts/CrimsonText-Regular.ttf", 20, 0, 0);
    m->fonts.textB = LoadFontEx("data/fonts/CrimsonText-Bold.ttf", 20, 0, 0);
    m->fonts.textI = LoadFontEx("data/fonts/CrimsonText-Italic.ttf", 20, 0, 0);
    m->fonts.title = LoadFontEx("data/fonts/Coelacanth.otf", 64, 0, 0);
    m->fonts.titleB = LoadFontEx("data/fonts/CoelacanthBold.otf", 64, 0, 0);
    m->fonts.titleI = LoadFontEx("data/fonts/CoelacanthItalic.otf", 64, 0, 0);

    Vector2 dragStart;

    map_generate(&m->map, 1);

    m->partyFacing = 2; // Party always enters facing South
    m->partyX = m->map.entryX;
    m->partyY = m->map.entryY + 1;
    m->partyMoveFreq = 0.2f;

    m->camera = map_cameraForTile(m->partyX, m->partyY, m->partyFacing);

    SetTargetFPS(200); /* TODO Make configurable, prefer VSync */

    while (!WindowShouldClose() && !(m->flags & GlobalFlags_RequestQuit)) {

        /* Update */
        m->deltaTime = GetFrameTime();

        if (IsKeyPressed(KEY_F4) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
            m->flags |= GlobalFlags_RequestQuit;

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
                currentTile = m->partyX + m->partyY * TILE_COUNT;
                switch (direction % 4) {
                    case 0: { /* North */
                        targetTile = m->partyX + (m->partyY - 1) * TILE_COUNT;
                        if (targetTile < TILE_COUNT * TILE_COUNT
                            && m->map.tiles[targetTile] & TileFlags_AllowEntry
                            && m->map.tiles[targetTile] & TileFlags_AllowSouth)
                        {
                            m->partyY -= 1;
                        } else {
                            m->camera.position.z -= 0.1f;
                        }
                    } break;

                    case 1: { /* East */
                        targetTile = m->partyX + 1 + m->partyY * TILE_COUNT;
                        if (targetTile < TILE_COUNT * TILE_COUNT
                            && m->map.tiles[targetTile] & TileFlags_AllowEntry
                            && m->map.tiles[currentTile] & TileFlags_AllowEast)
                        {
                            m->partyX += 1;
                        } else {
                            m->camera.position.x += 0.1f;
                        }
                    } break;

                    case 2: { /* South */
                        targetTile = m->partyX + (m->partyY + 1) * TILE_COUNT;
                        if (targetTile < TILE_COUNT * TILE_COUNT
                            && m->map.tiles[targetTile] & TileFlags_AllowEntry
                            && m->map.tiles[currentTile] & TileFlags_AllowSouth)
                        {
                            m->partyY += 1;
                        } else {
                            m->camera.position.z += 0.1f;
                        }
                    } break;

                    case 3: { /* West */
                        targetTile = m->partyX - 1 + m->partyY * TILE_COUNT;
                        if (targetTile < TILE_COUNT * TILE_COUNT
                            && m->map.tiles[targetTile] & TileFlags_AllowEntry
                            && m->map.tiles[targetTile] & TileFlags_AllowEast)
                        {
                            m->partyX -= 1;
                        } else {
                            m->camera.position.x -= 0.1f;
                        }
                    } break;
                }

                /* TODO Play different sounds based on if the move is allowed or not */
            }

            if (IsKeyPressed(KEY_Q)) {
                m->partyFacing += 3;
                m->partyFacing %= 4;
            }

            if (IsKeyPressed(KEY_E)) {
                m->partyFacing += 1;
                m->partyFacing %= 4;
            }

            Camera intended = map_cameraForTile(m->partyX, m->partyY, m->partyFacing);
            float lerp = Clamp(m->deltaTime * 10.0f, 0.f, 1.f);
            m->camera.position = Vector3Lerp(m->camera.position, intended.position, lerp);
            m->camera.target = Vector3Lerp(m->camera.target, intended.target, lerp);
        }

        /* Rendering */

        { /* Determine active play area */
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

        BeginDrawing();
        {
            ClearBackground(CLEAR_COLOR);

            BeginMode3D(m->camera);

            if (m->map.name[0])
                map_draw(&m->map, GLOOM_COLOR, 4.f * TILE_SIDE_LENGTH);

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

            /*
            char* message = "7DRL 2026. Let's Rock!";
            Vector2 position = MeasureTextEx(m->fonts.text, message, m->fonts.text.baseSize, 0);
            position.x = m->area.left + m->area.width / 2 - position.x / 2;
            position.y = m->area.top + m->area.height * 3 / 4 - position.y / 2;
            ui_text(m->fonts.text, message, position, TEXT_COLOR, 0);
            */

            if (m->map.name[0]) {
                Vector2 position = MeasureTextEx(m->fonts.title, m->map.name, m->fonts.title.baseSize, 0);
                position.x = m->area.left + m->area.width - position.x - 30;
                position.y = m->area.top + 20;
                ui_text(m->fonts.title, m->map.name, position, TEXT_COLOR, 0);
            }

            if (m->flags & GlobalFlags_PartyStats) {
                char buf[128] = {0};
                char* facing = 0;
                Vector2 position = (Vector2){GetRenderWidth() - 120, GetRenderHeight() / 3};
                switch (m->partyFacing) {
                    case 0: facing = "North"; break;
                    case 1: facing = "East"; break;
                    case 2: facing = "South"; break;
                    case 3: facing = "West"; break;
                }
                snprintf(buf, sizeof(buf),
                        "Debug View: %s\n"
                        "Camera: %4.1f, %4.1f\n"
                        "Tile: %2i, %2i\n"
                        "Facing: %s",
                        m->flags & GlobalFlags_EditorMode ? "ON" : "off",
                        m->camera.position.x, m->camera.position.z,
                        m->partyX, m->partyY,
                        facing);
                ui_text(GetFontDefault(), buf, position, TEXT_COLOR, 1);
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
    }

    UnloadFont(m->fonts.text);
    UnloadFont(m->fonts.textB);
    UnloadFont(m->fonts.textI);
    UnloadFont(m->fonts.title);
    UnloadFont(m->fonts.titleB);
    UnloadFont(m->fonts.titleI);
    ext_deinit(&m->ext);
    RL_FREE(m);
    CloseWindow();
    PHYSFS_deinit();
    return 0;
}

