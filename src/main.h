typedef struct Memory_ {
    GlobalFlags_t flags;
    External ext;
    Camera3D camera;
    Map map;
    Font font;

    float deltaTime;
    int partyX;
    int partyY;
    int partyFacing; /* 0 is north, increment to spin clockwise 90 degrees */
    float partyMoveTimer;
    float partyMoveFreq; /* Number of seconds between repeat moves when holding the buttons down */
} Memory;
