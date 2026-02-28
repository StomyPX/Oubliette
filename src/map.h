#define TILE_COUNT 48
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
} TileFlags;

typedef struct {
    int x, y;
    int w, h;
} MapChamber;
#define MAP_CHAMBERS_MAX (TILE_COUNT * 2)

/* Unresolved Passage */
typedef struct {
    int x, y;
    Facing facing; // Same as party: 0 is north, continue clockwise
} MapPassage;
#define MAP_PASSAGES_MAX (TILE_COUNT * 8)

/* Map is centered on origin, such that the floor is at z==0 and the first tile (index==0) is in the
 * Northwest at (-TILE_COUNT/2 * TILE_SIDE_LENGTH, TILE_COUNT/2 * TILE_SIDE_LENGTH) */
typedef struct {
    char        name    [32];
    TileFlags   tiles   [TILE_COUNT * TILE_COUNT];
    Model       wall;
    Texture     wallTex;
    Model       flor;
    Texture     florTex;
    Model       ceiling;
    Texture     ceilingTex;
    // TODO decorations (models/sprites)

    // Build Info
    PcgRandom   rng;
    int         chamberCount;
    MapChamber  chambers[MAP_CHAMBERS_MAX];
    int         passageCount;
    MapPassage  passages[MAP_PASSAGES_MAX];

    int         entryX, entryY;
    int         exitX, exitY;
} Map;

static Vector3 map_tileCenter(int x, int y);
static Vector3 map_tileCorner(int x, int y);
static Rectangle map_subImageUV(int num); /* Starts at 1 */

static Camera3D map_cameraForTile(int x, int y, Facing facing);

/* Returns non-zero on success */
static void map_generate(Map* map, uint64_t seed);
static void map_generatePassage(Map* map, MapPassage u);
static bool map_generateStepForward(Map* map, int* x, int* y, Facing facing);
static void map_generateChamber(Map* map, int x, int y, Facing facing);
static void map_generateChamberRandomPassage(Map* map, MapChamber chamber, Facing facing);

static void map_unload(Map* map);
static void map_draw(Map* map, Color light, float visibility);

