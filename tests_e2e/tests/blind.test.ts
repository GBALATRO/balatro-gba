import { describe, it, expect } from 'vitest';
import { createRuntime, getGameState, skipDisclaimer, startGame, readAddr } from '../helpers/runtime.js';
import { GameState, ADDR, BlindType } from '../helpers/addresses.js';

describe('Blind selection', () => {
  it('a blind can be selected', async () => {
    const runtime = await createRuntime('blind-select');
    const engine = runtime.engine;

    await skipDisclaimer(runtime);
    await startGame(runtime);

    // We're at BLIND_SELECT — wait for the start animation to finish
    expect(getGameState(runtime)).toBe(GameState.BLIND_SELECT);
    const roundBefore = readAddr(runtime, 'round');

    // Wait for animation substate to reach BLIND_SELECT (substate=1)
    await engine.wait({ frames: 30 });

    // Press UP (ensure on "select") then A to confirm
    await engine.press('up');
    await engine.wait({ frames: 5 });
    await engine.press('a');

    // Wait through animation substates until GAME_STATE_PLAYING
    await engine.wait({
      memory: { address: ADDR.game_state.address, equals: GameState.PLAYING },
      timeout: 600,
    });

    expect(getGameState(runtime)).toBe(GameState.PLAYING);
    // Round should have incremented
    expect(readAddr(runtime, 'round')).toBe(roundBefore + 1);
  });

  it('a blind can be skipped', async () => {
    const runtime = await createRuntime('blind-skip');
    const engine = runtime.engine;

    await skipDisclaimer(runtime);
    await startGame(runtime);

    // We're at BLIND_SELECT for the small blind
    expect(getGameState(runtime)).toBe(GameState.BLIND_SELECT);
    expect(readAddr(runtime, 'current_blind')).toBe(BlindType.SMALL);

    // Wait for animation substate
    await engine.wait({ frames: 30 });

    // Press DOWN to move to "skip", then A to confirm
    await engine.press('down');
    await engine.wait({ frames: 5 });
    await engine.press('a');
    await engine.wait({ frames: 10 });

    // After skipping, current_blind should advance from SMALL to BIG
    expect(readAddr(runtime, 'current_blind')).toBe(BlindType.BIG);
    // Should still be on BLIND_SELECT (showing the big blind now)
    expect(getGameState(runtime)).toBe(GameState.BLIND_SELECT);
  });
});
