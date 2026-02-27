/* Forward Declarations */
typedef struct Memory_ Memory;

typedef enum {
    GlobalFlags_RequestQuit         = 1 << 0,
    GlobalFlags_EditorModePermitted = 1 << 1,
    GlobalFlags_EditorMode          = 1 << 2,
    GlobalFlags_PartyStats          = 1 << 3,
    GlobalFlags_ShowCollision       = 1 << 4,
} GlobalFlags_t; /* Need to append Type to avoid clash with symbols on windows */

