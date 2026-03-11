#define COMBAT_BASE_DEFENSE 10

static void combat_randomEncounter(MonstrousCompendium* monstrous);
static void combat_fight(void);
static void combat_flee(void);

// Actions
static void combat_attack(Character* c, Stack* s);
static void combat_multiAttack(Character* c, Stack* s);
