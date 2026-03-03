typedef enum: uint32_t {
    CharacterFlags_Female = 1 << 0,
} CharacterFlags;

typedef enum: uint8_t {
    CharacterClass_None = 0,
    CharacterClass_Warrior,
    CharacterClass_Thief,
    CharacterClass_Mage,
} CharacterClass;
static char* CharacterClass_toString(CharacterClass c);

typedef struct {
    int32_t health;
    int32_t stamina;
    int32_t initiative;

    CharacterFlags flags;
    CharacterClass class;
    char name[32];
    Texture portrait;

    // Base characteristics
    uint8_t strength;
    uint8_t dexterity;
    uint8_t constitution;
    uint8_t intellect;
    uint8_t willpower;
    uint8_t charisma;

    // Thief skill points
    int16_t pickpocket;
    int16_t openlock;
    int16_t movesilent;
    int16_t removetrap;
    int16_t hide;
    int16_t detectsound;
    int16_t findsecret;

    int64_t experience;
    uint8_t level;
} Character;

static Character char_random(void); // Roll up an entirely random character, name will be blank on failure
static void char_free(Character* c);

static int32_t char_maxHealth(Character c);
static int32_t char_maxStamina(Character c);
static int char_modifier(int characteristic);
static void char_exp(Character* c, int xp);
static uint8_t char_level(CharacterClass class, int64_t xp);
