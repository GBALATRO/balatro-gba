import { describe, it, expect } from 'vitest';
import { createRuntime, getGameState } from '../helpers/runtime.js';
import { GameState } from '../helpers/addresses.js';

describe('Splash screen', () => {
  it('pressing any key dismisses the splash', async () => {
    const runtime = await createRuntime('splash-key-press');
    const engine = runtime.engine;

    // Wait for splash screen to initialize and be visible
    await engine.wait({ frames: 30 });
    expect(getGameState(runtime)).toBe(GameState.SPLASH_SCREEN);

    // Press a button
    await engine.press('a');

    // Should transition to main menu
    await engine.wait({
      memory: { address: 'game_sm.state', equals: GameState.MAIN_MENU },
      timeout: 60,
    });

    expect(getGameState(runtime)).toBe(GameState.MAIN_MENU);
  });

  it('auto-dismisses after 10 seconds without input', async () => {
    const runtime = await createRuntime('splash-auto-dismiss');
    const engine = runtime.engine;

    // Wait for splash screen to initialize
    await engine.wait({ frames: 10 });
    expect(getGameState(runtime)).toBe(GameState.SPLASH_SCREEN);

    // Wait without pressing anything — splash lasts 600 frames (10s at 60fps).
    // Advance to just before the transition point to confirm still on splash.
    await engine.wait({ frames: 570 });
    expect(getGameState(runtime)).toBe(GameState.SPLASH_SCREEN);

    // Now wait for the auto-dismiss transition
    await engine.wait({
      memory: { address: 'game_sm.state', equals: GameState.MAIN_MENU },
      timeout: 60,
    });
    expect(getGameState(runtime)).toBe(GameState.MAIN_MENU);
  });
});
