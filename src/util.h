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

/* Messages are shown for 3 seconds + 0.1 per character */
static int util_log(unsigned short channel, char* fmt, ...);
static int util_err(unsigned short channel, char* fmt, ...);
static void util_drawLog(void);


