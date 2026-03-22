
static Vector3
map_tileCenter(Map* map, int x, int y)
{
    Vector3 v;
    v.x = ((float)x - (float)map->width / 2 + 0.5f) * TILE_SIDE_LENGTH;
    v.y = TILE_SIDE_LENGTH / 2.f;
    v.z = ((float)y - (float)map->height / 2 + 0.5f) * TILE_SIDE_LENGTH;
    return v;
}

static Vector3
map_tileCorner(Map* map, int x, int y)
{
    Vector3 v;
    v.x = ((float)x - (float)map->width / 2) * TILE_SIDE_LENGTH;
    v.y = 0;
    v.z = ((float)y - (float)map->height / 2) * TILE_SIDE_LENGTH;
    return v;
}

static int
map_tileIndex(Map* map, int x, int y)
{
    if (x < 0 || x >= map->width || y < 0 || y >= map->height)
        return -1;
    return x + y * map->width;
}

static int
map_chambersMax(int w, int h)
{
    return w * h;
}

static int
map_passagesMax(int w, int h)
{
    return w * h * 4;
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
map_cameraForTile(Map* map, int x, int y, Facing facing)
{
    Camera c = {0};

    c.position = map_tileCenter(map, x, y);
    c.position.y = CAMERA_HEIGHT;
    c.target = c.position;
    c.target.y = 0.f;
    switch (facing % 4) {
        default:
        case Facing_North: {
            c.position.z += TILE_SIDE_LENGTH / 4.f;
            c.target.z -= TILE_SIDE_LENGTH * 10.f;
        } break;
        case Facing_East: {
            c.position.x -= TILE_SIDE_LENGTH / 4.f;
            c.target.x += TILE_SIDE_LENGTH * 10.f;
        } break;
        case Facing_South: {
            c.position.z -= TILE_SIDE_LENGTH / 4.f;
            c.target.z += TILE_SIDE_LENGTH * 10.f;
        } break;
        case Facing_West: {
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

    /* TODO This can later be made configurable */
    map->width = 48;
    map->height = 48;

    memset(map->tiles, 0, sizeof(map->tiles));
    memset(map->chambers, 0, sizeof(map->chambers));
    memset(map->passages, 0, sizeof(map->passages));

    // Starting location
    map->entryX = PcgRandom_randomu(&map->rng) % (map->width / 2) + map->height / 4;
    map->entryY = PcgRandom_randomu(&map->rng) % (map->height / 4);
    map->tiles[map->entryX + map->entryY * map->width] = TileFlags_Filled | TileFlags_AllowEntry;

    // Build out the map, recursively resolving passages
    //for (int i = 0; i < 3 && map->chamberCount < 3; i++) {
        map_generateChamber(map, map->entryX, map->entryY, Facing_South);
        map_generateLoop(map);
    //}
    /* TODO Better way to deal with insufficient chambers is to go back to previous ones and add more exits */

    /* Place the tomb in the farthest chamber from the exit. */
    int farthest = 0;
    for (int i = 0; i < map->chamberCount; i++) {
        MapChamber mc = map->chambers[i];
        int x = mc.x + mc.w / 2;
        int y = mc.y + mc.h / 2;
        int index = map_tileIndex(map, x, y);
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
    for (int i = 0; i < map->width * map->height && map->passageCount > 0
        && map->chamberCount < map_chambersMax(map->width, map->height); i++)
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
            int index = map_tileIndex(map, u.x, u.y);
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
            int index = map_tileIndex(map, u.x, u.y);
            if (index >= 0)
                map->tiles[index] |= TileFlags_Terminator;
        }
    }
}

static bool
map_generateStepForward(Map* map, int* x, int* y, Facing facing)
{
    int target; // Tile directly in front
    int wall; // Tile that controls passage between current and target
    int x2, y2;
    int x3, y3;

    util_traverse(facing, *x, *y, 1, 0, &x2, &y2);
    target = map_tileIndex(map, x2, y2);
    if (target < 0)
        return false;

    /* TODO Instead of checking bounds, just try the expected x and y and see if tileIndex return non-negative */
    switch (facing) {
        default: /* North */ {
            util_traverse(facing, *x, *y, 1, 0, &x3, &y3);
            wall = map_tileIndex(map, x3, y3);
            if (wall < 0)
                return false;
            map->tiles[wall] |= TileFlags_AllowSouth;
            *y -= 1;
        } break;
        case Facing_East: {
            util_traverse(facing, *x, *y, 0, 0, &x3, &y3);
            wall = map_tileIndex(map, x3, y3);
            if (wall < 0)
                return false;
            map->tiles[wall] |= TileFlags_AllowEast;
            *x += 1;
        } break;
        case Facing_South: {
            util_traverse(facing, *x, *y, 0, 0, &x3, &y3);
            wall = map_tileIndex(map, x3, y3);
            if (wall < 0)
                return false;
            map->tiles[wall] |= TileFlags_AllowSouth;
            *y += 1;
        } break;
        case Facing_West: {
            util_traverse(facing, *x, *y, 1, 0, &x3, &y3);
            wall = map_tileIndex(map, x3, y3);
            if (wall < 0)
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

    if (map->chamberCount >= map_chambersMax(map->width, map->height)) {
        util_err(0, "MAP: Exceeded maximum chambers!");
    }

    #if DEBUG_MODE
    { /* Debugging */
        int index = map_tileIndex(map, x, y);
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
        int index = map_tileIndex(map, x, y);
        int restoreIndex;
        int restoreWall;
        int wall = -1;
        int tx = x;
        int ty = y;

        restoreIndex = map->tiles[index];
        switch (facing) {
            case Facing_North: {
                wall = map_tileIndex(map, x, y - 1);
                if (wall >= 0)
                    restoreWall = map->tiles[wall];
            } break;
            case Facing_West: {
                wall = map_tileIndex(map, x - 1, y);
                if (wall >= 0)
                    restoreWall = map->tiles[wall];
            } break;
            default: break;
        }

        if (!map_generateStepForward(map, &tx, &ty, facing)) {
            map->tiles[index] = restoreIndex;
            map->tiles[index] |= TileFlags_Failure;
            if (wall >= 0)
                map->tiles[wall] = restoreWall;

            return;
        }
    }

    /* Write chamber into list */
    map->chambers[map->chamberCount++] = c;

    /* Bore out collision area */
    for (int i = 0; i < c.w; i++) {
        for (int j = 0; j < c.h; j++) {
            int index = map_tileIndex(map, c.x + i, c.y + j);
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
    if (map->chamberCount % 8 == 1 && map->chamberCount <= map_chambersMax(map->width, map->height) / 4)
        exitD6 += 3;
    if (map->chamberCount > map_chambersMax(map->width, map->height) / 2)
        exitD6 -= 1;
    if (map->chamberCount > map_chambersMax(map->width, map->height) * 3 / 4)
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

    if (map->passageCount >= map_passagesMax(map->width, map->height)) {
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

    index = map_tileIndex(map, passage.x, passage.y);
    util_traverse(facing, passage.x, passage.y, 1, 0, &nx, &ny);
    next = map_tileIndex(map, nx, ny);

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
        position = map_tileCenter(map, m->partyX, m->partyY);
        position.y += CAMERA_HEIGHT;
    } else {
        position = m->camera.position;
    }

    // TODO Draw outward in a spiral pattern, skip walls between solid cells
    for (int i = 0; i < total; i++) {
        int x = m->partyX - radius - 1 + i;
        for (int j = 0; j < total; j++) {
            int y = m->partyY - radius - 1 + j;
            int index = x + y * map->width;

            if (x < 0 || x >= map->width || y < 0 || y >= map->height)
                continue;

            if (!(m->map.tiles[index] & TileFlags_AllowEast) || x == map->width - 1) {
                origin = map_tileCorner(map, x + 1, y + 1);
                center = map_tileCenter(map, x, y);
                center.x += TILE_SIDE_LENGTH / 2.f;
                center.y += TILE_SIDE_LENGTH / 2.f;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModelEx(map->wall, origin, up, -90.f, one, color);
            }

            if (!(m->map.tiles[index] & TileFlags_AllowSouth) || y == map->height - 1) {
                origin = map_tileCorner(map, x + 1, y + 1);
                center = map_tileCenter(map, x, y);
                center.z += TILE_SIDE_LENGTH / 2.f;
                center.y += TILE_SIDE_LENGTH / 2.f;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->wall, origin, 1.f, color);
            }

            if (x == 0) { /* West Edge */
                origin = map_tileCorner(map, x, y + 1);
                center = map_tileCenter(map, x, y);
                center.x -= TILE_SIDE_LENGTH / 2.f;
                center.y += TILE_SIDE_LENGTH / 2.f;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModelEx(map->wall, origin, up, -90.f, one, color);
            }

            if (y == 0) { /* North Edge */
                origin = map_tileCorner(map, x + 1, y);
                center = map_tileCenter(map, x, y);
                center.z += TILE_SIDE_LENGTH / 2.f;
                center.y += TILE_SIDE_LENGTH / 2.f;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->wall, origin, 1.f, color);
            }

            /* floor */
            if (map->tiles[index] & (TileFlags_AllowEntry | TileFlags_Filled)) {
                origin = map_tileCorner(map, x + 1, y + 1);
                center = map_tileCenter(map, x, y);
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->flor, origin, 1.f, color);
            }

            /* ceiling */
            if ((map->tiles[index] & TileFlags_AllowEntry) && !(map->tiles[index] & TileFlags_Filled)) {
                origin = map_tileCorner(map, x + 1, y + 1);
                center = map_tileCenter(map, x, y);
                center.y += TILE_SIDE_LENGTH;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->ceiling, origin, 1.f, color);
            }

            /* Entrance Ladder */
            if (x == map->entryX && y == map->entryY) {
                origin = map_tileCorner(map, x + 1, y + 1);
                center = map_tileCenter(map, x, y);
                center.y += TILE_SIDE_LENGTH;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->entry, origin, 1.f, color);
            }

            /* Tomb */
            if (x == map->goalX && y == map->goalY) {
                origin = map_tileCorner(map, x + 1, y + 1);
                center = map_tileCenter(map, x, y);
                center.y += TILE_SIDE_LENGTH;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, BLACK, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->goal, origin, 1.f, color);
            }

            #if DEBUG_MODE
            if (m->flags & GlobalFlags_ShowTileFlags) {
                if (map->tiles[index] & TileFlags_Passage) {
                    center = map_tileCenter(map, x, y);
                    float side = 0.6f;
                    center.y -= 1.f;
                    DrawCube(center, side, side, side, BLACK);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Next) {
                    center = map_tileCenter(map, x, y);
                    float side = 0.4f;
                    center.y -= 1.5f;
                    DrawCube(center, side, side, side, WHITE);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Filled) {
                    center = map_tileCenter(map, x, y);
                    center.x += 1.f;
                    center.z += 1.f;
                    center.y -= 1.f;
                    float side = 0.3f;
                    DrawCube(center, side, 1.f, side, DARKBROWN);
                    DrawCubeWires(center, side, 1.f, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Feature) {
                    center = map_tileCenter(map, x, y);
                    center.x -= 1.f;
                    center.z -= 1.f;
                    center.y -= 1.f;
                    float side = 0.3f;
                    DrawCube(center, side, 1.f, side, GOLD);
                    DrawCubeWires(center, side, 1.f, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Visited) {
                    center = map_tileCenter(map, x, y);
                    center.x += 1.f;
                    center.z -= 1.f;
                    center.y -= 1.5f;
                    float side = 0.3f;
                    DrawCube(center, side, side, side, DARKOLIVEGREEN);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Chamber) {
                    center = map_tileCenter(map, x, y);
                    center.x -= 1.f;
                    center.z += 1.f;
                    center.y -= 1.5f;
                    float side = 0.3f;
                    DrawCube(center, side, side, side, SKYBLUE);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Terminator) {
                    center = map_tileCenter(map, x, y);
                    float side = 0.4f;
                    center.y += 1.0f;
                    DrawCube(center, side, side, side, RED);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Builder) {
                    center = map_tileCenter(map, x, y);
                    float side = 0.4f;
                    center.y += 2.0f;
                    DrawCube(center, side, side, side, SKYBLUE);
                    DrawCubeWires(center, side, side, side, GRAY);
                }
                if (map->tiles[index] & TileFlags_Failure) {
                    center = map_tileCenter(map, x, y);
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
    int start = map_tileIndex(map, map->entryX, map->entryY);
    int goal = map_tileIndex(map, map->goalX, map->goalY);

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
        for (int y = 0; y < map->height; y++) {
            for (int x = 0; x < map->width; x++) {
                int index = map_tileIndex(map, x, y);
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

                    neighbors[0] = map_tileIndex(map, x, y - 1); // North
                    neighbors[1] = map_tileIndex(map, x, y + 1); // South
                    neighbors[2] = map_tileIndex(map, x + 1, y); // East
                    neighbors[3] = map_tileIndex(map, x - 1, y); // West

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
