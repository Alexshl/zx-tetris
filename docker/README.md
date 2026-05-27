# Docker environment for ZX Spectrum Tetris

## Requirements

- Docker Desktop (macOS, Linux or Windows WSL2)
- No host z88dk or Fuse installation needed

## Quick start

```sh
# Build & dev
docker compose build                              # собрать/обновить образ
docker compose run --rm build                     # собрать build/tetris.tap
docker compose run --rm shell                     # интерактивный shell в контейнере

# Test
docker compose run --rm smoke                     # smoke-тест через ZRCP
docker compose run --rm integration               # integration-сценарии

# Debug
BIN=build/tetris_CODE.bin docker compose run --rm trace
CYCLES=200000 docker compose run --rm trace

# Disassembly
docker compose run --rm disasm                    # z88dk-dis с символами
docker compose run --rm disasm-alt                # z80dasm

# Research
FILE=build/tetris.tap Q="..." docker compose run --rm investigate
```

## Environment variables

Copy `.env.example` to `.env` and adjust as needed:

```bash
cp .env.example .env
```

Key variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `UID` | `1000` | Host user ID for artifact ownership (Linux/macOS) |
| `GID` | `1000` | Host group ID for artifact ownership (Linux/macOS) |
| `IMAGE_TAG` | `zx-tetris-emu:latest` | Docker image tag |
| `BIN` | `build/tetris_CODE.bin` | Binary for `trace` service |
| `CYCLES` | _(empty)_ | CPU cycle limit for `trace` |
| `FILE` | _(required)_ | Input file for `investigate` service |
| `Q` | _(required)_ | Question for `investigate` service |
| `ORG` | `0x6000` | Origin address for `investigate` disassembly |

## Image contents

| Tool | Source |
|------|--------|
| `zcc`, `z88dk-ticks`, `z88dk-dis` | Official `z88dk/z88dk:latest` image |
| `zesarux` | Built from source tag `ZEsarUX-12.1` with SDL2 |
| `z80dasm` | Ubuntu 24.04 apt package |
| `make`, `gcc` | Ubuntu 24.04 apt packages |

## Headless ZEsarUX

ZEsarUX runs headless via `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy`.
The ZRCP (remote protocol) is enabled at runtime with `--enable-remoteprotocol`
and listens on port 10000.

## Apple Silicon (arm64)

`z88dk/z88dk:latest` is an amd64-only image. Docker Desktop runs it via
Rosetta 2 transparently. Build times may be slightly longer.

## entrypoint subcommands

| Subcommand | Description |
|------------|-------------|
| `build` (default) | Runs `make -f Makefile.inner` to produce `build/tetris.tap` |
| `shell` | Drops into an interactive bash shell |
| `smoke` | Loads `.tap` into ZEsarUX headless, verifies PC halts at 0x0038 |
| `integration` | Runs ZRCP integration scenarios from `tools/integration/scenarios/` |
| `trace` | Execution tracing via ZEsarUX ZRCP |
| `disasm` | Disassembly via z88dk-dis |
| `disasm-alt` | Disassembly via z80dasm |
| `investigate` | Ad-hoc binary analysis with recon + AI investigation |
