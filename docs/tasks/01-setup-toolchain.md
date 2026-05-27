# Task 01: Setup toolchain + project skeleton

**Status**: DONE
**Зависит от**: —
**Цель**: Установить z88dk и Fuse, собрать минимальный `.tap`, который Fuse грузит и показывает «TETRIS» на экране. Это «hello world», проверяющий весь тулчейн.

## Acceptance criteria

- [ ] `zcc +zx --help` работает в терминале
- [ ] `brew list --cask` показывает `fredm-fuse`
- [ ] `make` собирает `build/tetris.tap` без warnings
- [ ] `make run` открывает Fuse и грузит `.tap`, на экране видна строка «TETRIS»
- [ ] Машина в Fuse выставлена как **Spectrum 128**

## Шаги

### 1. Установить z88dk (nightly из исходников)

```bash
git clone --recursive https://github.com/z88dk/z88dk.git ~/tools/z88dk
cd ~/tools/z88dk
./build.sh
```

Сборка занимает 10–20 минут. На macOS могут потребоваться `xcode-select --install` и `brew install cmake`.

Добавить в `~/.zshrc`:

```bash
export Z88DK=$HOME/tools/z88dk
export PATH=$Z88DK/bin:$PATH
export ZCCCFG=$Z88DK/lib/config
```

`source ~/.zshrc` или открыть новый терминал. Проверить:

```bash
zcc +zx -clib=new --help | head
```

### 2. Установить Fuse

```bash
brew install --cask fredm-fuse
```

Запустить Fuse, в меню `Machine → Select` выбрать **Spectrum 128**. Сохранить настройки.

### 3. Создать структуру проекта

В `/Users/oleksiishkurpela/projects/zx/`:

```
src/main.c
Makefile
zpragma.inc
```

### 4. Содержимое файлов

**`zpragma.inc`** — настройки памяти z88dk для 128K:

```
#pragma output STACKPTR=61440
#pragma output CRT_ORG_CODE=24576
#pragma output REGISTER_SP=-1
```

**`src/main.c`** — минимальное «hello»:

```c
#include <stdio.h>
#include <input.h>

int main(void) {
    printf("\x16\x05\x0B" "TETRIS\n");   // AT 5,11 (управляющие байты ZX)
    printf("\x16\x07\x09" "press any key");
    in_wait_key();
    return 0;
}
```

`\x16 row col` — управляющая последовательность ZX ROM-printer для позиционирования.

**`Makefile`**:

```make
PROJECT = tetris
SRCS = src/main.c
CFLAGS = +zx -vn -SO3 -clib=new -pragma-include:zpragma.inc
LDFLAGS = -create-app -subtype=tap

all: build/$(PROJECT).tap

build/$(PROJECT).tap: $(SRCS) zpragma.inc
	@mkdir -p build
	zcc $(CFLAGS) $(SRCS) -o build/$(PROJECT) $(LDFLAGS)

run: build/$(PROJECT).tap
	open -a "Fuse" build/$(PROJECT).tap

clean:
	rm -rf build/

.PHONY: all run clean
```

### 5. Сборка и запуск

```bash
cd /Users/oleksiishkurpela/projects/zx
make
make run
```

В Fuse должна загрузиться лента, появиться «TETRIS» по центру и приглашение нажать клавишу.

## Verification

Если что-то не работает:

- `zcc: command not found` → `source ~/.zshrc` и проверь `echo $PATH`
- Ошибка `cannot find crt0` → проверь `echo $ZCCCFG` (должен указывать на `lib/config`)
- Fuse открыт, но `.tap` не грузится → в меню `Media → Tape → Open` выбрать вручную
- Текст не появляется → убедись, что машина = Spectrum 128 (`Machine → Select`)

## Заметки

- На M1/M2 Mac сборка z88dk может ругаться на отсутствие старых libs; обычно решается установкой `brew install bsdmainutils`.
- `printf` z88dk-newlib для target zx работает через ROM, поэтому шрифт — стандартный спектрумовский 8×8.
- `subtype=tap` — стандартный формат, грузится в Fuse одним кликом. Альтернатива `-subtype=sna` (снапшот, мгновенная загрузка) тоже годится, но `.tap` ближе к реальному опыту.

## Completion note

Реализовано 2026-05-25. z88dk v24793 и Fuse установлены и работают; `make` собирает `build/tetris.tap` (5438 байт) без warnings. Из Makefile убран `-subtype=tap` — текущий z88dk этот параметр не принимает, `-create-app` без subtype для `+zx` автоматически генерирует `.tap` с BASIC-лоадером.
