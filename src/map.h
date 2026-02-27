#define TILE_COUNT 24
#define TILE_SIDE_LENGTH 4.f
#define MAP_ATLAS_SUBIMAGE_COUNT 16
#define CLIP_COLOR (Color){250, 255, 30, 64}
#define CLEAR_COLOR (Color){3, 20, 2, 255}

/* A face is two 4-bit values ORed together. The lower bits indicate atlas index plus one, while the lower
 * bits indicate what piece of that atlas is used, subdividing each into a 4x4 grid (zero at top-left,
 * row-major order). */
typedef unsigned char Face;

typedef enum {
    TileFlags_Solid    = 1 << 0,
    TileFlags_BarEast  = 1 << 1,
    TileFlags_BarSouth = 1 << 2,
} TileFlags;

/* Map is centered on origin, such that the floor is at z==0 and the first tile (index==0) is in the
 * Northwest at (-TILE_COUNT/2 * TILE_SIDE_LENGTH, TILE_COUNT/2 * TILE_SIDE_LENGTH) */
typedef struct {
    char        name    [32];
    Mesh        mesh;
    Material    material;
    Matrix      transform;
    TileFlags   tiles   [TILE_COUNT * TILE_COUNT];
    Face        north   [TILE_COUNT * TILE_COUNT];
    Face        east    [TILE_COUNT * TILE_COUNT];
    Face        south   [TILE_COUNT * TILE_COUNT];
    Face        west    [TILE_COUNT * TILE_COUNT];
    Face        floor   [TILE_COUNT * TILE_COUNT];
    Face        ceiling [TILE_COUNT * TILE_COUNT];
    Face        roof    [TILE_COUNT * TILE_COUNT];
} Map;

static Vector3 map_tileCenter(int x, int y);
static Vector3 map_tileCorner(int x, int y);
static Rectangle map_subImageUV(int num); /* Starts at 1 */

static Camera3D map_cameraForTile(int x, int y, int facing);
/* Returns non-zero on success */
static int map_load(Map* map, char* path);
static void map_unload(Map* map);
static void map_generate(Map* map);
static void map_draw(Map* map);

