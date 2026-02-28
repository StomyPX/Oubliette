#define CLIP_COLOR (Color){250, 255, 30, 64}
#define CLEAR_COLOR (Color){6, 0, 3, 255}
#define TEXT_COLOR (Color){218, 209, 200, 255}
#define ERROR_COLOR (Color){255, 15, 21, 255}
#define GLOOM_COLOR (Color){61, 81, 20, 255}

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

/* Messages are shown for 3 seconds + 0.1 per character */
static int util_log(unsigned short channel, char* fmt, ...);
static int util_err(unsigned short channel, char* fmt, ...);
static void util_drawLog(void);


