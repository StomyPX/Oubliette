
static int
util_stricmp(char* a, char* b)
{
    int ca, cb;
    do {
        ca = tolower(*a++);
        cb = tolower(*b++);
    } while (ca == cb && ca != 0);
    return ca - cb;
}

static int
util_pseudohex(char c)
{
    switch (toupper(c)) {
        case '.':
        case ' ':
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'A': return 10;
        case 'B': return 11;
        case 'C': return 12;
        case 'D': return 13;
        case 'E': return 14;
        case 'F': return 15;
        case 'G': return 16;
    }
    return -1;
}

static const char*
Facing_toString(Facing facing)
{
    switch (facing) {
        default: return "North";
        case Facing_East: return "East";
        case Facing_South: return "South";
        case Facing_West: return "West";
    }
}

static unsigned
util_traverse(Facing facing, int inx, int iny, int forward, int right, int* outX, int* outY)
{
    int x, y;
    switch (facing) {
        default: {
            x = inx + right;
            y = iny - forward;
        } break;

        case Facing_East: {
            x = inx + forward;
            y = iny + right;
        } break;

        case Facing_South: {
            x = inx - right;
            y = iny + forward;
        } break;

        case Facing_West: {
            x = inx - forward;
            y = iny - right;
        } break;
    }

    if (outX)
        *outX = x;
    if (outY)
        *outY = y;
    return x + y * TILE_COUNT;
}

static bool
util_facingNorthSouth(Facing facing)
{
    if (facing == Facing_North || facing == Facing_South) {
        return true;
    } else {
        return false;
    }
}

#define UTIL_LOGLINE_COUNT 32
#define UTIL_LOGLINE_LENGTH 128

typedef struct Util_LogLine
{
    float seconds;
    int channel;
    char text[UTIL_LOGLINE_LENGTH];
} Util_LogLine;

static Util_LogLine g_util_logLines[UTIL_LOGLINE_COUNT] = {};
static unsigned g_util_logLinesCursor = 0;

static int
util_log(unsigned short channel, char* fmt, ...)
{
    va_list ap;
    int count;
    // TODO Handle multiline
    // TODO Split over-long strings into multiple log lines.

#if DEBUG_MODE
    if (channel) {
        for (unsigned i = 0; i < UTIL_LOGLINE_COUNT; i++) {
            if (g_util_logLines[i].channel == channel) {
                memset(g_util_logLines + i, 0, sizeof(Util_LogLine));
            }
        }
    }

    va_start(ap, fmt);
    count = vsnprintf(g_util_logLines[g_util_logLinesCursor].text, UTIL_LOGLINE_LENGTH, fmt, ap);
    va_end(ap);
#endif

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);

    g_util_logLines[g_util_logLinesCursor].seconds
        = 5.f + (float)(count < UTIL_LOGLINE_LENGTH ? count : UTIL_LOGLINE_LENGTH);
    g_util_logLines[g_util_logLinesCursor].channel = channel;
    g_util_logLinesCursor += 1;
    g_util_logLinesCursor %= UTIL_LOGLINE_COUNT;
    return count;
}

static int
util_err(unsigned short channel, char* fmt, ...)
{
    va_list ap;
    int count;
    int errchannel = (int)channel * -1 - 1;
    // TODO Handle multiline
    // TODO Split over-long strings into multiple log lines.

    if (channel) {
        for (unsigned i = 0; i < UTIL_LOGLINE_COUNT; i++) {
            if (g_util_logLines[i].channel == errchannel) {
                memset(g_util_logLines + i, 0, sizeof(Util_LogLine));
            }
        }
    }

    va_start(ap, fmt);
    count = vsnprintf(g_util_logLines[g_util_logLinesCursor].text, UTIL_LOGLINE_LENGTH, fmt, ap);
    va_end(ap);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    g_util_logLines[g_util_logLinesCursor].seconds
        = 5.f + (float)(count < UTIL_LOGLINE_LENGTH ? count : UTIL_LOGLINE_LENGTH);
    g_util_logLines[g_util_logLinesCursor].channel = errchannel;
    g_util_logLinesCursor += 1;
    g_util_logLinesCursor %= UTIL_LOGLINE_COUNT;
    return count;
}

static void
util_drawLog(void)
{
    float deltaTime = GetFrameTime();
    Color color;
    Vector2 position = (Vector2){10.f, 10.f};
    Font font = GetFontDefault();

    for (int i = 0; i < UTIL_LOGLINE_COUNT; i++) {
        int index = (g_util_logLinesCursor - i + UTIL_LOGLINE_COUNT) % UTIL_LOGLINE_COUNT;
        if (g_util_logLines[index].seconds >= 0.f) {
            if (g_util_logLines[index].channel >= 0) {
                color = TEXT_COLOR;
            } else {
                color = ERROR_COLOR;
            }
            color = ColorAlpha(color, Clamp(g_util_logLines[index].seconds, 0.f, 1.f));
            ui_text(font, g_util_logLines[index].text, position, color, 1);
            position.y += font.baseSize + 2;
        }
    }

    for (unsigned i = 0; i < UTIL_LOGLINE_COUNT; i++) {
        g_util_logLines[i].seconds -= deltaTime;
    }
}
