#define CLIP_COLOR (Color){250, 255, 30, 64}
#define CLEAR_COLOR (Color){6, 0, 3, 255}
#define BONE (Color){227, 218, 201, 255}
#define ERROR_COLOR (Color){255, 15, 21, 255}
#define DARKOLIVEGREEN (Color){61, 81, 20, 255}
#define MOSSGREEN (Color){138,154,91,255}
#define LIMEPULPGREEN (Color){209,225,137,255}
#define MINDAROGREEN (Color){227,249,136,255}
#define ZINNWALDITEBROWN (Color){44,22,8,255}
#ifdef MAROON
#undef MAROON
#endif
#define MAROON (Color){128,0,0,255}

#define KiB(exp) (1024 * exp)
#define MiB(exp) (1024 * KiB(exp))
#define GiB(exp) (1024ll * MiB(exp))
#define arrlen(arr) (sizeof(arr) / sizeof(arr[0]))

static int util_stricmp(char* a, char* b);
static int util_pseudohex(char c);

typedef enum {
    Facing_North = 0,
    Facing_East,
    Facing_South,
    Facing_West,
} Facing;

static const char* Facing_toString(Facing facing);
static unsigned util_traverse(Facing facing, int inx, int iny, int forward, int right, int* outX, int* outY);
static bool util_facingNorthSouth(Facing facing);

typedef enum: unsigned short {
    LogChannel_None = 0,
} LogChannel;

typedef enum: unsigned short {
    LogChannelErr_None = 0,
    LogChannelErr_AmbientLoop,
    LogChannelErr_MusicLoop,
} LogChannelErr;

/* Messages are shown for 3 seconds + 0.1 per character */
static int util_log(unsigned short channel, char* fmt, ...);
static int util_err(unsigned short channel, char* fmt, ...);
static void util_trace(int loglevel, const char* text, va_list args); // For raylib
static void util_drawLog(void);
static void util_clearLog(void);

/* File handling wrappers for Raylib */
static unsigned char* util_readFileData(const char* filename, int* size);
static bool util_writeFileData(const char* filename, void* data, int size);
static char* util_readFileText(const char* filename);
static bool util_writeFileText(const char* filename, char* text);

static uint64_t util_rdtsc(void);
static int util_intmin(int a, int b);
static int util_intmax(int a, int b);
static int util_intclamp(int val, int min, int max);

/* JSON Parsing Helpers */
static bool util_jsonParseString(struct json_object_element_s* element, char* key, char* target, size_t size);
static bool util_jsonParseInteger(struct json_object_element_s* element, char* key, int64_t* target);
static bool util_jsonParseFloat(struct json_object_element_s* element, char* key, double* target);

/* Math helpers */
static Vector2 Vector2Floor(Vector2 v);
static Rectangle RectangleFloor(Rectangle r);

