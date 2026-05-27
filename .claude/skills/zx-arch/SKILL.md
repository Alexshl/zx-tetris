---
name: zx-arch
description: Reference for ZX Spectrum 48K/128K hardware architecture — memory map, screen layout (bitmap + attribute file), color attribute encoding, keyboard matrix on port 0xFE, beeper, AY-3-8912 sound (128K), 50 Hz interrupt, and 128K bank switching via port 0x7FFD. Invoke when planning or implementing any low-level Spectrum behaviour, when verifying that a hardware-related claim is correct, or when a z88dk helper abstracts something whose underlying semantics you need to understand. Facts here are from authoritative sources (worldofspectrum.org, sinclair.wiki.zxnet.co.uk, breakintoprogram.co.uk) — when in doubt, follow the linked references rather than inventing.
---

# ZX Spectrum architecture — quick reference

Эта справка покрывает **только то, что нужно для проекта Tetris** (128K target). Углублённые темы (contended memory timing, ULA snow, AY envelopes, +3 disk) — по ссылкам в конце.

## 1. Memory map

### 48K Spectrum
| Address           | Size  | Contents                                  |
|-------------------|-------|-------------------------------------------|
| `0x0000–0x3FFF`   | 16 KB | ROM (BASIC interpreter)                   |
| `0x4000–0x57FF`   | 6 KB  | Screen bitmap (192 lines × 32 bytes)      |
| `0x5800–0x5AFF`   | 768 B | Attribute file (32 × 24)                  |
| `0x5B00–0x5BFF`   | 256 B | Printer buffer / system use               |
| `0x5C00–0x5CBF`   | 192 B | System variables                          |
| `0x5CC0–0xFFFF`   | ~42 KB| BASIC / user RAM                          |

### 128K Spectrum
То же первые 32 КБ, **плюс** банки 0..7 (по 16 КБ), переключаемые в окно `0xC000–0xFFFF` через port `0x7FFD`:

| 0x0000–0x3FFF | ROM (bit 4 of 0x7FFD выбирает: 0 = 128 editor, 1 = 48 BASIC) |
| 0x4000–0x7FFF | bank 5 (нормальный screen)                                   |
| 0x8000–0xBFFF | bank 2                                                       |
| 0xC000–0xFFFF | один из banks 0..7 (bits 0-2 of 0x7FFD)                      |

Bit 3 of 0x7FFD: 0 = normal screen (bank 5), 1 = shadow screen (bank 7).
Bit 5: «lock» — после установки в 1 порт игнорирует записи до сброса машины.

**Important**: при пейджинге нужно отключать прерывания (`di`) и держать стек вне меняющейся области. z88dk `+zx` (subtype zx-128) умеет работать с банками через `mem128_push_di()` / `mem128_pop_ei()` — см. z88dk header `<arch/zx/spectrum.h>`.

Для Тетриса 128K bank switching, скорее всего, не понадобится — игра целиком поместится в base 48K RAM. Доп. банки могут пригодиться позже для AY-музыки или таблиц.

## 2. Screen bitmap layout (0x4000)

256 × 192 пикселя. 6144 байта. Адресация **не линейная** — это критично для прямой записи.

Структура Y-адреса (биты 5..12 пиксельного адреса) при пиксельной координате `y` (0..191):

```
y = y7 y6 y5 y4 y3 y2 y1 y0   (где y7 всегда 0, т.к. 192<256)
       │  │  │  │  │  │  │
       Y2 Y1 Y0 Y5 Y4 Y3 Y2 Y1 Y0  ← мнемоника как биты адреса
```

Точнее, **полная формула адреса первого байта** для пикселя `(x, y)`:

```c
addr = 0x4000
     | ((y & 0xC0) << 5)        // верхние 2 бита Y — выбор трети экрана
     | ((y & 0x07) << 8)        // нижние 3 бита Y — строка внутри char-cell
     | ((y & 0x38) << 2)        // средние 3 бита Y — char-row внутри трети
     |  (x >> 3);               // X в байтах (8 пикс/байт)
```

Бит внутри байта: `0x80 >> (x & 7)` (MSB = левый пиксель).

**Практическое следствие**: соседние пиксельные строки внутри одного знакоместа лежат с шагом 256 байт, а соседние знакоместа по горизонтали — 1 байт. z88dk инкапсулирует это в:
- `zx_pxy2saddr(x, y)` — пиксельные → адрес байта в bitmap
- `zx_cxy2saddr(cx, cy)` — char-cell координаты (0..31, 0..23) → адрес верхней пиксельной строки знакоместа
- `zx_cxy2aaddr(cx, cy)` — char-cell → адрес атрибутного байта

Источник: [breakintoprogram.co.uk — Screen Memory Layout](http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/screen-memory-layout), [Wikipedia — ZX Spectrum graphic modes](https://en.wikipedia.org/wiki/ZX_Spectrum_graphic_modes).

## 3. Attribute file (0x5800)

768 байт, **линейно**: `addr = 0x5800 + row * 32 + col`, где `row` ∈ [0..23], `col` ∈ [0..31].

Один байт описывает цвет всего знакоместа 8×8:

```
bit:  7      6      5    4    3    2    1    0
     FLASH BRIGHT P2   P1   P0   I2   I1   I0
                 └─── PAPER ──┘└──── INK ────┘
```

PAPER и INK — три бита (8 цветов). Цвета:

| Код | Цвет     |
|-----|----------|
| 0   | BLACK    |
| 1   | BLUE     |
| 2   | RED      |
| 3   | MAGENTA  |
| 4   | GREEN    |
| 5   | CYAN     |
| 6   | YELLOW   |
| 7   | WHITE    |

BRIGHT = 1 → яркая версия (но BLACK остаётся чёрным). FLASH = 1 → инверсия INK/PAPER 1.6 раза в секунду (аппаратно).

**Attribute clash**: в одном знакоместе одновременно только 2 цвета (один INK + один PAPER). Это родовая особенность Спектрума.

В z88dk константы — `INK_BLACK`, `INK_BLUE`, ..., `PAPER_*`, `BRIGHT`, `FLASH` из `<arch/zx.h>`. Объединяются через `|`.

## 4. Keyboard — port 0xFE

40 клавиш, организованы как **8 полу-рядов по 5 клавиш**. Чтение: записываешь маску в **верхний байт** адреса I/O, нижний байт = `0xFE`. Каждый бит верхнего байта = «опросить полу-ряд N»; **0 = опрашивать, 1 = не опрашивать**. В ответе биты 0..4 = состояние клавиш (**0 = нажата**).

| Высокий байт I/O | Полу-ряд | Клавиши (бит 0 → бит 4)                     |
|------------------|----------|---------------------------------------------|
| `0xFE`           | 0        | CAPS SHIFT, Z, X, C, V                      |
| `0xFD`           | 1        | A, S, D, F, G                               |
| `0xFB`           | 2        | Q, W, E, R, T                               |
| `0xF7`           | 3        | 1, 2, 3, 4, 5                               |
| `0xEF`           | 4        | 0, 9, 8, 7, 6                               |
| `0xDF`           | 5        | P, O, I, U, Y                               |
| `0xBF`           | 6        | ENTER, L, K, J, H                           |
| `0x7F`           | 7        | SPACE, SYMBOL SHIFT, M, N, B                |

z88dk `<input.h>`: `in_key_pressed(IN_KEY_SCANCODE_o)` инкапсулирует это. Scancodes — макросы вида `IN_KEY_SCANCODE_<key>` (нижний регистр для букв, ENTER/SPACE — заглавные).

Источник: [breakintoprogram.co.uk — Keyboard](http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/keyboard), [sinclair.wiki — ZX Spectrum ULA](https://sinclair.wiki.zxnet.co.uk/wiki/ZX_Spectrum_ULA).

## 5. Border & beeper — port 0xFE (запись)

Запись в `0xFE`:

```
bit:  7  6  5    4    3   2   1   0
                EAR MIC  B2  B1  B0
                            └ BORDER ┘
```

- Биты 0-2: цвет бордюра (0..7, без BRIGHT).
- Бит 3: MIC (запись на ленту).
- Бит 4: **EAR / speaker**. Тоггл создаёт щелчок — это и есть бипер.

В z88dk: `zx_border(color)` для бордюра, `bit_beep(pitch, duration)` или `bit_beepfx_di(...)` для звуков (используют ROM-routine на 0x03B5).

## 6. AY-3-8912 — sound chip (только 128K)

3 квадратных канала + 1 шумовой + envelope. Управление через 2 порта:

| Порт      | Назначение                       |
|-----------|----------------------------------|
| `0xFFFD`  | Запись: выбор регистра (0..14). Чтение: данные текущего регистра. |
| `0xBFFD`  | Запись: данные в выбранный регистр.                                |

14 регистров: 6 для частот каналов A/B/C, регистр шума, mixer, 3 регистра громкости, 3 для envelope.

Для **Tetris MVP AY не используется** — звуки через бипер. Если в будущем понадобится — z88dk имеет `ay_*` функции и AYFX library для эффектов.

## 7. 50 Hz interrupt

Z80 в режиме IM 1, прерывание по vsync = 50 Гц (PAL UK / СССР). По умолчанию ISR в ROM по адресу `0x0038` инкрементирует системный счётчик `FRAMES` (`0x5C78` на 48K), обрабатывает клавиатуру для BASIC и возвращается.

`intrinsic_halt()` (z88dk) выполняет `HALT` — CPU засыпает до следующего прерывания. Это правильный способ idle в цикле игры на 50 Hz: не греет процессор и даёт стабильный таймер.

Один кадр = 20 мс = 70'000 T-states на Z80 3.5 МГц.

## 8. Z80 CPU (краткое)

- 3.5 МГц (стандартная частота Спектрума)
- Регистры: AF, BC, DE, HL, IX, IY, SP, PC + shadow AF', BC', DE', HL'
- 8-битная шина данных, 16-битная адресная (64 КБ адресного пространства)
- Контентионная память: при чтении ULA экрана (0x4000–0x7FFF) CPU доступ туда задержан. Это влияет на тайминги тонких эффектов (border tricks), но не на обычный игровой код.

## 9. z88dk helpers — sammary

Самое нужное для Тетриса:

| z88dk function/macro              | Что делает                                                      |
|-----------------------------------|-----------------------------------------------------------------|
| `zx_border(color)`                | Цвет бордюра                                                    |
| `zx_cls(attr)`                    | Очистка экрана с заданным атрибутом                             |
| `zx_cxy2saddr(cx, cy)`            | Char cell (0..31, 0..23) → адрес битмап-байта                   |
| `zx_cxy2aaddr(cx, cy)`            | Char cell → адрес атрибутного байта                             |
| `zx_pxy2saddr(x, y)`              | Пиксельные координаты → адрес битмап-байта                      |
| `intrinsic_halt()`                | Ждать следующего vsync (HALT)                                   |
| `in_key_pressed(SCANCODE)`        | Возвращает 0 или ненулевое, есть ли нажатие клавиши            |
| `bit_beep(pitch, duration)`       | Бипер: тон заданной частоты и длительности (блокирует CPU)      |
| `printf("\x16<row><col>...")`     | ROM AT — позиционирование текста в char-cell координатах         |
| `<arch/zx.h>`                     | Константы цветов: `INK_*`, `PAPER_*`, `BRIGHT`, `FLASH`         |
| `<input.h>`                       | Scancodes клавиатуры                                            |
| `<sound.h>`                       | Бипер и звуковые функции                                        |

**При сомнении** в сигнатуре — открыть header в локально установленном z88dk:
```bash
find $Z88DK/include -name 'zx.h' -o -name 'input.h' -o -name 'sound.h'
```

## 10. Когда использовать этот скил

Агенту (особенно `planner` и `coder`) консультироваться с этим документом, когда задача затрагивает:

- Прямые адреса (`0x4000`, `0x5800`, `0xFE`, `0x7FFD`)
- Расчёт адресов пикселей или атрибутов вручную
- Чтение клавиатуры на низком уровне (если z88dk-обёртка недоступна)
- Звук (бипер vs AY)
- Цвета и attribute clash
- Тайминг кадра, vsync, 50 Hz
- Bank switching на 128K

**Если факт из этого документа противоречит сторонней информации** — следуй ссылкам в конце и проверяй через WebFetch. Hardware-факты Спектрума стабильны с 1982 года, но текстовая выжимка может содержать опечатку.

## Источники

- [breakintoprogram.co.uk — Screen Memory Layout](http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/screen-memory-layout)
- [breakintoprogram.co.uk — Memory Map](http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/memory-map)
- [breakintoprogram.co.uk — Keyboard](http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/keyboard)
- [World of Spectrum — 128K Technical Information](https://worldofspectrum.org/faq/reference/128kreference.htm)
- [sinclair.wiki — Memory paging](https://sinclair.wiki.zxnet.co.uk/wiki/Memory_paging)
- [sinclair.wiki — ZX Spectrum ULA](https://sinclair.wiki.zxnet.co.uk/wiki/ZX_Spectrum_ULA)
- [Wikipedia — ZX Spectrum graphic modes](https://en.wikipedia.org/wiki/ZX_Spectrum_graphic_modes)
- [z88dk wiki — Sinclair ZX Spectrum platform](https://github.com/z88dk/z88dk/wiki/Platform---Sinclair-ZX-Spectrum)
- [z88dk Getting Started (newlib)](https://github.com/z88dk/z88dk/blob/master/doc/ZXSpectrumZSDCCnewlib_01_GettingStarted.md)
