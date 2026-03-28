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
#include "ui_elements.c"
#include "ui_screens.c"
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
    m->flags |= GlobalFlags_ShowTooltips;

    // TODO settings.ini
    // TODO ArgParser

    ext_init(&m->ext);
    PcgRandom_init(&m->rng, util_rdtsc());
    PcgRandom_init(&m->rng2, util_rdtsc());
    monster_init(&m->monsters);

    m->encounter.speed = CombatSpeed_Fast;

    m->fonts.text = LoadFontEx("data/fonts/CrimsonText-Bold.ttf", 24, 0, 0);
    m->fonts.heading = LoadFontEx("data/fonts/CoelacanthBold.otf", 30, 0, 0);
    m->fonts.title = LoadFontEx("data/fonts/Coelacanth.otf", 64, 0, 0);
    m->fonts.big = LoadFontEx("data/fonts/Coelacanth.otf", 200, 0, 0);

    m->border = LoadTexture("data/textures/panel-border.png");
    m->marble = LoadTexture("data/textures/Marble023B.png");
    m->vellum = LoadTexture("data/textures/parchment_2.png");
    m->dead = LoadTexture("data/textures/dead.jpg");
    m->flash = LoadTexture("data/textures/portrait_flash.png");
    m->options = LoadTexture("data/textures/options.png");
    m->panel = LoadTexture("data/textures/panel.png");

    m->music.ambient = LoadMusicStream("darkambient.mp3");
    m->music.intro   = LoadMusicStream("specters.mp3");
    m->music.general = LoadMusicStream("weltherrscherer.ogg");
    m->music.failure = LoadMusicStream("apologies.ogg");
    m->music.victory = LoadMusicStream("lament.mp3");

    SetMusicVolume(m->music.ambient, 0.3f);
    /* The order of these matters for some unknown reason, otherwise the ambient track doesn't play */
    main_changeSong(&m->music.general - &m->music.ambient);
    PlayMusicStream(m->music.ambient);

    util_log(LOG_WARNING, "Ambient stream playing: %s", IsMusicStreamPlaying(m->music.ambient) ? "true" : "false");

    for (unsigned i = 0; i < arrlen(m->footstep); i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "data/sounds/footstep_%02u.wav", i);
        m->footstep[i] = LoadSound(buf);
    }
    m->hover = LoadSound("data/sounds/button_hover.wav");
    m->click = LoadSound("data/sounds/button_click.wav");
    m->click2 = LoadSound("data/sounds/button_click2.wav");
    m->klaxon = LoadSound("data/sounds/encounter_bell.wav");

    m->hit[0] = LoadSound("data/sounds/hit_00.wav");
    m->hit[1] = LoadSound("data/sounds/hit_01.wav");
    m->hit[2] = LoadSound("data/sounds/hit_02.wav");
    m->hit[3] = LoadSound("data/sounds/hit_03.wav");
    m->whiff[0] = LoadSound("data/sounds/whiff_00.wav");
    m->whiff[1] = LoadSound("data/sounds/whiff_01.wav");
    m->whiff[2] = LoadSound("data/sounds/whiff_02.wav");
    m->whiff[3] = LoadSound("data/sounds/whiff_03.wav");
    m->critical = LoadSound("data/sounds/crit.wav");
    m->deathMale = LoadSound("data/sounds/death_male.wav");
    m->deathFemale = LoadSound("data/sounds/death_female.wav");
    m->chainLightning = LoadSound("data/sounds/lightning.wav");
    m->hide = LoadSound("data/sounds/whoosh.wav");
    m->experience = LoadSound("data/sounds/exp.wav");
    m->objective = LoadSound("data/sounds/discovery.wav");
    m->gameOver = LoadSound("data/sounds/game_over.wav");
    m->victory = LoadSound("data/sounds/victory.wav");

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

    map_generate(&m->map, util_rdtsc());

    m->partyFacing = 2; // Party always enters facing South
    m->partyX = m->map.entryX;
    m->partyY = m->map.entryY + 1;
    m->partyMoveFreq = 0.2f;

    m->camera = map_cameraForTile(&m->map, m->partyX, m->partyY, m->partyFacing);

    SetTargetFPS(200); /* TODO Make configurable, prefer VSync */

    /* Clear out all of Raylib's noise */
    for (unsigned i = 0; i < UTIL_LOGLINE_COUNT; i++) {
        if (g_util_logLines[i].seconds > 0.f && g_util_logLines[i].channel >= 0)
            g_util_logLines[i].seconds = 0.f;
    }

    ui_log(ZINNWALDITEBROWN, "Oubliette, Version 1.2");

    while (!WindowShouldClose() && !(m->flags & GlobalFlags_RequestQuit)) {
        /* Update */
        m->deltaTime = GetFrameTime();
        m->second += m->deltaTime;
        m->elementHoverLast = m->elementHover;
        m->elementHover = 0;
        m->elementCount = 0;
        m->tooltip[0] = 0;

        UpdateMusicStream(m->music.ambient);

        if (m->music.delay <= 0.f) {
            UpdateMusicStream(m->music.all[m->music.track]);
        } else {
            m->music.delay -= m->deltaTime;
            if (m->music.delay <= 0.f) {
                m->music.delay = 0.f;
            }
        }

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

        ui_dungeon();

        if (m->elementHover && m->elementHoverLast != m->elementHover)
            PlaySound(m->hover);
    }

    UnloadFont(m->fonts.text);
    UnloadFont(m->fonts.heading);
    UnloadFont(m->fonts.title);
    UnloadFont(m->fonts.big);
    UnloadTexture(m->border);
    UnloadTexture(m->marble);
    UnloadTexture(m->vellum);
    UnloadTexture(m->flash);

    for (int i = 0; i < arrlen(m->music.all); i++)
        UnloadMusicStream(m->music.all[i]);

    for (int i = 0; i < arrlen(m->sfx); i++)
        UnloadSound(m->sfx[i]);

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

static void
main_changeSong(int track)
{
    if (track == m->music.track)
        return;

    if (m->music.track > 0 && IsMusicStreamPlaying(m->music.all[m->music.track])) {
        StopMusicStream(m->music.all[m->music.track]);
    }

    if (track > arrlen(m->music.all) || track < 1) {
        m->music.track = 0;
    } else {
        m->music.track = track;
        PlayMusicStream(m->music.all[track]);
    }
}
