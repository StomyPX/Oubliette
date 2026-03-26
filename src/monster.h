#define MONSTER_STACK_MAX 20

typedef enum {
    MonsterStatus_Surprised = 1 << 0,
} MonsterStatus;

typedef struct
{
    /* Core Information */
    Texture texture;
    char truename[32];
    char truenamePlural[32]; /* These default to appending an s if not found */
    char guessname[32]; /* Default to same as truename if common enough */
    char guessnamePlural[32];
    uint32_t lore;

    /* Encounter details */
    int32_t abundance;
    int32_t danger;
    uint32_t groupDie;
    int32_t groupModifier;
    int8_t level;
    int8_t stealth; /* Affects chance to inflict surprise */

    /* Combat stats */
    int8_t hitDice;
    int8_t defense;
    int8_t attack;
    uint8_t damageDie;
    int8_t damageModifier;
    int8_t initiative;
    /* TODO Four slots for attacks/abilities */

    /* Post-combat drops */
    uint32_t experience;
    /* TODO Loot */

    /* Misc */
    Vector2 anchor;
} MonsterClass;
static MonsterClass MonsterClass_init(struct json_object_s* object); // Blank truename indicates failure

typedef struct
{
    MonsterClass* compendium;
    size_t total;
    size_t capacity;
} MonstrousCompendium;

typedef struct
{
    MonsterClass class;
    MonsterStatus status;
    uint32_t alive;
    uint32_t total;
    int64_t health[MONSTER_STACK_MAX];
    int32_t initiative[MONSTER_STACK_MAX];

    PortraitEffects effects;
} MonsterStack;

static void monster_init(MonstrousCompendium* monstrous);
static void monster_cleanup(MonstrousCompendium* monstrous);

