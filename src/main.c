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

    m->font = LoadFontEx("data/fonts/Caudex-Regular.ttf", 32, 0, 0);
    GenTextureMipmaps(&m->font.texture);
    m->textColor = ColorFromNormalized((Vector4){218.f / 255.f, 209.f / 255.f, 200.f / 255.f, 1.f});
    m->errorColor = ColorFromNormalized((Vector4){255.f / 255.f,  15.f / 255.f,  21.f / 255.f, 1.f});

    Vector2 dragStart;

    /* TODO Load Map */
    map_load(&m->map, "data/maps/example.map");

    /* TODO Load start location */
    m->partyX = 12;
    m->partyY = 22;
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
                bool moveAllowed = true;
                currentTile = m->partyX + m->partyY * TILE_COUNT;
                switch (direction % 4) {
                    case 0: { /* North */
                        targetTile = m->partyX + (m->partyY - 1) * TILE_COUNT;
                        if (m->partyY <= 0) {
                            moveAllowed = false;
                        } else if (m->map.tiles[targetTile] & TileFlags_Solid) {
                            moveAllowed = false;
                        } else if (m->map.tiles[targetTile] & TileFlags_BarSouth) {
                            moveAllowed = false;
                        }
                        if (moveAllowed) {
                            m->partyY -= 1;
                        } else {
                            m->camera.position.z -= 0.1f;
                        }
                    } break;

                    case 1: { /* East */
                        targetTile = m->partyX + 1 + m->partyY * TILE_COUNT;
                        if (m->partyX >= TILE_COUNT - 1) {
                            moveAllowed = false;
                        } else if (m->map.tiles[targetTile] & TileFlags_Solid) {
                            moveAllowed = false;
                        } else if (m->map.tiles[currentTile] & TileFlags_BarEast) {
                            moveAllowed = false;
                        }
                        if (moveAllowed) {
                            m->partyX += 1;
                        } else {
                            m->camera.position.x += 0.1f;
                        }
                    } break;

                    case 2: { /* South */
                        targetTile = m->partyX + (m->partyY + 1) * TILE_COUNT;
                        if (m->partyY >= TILE_COUNT - 1) {
                            moveAllowed = false;
                        } else if (m->map.tiles[targetTile] & TileFlags_Solid) {
                            moveAllowed = false;
                        } else if (m->map.tiles[currentTile] & TileFlags_BarSouth) {
                            moveAllowed = false;
                        }
                        if (moveAllowed) {
                            m->partyY += 1;
                        } else {
                            m->camera.position.z += 0.1f;
                        }
                    } break;

                    case 3: { /* West */
                        targetTile = m->partyX - 1 + m->partyY * TILE_COUNT;
                        if (m->partyX <= 0) {
                            moveAllowed = false;
                        } else if (m->map.tiles[targetTile] & TileFlags_Solid) {
                            moveAllowed = false;
                        } else if (m->map.tiles[targetTile] & TileFlags_BarEast) {
                            moveAllowed = false;
                        }
                        if (moveAllowed) {
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

        BeginDrawing();
        {
            ClearBackground(CLEAR_COLOR);

            BeginMode3D(m->camera);

            map_draw(&m->map);
            if (m->flags & (GlobalFlags_PartyStats | GlobalFlags_EditorMode))
                DrawGrid(TILE_COUNT + 1, TILE_SIDE_LENGTH);
            if (m->flags & GlobalFlags_ShowCollision) {
                for (int x = 0; x < TILE_COUNT; x++) {
                    for (int y = 0; y < TILE_COUNT; y++) {
                        int index = x + y * TILE_COUNT;
                        if (m->map.tiles[index] & TileFlags_Solid) {
                            Vector3 position = map_tileCenter(x, y);
                            DrawCubeWires(position, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, LIGHTGRAY);
                            DrawCube(position, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, CLIP_COLOR);
                        }

                        if (m->map.tiles[index] & TileFlags_BarEast) {
                            Vector3 position = map_tileCenter(x, y);
                            position.x += (float)TILE_SIDE_LENGTH / 2.f;
                            DrawCube(position, 0.1f, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, CLIP_COLOR);
                            DrawCubeWires(position, 0.1f, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, LIGHTGRAY);
                        }

                        if (m->map.tiles[index] & TileFlags_BarSouth) {
                            Vector3 position = map_tileCenter(x, y);
                            position.z += (float)TILE_SIDE_LENGTH / 2.f;
                            DrawCube(position, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, 0.1f, CLIP_COLOR);
                            DrawCubeWires(position, TILE_SIDE_LENGTH, TILE_SIDE_LENGTH, 0.1f, LIGHTGRAY);
                        }
                    }
                }
            }

            EndMode3D();

            char* message = "7DRL 2026. Let's Rock!";
            Vector2 position = MeasureTextEx(m->font, message, m->font.baseSize, 0);
            position.x = GetScreenWidth() / 2 - position.x / 2;
            position.y = GetScreenHeight() * 3 / 4 - position.y / 2;
            ui_text(m->font, message, position, m->textColor, 0);

            if (m->flags & GlobalFlags_PartyStats) {
                char buf[128] = {0};
                char* facing = 0;
                Vector2 position = (Vector2){GetScreenWidth() - 100, 10};
                switch (m->partyFacing) {
                    case 0: facing = "North"; break;
                    case 1: facing = "East"; break;
                    case 2: facing = "South"; break;
                    case 3: facing = "West"; break;
                }
                snprintf(buf, sizeof(buf),
                        "Editor Mode: %s\n"
                        "Camera: %4.1f, %4.1f\n"
                        "Tile: %2i, %2i\n"
                        "Facing: %s",
                        m->flags & GlobalFlags_EditorMode ? "ON" : "off",
                        m->camera.position.x, m->camera.position.z,
                        m->partyX, m->partyY,
                        facing);
                ui_text(GetFontDefault(), buf, position, m->textColor, 1);
            }

            if (m->ext.cimgui.handle && (m->flags & GlobalFlags_EditorModePermitted)) {
                ext_CImguiNewFrame(m);
                if (m->flags & GlobalFlags_EditorMode) {
                    bool show = 1;
                    m->ext.cimgui.ShowDemoWindow(&show);
                    if (!show)
                        m->flags &= ~(GlobalFlags_EditorMode);
                }
                ext_CImguiRender(&m->ext.cimgui);
            }

            util_drawLog();
        }
        EndDrawing();
    }

    UnloadFont(m->font);
    ext_deinit(&m->ext);
    RL_FREE(m);
    CloseWindow();
    PHYSFS_deinit();
    return 0;
}

