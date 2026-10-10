# E2E Tests

Runs the compiled ROM using [`@gba-kit/gba-node`](https://github.com/macabeus/gba-kit), a headless GBA emulator written in TypeScript.

## Setup

Before running the E2E tests locally, make sure you have:

- Node.js >= 22
- A built ROM and its ELF file (`build/balatro-gba.elf` + `build/balatro-gba.gba`), e.g. `UID=$(id -u) GID=$(id -g) docker compose up`

```bash
# Install dependencies (pulls @gba-kit/* from npm)
cd tests_e2e
npm install
```

## Running

```bash
npm test            # single run
npm run test:watch  # watch mode
```

## Writing a new test

### 1. Create a test file

Add a file under [`tests/`](./tests/), e.g. `tests/shop.test.ts`:

```ts
import { describe, it, expect } from 'vitest';
import { createRuntime, navigateToPlaying, readAddr } from '../helpers/runtime.js';
import { ADDR } from '../helpers/addresses.js';

describe('Shop', () => {
  it('does something', async () => {
    const runtime = await createRuntime('shop-test');
    const engine = runtime.engine;

    // Navigate to a known state using helpers
    await navigateToPlaying(runtime);

    // Send inputs
    await engine.press('a');
    await engine.wait({ frames: 30 });

    // Assert on memory
    expect(readAddr(runtime, 'score')).toBeGreaterThan(0);
  });
});
```

### 2. Key concepts

- **`createRuntime(name)`** boots a fresh emulator with the ROM **and ELF** (so the engine owns the symbols + DWARF). Each test gets an isolated instance.
- **`runtime.engine.readVariable('symbol.field')`** reads a game variable by its C `symbol.field` path (built into gba-kit).
- **`engine.press(button)`** presses a GBA button (`a`, `b`, `l`, `r`, `up`, `down`, `left`, `right`, `start`, `select`).
- **`engine.wait({ frames })`** advances the emulator by N frames (~60 fps).
- **`engine.wait({ memory: { address: 'game_sm.state', equals }, timeout })`** advances until a variable (a `symbol.field` path, resolved from the DWARF) holds the expected value, or throws after `timeout` frames.

> 🔖 Discover all the engine methods available by reading the [Scripting Guide](https://github.com/macabeus/gba-kit/blob/main/docs/scripting.md)

### 3. Navigation helpers

`helpers/runtime.ts` provides shortcuts to reach common game states:

| Helper                       | Destination                              |
| ---------------------------- | ---------------------------------------- |
| `skipDisclaimer(runtime)`    | Main menu                                |
| `startGame(runtime)`         | Blind select                             |
| `selectBlind(runtime)`       | Playing (cards being dealt)              |
| `waitForHandSelect(runtime)` | Playing (cards in hand, ready for input) |
| `navigateToPlaying(runtime)` | All of the above combined                |

### 4. Reading game variables

There's no name→address mapping file to maintain. Tests reference the game's C names directly and the engine resolves them from the build it was created with — the symbol address from the ELF's symbol table, and the field's byte offset + size from the variable's DWARF type:

```ts
runtime.engine.readVariable('play_state'); // a standalone global (a global/file-static)
runtime.engine.readVariable('g_game_vars.score'); // a struct field
runtime.engine.readVariable('g_game_vars.rng_info.seed'); // a nested field
```

`readVariable` reads the right number of bytes automatically and decodes bitfields. `engine.wait({ memory })` accepts the same `symbol.field` path for its `address`, so waiting on a variable needs no manual address resolution either.

If the game source moves a variable into a struct, or changes a field's type/size, nothing here needs updating — only a renamed symbol/field changes the path string at its call sites.

> The enum constants (`GameState`, `HandState`, `BlindType`, `HandType`) are read from the build's DWARF too, so they never go stale when the game's enums change.
