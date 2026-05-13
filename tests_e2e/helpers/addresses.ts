import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { ensureAddresses } from './ensure_addresses.js';
import varNames from '../var_names.json' with { type: 'json' };

ensureAddresses();

const __dirname = dirname(fileURLToPath(import.meta.url));
const NM_OUTPUT_PATH = join(__dirname, '..', 'nm_output.txt');
const nmOutput = readFileSync(NM_OUTPUT_PATH, 'utf-8');

type AddrKey = keyof typeof varNames;
type AddrEntry = { address: number; size: number };

/** Resolve a C symbol's address and size from the cached nm --print-size output. */
function lookup(testName: AddrKey, cSymbol: string): AddrEntry {
  const re = new RegExp(`^([0-9a-fA-F]+) ([0-9a-fA-F]+) \\S ${cSymbol}$`, 'm');
  const match = nmOutput.match(re);
  if (!match) {
    throw new Error(
      `Symbol '${cSymbol}' (mapped from test name '${testName}') not found in ${NM_OUTPUT_PATH} — check var_names.json`
    );
  }
  return { address: parseInt(match[1], 16), size: parseInt(match[2], 16) };
}

// Game variable addresses (keys and C symbols sourced from var_names.json)
export const ADDR = Object.fromEntries(
  (Object.entries(varNames) as [AddrKey, string][]).map(([testName, cSymbol]) => [
    testName,
    lookup(testName, cSymbol),
  ])
) as Record<AddrKey, AddrEntry>;

// Game state enum values (from include/def_state_info_table.h order)
export const GameState = {
  SPLASH_SCREEN: 0,
  MAIN_MENU: 1,
  PLAYING: 2,
  ROUND_END: 3,
  SHOP: 4,
  BLIND_SELECT: 5,
  LOSE: 6,
  WIN: 7,
} as const;

// Hand state enum values (from include/game.h)
export const HandState = {
  HAND_DRAW: 0,
  HAND_SELECT: 1,
  HAND_SHUFFLING: 2,
  HAND_DISCARD: 3,
  HAND_PLAY: 4,
  HAND_PLAYING: 5,
} as const;

// Blind type enum values (from include/blind.h)
export const BlindType = {
  SMALL: 0,
  BIG: 1,
  BOSS: 2,
} as const;

// Hand type enum values (from include/game.h)
export const HandType = {
  NONE: 0,
  HIGH_CARD: 1,
  PAIR: 2,
  TWO_PAIR: 3,
  THREE_OF_A_KIND: 4,
  STRAIGHT: 5,
  FLUSH: 6,
  FULL_HOUSE: 7,
  FOUR_OF_A_KIND: 8,
  STRAIGHT_FLUSH: 9,
  ROYAL_FLUSH: 10,
  FIVE_OF_A_KIND: 11,
  FLUSH_HOUSE: 12,
  FLUSH_FIVE: 13,
} as const;
