typedef struct Memory_ {
    GlobalFlags_t flags;
    External ext;
    Camera3D camera;
    RenderTexture rtex; // Huge overkill, I can't believe Raylib doesn't expose viewport
    int rtexW, rtexH;
    Map map;
    PcgRandom rng; // Primary RNG

    struct { /* Zone of the screen used by the game under extreme aspect ratios */
        int top, left, bottom, right;
        int width, height;
    } area;

    struct {
        Font text, textB, textI;
        Font title, titleB, titleI;
    } fonts;

    float deltaTime;
    int partyX;
    int partyY;
    Facing partyFacing; /* 0 is north, increment to spin clockwise 90 degrees */
    float partyMoveTimer;
    float partyMoveFreq; /* Number of seconds between repeat moves when holding the buttons down */
    Texture portraits[4];

    /* UI Resources */
    Texture border;
    Texture marble;
    Texture vellum;
} Memory;

static Memory* m = 0;
