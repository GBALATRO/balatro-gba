"""Check the ROM's __aeabi_uldivmod against the ARM EABI contract.

The test runs the function from a linked ELF inside Unicorn, so it exercises
the exact instruction stream that ships in the ROM. It is opt-in: it needs the
linked ELF plus unicorn 2.x (see the PR description for the invocation).
"""

import argparse
import random
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path
from typing import NamedTuple

from unicorn import Uc, UC_ARCH_ARM, UC_HOOK_CODE, UC_MODE_ARM
from unicorn.arm_const import (
    UC_ARM_REG_CPSR,
    UC_ARM_REG_LR,
    UC_ARM_REG_PC,
    UC_ARM_REG_R0,
    UC_ARM_REG_R1,
    UC_ARM_REG_R2,
    UC_ARM_REG_R3,
    UC_ARM_REG_R4,
    UC_ARM_REG_R5,
    UC_ARM_REG_R6,
    UC_ARM_REG_R7,
    UC_ARM_REG_R8,
    UC_ARM_REG_R9,
    UC_ARM_REG_R10,
    UC_ARM_REG_R11,
    UC_ARM_REG_SP,
)

# Emulated address space, mirroring the GBA memory map.
ROM_BASE = 0x08000000
WORK_RAM_BASE = 0x03000000
WORK_RAM_SIZE = 0x8000
PAGE_SIZE = 0x1000

# Caller-side state for one emulated call.
INITIAL_SP = 0x03007E00
RETURN_ADDRESS = 0x03000100
STACK_GUARD_BYTES = 128
STACK_GUARD_FILL = 0xA5
CALLEE_SAVED_BASE = 0x12340000
MAX_INSTRUCTIONS = 100_000

# CPSR: system mode, ARM state; bit 5 selects Thumb state.
CPSR_SYSTEM_MODE = 0x1F
CPSR_THUMB_BIT = 0x20

UINT32_MASK = 0xFFFFFFFF
UINT64_MAX = 2**64 - 1

# Fixed seed: the pseudo-random cases are identical on every run, so a failing
# case can be reproduced from the numerator/denominator in the failure message.
SEED = 20261112
RANDOM_CASES_PER_KIND = 512

# Function under test and its frame expectation: __udivmoddi4 receives a
# pointer to the 8-byte remainder slot, which the wrapper keeps above lr.
FUNCTION_UNDER_TEST = '__aeabi_uldivmod'
REQUIRED_SYMBOLS = [FUNCTION_UNDER_TEST, '__udivmoddi4', '__aeabi_ldiv0']
REMAINDER_SLOT_OFFSET = 8

# Encoding test: bits 27-25 == 0b100 is a block transfer, bit 21 (W) set and
# bit 20 (L) clear is a store-multiple with base writeback.
BLOCK_TRANSFER_FIELD_MASK = 0x0E300000
STORE_MULTIPLE_WITH_WRITEBACK = 0x08200000
BASE_REGISTER_FIELD_SHIFT = 16
REGISTER_LIST_BITS = 16

ARGUMENT_REGISTERS = [UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3]
CALLEE_SAVED_REGISTERS = [UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
                          UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11]


class DivisionResult(NamedTuple):
    quotient: int
    remainder: int


def split_uint64(value):
    return value & UINT32_MASK, value >> 32


def join_uint64(low, high):
    return low | (high << 32)


def align_up(value, alignment):
    return (value + alignment - 1) & ~(alignment - 1)


def is_store_multiple_with_listed_base(opcode):
    """True for an STM that writes back a base register it also stores."""
    is_stm_with_writeback = opcode & BLOCK_TRANSFER_FIELD_MASK == STORE_MULTIPLE_WITH_WRITEBACK
    base_register = (opcode >> BASE_REGISTER_FIELD_SHIFT) & (REGISTER_LIST_BITS - 1)
    base_is_in_register_list = opcode & (1 << base_register)
    return bool(is_stm_with_writeback and base_is_in_register_list)


def read_symbols(elf, tool_prefix):
    output = subprocess.check_output(
        [tool_prefix + 'nm', '-S', '--defined-only', str(elf)], text=True)
    symbols = {}
    for line in output.splitlines():
        fields = line.split()
        # nm -S prints "address size type name"; size is absent on some symbols.
        if len(fields) not in (3, 4):
            continue
        address = int(fields[0], 16)
        size = int(fields[1], 16) if len(fields) == 4 else 0
        symbols[fields[-1]] = (address, size)

    for name in REQUIRED_SYMBOLS:
        if name not in symbols:
            raise SystemExit(f'{elf}: missing symbol {name}')

    address, size = symbols[FUNCTION_UNDER_TEST]
    if size == 0:
        # Bound the function by the next symbol when nm reports no size.
        next_address = min(value[0] for value in symbols.values() if value[0] > address)
        symbols[FUNCTION_UNDER_TEST] = (address, next_address - address)
    return symbols


def load_rom(elf, tool_prefix):
    with tempfile.TemporaryDirectory() as directory:
        binary = Path(directory) / 'rom.bin'
        subprocess.run(
            [tool_prefix + 'objcopy', '-O', 'binary', str(elf), str(binary)], check=True)
        return binary.read_bytes()


class UnsignedDivisionTests(unittest.TestCase):
    rom = b''
    symbols = {}

    def setUp(self):
        self.cpu = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        self.cpu.mem_map(ROM_BASE, align_up(len(self.rom), PAGE_SIZE))
        self.cpu.mem_write(ROM_BASE, self.rom)
        self.cpu.mem_map(WORK_RAM_BASE, WORK_RAM_SIZE)
        self.division_calls = []
        self.divide_by_zero_calls = []
        self._hook_function('__udivmoddi4', self._record_division_call)
        self._hook_function('__aeabi_ldiv0', self._record_divide_by_zero)

    def _hook_function(self, name, callback):
        address = self.symbols[name][0] & ~1  # clear the Thumb bit
        self.cpu.hook_add(UC_HOOK_CODE, callback, begin=address, end=address)

    def _record_division_call(self, cpu, address, size, data):
        sp = cpu.reg_read(UC_ARM_REG_SP)
        remainder_pointer = struct.unpack('<I', cpu.mem_read(sp, 4))[0]
        self.division_calls.append((sp, remainder_pointer))

    def _record_divide_by_zero(self, cpu, address, size, data):
        self.divide_by_zero_calls.append(
            join_uint64(cpu.reg_read(UC_ARM_REG_R0), cpu.reg_read(UC_ARM_REG_R1)))

    def _set_up_call(self, numerator, denominator, thumb):
        cpu = self.cpu
        cpu.reg_write(UC_ARM_REG_CPSR, CPSR_SYSTEM_MODE)
        cpu.reg_write(UC_ARM_REG_SP, INITIAL_SP)
        cpu.reg_write(UC_ARM_REG_LR, RETURN_ADDRESS | thumb)
        cpu.mem_write(INITIAL_SP - STACK_GUARD_BYTES,
                      bytes([STACK_GUARD_FILL]) * STACK_GUARD_BYTES)
        self.division_calls.clear()
        self.divide_by_zero_calls.clear()
        for index, register in enumerate(CALLEE_SAVED_REGISTERS):
            cpu.reg_write(register, CALLEE_SAVED_BASE + index)
        arguments = split_uint64(numerator) + split_uint64(denominator)
        for register, value in zip(ARGUMENT_REGISTERS, arguments):
            cpu.reg_write(register, value)

    def _assert_return_state(self, thumb):
        cpu = self.cpu
        self.assertEqual(cpu.reg_read(UC_ARM_REG_PC), RETURN_ADDRESS)
        self.assertEqual(bool(cpu.reg_read(UC_ARM_REG_CPSR) & CPSR_THUMB_BIT), bool(thumb))
        self.assertEqual(cpu.reg_read(UC_ARM_REG_SP), INITIAL_SP)
        for index, register in enumerate(CALLEE_SAVED_REGISTERS):
            self.assertEqual(cpu.reg_read(register), CALLEE_SAVED_BASE + index)

    def divide(self, numerator, denominator, thumb):
        self._set_up_call(numerator, denominator, thumb)
        entry = self.symbols[FUNCTION_UNDER_TEST][0]
        self.cpu.emu_start(entry, RETURN_ADDRESS, count=MAX_INSTRUCTIONS)
        self._assert_return_state(thumb)
        return DivisionResult(
            quotient=join_uint64(self.cpu.reg_read(UC_ARM_REG_R0),
                                 self.cpu.reg_read(UC_ARM_REG_R1)),
            remainder=join_uint64(self.cpu.reg_read(UC_ARM_REG_R2),
                                  self.cpu.reg_read(UC_ARM_REG_R3)))

    def _assert_single_well_formed_division_call(self):
        self.assertEqual(len(self.division_calls), 1)
        sp, remainder_pointer = self.division_calls[0]
        self.assertEqual(sp % 8, 0, 'remainder buffer must keep sp 8-byte aligned')
        self.assertEqual(remainder_pointer, sp + REMAINDER_SLOT_OFFSET,
                         'remainder pointer must address the slot above lr')

    @staticmethod
    def division_cases():
        rng = random.Random(SEED)
        cases = [
            (0, 1),
            (1, 1),
            (123, 10),
            (UINT64_MAX, 1),
            (UINT64_MAX, 10),
            (UINT64_MAX, 2**32),
            (UINT64_MAX, UINT64_MAX),
            (1, 2**63),
        ]
        # "or 1" keeps the generated denominators non-zero.
        cases += [(rng.getrandbits(64), rng.getrandbits(64) or 1)
                  for _ in range(RANDOM_CASES_PER_KIND)]
        cases += [(rng.getrandbits(64), 10) for _ in range(RANDOM_CASES_PER_KIND)]
        return cases

    def test_division_and_remainder(self):
        for numerator, denominator in self.division_cases():
            for thumb in [0, 1]:
                with self.subTest(numerator=numerator, denominator=denominator, thumb=thumb):
                    result = self.divide(numerator, denominator, thumb)
                    self.assertEqual(result, DivisionResult(*divmod(numerator, denominator)))
                    self._assert_single_well_formed_division_call()
                    self.assertEqual(self.divide_by_zero_calls, [])

    def test_divide_by_zero(self):
        for numerator in [0, 1, 2**32, UINT64_MAX]:
            for thumb in [0, 1]:
                with self.subTest(numerator=numerator, thumb=thumb):
                    result = self.divide(numerator, 0, thumb)
                    expected = UINT64_MAX if numerator else 0
                    self.assertEqual(self.divide_by_zero_calls, [expected])
                    self.assertEqual(result.quotient, expected)
                    self.assertEqual(self.division_calls, [])

    def test_no_stm_base_register_writeback(self):
        address, size = self.symbols[FUNCTION_UNDER_TEST]
        self.assertEqual(address % 4, 0)
        self.assertGreater(size, 0)
        code = self.rom[address - ROM_BASE:address - ROM_BASE + size]
        for (opcode,) in struct.iter_unpack('<I', code):
            self.assertFalse(is_store_multiple_with_listed_base(opcode),
                             f'Unsafe STM writeback: {opcode:08x}')


def parse_arguments():
    parser = argparse.ArgumentParser(
        description='Check __aeabi_uldivmod in a linked ARM ELF; requires unicorn 2.x.')
    parser.add_argument('elf', type=Path)
    parser.add_argument('--tool-prefix', default='arm-none-eabi-',
                        help='prefix of the binutils that match the ELF (default: arm-none-eabi-)')
    return parser.parse_args()


def main():
    args = parse_arguments()
    UnsignedDivisionTests.symbols = read_symbols(args.elf, args.tool_prefix)
    UnsignedDivisionTests.rom = load_rom(args.elf, args.tool_prefix)
    unittest.main(argv=[__file__], verbosity=2)


if __name__ == '__main__':
    main()
