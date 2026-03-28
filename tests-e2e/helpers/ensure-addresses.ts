import { statSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ELF_PATH = join(__dirname, '..', '..', 'build', 'balatro-gba.elf');
const ADDRESSES_PATH = join(__dirname, '..', 'addresses.json');
const EXTRACT_SCRIPT = join(__dirname, 'extract-addresses.sh');

/**
 * Re-extract addresses from the ELF if it's newer than addresses.json.
 */
export function ensureAddresses(): void {
  const elfStat = statSync(ELF_PATH, { throwIfNoEntry: false });
  if (!elfStat) {
    throw new Error(`ROM not found at ${ELF_PATH} — build it first with: UID=$(id -u) GID=$(id -g) docker compose up`);
  }

  const addrStat = statSync(ADDRESSES_PATH, { throwIfNoEntry: false });
  if (!addrStat || elfStat.mtimeMs > addrStat.mtimeMs) {
    console.log('ROM is newer than addresses.json — re-running extract-addresses.sh');
    execFileSync('bash', [EXTRACT_SCRIPT], { stdio: 'inherit' });
  }
}
