GAME_NAME = Oubliette
FILE_NAME = oubliette

FLAGS = -std=c99 -march=x86-64 -m64 -msse -msse2 -msse3 -Werror -Wno-all -Wreturn-type -Wmultichar \
		-Wbuiltin-macro-redefined -Wdiv-by-zero -Wreturn-local-addr -Wendif-labels -Wuninitialized -Wswitch \
		-Wfree-nonheap-object -Werror=vla -Wunreachable-code -DGAME_NAME=\"$(GAME_NAME)\"

FLAGS += -I./ext/cimgui/
FLAGS += -I./ext/include/
FLAGS += -I./ext/raylib/src/

ifndef $(EARLY_FAIL)
	EARLY_FAIL = 0
endif
ifneq ($(EARLY_FAIL),0)
	FLAGS += -Wfatal-errors
endif

RUN_STEAM_RT = run --rm -it \
				-v $(shell pwd):/build \
				--user $(shell id -u):$(shell id -g) \
				registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest bash -c



LINUX_FLAGS = $(FLAGS) -pthread -DPLATFORM_LINUX=1
LINUX_LINKS = -Wl,-rpath=./ lib/libraylib.glibc.a \
			  -lX11 -lXinerama -lXrandr -lGL -lm -ldl $(APPEND)

# TODO Default target should be determined by platform
linux_debug: linux_libs
	@echo "Compiling for Linux in Debug mode using GCC"
	$(CC) $(LINUX_FLAGS) -DDEBUG_MODE=1 -g3 src/main.c -o $(FILE_NAME)_d $(LINUX_LINKS)

linux_release: linux_libs
	@echo "Compiling for Linux in Release mode using GCC"
	$(CC) $(LINUX_FLAGS) -DRELEASE_MODE=1 -O3 src/main.c -o $(FILE_NAME) $(LINUX_LINKS)

linux_docker: linux_libs
	docker $(RUN_STEAM_RT) "cd /build && make CC=gcc APPEND=-lrt linux_release"

linux_libs: lib/libraylib.glibc.a cimgui.so

cimgui.so: | $(cimgui_directory)
	@cd ext/cimgui/ && $(MAKE) fclean
	docker $(RUN_STEAM_RT) "cd /build/ext/cimgui && make -j8 CXX=g++ cimgui.so"
	mv ext/cimgui/cimgui.so cimgui.so

lib/libraylib.glibc.a: | $(raylib_directory)
	@cd ext/raylib/src/ && $(MAKE) clean
	docker $(RUN_STEAM_RT) "cd /build/ext/raylib/src/ && make -j8 CC=gcc PLATFORM=PLATFORM_DESKTOP"
	@mkdir -p lib/
	mv ext/raylib/src/libraylib.a lib/libraylib.glibc.a



# Even cross-compiling from Linux, Windows has better forward-compatibility than Linux does so Docker isn't
# necessary, just a locally-installed MinGW toolchain

WINDOWS_FLAGS = -mwindows -I/usr/x86_64-w64-mingw32/include $(FLAGS) -DPLATFORM_WINDOWS=1
WINDOWS_LINKS = -Wl,-rpath=./ -L/usr/x86_64-w64-mingw32/lib \
				./lib/libraylib.mingw.a -lopengl32 -lshlwapi -lwinmm $(APPEND)

windows_debug: windows_libs
	@echo "Compiling for Windows in Debug mode using MinGW-w64"
	x86_64-w64-mingw32-gcc $(WINDOWS_FLAGS) -DDEBUG_MODE=1 -g3 src/main.c src/ext_miniphysfs.c -o $(FILE_NAME)_d $(WINDOWS_LINKS)

windows_release: windows_libs
	@echo "Compiling for Windows in Release mode using MinGW-w64"
	x86_64-w64-mingw32-gcc $(WINDOWS_FLAGS) -DRELEASE_MODE=1 -O3 src/main.c src/ext_miniphysfs.c -o $(FILE_NAME) $(WINDOWS_LINKS)

windows_libs: lib/libraylib.mingw.a cimgui.dll \
			  libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll

cimgui.dll: | $(cimgui_directory)
	cd ext/cimgui/ && $(MAKE) clean
	cd ext/cimgui/ && $(MAKE) CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++ OS=Windows_NT cimgui.dll
	mv ext/cimgui/cimgui.dll cimgui.dll

lib/libraylib.mingw.a: | $(raylib_directory)
	cd ext/raylib/src/ && $(MAKE) clean
	cd ext/raylib/src/ && $(MAKE) CC=x86_64-w64-mingw32-gcc PLATFORM=PLATFORM_DESKTOP
	mv ext/raylib/src/libraylib.a lib/libraylib.mingw.a

# Just copy them out of the local MinGW toolchain
libgcc_s_seh-1.dll:
	cp /usr/x86_64-w64-mingw32/bin/libgcc_s_seh-1.dll ./libgcc_s_seh-1.dll
libstdc++-6.dll:
	cp /usr/x86_64-w64-mingw32/bin/libstdc++-6.dll ./libstdc++-6.dll
libwinpthread-1.dll:
	cp /usr/x86_64-w64-mingw32/bin/libwinpthread-1.dll ./libwinpthread-1.dll



# Submodule init
$(cimgui_directory):
	git submodule update --init --recursive ext/cimgui
$(raylib_directory):
	git submodule update --init --recursive ext/raylib

all: linux_docker windows_release

clean:
	@rm -f $(FILE_NAME) $(FILE_NAME)_d $(FILE_NAME).exe $(FILE_NAME)_d.exe
	@cd ext/raylib/src/ && $(MAKE) clean
	@cd ext/cimgui/ && $(MAKE) clean

fclean: clean
	@cd ext/cimgui/ && $(MAKE) fclean
	@rm -f cimgui.so cimgui.dll
	@rm -f libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll
	@rm -rf lib/
	@docker builder prune -f

