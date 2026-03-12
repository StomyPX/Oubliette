static void
combat_randomEncounter(MonstrousCompendium* monstrous)
{
    size_t count = 0;
    size_t count2 = 0;
    size_t pick;
    MonsterClass* monster = 0;
    int danger = -1;

    if (m->flags & GlobalFlags_MissionAccomplished) {
        danger = 1;
    //} else if (abs(m->partyX - m->map.entryX) + abs(m->partyY - m->map.entryY) > TILE_COUNT / 3) {
        //danger = 0;
    }
    TraceLog(LOG_TRACE, "MONSTER: Encounter danger: %i", danger);

    /* Traverse the compendium and total up all picks, then roll and loop again */
    for (int i = 0; i < monstrous->total; i++) {
        int contrib;
        monster = monstrous->compendium + i;

        contrib = monster->abundance + monster->danger * danger;
        if (contrib > 0)
            count += contrib;
    }

    pick = PcgRandom_randomu(&m->rng) % count;
    for (int i = 0; i < monstrous->total; i++) {
        int contrib;
        monster = monstrous->compendium + i;

        contrib = monster->abundance + monster->danger * danger;
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
    m->encounter.stack.class = *monster;
    m->encounter.stack.total = PcgRandom_roll(&m->rng, 1, monster->groupDie);
    m->encounter.stack.total += monster->groupModifier;
    m->encounter.stack.total = util_intclamp(m->encounter.stack.total, 1, MONSTER_STACK_MAX);
    m->encounter.stack.alive = m->encounter.stack.total;

    /* Encounter message */
    if (m->encounter.stack.total == 1) {
        ui_log(BLACK, "You encounter a %s!", monster->truename);
    } else {
        ui_log(BLACK, "You encounter %i %s!", m->encounter.stack.total, monster->truenamePlural);
    }

    { /* Init health for every monster in the stack */
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

        for (unsigned i = 0; i < m->encounter.stack.total; i++) {
            m->encounter.stack.health[i] = PcgRandom_roll(&m->rng, dieNum, dieFace);
            TraceLog(LOG_TRACE, "MONSTER: %s, HP: %lli", monster->truename, m->encounter.stack.health[i]);
        }
    }

    /* Automatic abilities */
    for (int i = 0; i < arrlen(m->party); i++) {
        Character* ch = m->party + i;
        ch->flags &= ~(CharacterFlags_Hidden);

        if (ch->health <= 0)
            continue;

        { /* Hide chance */
            int chance = 0;

            if (m->flags & GlobalFlags_Resting) {
                if (ch->activity == WaitActivity_Hide) {
                    chance += char_hideChance(ch);
                    chance -= danger * 30;
                    if (ch->class == CharacterClass_Thief)
                        chance += ch->level;
                }
            } else if (ch->class == CharacterClass_Thief) {
                chance += char_hideChance(ch);
                chance -= danger * 30;
            }

            if (ch->stamina < 1)
                chance -= 20;

            if ((int)PcgRandom_roll(&m->rng, 1, 100) < chance)
                ch->flags |= CharacterFlags_Hidden;
        }
    }

    /* Surprise check (per stack and per character) */
        bool ambush = true;
        for (int i = 0; i < arrlen(m->party); i++) {
            if (!(m->party[i].flags & CharacterFlags_Hidden)) {
                ambush = false;
                break;
            }
        }

    if (ambush) {
        ui_log(ZINNWALDITEBROWN, "You catch the enemy by surprise!");
        m->encounter.stack.status |= MonsterStatus_Surprised;
    } else if (m->flags & GlobalFlags_Resting) {
        int monsterSurprise = -monster->stealth;
        int roll;

        for (int i = 0; i < arrlen(m->party); i++) {
            Character* ch = m->party + i;
            int chance = monster->stealth;

            if (ch->flags & CharacterFlags_Hidden) {/* Can't be surprised if you're hidden */
                monsterSurprise += 1;
                continue;
            }

            switch (ch->activity) {
                default: {
                    chance += 1;
                } break;

                case WaitActivity_Rest: {
                    if (ch->health < char_maxHealth(*ch))
                        chance += 2;
                    if (ch->stamina < char_maxStamina(*ch))
                        chance += 1;
                } break;

                case WaitActivity_Guard: {
                    chance -= 1;
                } break;

                case WaitActivity_Hide: {
                    /* no change */
                } break;
            }

            roll = PcgRandom_roll(&m->rng, 1, 6);
            if (roll <= chance) {
                ch->flags |= CharacterFlags_Surprised;
            }
        }

        /* TODO If the party hasn't lit their lantern, the monsters may also be surprised */

        roll = PcgRandom_roll(&m->rng, 1, 6);
        if (roll <= monsterSurprise) {
            ui_log(ZINNWALDITEBROWN, "You catch the enemy by surprise!");
            m->encounter.stack.status |= MonsterStatus_Surprised;
        }

    } else {
        int partySurprise = monster->stealth;
        int monsterSurprise = -monster->stealth;
        int roll;

        /* TODO If the party hasn't lit their lantern, both sides have a chance of being caught by surprise */

        for (int i = 0; i < arrlen(m->party); i++) {
            Character* ch = m->party + i;

            if (ch->flags & CharacterFlags_Hidden) { /* Can't be surprised if you're hidden */
                monsterSurprise += 1;
                continue;
            }

            roll = PcgRandom_roll(&m->rng, 1, 6);
            if (roll <= partySurprise) {
                ch->flags |= CharacterFlags_Surprised;
            }
        }

        roll = PcgRandom_roll(&m->rng, 1, 6);
        if (roll <= monsterSurprise) {
            ui_log(ZINNWALDITEBROWN, "You catch the enemy by surprise!");
            m->encounter.stack.status |= MonsterStatus_Surprised;
        }
    }

    PlaySound(m->encounter.klaxon);
}

static void
combat_fight(void)
{
    Stack* stack;
    Character* ch;
    int32_t initHigh = INT32_MIN;
    int32_t initLow = INT32_MAX;
    char buffer[UI_LOGLINE_LENGTH];

    stack = &m->encounter.stack;

    /* Roll Initiative TODO Actions/Weapons have speed which affects initiative */
    for (int i = 0; i < stack->alive; i++) {
        stack->initiative[i] = PcgRandom_roll(&m->rng, 1, 20) + stack->class.initiative;
        if (stack->initiative[i] > initHigh)
            initHigh = stack->initiative[i];
        if (stack->initiative[i] < initLow)
            initLow = stack->initiative[i];
    }

    for (int i = 0; i < arrlen(m->party); i++) {
        ch = m->party + i;
        ch->initiative = PcgRandom_roll(&m->rng, 1, 20) + char_modifier(ch->dexterity);
        if (ch->stamina < 1)
            ch->initiative -= 3;
        if (ch->health < 1)
            ch->initiative -= 10;
        if (ch->action == CombatAction_CastSpell)
            ch->initiative += 3;
        if (ch->initiative > initHigh)
            initHigh = ch->initiative;
        if (ch->initiative < initLow)
            initLow = ch->initiative;
    }

    /* Target Weights based Charisma */
    int weights[4] = {};
    int weightTotal = 0;
    for (int i = 0; i < arrlen(m->party); i++) {
        ch = m->party + i;
        if (ch->health > 0 && !(ch->flags & CharacterFlags_Hidden)) {
            weights[i] = 24;
            if (ch->action == CombatAction_GuardOthers) {
                weights[i] += util_intmax(0, ch->charisma);
                weights[i] += 2;
            } else {
                weights[i] -= ch->charisma;
            }

            if (ch->action == CombatAction_DefendSelf)
                weights[i] -= 2;
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

            if (ch->initiative == segment && ch->health > 0 && stack->alive > 0 && !(ch->flags & CharacterFlags_Surprised)) {
                switch (ch->action) {
                    default:
                        TraceLog(LOG_ERROR, "Unknown combat action id: %i, defaulting to attack", ch->action);
                    case CombatAction_Attack: combat_attack(ch, stack); break;
                    case CombatAction_MultiAttack: combat_multiAttack(ch, stack); break;
                    case CombatAction_DefendSelf: break;
                    case CombatAction_GuardOthers: break;
                    case CombatAction_Hide: {
                        int danger = -1;
                        if (m->flags & GlobalFlags_MissionAccomplished) {
                            danger = 1;
                        } else if (abs(m->partyX - m->map.entryX) + abs(m->partyY - m->map.entryY) > TILE_COUNT / 3) {
                            danger = 0;
                        }

                        int chance = ch->hide + ch->level + char_modifier(ch->dexterity) - danger * 30;
                        if (!(ch->flags & CharacterFlags_Hidden) && ch->stamina < 1) {
                            chance -= 20;
                        }

                        if (PcgRandom_roll(&m->rng, 1, 100) < chance) {
                            ui_log(DARKBLUE, "%s melds with the shadows", ch->name);
                            ch->flags |= CharacterFlags_Hidden;
                        }

                        if (!(ch->flags & CharacterFlags_Hidden) && PcgRandom_roll(&m->rng, 1, 3) <= 1) {
                            ch->stamina -= 1;
                        }
                    } break;
                    case CombatAction_CastSpell: {
                        bool plural = stack->alive != 1;
                        int drain = 0;

                        drain += PcgRandom_roll(&m->rng, 3, 6);
                        drain -= ch->level / 4;
                        drain -= char_modifier(ch->intellect);
                        if (drain > 0) {
                            ch->stamina -= drain;
                        }

                        int target = PcgRandom_randomu(&m->rng) % stack->alive;
                        int damage = PcgRandom_roll(&m->rng, 3 + ch->level / 3, 6);

                        /* Modify the damage by Charisma. Because a big part of how effective it will be comes
                         * down to how impressively the mage can yell "CHAIN LIGHTNING!" */
                        damage += char_modifier(ch->charisma);
                        damage += char_modifier(ch->willpower);
                        damage = util_intmax(1, damage);

                        ui_log(SKYBLUE, "%s casts Chain Lightning at the %s", ch->name,
                                plural ? stack->class.truenamePlural : stack->class.truename);
                        for (int i = 0; i < stack->alive && damage > 0; i++) {
                            int index = (i + target) % stack->alive;
                            if (damage < stack->health[index]) {
                                if (plural) {
                                    ui_log(ZINNWALDITEBROWN, "...one is fried for %i damage", damage);
                                } else {
                                    ui_log(ZINNWALDITEBROWN, "...frying it for %i damage", damage);
                                }
                            } else {
                                if (plural) {
                                    ui_log(ZINNWALDITEBROWN, "...one is fried for %i damage and dies!", damage);
                                } else {
                                    ui_log(ZINNWALDITEBROWN, "...frying it dead for %i damage!", damage);
                                }
                            }
                            stack->health[index] -= damage;
                            damage -= PcgRandom_roll(&m->rng, 1, 4);
                        }

                        for (int i = 0; i < stack->alive; i++) {
                            if (stack->health[i] > 0) {
                                continue;
                            } else if (stack->alive > 1) {
                                stack->alive -= 1;
                                stack->health[i] = stack->health[stack->alive];
                                stack->initiative[i] = stack->initiative[stack->alive];
                                i -= 1;
                            } else {
                                stack->alive = 0;
                                break;
                            }
                        }

                        ch->flags &= ~(CharacterFlags_Hidden);
                    }
                }
            }

            /* Stamina Check */
            if (ch->stamina < 0) {
                if (ch->health <= 0) {
                    ch->stamina = 0;
                } else {
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
        }

        /* Monster attacks */
        if (weightTotal < 1) {
            if (stack->alive == 1) {
                ui_log(ZINNWALDITEBROWN, "The %s peers through the darkness but finds nobody to attack",
                        stack->class.truename);
            } else {
                ui_log(ZINNWALDITEBROWN, "The %s peer through the darkness but find nobody to attack",
                        stack->class.truenamePlural);
            }
            continue;
        } else if (stack->status & MonsterStatus_Surprised) {
            continue;
        }

        for (int i = 0; i < stack->alive; i++) {
            if (stack->initiative[i] == segment) {
                /* Select target based on weighting */
                int targetRoll = PcgRandom_randomu(&m->rng) % weightTotal;
                int target;
                int tohit;
                int defense;

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

                defense = COMBAT_BASE_DEFENSE + char_modifier(ch->dexterity);
                if (m->party[target].action == CombatAction_DefendSelf)
                    defense += 4;
                if (m->party[target].action == CombatAction_GuardOthers)
                    defense += 2;

                tohit = PcgRandom_roll(&m->rng, 1, 20);
                tohit -= util_intmax(0, char_modifier(ch->charisma)); // Only creatures of average intelligence should acknowledge this
                if (tohit == 20 || (tohit + stack->class.attack > defense && tohit != 1)) {
                    int damage = PcgRandom_roll(&m->rng, 1, stack->class.damageDie) + stack->class.damageModifier;
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
                                stack->class.truename, ch->name, damage,
                                (ch->flags & CharacterFlags_Female) ? "her" : "him");
                        ch->stamina = 0;
                    } else if (damage > 0) {
                        ui_log(BLACK, "A %s strikes %s for %i damage",
                                stack->class.truename, ch->name, damage);
                    } else {
                        TraceLog(LOG_TRACE, "LOG: A %s swings at %s but %s armor deflects all damage",
                                stack->class.truename, ch->name,
                                ch->flags & CharacterFlags_Female ? "her" : "his");
                    }
                    ch->health -= damage;
                } else {
                    TraceLog(LOG_TRACE, "LOG: A %s swings at %s but misses", stack->class.truename, ch->name);
                }
            }
        }
    }

    /* Check if the party is still alive */
    int survivors = 0;
    for (int i = 0; i < arrlen(m->party); i++) {
        if (m->party[i].health > 0)
            survivors += 1;
        m->party[i].flags &= ~(CharacterFlags_Surprised);
    }

    if (survivors < 1) {
        ui_log(MAROON, "GAME OVER!");
        m->flags |= GlobalFlags_GameOver;
        ui_dumpCredits();

    } else if (stack->alive < 1) {
        int xp;

        m->flags &= ~(GlobalFlags_Encounter);
        ui_log(ORANGE, "Victory!");

        /* Loot and XP */
        xp = stack->total * stack->class.experience / survivors;
        ui_log(ZINNWALDITEBROWN, "Survivors gain %i experience points", xp);
        for (int i = 0; i < arrlen(m->party); i++) {
            if (m->party[i].health > 0) {
                char_exp(m->party + i, xp);
            }
        }
    } else {
        TraceLog(LOG_TRACE, "COMBAT: New Round");
        stack->status &= (~MonsterStatus_Surprised);
    }
}

static void
combat_flee(void)
{
    ui_log(ZINNWALDITEBROWN, "The party attempts to flee...");
    /* Parting shots */
    MonsterClass* monster = &m->encounter.stack.class;
    for (int i = 0; i < arrlen(m->party); i++) {
        Character* c = m->party + i;
        int save = c->dexterity - monster->stealth;
        int roll = PcgRandom_roll(&m->rng, 1, 20);

        if (c->health < 1)
            continue;

        c->stamina -= PcgRandom_roll(&m->rng, 2, 6);

        if (m->encounter.stack.status & MonsterStatus_Surprised)
            continue;

        if (c->flags & CharacterFlags_Hidden)
            save += 4;

        if (c->class == CharacterClass_Thief)
            save += 1;

        if (c->flags & CharacterFlags_Surprised)
            save -= 4;

        if (c->stamina < 0)
            save -= 2;

        if (save > 19)
            save = 19;

        if (roll == 20 || roll > save) {
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
            ui_dumpCredits();
        } else {
            ui_log(ZINNWALDITEBROWN, "The party escapes");
            m->flags &= ~(GlobalFlags_Encounter);
        }
    }
}

static void
combat_attack(Character* ch, Stack* stack)
{
    int target = PcgRandom_randomu(&m->rng) % stack->alive;
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

        if (tohit != 1 && tohit + accuracy >= COMBAT_BASE_DEFENSE + stack->class.defense) {
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

                if (tohit + accuracy >= COMBAT_BASE_DEFENSE + critRange + stack->class.defense) {
                    damage += PcgRandom_roll(&m->rng, 1, damageDie);
                    verb = "critically hits";
                    color = WHITE;
                    if (ch->class == CharacterClass_Thief) {
                        damage += ch->level;
                    }
                }
            }

            damage = util_intmax(1, damage);
            if (damage < stack->health[target]) {
                ui_log(color, "%s %s a %s for %i damage",
                        ch->name, verb, stack->class.truename, damage);
                ch->flags &= ~(CharacterFlags_Hidden);
            } else {
                ui_log(color, "%s %s a %s for %i damage, killing it!",
                        ch->name, verb, stack->class.truename, damage);
                if (ch->class != CharacterClass_Thief)
                    ch->flags &= ~(CharacterFlags_Hidden);
            }
            stack->health[target] -= damage;
        } else {
            if (ch->flags & CharacterFlags_Hidden) {
                int chance = char_hideChance(ch);
                if (PcgRandom_roll(&m->rng, 1, 100) >= chance)
                    ch->flags &= ~(CharacterFlags_Hidden);
            }
            ui_log(ZINNWALDITEBROWN, "%s swings but misses", ch->name);
        }
    }

    if (stack->health[target] > 0) {
        target += 1;
        target %= stack->alive;
    } else if (stack->alive > 1) {
        stack->alive -= 1;
        stack->health[target] = stack->health[stack->alive];
        stack->initiative[target] = stack->initiative[stack->alive];
    } else {
        stack->alive = 0;
    }

    /* Stamina Expenditure */
    if (PcgRandom_roll(&m->rng, 1, 3) == 1) {
        ch->stamina -= 1;
    }
}

static void
combat_multiAttack(Character* ch, Stack* stack)
{
    int attacks = 1;
    if (ch->level > stack->class.hitDice) {
        int cl = ch->level;
        int ml = stack->class.hitDice;

        if (stack->class.hitDice == 0) {
            cl *= 4;
            ml = 3;
        } else if (stack->class.hitDice == -1) {
            cl *= 3;
            ml = 2;
        } else if (stack->class.hitDice < -1) {
            cl *= -stack->class.hitDice;
            ml = 1;
        }

        attacks = util_intclamp(cl / ml, attacks, stack->alive);
    }

    for (int i = 0; i < attacks; i++) {
        combat_attack(ch, stack);
    }
}
