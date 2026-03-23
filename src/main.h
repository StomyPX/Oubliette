typedef struct Memory_ {
    GlobalFlags_t flags;
    External ext;
    Camera3D camera;
    RenderTexture rtex; // Huge overkill, I can't believe Raylib doesn't expose viewport
    int rtexW, rtexH;
    Map map;
    PcgRandom rng; // Primary RNG
    PcgRandom rng2; // Secondary RNG used only for non-mechanical effects

    struct { /* Zone of the screen used by the game under extreme aspect ratios */
        int top, left, bottom, right;
        int width, height;
    } area;

    struct {
        Font text;
        Font heading;
        Font title;
        Font big;
    } fonts;

    float deltaTime;
    float second;
    int partyX;
    int partyY;
    Facing partyFacing; /* 0 is north, increment to spin clockwise 90 degrees */
    float partyMoveTimer;
    float partyMoveFreq; /* Number of seconds between repeat moves when holding the buttons down */
    Character party[4];

    MonstrousCompendium monsters;
    CombatEncounter encounter;

    /* UI Resources */
    char tooltip[128];
    float fadein;
    LogLine logs[UI_LOGLINE_COUNT];
    uint32_t logCursor; // index to write to next
    uint32_t logScroll;
    float logScrollSmooth;
    Texture border;
    Texture marble;
    Texture vellum;
    Texture dead;
    Music ambient;
    Music music;

    /* Sound Clips */
    union {
        Sound sfx[29];
        struct {
            Sound footstep[11];
            Sound hover;
            Sound click;
            Sound click2;
            Sound klaxon;

            Sound hit[4];
            Sound whiff[4];
            Sound critical;
            // TODO Hurt?
            Sound deathMale;
            Sound deathFemale;
            Sound chainLightning;
            Sound hide;
            Sound experience;
            // TODO Objective
            // TODO Game Over
            // TODO Victory
        };
    };
} Memory;

static Memory* m = 0;
