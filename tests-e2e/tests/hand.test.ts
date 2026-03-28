import { describe, it, expect } from 'vitest';
import { createRuntime, navigateToPlaying, readAddr, getHandState } from '../helpers/runtime.js';
import { ADDR, HandState } from '../helpers/addresses.js';

describe('Hand actions', () => {
  it('playing a hand produces the expected outcome', async () => {
    const runtime = await createRuntime('hand-play');
    const engine = runtime.engine;

    await navigateToPlaying(runtime);

    // We should be in HAND_SELECT state with cards dealt
    expect(getHandState(runtime)).toBe(HandState.HAND_SELECT);

    const handsBefore = readAddr(runtime, 'hands');
    const scoreBefore = readAddr(runtime, 'score');

    // Select the first card (cursor starts on the first card in hand row)
    await engine.press('a');
    await engine.wait({ frames: 10 });

    // Play the hand by pressing L (PLAY_HAND_KEY)
    await engine.press('l');

    // Wait for the scoring sequence to complete.
    // After playing, hand_state goes through HAND_PLAY → HAND_PLAYING,
    // then eventually back to HAND_SELECT or HAND_DRAW for the next turn.
    // The score should change, so wait for hand_state to return to HAND_SELECT.
    await engine.wait({
      memory: { address: ADDR.hand_state.address, equals: HandState.HAND_SELECT },
      timeout: 1800,
    });

    // Hands remaining should have decremented by 1
    expect(readAddr(runtime, 'hands')).toBe(handsBefore - 1);

    // Score should have increased (we played at least a high card)
    expect(readAddr(runtime, 'score')).toBeGreaterThan(scoreBefore);
  });

  it('discarding a hand produces the expected outcome', async () => {
    const runtime = await createRuntime('hand-discard');
    const engine = runtime.engine;

    await navigateToPlaying(runtime);

    expect(getHandState(runtime)).toBe(HandState.HAND_SELECT);

    const discardsBefore = readAddr(runtime, 'discards');
    const scoreBefore = readAddr(runtime, 'score');
    const handsBefore = readAddr(runtime, 'hands');

    // Select the first card
    await engine.press('a');
    await engine.wait({ frames: 10 });

    // Discard by pressing R (DISCARD_HAND_KEY)
    await engine.press('r');

    // Wait for discard animation to complete and return to HAND_SELECT
    await engine.wait({
      memory: { address: ADDR.hand_state.address, equals: HandState.HAND_SELECT },
      timeout: 1800,
    });

    // Discards should have decremented
    expect(readAddr(runtime, 'discards')).toBe(discardsBefore - 1);

    // Score should NOT have changed (discarding doesn't score)
    expect(readAddr(runtime, 'score')).toBe(scoreBefore);

    // Hands remaining should be unchanged
    expect(readAddr(runtime, 'hands')).toBe(handsBefore);
  });
});
