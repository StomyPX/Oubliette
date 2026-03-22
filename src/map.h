#define TILE_COUNT_MAX 128
#define TILE_SIDE_LENGTH 3.f
#define MAP_ATLAS_SUBIMAGE_COUNT 16
#define CAMERA_HEIGHT 1.6f

typedef enum {
    TileFlags_AllowEntry = 1 << 0,
    TileFlags_AllowEast  = 1 << 1,
    TileFlags_AllowSouth = 1 << 2,
    TileFlags_Chamber    = 1 << 3,
    TileFlags_Filled     = 1 << 4,
    TileFlags_Feature    = 1 << 5,

    // Debugging only
    TileFlags_Failure    = 1 << 24,
    TileFlags_Builder    = 1 << 25,
    TileFlags_Terminator = 1 << 26,
    TileFlags_Visited    = 1 << 27,
    TileFlags_Passage    = 1 << 28,
    TileFlags_Next       = 1 << 29,
    TileFlags_FloodTest  = 1 << 30,
    TileFlags_Flooded    = 1 << 31,
} TileFlags;

typedef struct {
    int x, y;
    int w, h;
} MapChamber;

/* Unresolved Passage */
typedef struct {
    int x, y;
    Facing facing; // Same as party: 0 is north, continue clockwise
} MapPassage;

/* Map is centered on origin, such that the floor is at z==0 and the first tile (index==0) is in the
 * Northwest at (-map->width/2 * TILE_SIDE_LENGTH, map->height/2 * TILE_SIDE_LENGTH) */
typedef struct {
    char        name    [32];
    TileFlags   tiles [TILE_COUNT_MAX * TILE_COUNT_MAX];
    unsigned    width;
    unsigned    height;
    Model       wall;
    Texture     wallTex;
    Model       flor;
    Texture     florTex;
    Model       ceiling;
    Texture     ceilingTex;
    Model       goal;
    Texture     goalTex;
    Model       entry;
    Texture     entryTex;
    // TODO decorations (models/sprites)

    // Build Info
    PcgRandom   rng;
    uint64_t    seed;
    int         chamberCount;
    MapChamber  chambers [TILE_COUNT_MAX * TILE_COUNT_MAX];
    int         passageCount;
    MapPassage  passages [TILE_COUNT_MAX * TILE_COUNT_MAX * 4];

    int         entryX, entryY;
    int         goalX, goalY;
    int         encounterFreq;  /* Multiple of encounter ticks before rolling. 100 ticks is roughly one minute */
} Map;

static Vector3 map_tileCenter(Map* map, int x, int y);
static Vector3 map_tileCorner(Map* map, int x, int y);
static int map_tileIndex(Map* map, int x, int y); // Negative indicates invalid
static int map_chambersMax(int w, int h);
static int map_passagesMax(int w, int h);
static Rectangle map_subImageUV(int num); /* Starts at 1 */
static Camera3D map_cameraForTile(Map* map, int x, int y, Facing facing);

/* Returns non-zero on success */
static void map_generate(Map* map, uint64_t seed);
static void map_generateLoop(Map* map);
static void map_generatePassage(Map* map, MapPassage u);
static bool map_generateStepForward(Map* map, int* x, int* y, Facing facing);
static void map_generateChamber(Map* map, int x, int y, Facing facing);
static void map_generateChamberRandomPassage(Map* map, MapChamber chamber, Facing facing);

static void map_unload(Map* map);
static void map_draw(Map* map, Color light, float visibility, float power);

#if DEBUG_MODE
static bool map_floodTest(Map* map);
#endif
