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
        if (m->party[i].health > 0) {
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

    for (int step = initHigh; step >= initLow; step--) {
        /* Player Actions */
        for (int i = 0; i < arrlen(m->party); i++) {
            ch = m->party + i;

            if (ch->initiative == step && ch->health > 0 && unit->alive > 0) {
                int target = PcgRandom_randomu(&m->rng) % unit->alive;
                int attacks = 1;
                int damageDie = 6;
                int accuracy = char_modifier(ch->strength);
                int damageMod = char_modifier(ch->strength);

                if (ch->class == CharacterClass_Warrior) {
                    damageDie = 8;
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
                    accuracy += ch->level / 2;
                } else if (ch->class == CharacterClass_Mage) {
                    damageDie = 4;
                    accuracy += ch->level / 4;
                } else {
                    accuracy += ch->level / 3;
                }

                for (int attack = 0; attack < attacks; attack++) {
                    int tohit = PcgRandom_roll(&m->rng, 1, 20);

                    if (tohit == 20 || (tohit + accuracy >= 10 + unit->class.defense && tohit != 1)) {
                        int damage = PcgRandom_roll(&m->rng, 1, damageDie) + damageMod;
                        damage = util_intmax(1, damage);
                        if (damage < unit->health[target]) {
                            ui_log(ZINNWALDITEBROWN, "%s strikes a %s for %i damage",
                                    ch->name, unit->class.truename, damage);
                        } else {
                            ui_log(ZINNWALDITEBROWN, "%s strikes a %s for %i damage, killing it!",
                                    ch->name, unit->class.truename, damage);
                        }
                        unit->health[target] -= damage;
                    } else {
                        ui_log(ZINNWALDITEBROWN, "%s swings but misses", ch->name);
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
                }
            }
        }

        /* Monster attacks */
        for (int i = 0; i < unit->alive; i++) {
            if (unit->initiative[i] == step) {
                /* Select target based on weighting */
                int targetRoll = PcgRandom_randomu(&m->rng) % weightTotal;
                int target;
                int tohit;

                for (target = 0; target < arrlen(m->party) - 1; target++) {
                    if (m->party[target].health < 1)
                        continue;
                    if (targetRoll <= weights[target]) {
                        break;
                    } else {
                        targetRoll -= weights[target];
                    }
                }

                ch = m->party + target;
                if (ch->health <= 0)
                    continue; /* Stop, he's already dead! */

                tohit = PcgRandom_roll(&m->rng, 1, 20);
                tohit -= util_intmax(0, char_modifier(ch->charisma)); // Only creatures of average intelligence should acknowledge this
                if (tohit == 20 || (tohit + unit->class.attack > 10 + char_modifier(ch->dexterity) && tohit != 1)) {
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
                    } else if (damage > 0) {
                        ui_log(BLACK, "A %s strikes %s for %i damage",
                                unit->class.truename, ch->name, damage);
                    }
                    ch->health -= damage;
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
        ui_log(ZINNWALDITEBROWN, "Everybody gains %i experience points", xp);
        for (int i = 0; i < arrlen(m->party); i++) {
            if (m->party[i].health > 0) {
                char_exp(m->party + i, xp);
            }
        }
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
