import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { ensureAddresses } from './ensure_addresses.js';

ensureAddresses();

const __dirname = dirname(fileURLToPath(import.meta.url));
const ADDRESSES_PATH = join(__dirname, '..', 'addresses.json');

const raw: Record<string, { address: string; size: number }> = JSON.parse(readFileSync(ADDRESSES_PATH, 'utf-8'));

function parseEntry(entry: { address: string; size: number }): { address: number; size: number } {
  return { address: parseInt(entry.address, 16), size: entry.size };
}

export const ADDR = {
  game_state: parseEntry(raw.game_state),
  hand_state: parseEntry(raw.hand_state),
  play_state: parseEntry(raw.play_state),
  score: parseEntry(raw.score),
  chips: parseEntry(raw.chips),
  mult: parseEntry(raw.mult),
  hands: parseEntry(raw.hands),
  discards: parseEntry(raw.discards),
  rng_seed: parseEntry(raw.rng_seed),
  hand_type: parseEntry(raw.hand_type),
  ante: parseEntry(raw.ante),
  round: parseEntry(raw.round),
  money: parseEntry(raw.money),
  current_blind: parseEntry(raw.current_blind),
} as const;

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
