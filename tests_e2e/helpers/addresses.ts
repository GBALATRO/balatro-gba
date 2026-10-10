import { readFileSync, statSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { DebugInfo } from '@gba-kit/debug-info';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ELF_PATH = join(__dirname, '..', '..', 'build', 'balatro-gba.elf');
const elfStat = statSync(ELF_PATH, { throwIfNoEntry: false });
if (!elfStat) {
  throw new Error(`ELF not found at ${ELF_PATH} — build it first with: UID=$(id -u) GID=$(id -g) docker compose up`);
}
const debug = DebugInfo.fromElf(new Uint8Array(readFileSync(ELF_PATH)));

/**
 * Read a C enum's constants from the DWARF. Throws if the enum isn't in the ELF.
 *
 * @argument stripPrefix Drops the prefix on the enum member names
 */
function buildEnum(name: string, stripPrefix = ''): Record<string, number> {
  const values = debug.enumValues(name);
  if (!values) {
    throw new Error(`Enum '${name}' not found in DWARF (${ELF_PATH})`);
  }
  return Object.fromEntries(
    Object.entries(values).map(([key, value]) => [
      key.startsWith(stripPrefix) ? key.slice(stripPrefix.length) : key,
      value,
    ]),
  );
}

// Enum constants, read from the build's DWARF
export const GameState = buildEnum('GameState', 'GAME_STATE_');
export const HandState = buildEnum('HandState');
export const BlindType = buildEnum('BlindType', 'BLIND_TYPE_');
export const HandType = buildEnum('HandType');
