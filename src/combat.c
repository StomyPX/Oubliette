static void
combat_fight(void)
{
    Unit* unit;
    Character* ch;
    int32_t initHigh = INT32_MIN;
    int32_t initLow = INT32_MAX;
    char buffer[UI_LOGLINE_LENGTH];

    unit = &m->encounter.unit;

    /* Roll Initiative TODO Actions/Weapons have speed which affects initiative */
    for (int i = 0; i < unit->alive; i++) {
        unit->initiative[i] = PcgRandom_roll(&m->rng, 1, 20) + unit->class.initiative;
        if (unit->initiative[i] > initHigh)
            initHigh = unit->initiative[i];
        if (unit->initiative[i] < initLow)
            initLow = unit->initiative[i];
    }

    for (int i = 0; i < arrlen(m->party); i++) {
        ch = m->party + i;
        ch->initiative = PcgRandom_roll(&m->rng, 1, 20) + char_modifier(ch->dexterity);
        if (ch->health < 1)
            ch->initiative -= 10;
        if (ch->initiative > initHigh)
            initHigh = ch->initiative;
        if (ch->initiative < initLow)
            initLow = ch->initiative;
    }

    /* Target Weights based Charisma */
    int weights[4] = {};
    int weightTotal = 0;
    for (int i = 0; i < arrlen(m->party); i++) {
        if (m->party[i].health > 0 && !(m->party[i].flags & CharacterFlags_Hidden)) {
            ch = m->party + i;
            weights[i] = 24;
            weights[i] -= ch->charisma;
            switch (ch->class) {
                default:                        weights[i] += 2; break;
                case CharacterClass_Warrior:    weights[i] += 4; break;
                case CharacterClass_Mage:       break;
            }
            // TODO Using a melee weapon increases by a further +6
            weights[i] = util_intmax(1, weights[i]);
            weightTotal += weights[i];
        }
    }

    for (int segment = initHigh; segment >= initLow; segment--) {
        /* Player Actions */
        for (int i = 0; i < arrlen(m->party); i++) {
            ch = m->party + i;

            if (ch->initiative == segment && ch->health > 0 && unit->alive > 0) {
                switch (ch->action) {
                    default:
                        TraceLog(LOG_ERROR, "Unknown combat action id: %i, defaulting to attack", ch->action);
                    case CombatAction_Attack: combat_attack(ch, unit); break;
                    case CombatAction_MultiAttack: combat_multiAttack(ch, unit); break;
                    case CombatAction_Hide: {
                        int danger = -1;
                        if (m->flags & GlobalFlags_MissionAccomplished) {
                            danger = 1;
                        } else if (abs(m->partyX - m->map.entryX) + abs(m->partyY - m->map.entryY) > TILE_COUNT / 3) {
                            danger = 0;
                        }
                        int chance = ch->hide + ch->level + char_modifier(ch->dexterity) - danger * 30;
                        if (PcgRandom_roll(&m->rng, 1, 100) < chance) {
                            ui_log(ZINNWALDITEBROWN, "%s melds with the shadows", ch->name);
                            ch->flags |= CharacterFlags_Hidden;
                        }
                    } break;
                }
            }

            /* Stamina Check */
            if (ch->stamina < 0) {
                int multiple = 3;
                int loss = -ch->stamina / multiple;
                ch->stamina %= multiple;
                ch->health -= loss;

                if (ch->health <= 0) {
                    ch->health = 0;
                    ui_log(MAROON, "%s collapses from exhaustion!", ch->name);
                }
            }
        }

        /* Monster attacks */
        if (weightTotal < 1) {
            if (unit->alive == 1) {
                ui_log(ZINNWALDITEBROWN, "The %s peers through the darkness but finds nobody to attack",
                        unit->class.truename);
            } else {
                ui_log(ZINNWALDITEBROWN, "The %s peer through the darkness but find nobody to attack",
                        unit->class.truenamePlural);
            }
            continue;
        }

        for (int i = 0; i < unit->alive; i++) {
            if (unit->initiative[i] == segment) {
                /* Select target based on weighting */
                int targetRoll = PcgRandom_randomu(&m->rng) % weightTotal;
                int target;
                int tohit;

                for (target = 0; target < arrlen(m->party) - 1; target++) {
                    if (m->party[target].health < 1)
                        continue;
                    if (m->party[target].flags & CharacterFlags_Hidden)
                        continue;
                    if (targetRoll <= weights[target]) {
                        break;
                    } else {
                        targetRoll -= weights[target];
                    }
                }

                ch = m->party + target;
                if (ch->flags & CharacterFlags_Hidden)
                    continue; /* Wha!? Where did he go!? */
                if (ch->health <= 0)
                    continue; /* Stop, he's already dead! */

                tohit = PcgRandom_roll(&m->rng, 1, 20);
                tohit -= util_intmax(0, char_modifier(ch->charisma)); // Only creatures of average intelligence should acknowledge this
                if (tohit == 20 || (tohit + unit->class.attack > COMBAT_BASE_DEFENSE + char_modifier(ch->dexterity) && tohit != 1)) {
                    int damage = PcgRandom_roll(&m->rng, 1, unit->class.damageDie) + unit->class.damageModifier;
                    damage = util_intmax(1, damage);
                    switch (ch->class) {
                        case CharacterClass_Warrior: {
                            damage -= 3;
                        } break;
                        case CharacterClass_Mage: {
                            // No protection
                        } break;
                        default: {
                            damage -= 1;
                        } break;
                    }
                    damage = util_intmax(0, damage);
                    if (damage >= ch->health) {
                        ui_log(MAROON, "A %s strikes %s for %i damage and kills %s!",
                                unit->class.truename, ch->name, damage,
                                (ch->flags & CharacterFlags_Female) ? "her" : "him");
                        ch->stamina = 0;
                    } else if (damage > 0) {
                        ui_log(BLACK, "A %s strikes %s for %i damage",
                                unit->class.truename, ch->name, damage);
                    } else {
                        TraceLog(LOG_TRACE, "LOG: A %s swings at %s but %s armor deflects all damage",
                                unit->class.truename, ch->name,
                                ch->flags & CharacterFlags_Female ? "her" : "his");
                    }
                    ch->health -= damage;
                } else {
                    TraceLog(LOG_TRACE, "LOG: A %s swings at %s but misses", unit->class.truename, ch->name);
                }
            }
        }
    }

    /* Check if the party is still alive */
    int survivors = 0;
    for (int i = 0; i < arrlen(m->party); i++) {
        if (m->party[i].health > 0)
            survivors += 1;
    }

    if (survivors < 1) {
        ui_log(MAROON, "GAME OVER!");
        m->flags |= GlobalFlags_GameOver;

    } else if (unit->alive < 1) {
        int xp;

        m->flags &= ~(GlobalFlags_Encounter);
        ui_log(ORANGE, "Victory!");

        /* Loot and XP */
        xp = unit->total * unit->class.experience / survivors;
        ui_log(ZINNWALDITEBROWN, "Survivors gain %i experience points", xp);
        for (int i = 0; i < arrlen(m->party); i++) {
            if (m->party[i].health > 0) {
                char_exp(m->party + i, xp);
            }
        }
    } else {
        TraceLog(LOG_TRACE, "COMBAT: New Round");
    }
}

static void
combat_flee(void)
{
    ui_log(ZINNWALDITEBROWN, "The party attempts to flee...");
    /* Parting shots */
    for (int i = 0; i < arrlen(m->party); i++) {
        Character* c = m->party + i;
        int save = c->dexterity;
        int roll = PcgRandom_roll(&m->rng, 1, 20);

        if (c->health < 1)
            continue;

        if (c->flags & CharacterFlags_Hidden)
            continue;

        if (c->class == CharacterClass_Thief) {
            save += 1;
        }

        if (save > 19)
            save = 19;

        if (roll == 20 || roll > save) {
            MonsterClass* monster = &m->encounter.unit.class;
            int damage = PcgRandom_roll(&m->rng, 1, monster->damageDie);
            damage += monster->damageModifier;
            if (damage < 1)
                damage = 1;
            switch (c->class) {
                case CharacterClass_Warrior: {
                    damage -= 3;
                } break;
                case CharacterClass_Mage: {
                    // No protection
                } break;
                default: {
                    damage -= 1;
                } break;
            }
            if (damage < 0)
                damage = 0;
            c->health -= damage;
            if (c->health <= 0) {
                ui_log(MAROON, "%s is hit with a parting shot from a %s for %i damage, killing %s!",
                        c->name, monster->truename, damage,
                        (c->flags & CharacterFlags_Female) ? "her" : "him");
            } else if (damage > 0) {
                ui_log(BLACK, "%s is hit with a parting shot from a %s for %i damage",
                        c->name, monster->truename, damage);
            }
        }
    }


    { /* Check if the party is still alive */
        int survivors = 0;
        for (int i = 0; i < arrlen(m->party); i++) {
            if (m->party[i].health > 0)
                survivors += 1;
        }

        if (survivors < 1) {
            ui_log(BLACK, "GAME OVER!");
            m->flags |= GlobalFlags_GameOver;
        } else {
            ui_log(ZINNWALDITEBROWN, "The party escapes");
            m->flags &= ~(GlobalFlags_Encounter);
        }
    }
}

static void
combat_attack(Character* ch, Unit* unit)
{
    int target = PcgRandom_randomu(&m->rng) % unit->alive;
    int damageDie = 6; // TODO Determined by weapon
    int accuracy = char_modifier(ch->strength);
    int damageMod = char_modifier(ch->strength);

    if (ch->class == CharacterClass_Warrior) {
        accuracy += 1 + ch->level / 2;
    } else if (ch->class == CharacterClass_Mage) {
        accuracy += ch->level / 4;
    } else {
        accuracy += ch->level / 3;
    }

    if (ch->stamina < 1) {
        accuracy -= 2;
    }

    {
        int tohit = 0;

        { /* Explosive Roll */
            int roll;
            int i = 0;
            do {
                roll = PcgRandom_roll(&m->rng, 1, 20);
                tohit += roll;
                i++;
            } while (roll == 20 && i < 3);
        }

        if (tohit != 1 && tohit + accuracy >= COMBAT_BASE_DEFENSE + unit->class.defense) {
            int damage = damageMod;
            int critRange = 20;
            char* verb = "strikes";
            Color color = ZINNWALDITEBROWN;

            damage += PcgRandom_roll(&m->rng, 1, damageDie);

            { /* check for critical hit */
                if (ch->class == CharacterClass_Thief) {
                    if (ch->flags & CharacterFlags_Hidden) {
                        critRange = 0;
                    } else {
                        critRange -= 2;
                    }
                }

                if (tohit + accuracy >= COMBAT_BASE_DEFENSE + critRange + unit->class.defense) {
                    damage += PcgRandom_roll(&m->rng, 1, damageDie);
                    verb = "critically hits";
                    color = WHITE;
                }
            }

            damage = util_intmax(1, damage);
            if (damage < unit->health[target]) {
                ui_log(color, "%s %s a %s for %i damage",
                        ch->name, verb, unit->class.truename, damage);
                ch->flags &= ~(CharacterFlags_Hidden);
            } else {
                ui_log(color, "%s %s a %s for %i damage, killing it!",
                        ch->name, verb, unit->class.truename, damage);
                if (ch->class != CharacterClass_Thief)
                    ch->flags &= ~(CharacterFlags_Hidden);
            }
            unit->health[target] -= damage;
        } else {
            if (ch->flags & CharacterFlags_Hidden) {
                int chance = ch->hide * 2 + ch->level + char_modifier(ch->dexterity);
                if (PcgRandom_roll(&m->rng, 1, 100) >= chance)
                    ch->flags &= ~(CharacterFlags_Hidden);
            }
            ui_log(ZINNWALDITEBROWN, "%s swings but misses", ch->name);
        }
    }

    if (unit->health[target] > 0) {
        target += 1;
        target %= unit->alive;
    } else if (unit->alive > 1) {
        unit->alive -= 1;
        unit->health[target] = unit->health[unit->alive];
        unit->initiative[target] = unit->initiative[unit->alive];
    } else {
        unit->alive = 0;
    }

    /* Stamina Expenditure */
    if (PcgRandom_roll(&m->rng, 1, 3) == 1) {
        ch->stamina -= 1;
    }
}

static void
combat_multiAttack(Character* ch, Unit* unit)
{
    int attacks = 1;
    if (ch->level > unit->class.hitDice) {
        int cl = ch->level;
        int ml = unit->class.hitDice;

        if (unit->class.hitDice == 0) {
            cl *= 4;
            ml = 3;
        } else if (unit->class.hitDice == -1) {
            cl *= 3;
            ml = 2;
        } else if (unit->class.hitDice < -1) {
            cl *= -unit->class.hitDice;
            ml = 1;
        }

        attacks = util_intclamp(cl / ml, attacks, unit->alive);
    }

    for (int i = 0; i < attacks; i++) {
        combat_attack(ch, unit);
    }
}
