#define COMBAT_BASE_DEFENSE 10
#define COMBAT_FAST_TIME 0.3f
#define COMBAT_SLOW_TIME 2.0f

typedef enum
{
    CombatSpeed_Instant = 0,
    CombatSpeed_Fast = 1,
    CombatSpeed_Slow = 2,
    CombatSpeed_Count = 3,
} CombatSpeed;

static float CombatSpeed_time(CombatSpeed cs);

typedef enum
{
    CombatState_Menu = 0,
    CombatState_Fighting,
    CombatState_Fleeing,
} CombatState;

typedef struct
{
    // Enemies
    MonsterStack stack;

    // Time Tracking
    CombatState state;
    int ticks;
    int segment;
    int initLow;
    int initHigh;
    float timer;
    CombatSpeed speed;

    // Target Selection
    int weights[4];
    int weightTotal;
} CombatEncounter;

static void combat_randomEncounter(MonstrousCompendium* monstrous);
static void combat_startFight(void);
static void combat_startFlee(void);

// Advance the clock
static void combat_resolveFight(void);
static void combat_resolveFlee(void);

// Actions
static void combat_attack(Character* c, MonsterStack* s);
static void combat_multiAttack(Character* c, MonsterStack* s);
