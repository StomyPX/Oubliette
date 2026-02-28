
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
    util_log(0, "MAP: Loading, seed: %llu\n", seed);
    PcgRandom_init(&map->rng, seed);
    map->wall = LoadModel("data/statics/wall0.glb");
    map->wallTex = LoadTexture("data/textures/walls.png");
    SetMaterialTexture(&map->wall.materials[0], MATERIAL_MAP_DIFFUSE, map->wallTex);

    map->flor = LoadModel("data/statics/floor0.glb");
    map->florTex = LoadTexture("data/textures/str_stoneflr3.png");
    SetMaterialTexture(&map->flor.materials[0], MATERIAL_MAP_DIFFUSE, map->florTex);

    map->ceiling = LoadModel("data/statics/ceiling0.glb");
    map->ceilingTex = LoadTexture("data/textures/dlv_metalstr1c.png");
    SetMaterialTexture(&map->ceiling.materials[0], MATERIAL_MAP_DIFFUSE, map->ceilingTex);

    // Starting location
    map->entryX = PcgRandom_randomu(&map->rng) % (TILE_COUNT - 6) + 3;
    map->entryY = PcgRandom_randomu(&map->rng) % (TILE_COUNT / 4);
    map->tiles[map->entryX + map->entryY * TILE_COUNT] = TileFlags_Filled;

    map_generateChamber(map, map->entryX, map->entryY, Facing_South);
    // Build out the map, recursively resolving passages
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

    /* TODO Flood fill chambers from the end of the list until we find one far enough away from the entrance
     * to add the exit to. */
    // TODO Place features

    snprintf(map->name, sizeof(map->name), "Oubliette, L1");
}

static void
map_generatePassage(Map* map, MapPassage u)
{
    // Advance D4+2
    bool success = true;
    unsigned steps = PcgRandom_roll(&map->rng, 1, 4) + 2;
    int i;

    for (i = 0; i < steps && success; i++) {
        map_generateStepForward(map, &u.x, &u.y, u.facing);
    }

    // Either place a chamber or leave marked for another passage
    if (success) {
        uint32_t result;
        result = PcgRandom_roll(&map->rng, 1, 3);

        if (result <= 1) {
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
        }
    }
}

static bool
map_generateStepForward(Map* map, int* x, int* y, Facing facing)
{
    unsigned target; // Tile directly in front
    unsigned wall; // Tile that controls passage between current and target

    target = util_traverse(facing, *x, *y, 1, 0, 0, 0);
    if (target > TILE_COUNT * TILE_COUNT)
        return false;
    if (map->tiles[target] & TileFlags_Filled)
        return false;

    switch (facing) {
        default: {
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

    map->tiles[target] |= TileFlags_AllowEntry;
    if (map->tiles[target] & TileFlags_Chamber)
        return false;

    return true;
}

static void
map_generateChamber(Map* map, int x, int y, Facing facing)
{
    MapChamber c = {};
    uint32_t exitD6;
    uint32_t rectD6;

    if (map->chamberCount >= MAP_CHAMBERS_MAX) {
        util_err(0, "MAP: Exceeded maximum chambers!\n");
    }

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
            c.y = y + c.h;
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
            c.x = x + c.w;
            c.y = y - (PcgRandom_roll(&map->rng, 1, c.h) - 1);
        } break;
    }

    /* Ensure passage into new chamber */
    if (!map_generateStepForward(map, &x, &y, facing)) {
        return;
    }

    /* Write chamber into list */
    map->chambers[map->chamberCount++] = c;

    /* Bore out collision area */
    for (int i = 0; i < c.w; i++) {
        for (int j = 0; j < c.h; j++) {
            int index = c.x + i + (c.y + j) * TILE_COUNT;
            if (c.x + i < 0 || c.x + i >= TILE_COUNT || c.y + j < 0 || c.y + j >= TILE_COUNT)
                continue;

            /* Skip manually filled tiles */
            if (map->tiles[index] & TileFlags_Filled)
                continue;

            map->tiles[index] |= TileFlags_AllowEntry | TileFlags_Chamber;
            if (i < c.w - 1)
                map->tiles[index] |= TileFlags_AllowEast;
            if (j < c.h - 1)
                map->tiles[index] |= TileFlags_AllowSouth;
        }
    }

    /* Generate exits */
    exitD6 = PcgRandom_roll(&map->rng, 1, 6);
    if (map->chamberCount > MAP_CHAMBERS_MAX / 4)
        exitD6 -= 1;
    if (map->chamberCount > MAP_CHAMBERS_MAX / 2)
        exitD6 -= 1;
    if (map->chamberCount > MAP_CHAMBERS_MAX * 3 / 4)
        exitD6 -= 1;
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

    if (map->passageCount >= MAP_PASSAGES_MAX) {
        util_err(0, "MAP: Exceeded maximum passages!\n");
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

    if (passage.x >= 0 && passage.x < TILE_COUNT && passage.y >= 0 && passage.y < TILE_COUNT)
        map->passages[map->passageCount++] = passage;
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
map_draw(Map* map, Color light, float visibility)
{
    const int radius = visibility / TILE_SIDE_LENGTH + 1;
    const int total = radius * 2 + 1;
    float distance;
    float power = 1.f / 2.f;
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
                color = ColorLerp(light, CLEAR_COLOR, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModelEx(map->wall, origin, up, -90.f, one, color);
            }

            if (!(m->map.tiles[index] & TileFlags_AllowSouth) || y == TILE_COUNT - 1) {
                origin = map_tileCorner(x + 1, y + 1);
                center = map_tileCenter(x, y);
                center.z += TILE_SIDE_LENGTH / 2.f;
                center.y += TILE_SIDE_LENGTH / 2.f;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, CLEAR_COLOR, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->wall, origin, 1.f, color);
            }

            if (x == 0) { /* West Edge */
                origin = map_tileCorner(x, y + 1);
                center = map_tileCenter(x, y);
                center.x -= TILE_SIDE_LENGTH / 2.f;
                center.y += TILE_SIDE_LENGTH / 2.f;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, CLEAR_COLOR, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModelEx(map->wall, origin, up, -90.f, one, color);
            }

            if (y == 0) { /* North Edge */
                origin = map_tileCorner(x + 1, y);
                center = map_tileCenter(x, y);
                center.z += TILE_SIDE_LENGTH / 2.f;
                center.y += TILE_SIDE_LENGTH / 2.f;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, CLEAR_COLOR, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->wall, origin, 1.f, color);
            }

            if (map->tiles[index] & TileFlags_AllowEntry) {
                origin = map_tileCorner(x + 1, y + 1);
                center = map_tileCenter(x, y);
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, CLEAR_COLOR, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->flor, origin, 1.f, color);
            }

            if (map->tiles[index] & TileFlags_AllowEntry) {
                origin = map_tileCorner(x + 1, y + 1);
                center = map_tileCenter(x, y);
                center.y += TILE_SIDE_LENGTH;
                distance = Vector3Distance(position, center);
                color = ColorLerp(light, CLEAR_COLOR, powf(Clamp(distance / visibility, 0.f, 1.f), power));
                DrawModel(map->ceiling, origin, 1.f, color);
            }
        }
    }
}

