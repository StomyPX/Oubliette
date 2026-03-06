
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

#define UTIL_LOGLINE_COUNT 64
#define UTIL_LOGLINE_LENGTH 128

typedef struct Util_LogLine
{
    float seconds;
    int channel;
    char text[UTIL_LOGLINE_LENGTH];
} Util_LogLine;

static Util_LogLine g_util_logLines[UTIL_LOGLINE_COUNT] = {};
static unsigned g_util_logLinesCursor = 0;
static PHYSFS_File* g_util_logFile;

static void
util_logInit(void)
{
    g_util_logFile = PHYSFS_openWrite("console.log");
    if (!g_util_logFile) {
        util_err(0, "PHYSFS: Failed to open \"console.log\"! Error: %s",
                    PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }
}

static void
util_logCleanup(void)
{
    PHYSFS_close(g_util_logFile);
}

static int
util_log(unsigned short channel, char* fmt, ...)
{
    int count;
    va_list ap;
    // TODO Handle multiline
    // TODO Split over-long strings into multiple log lines.

#if DEBUG_MODE
    bool safe = false;

    if (channel) {
        for (unsigned i = 0; i < UTIL_LOGLINE_COUNT; i++) {
            if (g_util_logLines[i].channel == channel) {
                memset(g_util_logLines + i, 0, sizeof(Util_LogLine));
            }
        }
    }

    // Don't overwrite error messages
    for (int i = 0; i < UTIL_LOGLINE_COUNT && !safe; i++) {
        Util_LogLine* line = g_util_logLines + g_util_logLinesCursor;
        if (line->channel >= 0 || line->seconds <= 0.f) {
            safe = true;
            break;
        } else {
            g_util_logLinesCursor += 1;
            g_util_logLinesCursor %= UTIL_LOGLINE_COUNT;
        }
    }

    if (!safe)
        goto skip;

    va_start(ap, fmt);
    count = vsnprintf(g_util_logLines[g_util_logLinesCursor].text, UTIL_LOGLINE_LENGTH, fmt, ap);
    va_end(ap);

    g_util_logLines[g_util_logLinesCursor].seconds
        = 5.f + 0.1f * (float)(count < UTIL_LOGLINE_LENGTH ? count : UTIL_LOGLINE_LENGTH);
    g_util_logLines[g_util_logLinesCursor].channel = channel;
    g_util_logLinesCursor += 1;
    g_util_logLinesCursor %= UTIL_LOGLINE_COUNT;
skip:
#endif

    va_start(ap, fmt);
    count = vfprintf(stdout, fmt, ap);
    va_end(ap);

    if (g_util_logFile) {
        char line[256] = {};

        va_start(ap, fmt);
        vsnprintf(line, sizeof(line), fmt, ap);
        va_end(ap);

        PHYSFS_writeBytes(g_util_logFile, line, strlen(line));
        PHYSFS_writeBytes(g_util_logFile, "\n", 1);
    }
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

    g_util_logLines[g_util_logLinesCursor].seconds
        = 5.f + 0.1f * (float)(count < UTIL_LOGLINE_LENGTH ? count : UTIL_LOGLINE_LENGTH);
    g_util_logLines[g_util_logLinesCursor].channel = errchannel;
    g_util_logLinesCursor += 1;
    g_util_logLinesCursor %= UTIL_LOGLINE_COUNT;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (g_util_logFile) {
        char* line = g_util_logLines[g_util_logLinesCursor].text;
        PHYSFS_writeBytes(g_util_logFile, line, strlen(line));
        PHYSFS_writeBytes(g_util_logFile, "\n", 1);
    }
    return count;
}

static void
util_trace(int loglevel, const char* text, va_list args)
{
    int maxLog;
    int count;
    int channel = 0;
    va_list args2, args3;
    va_copy(args2, args);
    va_copy(args3, args);

    if (loglevel >= LOG_ERROR) {
        vfprintf(stderr, text, args2);
        fprintf(stderr, "\n");
        channel = -1;
    } else {
        vfprintf(stdout, text, args2);
        fprintf(stdout, "\n");
    }

#if DEBUG_MODE
    maxLog = LOG_DEBUG;
#else
    maxLog = LOG_WARNING;
#endif

    if (loglevel >= maxLog) { // Don't overwrite error messages
        if (channel >= 0) {
            bool safe = false;
            for (int i = 0; i < UTIL_LOGLINE_COUNT && !safe; i++) {
                Util_LogLine* line = g_util_logLines + g_util_logLinesCursor;
                if (line->channel >= 0 || line->seconds <= 0.f) {
                    safe = true;
                    break;
                } else {
                    g_util_logLinesCursor += 1;
                    g_util_logLinesCursor %= UTIL_LOGLINE_COUNT;
                }
            }

            if (!safe)
                goto skip;
        }

        count = vsnprintf(g_util_logLines[g_util_logLinesCursor].text, UTIL_LOGLINE_LENGTH, text, args);

        g_util_logLines[g_util_logLinesCursor].seconds
            = 5.f + 0.1f * (float)(count < UTIL_LOGLINE_LENGTH ? count : UTIL_LOGLINE_LENGTH);
        g_util_logLines[g_util_logLinesCursor].channel = channel;
        g_util_logLinesCursor += 1;
        g_util_logLinesCursor %= UTIL_LOGLINE_COUNT;

        skip:
    }

    if (g_util_logFile) {
        char line[256] = {};

        vsnprintf(line, sizeof(line), text, args3);

        PHYSFS_writeBytes(g_util_logFile, line, strlen(line));
        PHYSFS_writeBytes(g_util_logFile, "\n", 1);
    }
}

static void
util_drawLog(void)
{
    float deltaTime = GetFrameTime();
    Color color;
    Vector2 position = (Vector2){20.f, 20.f};
    Font font = GetFontDefault();

    if (IsKeyDown(KEY_KP_ENTER))
        deltaTime *= 10.f;

    for (unsigned i = 0; i < UTIL_LOGLINE_COUNT; i++) {
        unsigned index = (g_util_logLinesCursor + i) % UTIL_LOGLINE_COUNT;
        if (g_util_logLines[index].seconds >= 0.f) {
            if (g_util_logLines[index].channel >= 0) {
                color = BONE;
            } else {
                color = ERROR_COLOR;
            }
            color = ColorAlpha(color, Clamp(g_util_logLines[index].seconds, 0.f, 1.f));
            ui_text(font, g_util_logLines[index].text, position, font.baseSize, 1, color);
            position.y += font.baseSize + 2;
        }
    }

    for (unsigned i = 0; i < UTIL_LOGLINE_COUNT; i++) {
        unsigned index = (g_util_logLinesCursor + i) % UTIL_LOGLINE_COUNT;
        g_util_logLines[index].seconds -= deltaTime;
        deltaTime *= 0.99f;
    }
}

static void
util_clearLog(void)
{
    for (unsigned i = 0; i < UTIL_LOGLINE_COUNT; i++) {
        if (g_util_logLines[i].seconds > 1.f)
            g_util_logLines[i].seconds = 1.f;
    }
}

static unsigned char*
util_readFileData(const char* filename, int* size)
{
    PHYSFS_Stat stat;
    PHYSFS_File* file;
    PHYSFS_sint64 count;
    void* buffer;

    if (!PHYSFS_exists(filename)) {
        TraceLog(LOG_WARNING, "PHYSFS: [%s] does not exist", filename);
        *size = 0;
        return 0;
    }

    PHYSFS_stat(filename, &stat);
    buffer = MemAlloc(stat.filesize);
    if (!buffer) {
        TraceLog(LOG_ERROR, "PHYSFS: could not allocate memory to read [%s]", filename);
        return 0;
    }

    file = PHYSFS_openRead(filename);
    if (!file) {
        TraceLog(LOG_ERROR, "PHYSFS: [%s] could not be opened, err: %s",
                filename, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        return 0;
    }

    count = PHYSFS_readBytes(file, buffer, stat.filesize);
    if (count < 0) {
        TraceLog(LOG_ERROR, "PHYSFS: [%s] could not be read, err: %s",
                filename, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        return 0;
    } else if (count < stat.filesize) {
        TraceLog(LOG_WARNING, "PHYSFS: [%s] did not read to completion %lli/%i, err: %s",
                filename, count, size, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }

    if (!PHYSFS_close(file)) {
        TraceLog(LOG_WARNING, "PHYSFS: [%s] could not be closed after reading, err: %s",
                filename, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }

    if (size)
        *size = stat.filesize;
    TraceLog(LOG_TRACE, "PHYSFS: [%s] read successfully", filename);
    return buffer;
}

static bool
util_writeFileData(const char* filename, void* data, int size)
{
    PHYSFS_File* file;
    PHYSFS_sint64 count;

    file = PHYSFS_openWrite(filename);
    if (!file) {
        TraceLog(LOG_ERROR, "PHYSFS: [%s] could not be opened for writing, err: %s",
                filename, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        return false;
    }

    count = PHYSFS_writeBytes(file, data, size);
    if (count < 0) {
        TraceLog(LOG_ERROR, "PHYSFS: [%s] could not be written, err: %s",
                filename, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        return false;
    } else if (count < size) {
        TraceLog(LOG_WARNING, "PHYSFS: [%s] did not write to completion %lli/%i, err: %s",
                filename, count, size, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }

    if (!PHYSFS_close(file)) {
        TraceLog(LOG_WARNING, "PHYSFS: [%s] could not be closed after writing, err: %s",
                filename, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }

    TraceLog(LOG_TRACE, "PHYSFS: [%s] written successfully", filename);
    return true;
}

static char*
util_readFileText(const char* filename)
{
    return util_readFileData(filename, 0);
}

static bool
util_writeFileText(const char* filename, char* text)
{
    return util_writeFileData(filename, text, strlen(text));
}

static uint64_t
util_rdtsc(void)
{
    uint32_t low, high;
    __asm__ __volatile__ ("rdtsc" : "=a" (low), "=d" (high));
    return ((uint64_t) high << 32) | low;
}

static int
util_intmin(int val, int min)
{
    if (val > min)
        val = min;
    return val;
}

static int
util_intmax(int val, int max)
{
    if (val < max)
        val = max;
    return val;
}

static int
util_intclamp(int val, int min, int max)
{
    if (min > max) {
        int temp = max;
        max = min;
        min = temp;
    }

    if (val < min)
        val = min;
    if (val > max)
        val = max;
    return val;
}

static bool
util_jsonParseString(struct json_object_element_s* element, char* key, char* target, size_t size)
{
    if (util_stricmp((char*)element->name->string, key) != 0)
        return false;

    if (element->value->type == json_type_string) {
        snprintf(target, size, json_value_as_string(element->value)->string);
    } else {
        TraceLog(LOG_DEBUG, "MONSTERS: Element for %s was expected to be a string but was a %s",
                key, json_type_toString(element->value->type));
    }

    return true;
}

static bool
util_jsonParseInteger(struct json_object_element_s* element, char* key, int64_t* target)
{
    if (util_stricmp((char*)element->name->string, key) != 0)
        return false;

    if (element->value->type == json_type_number) {
        *target = strtoll(json_value_as_number(element->value)->number, 0, 0);
    } else {
        TraceLog(LOG_DEBUG, "MONSTERS: Element for %s was expected to be a number but was a %s",
                key, json_type_toString(element->value->type));
    }

    return true;
}

static bool
util_jsonParseFloat(struct json_object_element_s* element, char* key, double* target)
{
    if (util_stricmp((char*)element->name->string, key) != 0)
        return false;

    if (element->value->type == json_type_number) {
        *target = strtod(json_value_as_number(element->value)->number, 0);
    } else {
        TraceLog(LOG_DEBUG, "MONSTERS: Element for %s was expected to be a number but was a %s",
                key, json_type_toString(element->value->type));
    }

    return true;
}

static Vector2
Vector2Floor(Vector2 v)
{
    v.x = floorf(v.x);
    v.y = floorf(v.y);
    return v;
}

static Rectangle
RectangleFloor(Rectangle v)
{
    v.x = floorf(v.x);
    v.y = floorf(v.y);
    v.width = floorf(v.width);
    v.height = floorf(v.height);
    return v;
};
