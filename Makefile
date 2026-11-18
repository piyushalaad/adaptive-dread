CC      = cc
RAYLIB  = external/raylib/src
CFLAGS  = -Wall -Wextra -std=c99 -O2 -I$(RAYLIB)
LDLIBS  = $(RAYLIB)/libraylib.a -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL -framework QuartzCore -lm

SRC     = $(wildcard src/*.c)
OUT     = build/adaptive-dread

all: $(OUT)

$(OUT): $(SRC) $(RAYLIB)/libraylib.a
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDLIBS)

# Download and build raylib locally (first time only)
$(RAYLIB)/libraylib.a:
	@if [ ! -d external/raylib ]; then git clone --depth 1 https://github.com/raysan5/raylib.git external/raylib; fi
	$(MAKE) -C $(RAYLIB) PLATFORM=PLATFORM_DESKTOP

run: all
	./$(OUT)

clean:
	rm -rf build

.PHONY: all run clean
