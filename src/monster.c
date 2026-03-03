#define MONSTER_CAPACITY_PAGE 20

static void
monster_init(MonstrousCompendium* monstrous)
{
    // TODO Parse all json files under data/monsters and not just base.json
    char* file;
    int size;
    struct json_value_s* json;
    struct json_object_s* object;
    struct json_parse_result_s result;

    file = util_readFileData("data/monsters/base.json", &size);
    if (!file) {
        TraceLog(LOG_ERROR, "MONSTERS ERROR: Cannot load monsters from data/monsters/base.json");
        return;
    }

    json = json_parse_ex(file, size, JSON_PARSE_FLAGS, 0, 0, &result);
    if (!json) {
        TraceLog(LOG_ERROR, "MONSTERS ERROR: [%s] Failed to load, could not parse JSON: [line %llu, %llu] %s",
                "data/monsters/base.json", result.error_line_no, result.error_row_no, json_parse_error_toString(result.error));
        return;
    }
    if (json->type != json_type_object) {
        TraceLog(LOG_ERROR, "MONSTERS ERROR: [%s] Failed to load, JSON top-level structure isn't an object [%i]",
                "data/monsters/base.json", json->type);
        return;
    }

    TraceLog(LOG_DEBUG, "MONSTERS: Reading monsters from %s", "data/monsters/base.json");
    monstrous->capacity = MONSTER_CAPACITY_PAGE;
    monstrous->compendium = MemAlloc(sizeof(*monstrous->compendium) * monstrous->capacity);

    object = json_value_as_object(json);
    for (struct json_object_element_s* monster = object->start; monster; monster = monster->next) {
        if (util_stricmp((char*)monster->name->string, "monster") != 0) {
            TraceLog(LOG_DEBUG, "MONSTERS: Unexpected top-level element in %s: %s, skipping",
                    "data/monsters/base.json", monster->name->string);
            continue;
        }

        if (monster->value->type != json_type_object) {
            TraceLog(LOG_DEBUG, "MONSTERS: Monster in %s not specified as object, skipping",
                    "data/monsters/base.json");
            continue;
        }

        MonsterClass newMonsterClass = MonsterClass_init(json_value_as_object(monster->value));
        if (newMonsterClass.truename[0]) {
            if (monstrous->capacity >= monstrous->total) {
                monstrous->capacity += MONSTER_CAPACITY_PAGE;
                monstrous->compendium = MemRealloc(monstrous->compendium,
                        sizeof(*monstrous->compendium) * monstrous->capacity);
            }

            monstrous->compendium[monstrous->total++] = newMonsterClass;
        }
    }
}

static void
monster_cleanup(MonstrousCompendium* monstrous)
{
    MemFree(monstrous->compendium);
}

static void
monster_encounter(MonstrousCompendium* monstrous)
{
    /* TODO Danger modifier applies based on proximity to features, with entry and exit overriding all else */
    size_t count = 0;
    size_t count2 = 0;
    size_t pick;
    MonsterClass* monster = 0;

    /* TODO Traverse the compendium and total up all picks, then roll and loop again */
    for (int i = 0; i < monstrous->total; i++) {
        int contrib;
        monster = monstrous->compendium + i;

        contrib = monster->abundance - monster->danger;
        if (contrib > 0)
            count += contrib;
    }

    pick = PcgRandom_randomu(&m->rng) % count;
    for (int i = 0; i < monstrous->total; i++) {
        int contrib;
        monster = monstrous->compendium + i;

        contrib = monster->abundance - monster->danger;
        if (contrib > 0)
            count2 += contrib;

        if (count2 >= pick) {
            break;
        }
    }

    if (!monster) {
        monster = monstrous->compendium;
        TraceLog(LOG_WARNING, "MONSTER: Could not pick a monster to encounter, "
                "this shouldn't ever happen. Defaulting to %s",
                monster->truename);
    }

    /* Set as current encounter */
    m->flags |= GlobalFlags_Encounter;
    m->encounter.unit.class = *monster;
    m->encounter.unit.total = PcgRandom_roll(&m->rng, 1, monster->groupDie);
    m->encounter.unit.total += monster->groupModifier;
    m->encounter.unit.total = util_intclamp(m->encounter.unit.total, 1, MONSTER_UNIT_MAX);
    m->encounter.unit.alive = m->encounter.unit.total;

    /* Encounter message */
    if (m->encounter.unit.total == 1) {
        ui_log(ZINNWALDITEBROWN, "You encounter a %s!", monster->truename);
    } else {
        ui_log(ZINNWALDITEBROWN, "You encounter %i %s!", m->encounter.unit.total, monster->truenamePlural);
    }

    { /* Init health for every monster in the unit */
        int dieFace;
        int dieNum = 1;

        if (monster->hitDice >= 1) {
            dieFace = 8;
            dieNum = monster->hitDice;
        } else if (monster->hitDice == 0) {
            dieFace = 6;
        } else if (monster->hitDice == -1) {
            dieFace = 5;
        } else {
            dieFace = 8 / monster->hitDice;
            if (dieFace < 1)
                dieFace = 1;
        }

        for (unsigned i = 0; i < m->encounter.unit.total; i++) {
            m->encounter.unit.health[i] = PcgRandom_roll(&m->rng, dieNum, dieFace);
            TraceLog(LOG_TRACE, "MONSTER: %s, HP: %lli", monster->truename, m->encounter.unit.health[i]);
        }
    }

    PlaySound(m->encounter.klaxon);
}

static MonsterClass
MonsterClass_init(struct json_object_s* object)
{
    MonsterClass mc = {};
    char texture[64] = {};
    int64_t integer;
    bool defaultAttack = true;

    for (struct json_object_element_s* element = object->start; element; element = element->next) {

        /* Core Information */
        if (util_jsonParseString(element, "texture", texture, sizeof(texture))) {
        } else if (util_jsonParseString(element, "truename", mc.truename, sizeof(mc.truename))) {
        } else if (util_jsonParseString(element, "truenamePlural", mc.truenamePlural, sizeof(mc.truenamePlural))) {
        } else if (util_jsonParseString(element, "guessname", mc.guessname, sizeof(mc.guessname))) {
        } else if (util_jsonParseString(element, "guessnamePlural", mc.guessnamePlural, sizeof(mc.guessnamePlural))) {
        } else if (util_jsonParseInteger(element, "lore", &integer)) {
            mc.lore = integer;

        /* Encounter details */
        } else if (util_jsonParseInteger(element, "abundance", &integer)) {
            mc.abundance = integer;
        } else if (util_jsonParseInteger(element, "danger", &integer)) {
            mc.danger = integer;
        } else if (util_jsonParseInteger(element, "groupDie", &integer)) {
            mc.groupDie = integer;
        } else if (util_jsonParseInteger(element, "groupModifier", &integer)) {
            mc.groupModifier = integer;

        /* Combat stats */
        } else if (util_jsonParseInteger(element, "hitDice", &integer)) {
            mc.hitDice = integer;
        } else if (util_jsonParseInteger(element, "attack", &integer)) {
            mc.attack = integer;
            defaultAttack = false;
        } else if (util_jsonParseInteger(element, "defense", &integer)) {
            mc.defense = integer;
        } else if (util_jsonParseInteger(element, "damageDie", &integer)) {
            mc.damageDie = integer;
        } else if (util_jsonParseInteger(element, "damageModifier", &integer)) {
            mc.damageModifier = integer;
        } else if (util_jsonParseInteger(element, "initiative", &integer)) {
            mc.initiative = integer;

        /* Post-combat drops */
        } else if (util_jsonParseInteger(element, "experience", &integer)) {
            mc.experience = integer;
        }
    }

    if (mc.truename[0] && texture[0]) {
        char path[80];
        snprintf(path, sizeof(path), "data/monsters/%s", texture);
        mc.texture = LoadTexture(path);
        GenTextureMipmaps(&mc.texture);

        /* Other initialization (plurals, derived defaults etc) */
        if (!mc.truenamePlural[0])
            snprintf(mc.truenamePlural, sizeof(mc.truenamePlural), "%ss", mc.truename);
        if (!mc.guessname[0])
            snprintf(mc.guessname, sizeof(mc.guessname), "%ss", mc.truename);
        if (!mc.guessnamePlural[0])
            snprintf(mc.guessnamePlural, sizeof(mc.guessnamePlural), "%ss", mc.guessname);
        if (defaultAttack && mc.hitDice > 0)
            mc.attack = mc.hitDice / 2;

        TraceLog(LOG_INFO, "MONSTERS: Class successfully parsed: %s", mc.truename);
    }

    return mc;
}

