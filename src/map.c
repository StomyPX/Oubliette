
static Vector3
map_tileCenter(int x, int y)
{
    Vector3 v;
    v.x = ((float)x - (float)TILE_COUNT / 2 + 0.5f) * TILE_SIDE_LENGTH;
    v.y = TILE_SIDE_LENGTH / 2.f;
    v.z = ((float)y - (float)TILE_COUNT / 2 + 0.5f) * TILE_SIDE_LENGTH;
    return v;
}

static Vector3
map_tileCorner(int x, int y)
{
    Vector3 v;
    v.x = ((float)x - (float)TILE_COUNT / 2) * TILE_SIDE_LENGTH;
    v.y = 0;
    v.z = ((float)y - (float)TILE_COUNT / 2) * TILE_SIDE_LENGTH;
    return v;
}

static int
map_tileIndex(int x, int y)
{
    if (x < 0 || x >= TILE_COUNT || y < 0 || y >= TILE_COUNT)
        return -1;
    return x + y * TILE_COUNT;
}

static Rectangle
map_subImageUV(int num)
{
    if (num == 0 || num > 16) {
        return (Rectangle){0, 0, 1, 1};
    } else {
        Rectangle r;
        int i = num - 1;
        r.x = (float)(i % 4) * 0.25f;
        r.y = (float)(i / 4) * 0.25f;
        r.width = 0.25f;
        r.height = 0.25f;
        return r;
    }
}

static Camera3D
map_cameraForTile(int x, int y, Facing facing)
{
    Camera c = {0};

    c.position = map_tileCenter(x, y);
    c.position.y = CAMERA_HEIGHT;
    c.target = c.position;
    c.target.y = 0.f;
    switch (facing % 4) {
        case 0: {
            c.position.z += TILE_SIDE_LENGTH / 4.f;
            c.target.z -= TILE_SIDE_LENGTH * 10.f;
        } break;
        case 1: {
            c.position.x -= TILE_SIDE_LENGTH / 4.f;
            c.target.x += TILE_SIDE_LENGTH * 10.f;
        } break;
        case 2: {
            c.position.z -= TILE_SIDE_LENGTH / 4.f;
            c.target.z += TILE_SIDE_LENGTH * 10.f;
        } break;
        case 3: {
            c.position.x += TILE_SIDE_LENGTH / 4.f;
            c.target.x -= TILE_SIDE_LENGTH * 10.f;
        } break;
    }

    c.up.y = 1.f;
    c.fovy = 75.f;
    c.projection = CAMERA_PERSPECTIVE;

    return c;
}

static void
map_generate(Map* map, uint64_t seed)
{
    util_log(0, "MAP: Loading, seed: %llu", seed);
    PcgRandom_init(&map->rng, seed);
    map->seed = seed;
    map->wall = LoadModel("data/statics/wall0.glb");
    map->wallTex = LoadTexture("data/textures/walls.png");
    SetMaterialTexture(&map->wall.materials[0], MATERIAL_MAP_DIFFUSE, map->wallTex);

    map->flor = LoadModel("data/statics/floor0.glb");
    map->florTex = LoadTexture("data/textures/str_stoneflr3.png");
    SetMaterialTexture(&map->flor.materials[0], MATERIAL_MAP_DIFFUSE, map->florTex);

    map->ceiling = LoadModel("data/statics/ceiling0.glb");
    map->ceilingTex = LoadTexture("data/textures/dlv_metalstr1c.png");
    SetMaterialTexture(&map->ceiling.materials[0], MATERIAL_MAP_DIFFUSE, map->ceilingTex);

    map->goal = LoadModel("data/statics/tomb.glb");
    map->goalTex = LoadTexture("data/textures/tomb.png");
    SetMaterialTexture(&map->goal.materials[0], MATERIAL_MAP_DIFFUSE, map->goalTex);

    map->entry = LoadModel("data/statics/ladderUp.glb");
    map->entryTex = LoadTexture("data/textures/dlv_woodstp2b.png");
    // There's an unused material in the first slot? Probably from the blender export
    SetMaterialTexture(&map->entry.materials[1], MATERIAL_MAP_DIFFUSE, map->ceilingTex);
    SetMaterialTexture(&map->entry.materials[2], MATERIAL_MAP_DIFFUSE, map->entryTex);

    // Starting location
    map->entryX = PcgRandom_randomu(&map->rng) % (TILE_COUNT / 2) + TILE_COUNT / 4;
    map->entryY = PcgRandom_randomu(&map->rng) % (TILE_COUNT / 4);
    map->tiles[map->entryX + map->entryY * TILE_COUNT] = TileFlags_Filled | TileFlags_AllowEntry;

    // Build out the map, recursively resolving passages
    //for (int i = 0; i < 3 && map->chamberCount < 3; i++) {
        map_generateChamber(map, map->entryX, map->entryY, Facing_South);
        map_generateLoop(map);
    //}

    /* Place the tomb in the farthest chamber from the exit. */
    int farthest = 0;
    for (int i = 0; i < map->chamberCount; i++) {
        MapChamber mc = map->chambers[i];
        int x = mc.x + mc.w / 2;
        int y = mc.y + mc.h / 2;
        int index = map_tileIndex(x, y);
        if (index >= 0) {
            int distance = abs(x - map->entryX) + abs(y - map->entryY);
            if (distance > farthest) {
                map->goalX = x;
                map->goalY = y;
                farthest = distance;
            }
        }
    } /* TODO Could instead try walking backwards through the chamber list to find an appropriate one */

    // TODO Place features

    /* Flood Test */
    #if DEBUG_MODE
    if (!map_floodTest(map)) {
        ui_log(MAROON, "ERROR: This map (seed %llu) has failed the flood test!", map->seed);
        ui_log(MAROON, "The map is not completable, please quit and start again");
    }
    #endif

    snprintf(map->name, sizeof(map->name), "Oubliette");
    map->encounterFreq = 600; // Hardcoded for now. 3000 is more like typical D&D freq of one roll every 30min
}

static void
map_generateLoop(Map* map)
{
    for (int i = 0;
        i < TILE_COUNT * TILE_COUNT && map->passageCount > 0 && map->chamberCount < MAP_CHAMBERS_MAX;
        i++)
    {
        int randomPassage = PcgRandom_randomu(&map->rng) % map->passageCount;
        MapPassage p = map->passages[randomPassage];
        map->passageCount -= 1;
        if (randomPassage < map->passageCount)
            map->passages[randomPassage] = map->passages[map->passageCount];
        map_generatePassage(map, p);
    }
}

static void
map_generatePassage(Map* map, MapPassage u)
{
    // Advance D4+2
    bool success = true;
    unsigned steps = PcgRandom_roll(&map->rng, 1, 4) + 2;
    unsigned stepsTaken;
    int ox = u.x;
    int oy = u.y;

    for (stepsTaken = 0; stepsTaken < steps && success; stepsTaken++) {
        if (!map_generateStepForward(map, &u.x, &u.y, u.facing)) {
            /* If we're stopped, at least make the target enterable */
            int index = map_tileIndex(u.x, u.y);
            if (index >= 0)
                map->tiles[index] |= TileFlags_AllowEntry;
            break;
        }
    }

    // Either place a chamber or leave marked for another passage
    if (success) {
        uint32_t result;
        result = PcgRandom_roll(&map->rng, 1, 3);

        if (map->chamberCount < 3 || result <= 1) {
            map_generateChamber(map, u.x, u.y, u.facing);
        } else {
            uint32_t turn = PcgRandom_roll(&map->rng, 1, 6);
            switch (turn) {
                case 1:
                case 2: {
                    u.facing += 1;
                    u.facing %= 4;
                } break;

                default: {
                } break;

                case 5:
                case 6: {
                    u.facing += 3;
                    u.facing %= 4;
                } break;
            }
            map->passages[map->passageCount++] = u;
            int index = map_tileIndex(u.x, u.y);
            if (index >= 0)
                map->tiles[index] |= TileFlags_Terminator;
        }
    }
}

static bool
map_generateStepForward(Map* map, int* x, int* y, Facing facing)
{
    unsigned target; // Tile directly in front
    unsigned wall; // Tile that controls passage between current and target

    util_traverse(facing, *x, *y, 1, 0, 0, 0);
    target = map_tileIndex(*x, *y);
    if (target < 0)
        return false;

    switch (facing) {
        default: /* North */ {
            if (*y <= 0) return false; // Edge of the map
            wall = util_traverse(facing, *x, *y, 1, 0, 0, 0);
            if (wall > TILE_COUNT * TILE_COUNT)
                return false;
            map->tiles[wall] |= TileFlags_AllowSouth;
            *y -= 1;
        } break;
        case Facing_East: {
            if (*x >= TILE_COUNT - 1) return false; // Edge of the map
            wall = util_traverse(facing, *x, *y, 0, 0, 0, 0);
            if (wall > TILE_COUNT * TILE_COUNT)
                return false;
            map->tiles[wall] |= TileFlags_AllowEast;
            *x += 1;
        } break;
        case Facing_South: {
            if (*y >= TILE_COUNT - 1) return false; // Edge of the map
            wall = util_traverse(facing, *x, *y, 0, 0, 0, 0);
            if (wall > TILE_COUNT * TILE_COUNT)
                return false;
            map->tiles[wall] |= TileFlags_AllowSouth;
            *y += 1;
        } break;
        case Facing_West: {
            if (*x <= 0) return false; // Edge of the map
            wall = util_traverse(facing, *x, *y, 1, 0, 0, 0);
            if (wall > TILE_COUNT * TILE_COUNT)
                return false;
            map->tiles[wall] |= TileFlags_AllowEast;
            *x -= 1;
        } break;
    }

    if (map->tiles[target] & TileFlags_Chamber) {
        return false;
    } else {
        map->tiles[target] |= TileFlags_AllowEntry;
        return true;
    }
}

static void
map_generateChamber(Map* map, int x, int y, Facing facing)
{
    MapChamber c = {};
    uint32_t exitD6;
    uint32_t rectD6;

    if (map->chamberCount >= MAP_CHAMBERS_MAX) {
        util_err(0, "MAP: Exceeded maximum chambers!");
    }

    #if DEBUG_MODE
    { /* Debugging */
        int index = map_tileIndex(x, y);
        if (index >= 0)
            map->tiles[index] |= TileFlags_Builder;
    }
    #endif

    /* Register new chamber and determine dimensions */
    rectD6 = PcgRandom_roll(&map->rng, 1, 6);
    switch (rectD6) {
        default: {
            c.w = 3;
            c.h = 3;
        } break;

        case 4: {
            c.w = 3;
            c.h = 5;
        } break;

        case 5: {
            c.w = 5;
            c.h = 3;
        } break;

        case 6: {
            c.w = 5;
            c.h = 5;
        } break;
    }

    /* Place chamber in front of the cursor */
    switch (facing) {
        default: {
            c.x = x - (PcgRandom_roll(&map->rng, 1, c.w) - 1);
            c.y = y - c.h;
        } break;
        case Facing_East: {
            c.x = x + 1;
            c.y = y - (PcgRandom_roll(&map->rng, 1, c.h) - 1);
        } break;
        case Facing_South: {
            c.x = x - (PcgRandom_roll(&map->rng, 1, c.w) - 1);
            c.y = y + 1;
        } break;
        case Facing_West: {
            c.x = x - c.w;
            c.y = y - (PcgRandom_roll(&map->rng, 1, c.h) - 1);
        } break;
    }

    /* Ensure passage into new chamber
     * This is a huge pain in the ass, we need to check beforehand if there's a separating wall and rebuild
     * it if the step results in a failure. */
    {
        int index = map_tileIndex(x, y);
        int mask = TileFlags_AllowSouth | TileFlags_AllowEast;
        int restore = 0;
        int wall;
        int tx = x;
        int ty = y;
        switch (facing) {
            default: /* North */ {
                wall = map_tileIndex(x, y - 1);
                if (wall >= 0)
                    restore = map->tiles[wall] & mask;
            } break;
            case Facing_East: {
                wall = index;
                if (wall >= 0)
                    restore = map->tiles[wall] & mask;
            } break;
            case Facing_South: {
                wall = index;
                if (wall >= 0)
                    restore = map->tiles[wall] & mask;
            } break;
            case Facing_West: {
                wall = map_tileIndex(x - 1, y);
                if (wall >= 0)
                    restore = map->tiles[wall] & mask;
            } break;
        }

        if (!map_generateStepForward(map, &tx, &ty, facing)) {
            map->tiles[index] |= TileFlags_Failure;
            if (wall >= 0) {
                map->tiles[wall] &= ~(mask);
                map->tiles[wall] |= restore;
            }

            return;
        }
    }

    /* Write chamber into list */
    map->chambers[map->chamberCount++] = c;

    /* Bore out collision area */
    for (int i = 0; i < c.w; i++) {
        for (int j = 0; j < c.h; j++) {
            int index = map_tileIndex(c.x + i, c.y + j);
            if (map->tiles[index] < 0)
                continue;

            /* Skip manually filled tiles */
            if (map->tiles[index] & TileFlags_Filled)
                continue;

            map->tiles[index] |= TileFlags_AllowEntry | TileFlags_Chamber;
            if (i < c.w - 1) {
                map->tiles[index] |= TileFlags_AllowEast;
            }
            if (j < c.h - 1) {
                map->tiles[index] |= TileFlags_AllowSouth;
            }
        }
    }

    /* Generate exits */
    exitD6 = PcgRandom_roll(&map->rng, 1, 6);
    if (map->chamberCount % 8 == 1 && map->chamberCount <= MAP_CHAMBERS_MAX / 4)
        exitD6 += 3;
    if (map->chamberCount > MAP_CHAMBERS_MAX / 2)
        exitD6 -= 1;
    if (map->chamberCount > MAP_CHAMBERS_MAX * 3 / 4)
        exitD6 -= 2;
    exitD6 = Clamp(exitD6, 0, 6);
    switch (exitD6) {
        default: {
            // No new exits!
        } break;
        case 1: {
            map_generateChamberRandomPassage(map, c, facing); // Opposite
        } break;
        case 2: {
            map_generateChamberRandomPassage(map, c, (facing + 3) % 4); // Left
        } break;
        case 3: {
            map_generateChamberRandomPassage(map, c, (facing + 1) % 4); // Right
        } break;
        case 4: {
            map_generateChamberRandomPassage(map, c, facing); // Opposite
            map_generateChamberRandomPassage(map, c, (facing + 3) % 4); // Left
        } break;
        case 5: {
            map_generateChamberRandomPassage(map, c, facing); // Opposite
            map_generateChamberRandomPassage(map, c, (facing + 1) % 4); // Right
        } break;
        case 6: {
            if (PcgRandom_roll(&map->rng, 1, 6) == 1) {
                // TODO Every tile has an exit!
                map_generateChamberRandomPassage(map, c, facing); // Opposite
                map_generateChamberRandomPassage(map, c, (facing + 3) % 4); // Left
                map_generateChamberRandomPassage(map, c, (facing + 1) % 4); // Right
            } else {
                map_generateChamberRandomPassage(map, c, facing); // Opposite
                map_generateChamberRandomPassage(map, c, (facing + 3) % 4); // Left
                map_generateChamberRandomPassage(map, c, (facing + 1) % 4); // Right
            }
        } break;
    }
}

static void
map_generateChamberRandomPassage(Map* map, MapChamber chamber, Facing facing)
{
    MapPassage passage = {};
    int index;
    int next, nx, ny;

    if (map->passageCount >= MAP_PASSAGES_MAX) {
        util_err(0, "MAP: Exceeded maximum passages!");
        return;
    }

    passage.facing = facing;
    switch (passage.facing) {
        default: {
            passage.x = PcgRandom_randomu(&map->rng) % chamber.w + chamber.x;
            passage.y = chamber.y;
        } break;
        case Facing_East: {
            passage.x = chamber.x + chamber.w - 1;
            passage.y = PcgRandom_randomu(&map->rng) % chamber.h + chamber.y;
        } break;
        case Facing_South: {
            passage.x = PcgRandom_randomu(&map->rng) % chamber.w + chamber.x;
            passage.y = chamber.y + chamber.h - 1;
        } break;
        case Facing_West: {
            passage.x = chamber.x;
            passage.y = PcgRandom_randomu(&map->rng) % chamber.h + chamber.y;
        } break;
    }

    index = map_tileIndex(passage.x, passage.y);
    util_traverse(facing, passage.x, passage.y, 1, 0, &nx, &ny);
    next = map_tileIndex(nx, ny);

    if (index >= 0 && next >= 0 && !(map->tiles[next] & TileFlags_Chamber)) {
        map->passages[map->passageCount++] = passage;
        map->tiles[index] |= TileFlags_Passage;
        map->tiles[next] |= TileFlags_Next;
    }
}

static void
map_unload(Map* map)
{
    UnloadModel(map->wall);
    UnloadModel(map->flor);
    UnloadModel(map->ceiling);
    UnloadTexture(map->wallTex);
    UnloadTexture(map->florTex);
    UnloadTexture(map->ceilingTex);
    memset(map, 0, sizeof(*map));
}

static void
map_draw(Map* map, Color light, float visibility, float power)
{
    const int radius = visibility / TILE_SIDE_LENGTH + 1;
    const int total = radius * 2 + 1;
    float distance;
    Vector3 up = (Vector3){0.f, 1.f, 0.f};
    Vector3 one = Vector3One();
    Vector3 position;
    Vector3 origin, center;
    Color color;

    if (m->flags & GlobalFlags_EditorMode) {
        position = map_tileCenter(m->partyX, m->partyY);
        position.y += CAMERA_HEIGHT;
    } else {
        position = m->camera.position;
    }

    // TODO Draw outward in a spiral pattern, skip walls between solid cells
    for (int i = 0; i < total; i++) {
        int x = m->partyX - radius - 1 + i;
        for (int j = 0; j < total; j++) {
            int y = m->partyY - radius - 1 + j;
            int index = x + y * TILE_COUNT;

            if (x < 0 || x >= TILE_COUNT || y < 0 || y >= TILE_COUNT)
                continue;

            if (!(m->map.tiles[index] & TileFlags_AllowEast) || x == TILE_COUNT - 1) {
                origin = map_tileCorner(x + 1, y + 1);
                center = map_tileCenter(x, y);
                center.x += TILE_SIDE_LENGTH / 2.f;
                center.y += TILE_SIDE_LENGTH / 2.f;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModelEx(map->wall, origin, up, -90.f, one, color);
            }

            if (!(m->map.tiles[index] & TileFlags_AllowSouth) || y == TILE_COUNT - 1) {
                origin = map_tileCorner(x + 1, y + 1);
                center = map_tileCenter(x, y);
                center.z += TILE_SIDE_LENGTH / 2.f;
                center.y += TILE_SIDE_LENGTH / 2.f;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->wall, origin, 1.f, color);
            }

            if (x == 0) { /* West Edge */
                origin = map_tileCorner(x, y + 1);
                center = map_tileCenter(x, y);
                center.x -= TILE_SIDE_LENGTH / 2.f;
                center.y += TILE_SIDE_LENGTH / 2.f;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModelEx(map->wall, origin, up, -90.f, one, color);
            }

            if (y == 0) { /* North Edge */
                origin = map_tileCorner(x + 1, y);
                center = map_tileCenter(x, y);
                center.z += TILE_SIDE_LENGTH / 2.f;
                center.y += TILE_SIDE_LENGTH / 2.f;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->wall, origin, 1.f, color);
            }

            /* floor */
            if (map->tiles[index] & (TileFlags_AllowEntry | TileFlags_Filled)) {
                origin = map_tileCorner(x + 1, y + 1);
                center = map_tileCenter(x, y);
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->flor, origin, 1.f, color);
            }

            /* ceiling */
            if ((map->tiles[index] & TileFlags_AllowEntry) && !(map->tiles[index] & TileFlags_Filled)) {
                origin = map_tileCorner(x + 1, y + 1);
                center = map_tileCenter(x, y);
                center.y += TILE_SIDE_LENGTH;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->ceiling, origin, 1.f, color);
            }

            /* Entrance Ladder */
            if (x == map->entryX && y == map->entryY) {
                origin = map_tileCorner(x + 1, y + 1);
                center = map_tileCenter(x, y);
                center.y += TILE_SIDE_LENGTH;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->entry, origin, 1.f, color);
            }

            /* Tomb */
            if (x == map->goalX && y == map->goalY) {
                origin = map_tileCorner(x + 1, y + 1);
                center = map_tileCenter(x, y);
                center.y += TILE_SIDE_LENGTH;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->goal, origin, 1.f, color);
            }

            #if DEBUG_MODE
            if (m->flags & GlobalFlags_ShowTileFlags) {
                if (map->tiles[index] & TileFlags_Passage) {
                    center = map_tileCenter(x, y);
                    float side = 0.6f;
                    center.y -= 1.f;
                    DrawCube(center, side, side, side, BLACK);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Next) {
                    center = map_tileCenter(x, y);
                    float side = 0.4f;
                    center.y -= 1.5f;
                    DrawCube(center, side, side, side, WHITE);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Filled) {
                    center = map_tileCenter(x, y);
                    center.x += 1.f;
                    center.z += 1.f;
                    center.y -= 1.f;
                    float side = 0.3f;
                    DrawCube(center, side, 1.f, side, DARKBROWN);
                    DrawCubeWires(center, side, 1.f, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Feature) {
                    center = map_tileCenter(x, y);
                    center.x -= 1.f;
                    center.z -= 1.f;
                    center.y -= 1.f;
                    float side = 0.3f;
                    DrawCube(center, side, 1.f, side, GOLD);
                    DrawCubeWires(center, side, 1.f, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Visited) {
                    center = map_tileCenter(x, y);
                    center.x += 1.f;
                    center.z -= 1.f;
                    center.y -= 1.5f;
                    float side = 0.3f;
                    DrawCube(center, side, side, side, DARKOLIVEGREEN);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Chamber) {
                    center = map_tileCenter(x, y);
                    center.x -= 1.f;
                    center.z += 1.f;
                    center.y -= 1.5f;
                    float side = 0.3f;
                    DrawCube(center, side, side, side, SKYBLUE);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Terminator) {
                    center = map_tileCenter(x, y);
                    float side = 0.4f;
                    center.y += 1.0f;
                    DrawCube(center, side, side, side, RED);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Builder) {
                    center = map_tileCenter(x, y);
                    float side = 0.4f;
                    center.y += 2.0f;
                    DrawCube(center, side, side, side, SKYBLUE);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Failure) {
                    center = map_tileCenter(x, y);
                    float side = 0.6f;
                    center.y += 1.5f;
                    DrawCube(center, side, side, side, ORANGE);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
            }
            #endif
        }
    }
}

#if DEBUG_MODE

static bool
map_floodTest(Map* map)
{
    int64_t tocheck;
    int start = map_tileIndex(map->entryX, map->entryY);
    int goal = map_tileIndex(map->goalX, map->goalY);

    if (start < 0) {
        TraceLog(LOG_FATAL, "Invalid starting tile [%i, %i]! This should be impossible!",
            map->entryX, map->entryY);
        return false;
    } else if (goal < 0) {
        TraceLog(LOG_FATAL, "Invalid goal tile [%i, %i]! This should be impossible!",
            map->goalX, map->goalY);
        return false;
    }

    /* Clear any previous result */
    for (int i = 0; i < arrlen(map->tiles); i++) {
        map->tiles[i] &= ~(TileFlags_FloodTest | TileFlags_Flooded);
    }

    map->tiles[start] |= TileFlags_FloodTest;
    tocheck = 1;

    /* Flood fill loop */
    for (uint32_t i = 0; i < UINT32_MAX && tocheck > 0; i++) {
        for (int y = 0; y < TILE_COUNT; y++) {
            for (int x = 0; x < TILE_COUNT; x++) {
                int index = map_tileIndex(x, y);
                if (index < 0)
                    TraceLog(LOG_ERROR, "Invalid tile index [%i, %i], this shouldn't happen", x, y);

                if (map->tiles[index] & TileFlags_FloodTest) {
                    int neighbors[4];

                    map->tiles[index] &= ~(TileFlags_FloodTest);
                    map->tiles[index] |= TileFlags_Flooded;
                    tocheck -= 1;

                    /*
                    if (index == goal) {
                        TraceLog(LOG_DEBUG, "Goal tile flooded, early-exit successful flood test");
                        return true;
                    }
                    */

                    neighbors[0] = map_tileIndex(x, y - 1); // North
                    neighbors[1] = map_tileIndex(x, y + 1); // South
                    neighbors[2] = map_tileIndex(x + 1, y); // East
                    neighbors[3] = map_tileIndex(x - 1, y); // West

                    for (int n = 0; n < arrlen(neighbors); n++) {
                        if (index >= 0) {
                            if (map->tiles[neighbors[n]] & (TileFlags_Flooded | TileFlags_FloodTest)) {
                                neighbors[n] = -1;
                            } else if (!(map->tiles[neighbors[n]] & TileFlags_AllowEntry)) {
                                neighbors[n] = -1;
                            }
                        }
                    }

                    if (neighbors[0] >= 0 && map->tiles[neighbors[0]] & TileFlags_AllowSouth) {
                        map->tiles[neighbors[0]] |= TileFlags_FloodTest;
                        tocheck += 1;
                    }

                    if (neighbors[1] >= 0 && map->tiles[index] & TileFlags_AllowSouth) {
                        map->tiles[neighbors[1]] |= TileFlags_FloodTest;
                        tocheck += 1;
                    }

                    if (neighbors[2] >= 0 && map->tiles[index] & TileFlags_AllowEast) {
                        map->tiles[neighbors[2]] |= TileFlags_FloodTest;
                        tocheck += 1;
                    }

                    if (neighbors[3] >= 0 && map->tiles[neighbors[3]] & TileFlags_AllowEast) {
                        map->tiles[neighbors[3]] |= TileFlags_FloodTest;
                        tocheck += 1;
                    }
                }
            }
        }
    }

    if (map->tiles[goal] & TileFlags_Flooded) {
        if (map->tiles[start] & TileFlags_Flooded) {
            return true;
        } else {
            TraceLog(LOG_ERROR, "ERROR: Entry tile not flooded, flood test failed! Map Seed: %llu", map->seed);
            return false;
        }
    } else {
        TraceLog(LOG_ERROR, "ERROR: Goal tile not flooded, flood test failed! Map Seed: %llu", map->seed);
        return false;
    }
}
#endif
