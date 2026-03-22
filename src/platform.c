static DLHandle
platform_dlopen(char* name)
{
    DLHandle handle;
    size_t size = strlen(name) + 5;
    char* path = RL_REALLOC(0, size);

    #if defined(_WIN32)
        snprintf(path, size, "%s.dll", name);
    #else
        snprintf(path, size, "%s.so", name);
    #endif

    const char* dir = PHYSFS_getRealDir(path);
    if (dir) {
        size += strlen(dir);
        path = RL_REALLOC(0, size);

        #if defined(_WIN32)
            snprintf(path, size, "%s%s.dll", dir, name);
            handle = LoadLibraryA(path);
            if (!handle) {
                LPVOID buffer;
                /* TODO If this doesn't compile, include winbase.h */
                FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        0,
                        GetLastError(),
                        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                        (LPSTR) &buffer,
                        0,
                        0);
                TraceLog(LOG_WARNING, "Couldn't open dynamic library (%s): %s", path, (LPSTR) buffer);
                LocalFree(buffer);
            }
        #else
            snprintf(path, size, "%s%s.so", dir, name);
            handle = dlopen(path, RTLD_NOW);
            if (!handle) {
                TraceLog(LOG_WARNING, "Couldn't open dynamic library (%s): %s", name, dlerror());
            }
        #endif
    } else {
        PHYSFS_ErrorCode errcode = PHYSFS_getLastErrorCode();
        TraceLog(LOG_WARNING, "Couldn't find dynamic library (%s): %s", path, PHYSFS_getErrorByCode(errcode));
        return 0;
    }

    RL_FREE(path);
    return handle;
}

static void*
platform_dlsym(DLHandle lib, char* name)
{
    #if defined(_WIN32)
        return GetProcAddress(lib, name);
    #else
        return dlsym(lib, name);
    #endif
}

/* Return zero on success, nonzero on failure */
static int
platform_dlclose(DLHandle lib)
{
    if (!lib) {
        TraceLog(LOG_DEBUG, "DLHandle empty, aborting close");
        return 0;
    }

    #if defined(_WIN32)
        int success = FreeLibrary(lib);
        if (!success) {
            LPVOID buffer;
            /* TODO If this doesn't compile, include winbase.h */
            FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    0,
                    GetLastError(),
                    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                    (LPSTR) &buffer,
                    0,
                    0);
            TraceLog(LOG_WARNING, "Failed to close dynamic library: %s", (LPSTR) buffer);
            LocalFree(buffer);
        }
        return !success;
    #else
        int errcode = dlclose(lib);
        if (errcode) {
            TraceLog(LOG_WARNING, "Failed to close dynamic library: %s", dlerror());
        }
        return errcode;
    #endif
}

