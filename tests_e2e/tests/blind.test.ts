import { describe, it, expect } from 'vitest';
import { createRuntime, getGameState, skipDisclaimer, startGame, readVar } from '../helpers/runtime.js';
import { GameState, BlindType } from '../helpers/addresses.js';

describe('Blind selection', () => {
  it('a blind can be selected', async () => {
    const runtime = await createRuntime('blind-select');
    const engine = runtime.engine;

    await skipDisclaimer(runtime);
    await startGame(runtime);

    // We're at BLIND_SELECT — wait for the start animation to finish
    expect(getGameState(runtime)).toBe(GameState.BLIND_SELECT);
    const roundBefore = readVar(runtime, 'g_game_vars.round');

    // Wait for animation substate to reach BLIND_SELECT (substate=1)
    await engine.wait({ frames: 30 });

    // Press UP (ensure on "select") then A to confirm
    await engine.press('up');
    await engine.wait({ frames: 5 });
    await engine.press('a');

    // Wait through animation substates until GAME_STATE_ROUND
    await engine.wait({
      memory: { address: 'game_sm.state', equals: GameState.ROUND },
      timeout: 600,
    });

    expect(getGameState(runtime)).toBe(GameState.ROUND);
    // Round should have incremented
    expect(readVar(runtime, 'g_game_vars.round')).toBe(roundBefore + 1);
  });

  it('a blind can be skipped', async () => {
    const runtime = await createRuntime('blind-skip');
    const engine = runtime.engine;

    await skipDisclaimer(runtime);
    await startGame(runtime);

    // We're at BLIND_SELECT for the small blind
    expect(getGameState(runtime)).toBe(GameState.BLIND_SELECT);
    expect(readVar(runtime, 'g_game_vars.current_blind')).toBe(BlindType.SMALL);

    // Wait for animation substate
    await engine.wait({ frames: 30 });

    // Press DOWN to move to "skip", then A to confirm
    await engine.press('down');
    await engine.wait({ frames: 5 });
    await engine.press('a');
    await engine.wait({ frames: 10 });

    // After skipping, current_blind should advance from SMALL to BIG
    expect(readVar(runtime, 'g_game_vars.current_blind')).toBe(BlindType.BIG);
    // Should still be on BLIND_SELECT (showing the big blind now)
    expect(getGameState(runtime)).toBe(GameState.BLIND_SELECT);
  });
});
