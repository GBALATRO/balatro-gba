# E2E Tests

Runs the compiled ROM using [`@gba-kit/gba-node`](https://github.com/macabeus/gba-kit), a headless GBA emulator written in TypeScript.

## Setup

Requires Node.js >= 22 and after the first ROM is built:

```bash
# Install dependencies
cd tests-e2e
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

- **`createRuntime(name)`** boots a fresh emulator with the ROM. Each test gets an isolated instance.
- **`readAddr(runtime, addressName)`** reads a game variable by name.
- **`engine.press(button)`** presses a GBA button (`a`, `b`, `l`, `r`, `up`, `down`, `left`, `right`, `start`, `select`).
- **`engine.wait({ frames })`** advances the emulator by N frames (~60 fps).
- **`engine.wait({ memory: { address: ADDR.game_state.address, equals }, timeout })`** advances until a memory address holds the expected value, or throws after `timeout` frames.

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

### 4. Available addresses

`helpers/addresses.ts` exports `ADDR` with all known game variable addresses and enum constants (`GameState`, `HandState`, `BlindType`, `HandType`). To add a new address, add the symbol name to the `SYMBOLS` list in `helpers/extract-addresses.sh` and a corresponding entry in `addresses.ts`.
