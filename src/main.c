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
    ext_init(&m->ext);

    /* TODO Always on in debug mode, requires cmdline switch "--editor" in release mode */
    #if DEBUG_MODE
        //m->flags |= GlobalFlags_PartyStats;
        m->flags |= GlobalFlags_EditorModePermitted;
        //m->flags |= GlobalFlags_ShowCollision;
    #endif
    m->flags |= GlobalFlags_ShowTooltips;

    m->encounter.speed = CombatSpeed_Fast;

    if (PHYSFS_exists("settings.ini")) {
        char* content;

        content = util_readFileData("settings.ini", 0);
        if (content) {
            ini_t* ini = ini_load(content, 0);

            if (ini) {
                int section, props;
                char* sectionName = "Options";

                TraceLog(LOG_TRACE, "INI: Loading options from settings.ini");
                section = ini_find_section(ini, sectionName, strlen(sectionName));
                props = ini_property_count(ini, section);
                for (unsigned i = 0; i < props; i++) {
                    char* key = (char*)ini_property_name(ini, section, i);
                    char* val = (char*)ini_property_value(ini, section, i);

                    if (strcmp(key, "enable music") == 0 && util_stricmp(val, "false") == 0) {
                        m->flags |= GlobalFlags_MusicStartMuted;
                    } else if (strcmp(key, "enable ambience") == 0 && util_stricmp(val, "false") == 0) {
                        m->flags |= GlobalFlags_AmbientStartMuted;
                    } else if (strcmp(key, "enable sfx") == 0 && util_stricmp(val, "false") == 0) {
                        m->flags |= GlobalFlags_MuteSFX;
                    } else if (strcmp(key, "enable fullscreen") == 0 && util_stricmp(val, "false") != 0) {
                        ToggleBorderlessWindowed();
                    } else if (strcmp(key, "enable tooltips") == 0 && util_stricmp(val, "false") == 0) {
                        m->flags &= ~(GlobalFlags_ShowTooltips);
                    } else if (strcmp(key, "combat speed") == 0) {
                        if (util_stricmp(val, "instant") == 0) {
                            m->encounter.speed = CombatSpeed_Instant;
                        } else if (util_stricmp(val, "fast") == 0) {
                            m->encounter.speed = CombatSpeed_Fast;
                        } else if (util_stricmp(val, "slow") == 0) {
                            m->encounter.speed = CombatSpeed_Slow;
                        } else {
                            TraceLog(LOG_DEBUG, "INI: Unknown value for combat speed: %s, "
                                    "defaulting to fast", val);
                        }
                    }
                }

                ini_destroy(ini);
            }

            MemFree(content);
        }
    }
    // TODO ArgParser

    PcgRandom_init(&m->rng, util_rdtsc());
    PcgRandom_init(&m->rng2, util_rdtsc());
    monster_init(&m->monsters);

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

    if (m->flags & GlobalFlags_MusicStartMuted)
        StopMusicStream(m->music.all[m->music.track]);
    if (m->flags & GlobalFlags_AmbientStartMuted)
        StopMusicStream(m->music.ambient);

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

    if (m->flags & GlobalFlags_MuteSFX) {
        for (int i = 0; i < arrlen(m->sfx); i++)
            SetSoundVolume(m->sfx[i], 0.f);
    }

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
        m->flags &= ~(GlobalFlags_IgnoreInput);

        if (m->map.name[0])
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

    /* Cleanup before quit */

    { /* Save Settings */
        ini_t* ini = ini_create(0);

        if (ini) {
            char* key = "Options";
            char* val;
            int section;
            char* content;
            int size;

            section = ini_section_add(ini, key, strlen(key));

            key = "enable music";
            val = IsMusicStreamPlaying(m->music.all[m->music.track]) ? "true" : "false";
            ini_property_add(ini, section, key, strlen(key), val, strlen(val));

            key = "enable ambience";
            val = IsMusicStreamPlaying(m->music.ambient) ? "true" : "false";
            ini_property_add(ini, section, key, strlen(key), val, strlen(val));

            key = "enable sfx";
            val = !(m->flags & GlobalFlags_MuteSFX) ? "true" : "false";
            ini_property_add(ini, section, key, strlen(key), val, strlen(val));

            key = "enable tooltips";
            val = (m->flags & GlobalFlags_ShowTooltips) ? "true" : "false";
            ini_property_add(ini, section, key, strlen(key), val, strlen(val));

            key = "enable fullscreen";
            val = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) ? "true" : "false";
            ini_property_add(ini, section, key, strlen(key), val, strlen(val));

            key = "combat speed";
            switch (m->encounter.speed) {
                case CombatSpeed_Instant: val = "instant"; break;
                default:
                case CombatSpeed_Fast: val = "fast"; break;
                case CombatSpeed_Slow: val = "slow"; break;
            }
            ini_property_add(ini, section, key, strlen(key), val, strlen(val));

            size = ini_save(ini, 0, 0);
            content = MemAlloc(size);
            size = ini_save(ini, content, size);
            ini_destroy(ini);

            util_writeFileData("settings.ini", content, size);
            MemFree(content);
        } else {
            TraceLog(LOG_ERROR, "Could not create ini context, settings will not be saved");
        }
    }

    UnloadFont(m->fonts.text);
    UnloadFont(m->fonts.heading);
    UnloadFont(m->fonts.title);
    UnloadFont(m->fonts.big);
    UnloadTexture(m->border);
    UnloadTexture(m->marble);
    UnloadTexture(m->vellum);
    UnloadTexture(m->flash);
    UnloadTexture(m->options);
    UnloadTexture(m->panel);

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
