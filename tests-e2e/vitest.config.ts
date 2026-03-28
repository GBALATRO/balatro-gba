import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    testTimeout: 60_000,
    hookTimeout: 60_000,
    globalSetup: './global-setup.ts',
    include: ['tests/**/*.test.ts'],
    forceRerunTriggers: ['../build/balatro-gba.gba', '../build/balatro-gba.elf'],
  },
});
