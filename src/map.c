
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
map_cameraForTile(int x, int y, int facing)
{
    Camera c = {0};

    c.position = map_tileCenter(x, y);
    c.position.y = 1.8f;
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

static void map_load_collision(Map* map, struct json_array_s* array);
static void map_load_faces(Face* faces, struct json_array_s* array);
static void map_load_info(Map* map, struct json_object_s* object);

static int
map_load(Map* map, char* path)
{
    char* file;
    size_t size;
    struct json_value_s *json;
    struct json_object_s *object;
    struct json_parse_result_s result = {0};

    TraceLog(LOG_INFO, "MAP: [%s] Loading", path);
    memset(map, 0, sizeof(*map));
    map->material = LoadMaterialDefault();
    map->transform = MatrixIdentity();

    platform_read(path, &file, &size);
    if (!file) {
        TraceLog(LOG_ERROR, "MAP: [%s] Failed to load, could not read file", path);
        return 0;
    }

    json = json_parse_ex(file, size, JSON_PARSE_FLAGS, 0, 0, &result);
    if (!json) {
        TraceLog(LOG_ERROR, "MAP: [%s] Failed to load, could not parse JSON: [line %llu, %llu] %s",
                path, result.error_line_no, result.error_row_no, json_parse_error_toString(result.error));
        return 0;
    }
    if (json->type != json_type_object) {
        TraceLog(LOG_ERROR, "MAP: [%s] Failed to load, JSON top-level structure isn't an object [%i]",
                path, json->type);
        return 0;
    }

    object = json_value_as_object(json);
    for (struct json_object_element_s* element = object->start; element; element = element->next) {
        if (util_stricmp((char*)element->name->string, "ceiling") == 0) {
            if (element->value->type == json_type_array) {
                map_load_faces(map->ceiling, json_value_as_array(element->value));
            } else {
                TraceLog(LOG_WARNING, "MAP: [%s] Face data must be specified as an array of strings", path);
            }
        } else if (util_stricmp((char*)element->name->string, "collision") == 0) {
            if (element->value->type == json_type_array) {
                map_load_collision(map, json_value_as_array(element->value));
            } else {
                TraceLog(LOG_WARNING, "MAP: [%s] Collision must be specified as an array of strings", path);
            }
        } else if (util_stricmp((char*)element->name->string, "east") == 0) {
            if (element->value->type == json_type_array) {
                map_load_faces(map->east, json_value_as_array(element->value));
            } else {
                TraceLog(LOG_WARNING, "MAP: [%s] Face data must be specified as an array of strings", path);
            }
        /* TODO entry (append) */
        /* TODO exit (append) */
        } else if (util_stricmp((char*)element->name->string, "floor") == 0) {
            if (element->value->type == json_type_array) {
                map_load_faces(map->floor, json_value_as_array(element->value));
            } else {
                TraceLog(LOG_WARNING, "MAP: [%s] Face data must be specified as an array of strings", path);
            }
        } else if (util_stricmp((char*)element->name->string, "info") == 0) {
            if (element->value->type == json_type_object) {
                map_load_info(map, json_value_as_object(element->value));
            } else {
                TraceLog(LOG_WARNING, "MAP: [%s] Collision must be specified as an array of strings", path);
            }
        } else if (util_stricmp((char*)element->name->string, "north") == 0) {
            if (element->value->type == json_type_array) {
                map_load_faces(map->north, json_value_as_array(element->value));
            } else {
                TraceLog(LOG_WARNING, "MAP: [%s] Face data must be specified as an array of strings", path);
            }
        } else if (util_stricmp((char*)element->name->string, "roof") == 0) {
            if (element->value->type == json_type_array) {
                map_load_faces(map->roof, json_value_as_array(element->value));
            } else {
                TraceLog(LOG_WARNING, "MAP: [%s] Face data must be specified as an array of strings", path);
            }
        } else if (util_stricmp((char*)element->name->string, "south") == 0) {
            if (element->value->type == json_type_array) {
                map_load_faces(map->south, json_value_as_array(element->value));
            } else {
                TraceLog(LOG_WARNING, "MAP: [%s] Face data must be specified as an array of strings", path);
            }
        } else if (util_stricmp((char*)element->name->string, "west") == 0) {
            if (element->value->type == json_type_array) {
                map_load_faces(map->west, json_value_as_array(element->value));
            } else {
                TraceLog(LOG_WARNING, "MAP: [%s] Face data must be specified as an array of strings", path);
            }
        } else {
            util_log(0, "[%s] Name: %s\n", json_type_toString(element->value->type), element->name->string);
        }
    }

    map_generate(map);

    TraceLog(LOG_INFO, "MAP: [%s] Successfully loaded", path);
    free(file);
    return 1;
}

static void
map_load_collision(Map* map, struct json_array_s* array)
{
    struct json_array_element_s* row = array->start;
    for (int y = 0; row && y < TILE_COUNT; y++, row = row->next) {
        struct json_string_s* line = json_value_as_string(row->value);
        if (line) {
            const char* c = line->string;
            for (int x = 0; c < line->string + line->string_size && x < TILE_COUNT; x++, c+=2) {
                int index = y * TILE_COUNT + x;
                switch (tolower(*c)) {
                    case 'x': {
                        map->tiles[index] |= TileFlags_Solid;
                    } break;
                    case 'e': {
                        map->tiles[index] |= TileFlags_BarEast;
                    } break;
                    case 's': {
                        map->tiles[index] |= TileFlags_BarSouth;
                    } break;
                }
            }
        } else {
            TraceLog(LOG_WARNING, "MAP: Collision must be specified as an array of strings");
        }
    }
}

static void
map_load_faces(Face* faces, struct json_array_s* array)
{
    struct json_array_element_s* row = array->start;
    for (int y = 0; row && y < TILE_COUNT; y++, row = row->next) {
        struct json_string_s* line = json_value_as_string(row->value);
        if (line) {
            const char* c = line->string;
            for (int x = 0; c < line->string + line->string_size && x < TILE_COUNT; x++, c+=2) {
                int index = y * TILE_COUNT + x;
                int face = util_pseudohex(*c);
                if (face >= 0) {
                    faces[index] = face;
                } else if (face < 0) {
                    TraceLog(LOG_WARNING, "MAP: Invalid face index [%c], must be psuedo-hexadecimal digit "
                            "in the range of 0-G (inclusive)", *c);
                } else {
                    faces[index] = 0;
                }
            }
        } else {
            TraceLog(LOG_WARNING, "MAP: Face array must be specified as an array of strings");
        }
    }
}

static void
map_load_info(Map* map, struct json_object_s* root)
{
    struct json_object_element_s* element = root->start;
    for (int i = 0; i < root->length && element; i++, element = element->next) {
        if (util_stricmp((char*)element->name->string, "atlas") == 0) {
            struct json_string_s* str;
            char* file, *ext;
            size_t size;

            str = json_value_as_string(element->value);
            if (!str) {
                TraceLog(LOG_WARNING, "MAP: Texture path for atlas must be string, was %s",
                        json_type_toString(element->value->type));
                continue;
            }

            ext = strrchr(str->string, '.');
            if (!ext) {
                TraceLog(LOG_WARNING, "MAP: Invalid atlas texture path, could not find extension in %s",
                        element->value);
                continue;
            }

            platform_read((char*) str->string, &file, &size);
            if (!file)
                continue;

            Image image = LoadImageFromMemory(ext, file, size);
            if (!image.data) {
                util_log(0, "\t[Path: %s, Ext: %s]\n", str->string, ext);
            }

            Texture texture = LoadTextureFromImage(image);
            SetMaterialTexture(&map->material, MATERIAL_MAP_DIFFUSE, texture);
            UnloadImage(image);
            free(file);
        } else if (util_stricmp((char*)element->name->string, "name") == 0) {
            if (element->value->type == json_type_string) {
                struct json_string_s* str = json_value_as_string(element->value);
                strncpy(map->name, str->string, sizeof(map->name) - 1);
            } else {
                TraceLog(LOG_WARNING, "MAP: info/name must be a string");
            }
        }
    }
}

static void
map_unload(Map* map)
{
    UnloadMaterial(map->material);
    if (map->mesh.vertices) {
        UnloadMesh(map->mesh);
        free(map->mesh.vertices);
        free(map->mesh.texcoords);
        free(map->mesh.normals);
        free(map->mesh.indices);
    }
    memset(map, 0, sizeof(*map));
}

static void
map_generate(Map* map)
{
    /* TODO New Approach */
    /* Deallocate if already existent */
    if (map->mesh.vertices) {
        UnloadMesh(map->mesh);
        free(map->mesh.vertices);
        free(map->mesh.texcoords);
        free(map->mesh.normals);
        free(map->mesh.indices);
    }

    /* Pre-Count */
    memset(&map->mesh, 0, sizeof(map->mesh));
    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->north[i]) {
            map->mesh.vertexCount += 4;
            map->mesh.triangleCount += 2;
        }
    }
    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->east[i]) {
            map->mesh.vertexCount += 4;
            map->mesh.triangleCount += 2;
        }
    }
    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->south[i]) {
            map->mesh.vertexCount += 4;
            map->mesh.triangleCount += 2;
        }
    }
    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->west[i]) {
            map->mesh.vertexCount += 4;
            map->mesh.triangleCount += 2;
        }
    }
    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->floor[i]) {
            map->mesh.vertexCount += 4;
            map->mesh.triangleCount += 2;
        }
    }
    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->ceiling[i]) {
            map->mesh.vertexCount += 4;
            map->mesh.triangleCount += 2;
        }
    }
    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->roof[i]) {
            map->mesh.vertexCount += 4;
            map->mesh.triangleCount += 2;
        }
    }

    /* Allocate */
    map->mesh.vertices =    malloc(sizeof(*map->mesh.vertices)  * map->mesh.vertexCount * 3);
    map->mesh.texcoords =   malloc(sizeof(*map->mesh.texcoords) * map->mesh.vertexCount * 2);
    map->mesh.normals =     malloc(sizeof(*map->mesh.normals)   * map->mesh.vertexCount * 3);
    map->mesh.indices =     malloc(sizeof(*map->mesh.indices)   * map->mesh.triangleCount * 3);

    /* Build Faces */
    unsigned vcount = 0;
    unsigned ecount = 0;
    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->north[i]) {
            Vector3 corner = map_tileCorner(i % TILE_COUNT, i / TILE_COUNT);
            map->mesh.vertices[vcount * 3 +  0] = corner.x;
            map->mesh.vertices[vcount * 3 +  1] = corner.y;
            map->mesh.vertices[vcount * 3 +  2] = corner.z;
            map->mesh.vertices[vcount * 3 +  3] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  4] = corner.y;
            map->mesh.vertices[vcount * 3 +  5] = corner.z;
            map->mesh.vertices[vcount * 3 +  6] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  7] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  8] = corner.z;
            map->mesh.vertices[vcount * 3 +  9] = corner.x;
            map->mesh.vertices[vcount * 3 + 10] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 + 11] = corner.z;

            Rectangle r = map_subImageUV(map->north[i]);
            map->mesh.texcoords[vcount * 2 + 0] = r.x;
            map->mesh.texcoords[vcount * 2 + 1] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 2] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 3] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 4] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 5] = r.y;
            map->mesh.texcoords[vcount * 2 + 6] = r.x;
            map->mesh.texcoords[vcount * 2 + 7] = r.y;

            map->mesh.normals[vcount * 3 +  0] = 0;
            map->mesh.normals[vcount * 3 +  1] = 0;
            map->mesh.normals[vcount * 3 +  2] = 1;
            map->mesh.normals[vcount * 3 +  3] = 0;
            map->mesh.normals[vcount * 3 +  4] = 0;
            map->mesh.normals[vcount * 3 +  5] = 1;
            map->mesh.normals[vcount * 3 +  6] = 0;
            map->mesh.normals[vcount * 3 +  7] = 0;
            map->mesh.normals[vcount * 3 +  8] = 1;
            map->mesh.normals[vcount * 3 +  9] = 0;
            map->mesh.normals[vcount * 3 + 10] = 0;
            map->mesh.normals[vcount * 3 + 11] = 1;

            map->mesh.indices[ecount * 3 +  0] = vcount + 0;
            map->mesh.indices[ecount * 3 +  1] = vcount + 1;
            map->mesh.indices[ecount * 3 +  2] = vcount + 2;
            map->mesh.indices[ecount * 3 +  3] = vcount + 0;
            map->mesh.indices[ecount * 3 +  4] = vcount + 2;
            map->mesh.indices[ecount * 3 +  5] = vcount + 3;
            vcount += 4;
            ecount += 2;
        }
    }

    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->east[i]) {
            Vector3 corner = map_tileCorner(i % TILE_COUNT, i / TILE_COUNT);
            map->mesh.vertices[vcount * 3 +  0] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  1] = corner.y;
            map->mesh.vertices[vcount * 3 +  2] = corner.z;
            map->mesh.vertices[vcount * 3 +  3] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  4] = corner.y;
            map->mesh.vertices[vcount * 3 +  5] = corner.z + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  6] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  7] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  8] = corner.z + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  9] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 + 10] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 + 11] = corner.z;

            Rectangle r = map_subImageUV(map->east[i]);
            map->mesh.texcoords[vcount * 2 + 0] = r.x;
            map->mesh.texcoords[vcount * 2 + 1] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 2] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 3] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 4] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 5] = r.y;
            map->mesh.texcoords[vcount * 2 + 6] = r.x;
            map->mesh.texcoords[vcount * 2 + 7] = r.y;

            map->mesh.normals[vcount * 3 +  0] = -1;
            map->mesh.normals[vcount * 3 +  1] = 0;
            map->mesh.normals[vcount * 3 +  2] = 0;
            map->mesh.normals[vcount * 3 +  3] = -1;
            map->mesh.normals[vcount * 3 +  4] = 0;
            map->mesh.normals[vcount * 3 +  5] = 0;
            map->mesh.normals[vcount * 3 +  6] = -1;
            map->mesh.normals[vcount * 3 +  7] = 0;
            map->mesh.normals[vcount * 3 +  8] = 0;
            map->mesh.normals[vcount * 3 +  9] = -1;
            map->mesh.normals[vcount * 3 + 10] = 0;
            map->mesh.normals[vcount * 3 + 11] = 0;

            map->mesh.indices[ecount * 3 +  0] = vcount + 0;
            map->mesh.indices[ecount * 3 +  1] = vcount + 1;
            map->mesh.indices[ecount * 3 +  2] = vcount + 2;
            map->mesh.indices[ecount * 3 +  3] = vcount + 0;
            map->mesh.indices[ecount * 3 +  4] = vcount + 2;
            map->mesh.indices[ecount * 3 +  5] = vcount + 3;
            vcount += 4;
            ecount += 2;
        }
    }

    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->south[i]) {
            Vector3 corner = map_tileCorner(i % TILE_COUNT, i / TILE_COUNT);
            map->mesh.vertices[vcount * 3 +  0] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  1] = corner.y;
            map->mesh.vertices[vcount * 3 +  2] = corner.z + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  3] = corner.x;
            map->mesh.vertices[vcount * 3 +  4] = corner.y;
            map->mesh.vertices[vcount * 3 +  5] = corner.z + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  6] = corner.x;
            map->mesh.vertices[vcount * 3 +  7] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  8] = corner.z + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  9] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 + 10] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 + 11] = corner.z + TILE_SIDE_LENGTH;

            Rectangle r = map_subImageUV(map->south[i]);
            map->mesh.texcoords[vcount * 2 + 0] = r.x;
            map->mesh.texcoords[vcount * 2 + 1] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 2] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 3] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 4] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 5] = r.y;
            map->mesh.texcoords[vcount * 2 + 6] = r.x;
            map->mesh.texcoords[vcount * 2 + 7] = r.y;

            map->mesh.normals[vcount * 3 +  0] = 0;
            map->mesh.normals[vcount * 3 +  1] = 0;
            map->mesh.normals[vcount * 3 +  2] = -1;
            map->mesh.normals[vcount * 3 +  3] = 0;
            map->mesh.normals[vcount * 3 +  4] = 0;
            map->mesh.normals[vcount * 3 +  5] = -1;
            map->mesh.normals[vcount * 3 +  6] = 0;
            map->mesh.normals[vcount * 3 +  7] = 0;
            map->mesh.normals[vcount * 3 +  8] = -1;
            map->mesh.normals[vcount * 3 +  9] = 0;
            map->mesh.normals[vcount * 3 + 10] = 0;
            map->mesh.normals[vcount * 3 + 11] = -1;

            map->mesh.indices[ecount * 3 +  0] = vcount + 0;
            map->mesh.indices[ecount * 3 +  1] = vcount + 1;
            map->mesh.indices[ecount * 3 +  2] = vcount + 2;
            map->mesh.indices[ecount * 3 +  3] = vcount + 0;
            map->mesh.indices[ecount * 3 +  4] = vcount + 2;
            map->mesh.indices[ecount * 3 +  5] = vcount + 3;
            vcount += 4;
            ecount += 2;
        }
    }

    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->west[i]) {
            Vector3 corner = map_tileCorner(i % TILE_COUNT, i / TILE_COUNT);
            map->mesh.vertices[vcount * 3 +  0] = corner.x;
            map->mesh.vertices[vcount * 3 +  1] = corner.y;
            map->mesh.vertices[vcount * 3 +  2] = corner.z + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  3] = corner.x;
            map->mesh.vertices[vcount * 3 +  4] = corner.y;
            map->mesh.vertices[vcount * 3 +  5] = corner.z;
            map->mesh.vertices[vcount * 3 +  6] = corner.x;
            map->mesh.vertices[vcount * 3 +  7] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  8] = corner.z;
            map->mesh.vertices[vcount * 3 +  9] = corner.x;
            map->mesh.vertices[vcount * 3 + 10] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 + 11] = corner.z + TILE_SIDE_LENGTH;

            Rectangle r = map_subImageUV(map->west[i]);
            map->mesh.texcoords[vcount * 2 + 0] = r.x;
            map->mesh.texcoords[vcount * 2 + 1] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 2] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 3] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 4] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 5] = r.y;
            map->mesh.texcoords[vcount * 2 + 6] = r.x;
            map->mesh.texcoords[vcount * 2 + 7] = r.y;

            map->mesh.normals[vcount * 3 +  0] = 1;
            map->mesh.normals[vcount * 3 +  1] = 0;
            map->mesh.normals[vcount * 3 +  2] = 0;
            map->mesh.normals[vcount * 3 +  3] = 1;
            map->mesh.normals[vcount * 3 +  4] = 0;
            map->mesh.normals[vcount * 3 +  5] = 0;
            map->mesh.normals[vcount * 3 +  6] = 1;
            map->mesh.normals[vcount * 3 +  7] = 0;
            map->mesh.normals[vcount * 3 +  8] = 0;
            map->mesh.normals[vcount * 3 +  9] = 1;
            map->mesh.normals[vcount * 3 + 10] = 0;
            map->mesh.normals[vcount * 3 + 11] = 0;

            map->mesh.indices[ecount * 3 +  0] = vcount + 0;
            map->mesh.indices[ecount * 3 +  1] = vcount + 1;
            map->mesh.indices[ecount * 3 +  2] = vcount + 2;
            map->mesh.indices[ecount * 3 +  3] = vcount + 0;
            map->mesh.indices[ecount * 3 +  4] = vcount + 2;
            map->mesh.indices[ecount * 3 +  5] = vcount + 3;
            vcount += 4;
            ecount += 2;
        }
    }

    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->floor[i]) {
            Vector3 corner = map_tileCorner(i % TILE_COUNT, i / TILE_COUNT);
            map->mesh.vertices[vcount * 3 +  0] = corner.x;
            map->mesh.vertices[vcount * 3 +  1] = corner.y;
            map->mesh.vertices[vcount * 3 +  2] = corner.z + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  3] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  4] = corner.y;
            map->mesh.vertices[vcount * 3 +  5] = corner.z + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  6] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  7] = corner.y;
            map->mesh.vertices[vcount * 3 +  8] = corner.z;
            map->mesh.vertices[vcount * 3 +  9] = corner.x;
            map->mesh.vertices[vcount * 3 + 10] = corner.y;
            map->mesh.vertices[vcount * 3 + 11] = corner.z;

            Rectangle r = map_subImageUV(map->floor[i]);
            map->mesh.texcoords[vcount * 2 + 0] = r.x;
            map->mesh.texcoords[vcount * 2 + 1] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 2] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 3] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 4] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 5] = r.y;
            map->mesh.texcoords[vcount * 2 + 6] = r.x;
            map->mesh.texcoords[vcount * 2 + 7] = r.y;

            map->mesh.normals[vcount * 3 +  0] = 0;
            map->mesh.normals[vcount * 3 +  1] = 1;
            map->mesh.normals[vcount * 3 +  2] = 0;
            map->mesh.normals[vcount * 3 +  3] = 0;
            map->mesh.normals[vcount * 3 +  4] = 1;
            map->mesh.normals[vcount * 3 +  5] = 0;
            map->mesh.normals[vcount * 3 +  6] = 0;
            map->mesh.normals[vcount * 3 +  7] = 1;
            map->mesh.normals[vcount * 3 +  8] = 0;
            map->mesh.normals[vcount * 3 +  9] = 0;
            map->mesh.normals[vcount * 3 + 10] = 1;
            map->mesh.normals[vcount * 3 + 11] = 0;

            map->mesh.indices[ecount * 3 +  0] = vcount + 0;
            map->mesh.indices[ecount * 3 +  1] = vcount + 1;
            map->mesh.indices[ecount * 3 +  2] = vcount + 2;
            map->mesh.indices[ecount * 3 +  3] = vcount + 0;
            map->mesh.indices[ecount * 3 +  4] = vcount + 2;
            map->mesh.indices[ecount * 3 +  5] = vcount + 3;
            vcount += 4;
            ecount += 2;
        }
    }

    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->ceiling[i]) {
            Vector3 corner = map_tileCorner(i % TILE_COUNT, i / TILE_COUNT);
            map->mesh.vertices[vcount * 3 +  0] = corner.x;
            map->mesh.vertices[vcount * 3 +  1] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  2] = corner.z;
            map->mesh.vertices[vcount * 3 +  3] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  4] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  5] = corner.z;
            map->mesh.vertices[vcount * 3 +  6] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  7] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  8] = corner.z + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  9] = corner.x;
            map->mesh.vertices[vcount * 3 + 10] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 + 11] = corner.z + TILE_SIDE_LENGTH;

            Rectangle r = map_subImageUV(map->ceiling[i]);
            map->mesh.texcoords[vcount * 2 + 0] = r.x;
            map->mesh.texcoords[vcount * 2 + 1] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 2] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 3] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 4] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 5] = r.y;
            map->mesh.texcoords[vcount * 2 + 6] = r.x;
            map->mesh.texcoords[vcount * 2 + 7] = r.y;

            map->mesh.normals[vcount * 3 +  0] = 0;
            map->mesh.normals[vcount * 3 +  1] = -1;
            map->mesh.normals[vcount * 3 +  2] = 0;
            map->mesh.normals[vcount * 3 +  3] = 0;
            map->mesh.normals[vcount * 3 +  4] = -1;
            map->mesh.normals[vcount * 3 +  5] = 0;
            map->mesh.normals[vcount * 3 +  6] = 0;
            map->mesh.normals[vcount * 3 +  7] = -1;
            map->mesh.normals[vcount * 3 +  8] = 0;
            map->mesh.normals[vcount * 3 +  9] = 0;
            map->mesh.normals[vcount * 3 + 10] = -1;
            map->mesh.normals[vcount * 3 + 11] = 0;

            map->mesh.indices[ecount * 3 +  0] = vcount + 0;
            map->mesh.indices[ecount * 3 +  1] = vcount + 1;
            map->mesh.indices[ecount * 3 +  2] = vcount + 2;
            map->mesh.indices[ecount * 3 +  3] = vcount + 0;
            map->mesh.indices[ecount * 3 +  4] = vcount + 2;
            map->mesh.indices[ecount * 3 +  5] = vcount + 3;
            vcount += 4;
            ecount += 2;
        }
    }

    for (unsigned i = 0; i < TILE_COUNT * TILE_COUNT; i++) {
        if (map->roof[i]) {
            Vector3 corner = map_tileCorner(i % TILE_COUNT, i / TILE_COUNT);
            map->mesh.vertices[vcount * 3 +  0] = corner.x;
            map->mesh.vertices[vcount * 3 +  1] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  2] = corner.z + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  3] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  4] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  5] = corner.z + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  6] = corner.x + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  7] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 +  8] = corner.z;
            map->mesh.vertices[vcount * 3 +  9] = corner.x;
            map->mesh.vertices[vcount * 3 + 10] = corner.y + TILE_SIDE_LENGTH;
            map->mesh.vertices[vcount * 3 + 11] = corner.z;

            Rectangle r = map_subImageUV(map->roof[i]);
            map->mesh.texcoords[vcount * 2 + 0] = r.x;
            map->mesh.texcoords[vcount * 2 + 1] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 2] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 3] = r.y + r.height;
            map->mesh.texcoords[vcount * 2 + 4] = r.x + r.width;
            map->mesh.texcoords[vcount * 2 + 5] = r.y;
            map->mesh.texcoords[vcount * 2 + 6] = r.x;
            map->mesh.texcoords[vcount * 2 + 7] = r.y;

            map->mesh.normals[vcount * 3 +  0] = 0;
            map->mesh.normals[vcount * 3 +  1] = 1;
            map->mesh.normals[vcount * 3 +  2] = 0;
            map->mesh.normals[vcount * 3 +  3] = 0;
            map->mesh.normals[vcount * 3 +  4] = 1;
            map->mesh.normals[vcount * 3 +  5] = 0;
            map->mesh.normals[vcount * 3 +  6] = 0;
            map->mesh.normals[vcount * 3 +  7] = 1;
            map->mesh.normals[vcount * 3 +  8] = 0;
            map->mesh.normals[vcount * 3 +  9] = 0;
            map->mesh.normals[vcount * 3 + 10] = 1;
            map->mesh.normals[vcount * 3 + 11] = 0;

            map->mesh.indices[ecount * 3 +  0] = vcount + 0;
            map->mesh.indices[ecount * 3 +  1] = vcount + 1;
            map->mesh.indices[ecount * 3 +  2] = vcount + 2;
            map->mesh.indices[ecount * 3 +  3] = vcount + 0;
            map->mesh.indices[ecount * 3 +  4] = vcount + 2;
            map->mesh.indices[ecount * 3 +  5] = vcount + 3;
            vcount += 4;
            ecount += 2;
        }
    }

    UploadMesh(&map->mesh, 1);
}

static void
map_draw(Map* map)
{
    DrawMesh(map->mesh, map->material, map->transform);
}

