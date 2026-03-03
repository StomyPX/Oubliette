static char*
CharacterClass_toString(CharacterClass c)
{
    switch (c) {
        default:
            TraceLog(LOG_ERROR, "CHARACTER: Unknown class type, id: %i", c);
        case CharacterClass_None: return "None";
        case CharacterClass_Warrior: return "Warrior";
        case CharacterClass_Thief: return "Thief";
        case CharacterClass_Mage: return "Mage";
    }
}

static Character
char_random(void)
{
    Character c = {};

    // Roll stats and pick class based on results
    c.strength = PcgRandom_roll(&m->rng, 3, 6);
    c.dexterity = PcgRandom_roll(&m->rng, 3, 6);
    c.constitution = PcgRandom_roll(&m->rng, 3, 6);
    c.intellect = PcgRandom_roll(&m->rng, 3, 6);
    c.willpower = PcgRandom_roll(&m->rng, 3, 6);
    c.charisma = PcgRandom_roll(&m->rng, 3, 6);

    if (PcgRandom_roll(&m->rng, 1, 2) <= 1) {
        c.flags |= CharacterFlags_Female;
        c.strength -= 1;
        c.charisma += 1;
    } else {
        c.strength += 1;
        c.charisma -= 1;
    }

    if (c.intellect > c.dexterity && c.intellect > c.strength) {
        c.class = CharacterClass_Mage;
    } else if (c.dexterity > c.strength) {
        c.class = CharacterClass_Thief;
    } else {
        c.class = CharacterClass_Warrior;
    }

    { // Scan through portraits and pick an appropriate one
        char** portraits;
        size_t matches = 0;
        char gender;
        char class;

        portraits = PHYSFS_enumerateFiles("data/portraits");
        if (!portraits) {
            TraceLog(LOG_ERROR, "CHARACTER: Could not find any files in \"data/portraits\"");
            return c;
        }

        if (c.flags & CharacterFlags_Female) {
            gender = 'f';
        } else {
            gender = 'm';
        }

        switch (c.class) {
            default:
                TraceLog(LOG_ERROR, "CHARACTER: Unknown class type, id: %i", c.class);
            case CharacterClass_Warrior: {
                class = 'w';
            } break;
            case CharacterClass_Thief: {
                class = 't';
            } break;
            case CharacterClass_Mage: {
                class = 'm';
            } break;
        }

        for (size_t i = 0; i < 1024 && portraits[i]; i++) {
            char* p = portraits[i];
            char* flags = strrchr(p, '-');

            if (flags) {
                bool matchGender = tolower(flags[1]) == gender;
                bool matchClass = tolower(flags[2]) == class;
                if (matchGender && matchClass) {
                    matches += 1;
                }
            }
        }

        if (matches < 1) {
            TraceLog(LOG_ERROR, "CHARACTER: no files ending with \"-%c%c\" \"data/portraits\"", gender, class);
            return c;
        } else {
            size_t count = 0;
            size_t pick = PcgRandom_roll(&m->rng, 1, matches) - 1;

            for (size_t i = 0; i < 1024 && portraits[i]; i++) {
                char* p = portraits[i];
                char* flags = strrchr(p, '-');

                if (flags) {
                    bool matchGender = tolower(flags[1]) == gender;
                    bool matchClass = tolower(flags[2]) == class;
                    if (matchGender && matchClass) {
                        if (count == pick) {
                            char path[64] = {};
                            snprintf(path, sizeof(path), "data/portraits/%s", p);

                            c.portrait = LoadTexture(path);
                            GenTextureMipmaps(&c.portrait);
                            strncpy(c.name, p, flags - p);
                            c.name[0] = toupper(c.name[0]);
                            for (int i = 1; i < sizeof(c.name) && c.name[i]; i++) {
                                c.name[i] = tolower(c.name[i]);
                            }
                            break;
                        } else {
                            count += 1;
                        }
                    }
                }
            }
        }

        PHYSFS_freeList(portraits);
    }

    /* Final Init */
    c.level = 1;
    c.health = char_maxHealth(c);
    c.stamina = char_maxStamina(c);

    return c;
}

static void
char_free(Character* c)
{
    UnloadTexture(c->portrait);
    memset(c, 0, sizeof(*c));
}

static int32_t
char_maxHealth(Character c)
{
    int32_t h = 0;

    h += c.strength;
    h += c.constitution;

    switch (c.class) {
        case CharacterClass_Warrior: {
            h += ((int)c.level - 1) * 2;
        } break;
        case CharacterClass_Mage: {
            h += ((int)c.level - 1) / 2;
        } break;
        default: {
            h += (int)c.level - 1;
        } break;
    }

    return h;
}

static int32_t
char_maxStamina(Character c)
{
    int32_t s = 0;

    s += c.constitution;
    s += c.willpower;
    s += c.level;

    return s;
}

static int
char_modifier(int characteristic)
{
    int modifier = characteristic / 3 - 3;
    return modifier;
}

static void
char_exp(Character* c, int xp)
{
    uint8_t level;
    c->experience += xp;
    int32_t health = char_maxHealth(*c);
    int32_t stamina = char_maxStamina(*c);

    level = char_level(c->class, c->experience);
    for (int i = 0; c->level < level && i < UINT8_MAX; i++) {
        c->level++;
        ui_log(ORANGE, "%s advances to level %u!", c->name, c->level);
    }
    c->health += char_maxHealth(*c) - health;
    c->stamina += char_maxStamina(*c) - stamina;
}

static uint8_t
char_level(CharacterClass class, int64_t xp)
{
    uint8_t level;
    int64_t base;
    int64_t req;
    int64_t highbase;

    if (xp < 0)
        return 0;

    switch (class) {
        case CharacterClass_Warrior: {
            base = 2000;
        } break;
        case CharacterClass_Mage: {
            base = 2500;
        } break;
        case CharacterClass_Thief: {
            base = 1250;
        } break;
        default: {
            TraceLog(LOG_WARNING, "Unknown class ID: %i", class);
            base = 1000;
        } break;
    }

    req = base;
    for (level = 1; level < UINT8_MAX && xp > req; level++) {
        if (level < 8) {
            req += base * level;
            highbase = req;
        } else {
            req += highbase;
        }
    }

    return level;
}
