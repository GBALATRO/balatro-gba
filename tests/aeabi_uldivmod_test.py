import argparse
import random
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

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

ROM_BASE = 0x08000000
STACK = 0x03007E00
RETURN = 0x03000100
ARGUMENTS = [UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3]
PRESERVED = [UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
             UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11]


class UnsignedDivisionTests(unittest.TestCase):
    def setUp(self):
        self.cpu = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        self.cpu.mem_map(ROM_BASE, (len(self.rom) + 0xFFF) & ~0xFFF)
        self.cpu.mem_write(ROM_BASE, self.rom)
        self.cpu.mem_map(0x03000000, 0x8000)
        self.frames = []
        self.zero_arguments = []

        def record_frame(cpu, address, size, data):
            sp = cpu.reg_read(UC_ARM_REG_SP)
            pointer = struct.unpack('<I', cpu.mem_read(sp, 4))[0]
            self.frames.append((sp, pointer))

        def record_zero(cpu, address, size, data):
            self.zero_arguments.append(cpu.reg_read(UC_ARM_REG_R0) |
                                       (cpu.reg_read(UC_ARM_REG_R1) << 32))

        for name, callback in [('__udivmoddi4', record_frame),
                               ('__aeabi_ldiv0', record_zero)]:
            address = self.symbols[name][0] & ~1
            self.cpu.hook_add(UC_HOOK_CODE, callback, begin=address, end=address)

    def divide(self, numerator, denominator, thumb):
        cpu = self.cpu
        cpu.reg_write(UC_ARM_REG_CPSR, 0x1F)
        cpu.reg_write(UC_ARM_REG_SP, STACK)
        cpu.reg_write(UC_ARM_REG_LR, RETURN | thumb)
        cpu.mem_write(STACK - 128, bytes([0xA5]) * 128)
        self.frames.clear()
        self.zero_arguments.clear()
        for index, reg in enumerate(PRESERVED):
            cpu.reg_write(reg, 0x12340000 + index)
        for reg, value in zip(ARGUMENTS, [numerator & 0xFFFFFFFF, numerator >> 32,
                                         denominator & 0xFFFFFFFF, denominator >> 32]):
            cpu.reg_write(reg, value)
        cpu.emu_start(self.symbols['__aeabi_uldivmod'][0], RETURN, count=100000)
        self.assertEqual(cpu.reg_read(UC_ARM_REG_PC), RETURN)
        self.assertEqual(bool(cpu.reg_read(UC_ARM_REG_CPSR) & 0x20), bool(thumb))
        self.assertEqual(cpu.reg_read(UC_ARM_REG_SP), STACK)
        for index, reg in enumerate(PRESERVED):
            self.assertEqual(cpu.reg_read(reg), 0x12340000 + index)
        result = [cpu.reg_read(reg) for reg in ARGUMENTS]
        return result[0] | (result[1] << 32), result[2] | (result[3] << 32)

    def test_division_and_remainder(self):
        rng = random.Random(20261112)
        cases = [(0, 1), (1, 1), (123, 10), (2**64 - 1, 1),
                 (2**64 - 1, 10), (2**64 - 1, 2**32),
                 (2**64 - 1, 2**64 - 1), (1, 2**63)]
        cases += [(rng.getrandbits(64), rng.getrandbits(64) or 1) for _ in range(512)]
        cases += [(rng.getrandbits(64), 10) for _ in range(512)]
        for thumb in [0, 1]:
            for numerator, denominator in cases:
                with self.subTest(numerator=numerator, denominator=denominator, thumb=thumb):
                    self.assertEqual(self.divide(numerator, denominator, thumb),
                                     divmod(numerator, denominator))
                    self.assertEqual(len(self.frames), 1)
                    sp, pointer = self.frames[0]
                    self.assertEqual(sp % 8, 0)
                    self.assertEqual(pointer, sp + 8)
                    self.assertEqual(self.zero_arguments, [])

    def test_divide_by_zero(self):
        for thumb in [0, 1]:
            for numerator in [0, 1, 2**32, 2**64 - 1]:
                with self.subTest(numerator=numerator, thumb=thumb):
                    quotient, _ = self.divide(numerator, 0, thumb)
                    expected = 2**64 - 1 if numerator else 0
                    self.assertEqual(self.zero_arguments, [expected])
                    self.assertEqual(quotient, expected)
                    self.assertEqual(self.frames, [])

    def test_no_stm_base_register_writeback(self):
        address, size = self.symbols['__aeabi_uldivmod']
        self.assertEqual(address % 4, 0)
        self.assertGreater(size, 0)
        code = self.rom[address - ROM_BASE:address - ROM_BASE + size]
        for (opcode,) in struct.iter_unpack('<I', code):
            is_stm_writeback = opcode & 0x0E300000 == 0x08200000
            base_in_list = opcode & (1 << ((opcode >> 16) & 15))
            self.assertFalse(is_stm_writeback and base_in_list,
                             f'Unsafe STM writeback: {opcode:08x}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Test a linked ARM ROM; requires unicorn 2.x.')
    parser.add_argument('elf', type=Path)
    parser.add_argument('--tool-prefix', default='arm-none-eabi-')
    args = parser.parse_args()
    output = subprocess.check_output([args.tool_prefix + 'nm', '-S', '--defined-only',
                                      str(args.elf)], text=True)
    symbols = {}
    for line in output.splitlines():
        fields = line.split()
        if len(fields) in [3, 4]:
            size = int(fields[1], 16) if len(fields) == 4 else 0
            symbols[fields[-1]] = (int(fields[0], 16), size)
    for name in ['__aeabi_uldivmod', '__udivmoddi4', '__aeabi_ldiv0']:
        if name not in symbols:
            parser.error(f'Missing symbol: {name}')
    address, size = symbols['__aeabi_uldivmod']
    if not size:
        end = min(value[0] for value in symbols.values() if value[0] > address)
        symbols['__aeabi_uldivmod'] = (address, end - address)
    with tempfile.TemporaryDirectory() as directory:
        binary = Path(directory) / 'rom.bin'
        subprocess.run([args.tool_prefix + 'objcopy', '-O', 'binary',
                        str(args.elf), str(binary)], check=True)
        UnsignedDivisionTests.rom = binary.read_bytes()
    UnsignedDivisionTests.symbols = symbols
    unittest.main(argv=[__file__], verbosity=2)
