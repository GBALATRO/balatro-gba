import { HeadlessRuntime } from '@gba-kit/gba-node';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { mkdirSync } from 'node:fs';
import { ADDR, GameState, HandState } from './addresses.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROM_PATH = join(__dirname, '..', '..', 'build', 'balatro-gba.gba');

/**
 * Create a fresh HeadlessRuntime for a test.
 * Each test gets its own output directory under tests_e2e/output/<testName>.
 */
export async function createRuntime(testName: string): Promise<HeadlessRuntime> {
  const outputDir = join(__dirname, '..', 'output', testName);
  mkdirSync(outputDir, { recursive: true });

  const runtime = await HeadlessRuntime.create({
    romPath: ROM_PATH,
    outputDir,
    logFn: () => {},
  });

  return runtime;
}

/** Read a game variable from memory, using the correct size from the ELF symbols. */
export function readAddr(runtime: HeadlessRuntime, addr: keyof typeof ADDR): number {
  const { address, size } = ADDR[addr];
  if (size === 1) {
    return runtime.engine.getMemory(address, 1)[0];
  }
  return runtime.engine.read32(address);
}

/** Get the current game state enum value. */
export function getGameState(runtime: HeadlessRuntime): number {
  return readAddr(runtime, 'game_state');
}

/** Get the current hand state enum value. */
export function getHandState(runtime: HeadlessRuntime): number {
  return readAddr(runtime, 'hand_state');
}

/**
 * Advance from boot past the disclaimer screen by pressing A.
 * Returns with game_state === MAIN_MENU.
 */
export async function skipDisclaimer(runtime: HeadlessRuntime): Promise<void> {
  const engine = runtime.engine;
  // Wait a few frames for the splash screen to initialize
  await engine.wait({ frames: 30 });
  await engine.press('a');
  // Wait for state transition to complete
  await engine.wait({
    memory: { address: ADDR.game_state.address, equals: GameState.MAIN_MENU },
    timeout: 60,
  });
}

/**
 * From main menu, press A to start the game.
 * The PLAY button is selected by default (selection_x starts at 0).
 * Returns with game_state === BLIND_SELECT.
 */
export async function startGame(runtime: HeadlessRuntime): Promise<void> {
  const engine = runtime.engine;
  // Wait a few frames on main menu for things to settle
  await engine.wait({ frames: 10 });
  await engine.press('a');
  await engine.wait({
    memory: { address: ADDR.game_state.address, equals: GameState.BLIND_SELECT },
    timeout: 300,
  });
}

/**
 * From blind select, select (play) the current blind.
 * selection_y starts at 0 = "select blind", so just press A.
 * Waits through the animation substates until GAME_STATE_PLAYING.
 */
export async function selectBlind(runtime: HeadlessRuntime): Promise<void> {
  const engine = runtime.engine;
  // Wait for the start animation to finish (substate goes to BLIND_SELECT=1)
  await engine.wait({ frames: 30 });
  // Press UP to ensure we're on "select" (y=0), then A to confirm
  await engine.press('up');
  await engine.wait({ frames: 5 });
  await engine.press('a');
  // Wait through animation substates until GAME_STATE_PLAYING
  await engine.wait({
    memory: { address: ADDR.game_state.address, equals: GameState.PLAYING },
    timeout: 600,
  });
}

/**
 * Wait for cards to be dealt and hand_state to become HAND_SELECT.
 */
export async function waitForHandSelect(runtime: HeadlessRuntime): Promise<void> {
  await runtime.engine.wait({
    memory: { address: ADDR.hand_state.address, equals: HandState.HAND_SELECT },
    timeout: 600,
  });
}

/**
 * Navigate from boot to the PLAYING state with cards dealt.
 * Combines: skip disclaimer → start game → select blind → wait for hand.
 */
export async function navigateToPlaying(runtime: HeadlessRuntime): Promise<void> {
  await skipDisclaimer(runtime);
  await startGame(runtime);
  await selectBlind(runtime);
  await waitForHandSelect(runtime);
}
