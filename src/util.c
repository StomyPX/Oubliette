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
        = 1.f + 0.05f * (float)(count < UTIL_LOGLINE_LENGTH ? count : UTIL_LOGLINE_LENGTH);
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
        = 1.f + 0.05f * (float)(count < UTIL_LOGLINE_LENGTH ? count : UTIL_LOGLINE_LENGTH);
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
                color = m->textColor;
            } else {
                color = m->errorColor;
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
