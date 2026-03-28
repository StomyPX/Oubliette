/* Forward Declarations */
typedef struct Memory_ Memory;

typedef enum {
    GlobalFlags_RequestQuit         = 1 << 0,
    GlobalFlags_EditorModePermitted = 1 << 1,
    GlobalFlags_EditorMode          = 1 << 2,
    GlobalFlags_PartyStats          = 1 << 3,
    GlobalFlags_ShowCollision       = 1 << 4,
    GlobalFlags_ShowMap             = 1 << 5,
    GlobalFlags_AdvanceTurn         = 1 << 6, // Whether time should be treated as advancing
    GlobalFlags_CombatRound         = 1 << 6, // These should safely be able to share the same bit
    GlobalFlags_Encounter           = 1 << 7,
    GlobalFlags_GameOver            = 1 << 8,
    GlobalFlags_TheEnd              = 1 << 9,
    GlobalFlags_MissionAccomplished = 1 << 10,
    GlobalFlags_Resting             = 1 << 11,
    GlobalFlags_ConfirmQuit         = 1 << 12,
    GlobalFlags_MuteSFX             = 1 << 13,
    GlobalFlags_IgnoreEncounters    = 1 << 14,
    GlobalFlags_ShowTileFlags       = 1 << 15,
    GlobalFlags_ShowTooltips        = 1 << 16,
    GlobalFlags_IgnoreInput         = 1 << 17,
} GlobalFlags_t; /* Need to append Type to avoid clash with symbols on windows */

