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

static MonsterClass
MonsterClass_init(struct json_object_s* object)
{
    MonsterClass mc = {};
    char texture[64] = {};
    int64_t integer;
    double floating;
    bool defaultAttack = true;
    bool defaultLevel = true;
    mc.anchor.x = -1.f;
    mc.anchor.y = -1.f;

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
        } else if (util_jsonParseInteger(element, "level", &integer)) {
            mc.level = integer;
            defaultLevel = false;
        } else if (util_jsonParseInteger(element, "abundance", &integer)) {
            mc.abundance = integer;
        } else if (util_jsonParseInteger(element, "danger", &integer)) {
            mc.danger = integer;
        } else if (util_jsonParseInteger(element, "groupDie", &integer)) {
            mc.groupDie = integer;
        } else if (util_jsonParseInteger(element, "groupModifier", &integer)) {
            mc.groupModifier = integer;
        } else if (util_jsonParseInteger(element, "stealth", &integer)) {
            mc.stealth = integer;

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

        /* Misc */
        } else if (util_jsonParseFloat(element, "anchor_x", &floating)) {
            mc.anchor.x = floating;
        } else if (util_jsonParseFloat(element, "anchor_y", &floating)) {
            mc.anchor.y = floating;
        }
    }

    if (mc.truename[0] && texture[0]) {
        char path[80];
        snprintf(path, sizeof(path), "data/monsters/%s", texture);
        mc.texture = LoadTexture(path);
        GenTextureMipmaps(&mc.texture);
        SetTextureFilter(mc.texture, TEXTURE_FILTER_TRILINEAR);
        SetTextureFilter(mc.texture, TEXTURE_FILTER_ANISOTROPIC_16X);

        /* Other initialization (plurals, derived defaults etc) */
        if (!mc.truenamePlural[0])
            snprintf(mc.truenamePlural, sizeof(mc.truenamePlural), "%ss", mc.truename);
        if (!mc.guessname[0])
            snprintf(mc.guessname, sizeof(mc.guessname), "%ss", mc.truename);
        if (!mc.guessnamePlural[0])
            snprintf(mc.guessnamePlural, sizeof(mc.guessnamePlural), "%ss", mc.guessname);
        if (defaultAttack && mc.hitDice > 0)
            mc.attack = mc.hitDice / 2;
        if (defaultLevel)
            mc.level = mc.hitDice;
        if (mc.groupDie < 1)
            mc.groupDie = 1;
        if (mc.experience < 1) {
            if (mc.level > 0) {
                mc.experience = mc.level * 100;
            } else if (mc.level == 0) {
                mc.experience = 75;
            } else {
                mc.experience = util_intmax(1, 100 / -mc.level);
            }
        }

        if (mc.anchor.x < -0.0001f || mc.anchor.x > 1.0001f) {
            mc.anchor.x = 0.5f;
        } else {
            mc.anchor.x = Clamp(mc.anchor.x, 0.f, 1.f);
        }

        if (mc.anchor.y < -0.0001f || mc.anchor.y > 1.0001f) {
            mc.anchor.y = 0.5f;
        } else {
            mc.anchor.y = Clamp(mc.anchor.y, 0.f, 1.f);
        }

        TraceLog(LOG_INFO, "MONSTERS: Class successfully parsed: %s", mc.truename);
    }

    return mc;
}

