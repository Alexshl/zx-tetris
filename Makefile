PROJECT = tetris
SRCS = src/main.c src/render.c src/pieces.c src/game.c src/input.c src/sound.c
CFLAGS = +zx -vn -SO3 -clib=new -pragma-include:zpragma.inc
LDFLAGS = -create-app

all: build/$(PROJECT).tap

build/$(PROJECT).tap: $(SRCS) zpragma.inc
	@mkdir -p build
	zcc $(CFLAGS) $(SRCS) -o build/$(PROJECT) $(LDFLAGS)

run: build/$(PROJECT).tap
	open -a "Fuse" build/$(PROJECT).tap

clean:
	rm -rf build/

.PHONY: all run clean
