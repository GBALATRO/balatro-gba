import { statSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ELF_PATH = join(__dirname, '..', '..', 'build', 'balatro-gba.elf');
const NM_OUTPUT_PATH = join(__dirname, '..', 'nm_output.txt');
const EXTRACT_SCRIPT = join(__dirname, 'extract_addresses.sh');

/**
 * Re-dump the ELF symbol table to nm_output.txt if the ELF is newer.
 */
export function ensureAddresses(): void {
  const elfStat = statSync(ELF_PATH, { throwIfNoEntry: false });
  if (!elfStat) {
    throw new Error(`ELF not found at ${ELF_PATH} — build it first with: UID=$(id -u) GID=$(id -g) docker compose up`);
  }

  const nmStat = statSync(NM_OUTPUT_PATH, { throwIfNoEntry: false });
  if (!nmStat || elfStat.mtimeMs > nmStat.mtimeMs) {
    console.log('ELF is newer than nm_output.txt — re-running extract_addresses.sh');
    execFileSync('bash', [EXTRACT_SCRIPT], { stdio: 'inherit' });
  }
}
