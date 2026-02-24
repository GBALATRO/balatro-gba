# Developer Guide

Thank you for reading this. Below is a quick summary of expectations and tips to contribute.

## Scope
We are limiting the scope of the project in order to not compete with the original Balatro and avoid a takedown by Playstack.
We limited the scope to 52 jokers and reached that limit so currently there is no plan to add more jokers.
See [the scope discussion](https://github.com/GBALATRO/balatro-gba/discussions/355).

## Art
Before contributing art or if you need art for a code contribution check the [existing additional art thread](https://github.com/GBALATRO/balatro-gba/discussions/131) and the [joker art discussion](https://github.com/GBALATRO/balatro-gba/discussions/69) (though as said no more jokers are currently planned).

Note that there are color limitations for sprites and backgrounds resulting due to the GBA hardware. Sprites may not use more than 16 colors per sprite including transparency.
Backgrounds may use more colors but notice that their palette is encoded in their PNGs and new colors need to be added manually to the palette. [See relevant PR](https://github.com/GBALATRO/balatro-gba/pull/350).

## CI Checks
On pull-requests, various checks will be performed:
1. **Formatting**: `clang-format` will be ran on every `.c/.h` file with [this configuration](https://github.com/GBALATRO/balatro-gba/blob/main/.clang-format). Failures will cause the CI to fail.
2. **Unit Tests**: Unit tests are required to pass and are located in the repo [here](https://github.com/GBALATRO/balatro-gba/tree/main/tests).
3. **Rom Build**: The ROM must successfully build with the `make -j$(nproc)` command.

## Code Style
Besides the automatic formatting checks from `clang-format`, there is a looser set of code style rules. These are not strictly required, but is encouraged to be followed.  
The following details the code style rules including the enforced clang-format style.
[Link in wiki](https://github.com/GBALATRO/balatro-gba/wiki/Code-Style-Guide)

## Building Documentation 
Doxygen is used to build documentation that can be opened in browser.

[Link in wiki](https://github.com/GBALATRO/balatro-gba/wiki/Documentation-for-Developers)

## Tools

### clang-format

Running `clang-format` locally is recommended before submitting a PR as it will fail the **CI Checks** if not properly formatted. It is recommended either: 
1. Run `clang-format` periodically and only commit formatted code. 
2. Run `clang-format` as a separate commit on larger changes, and review each modified hunk. 

Either way, just ensure you manually review automatic changes.

#### VSCode

The recommended setup for VSCode is to install the [**clangd**](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) extension. It will provide helpful information in VSCode and can be used to format the code automatically according to the `.clang-format` file with **`Ctrl+Shift+I`**

There is an option to enable `"editor.formatOnSave"` in the VSCode `settings.json` file. You can also do this by opening the settings menu (`File->Preferences->Settings`) and searching `format on save`.

#### Manually

If installed locally and you'd prefer to use it in your shell. You can do the following

```sh
# List warnings
clang-format --dry-run -Werror include/*.h source/*.c
# Modify all files inplace
clang-format -i include/*.h source/*.c
# Or just one
clang-format -i include/blind.h
```

#### Disabling Formatting

Sometimes `clang-format` rules need to be broken, like in the case of the [joker registry](https://github.com/GBALATRO/balatro-gba/blob/8fb0813cf5f7235b6450dc9a76252dda4d9b4a27/source/joker_effects.c#L333) and other tables or maps. If it makes sense, you can wrap code in `// clang-format off` and `// clang-format on`:

- **Without clang-format**:
```c
// clang-format off
const table_of_tables = 
{
    { TABLE_A,   1,   tableAMap },
    { TABLE_B,   2,   tableBMap },
    { TABLE_C,   3,   tableCMap },
    { TABLE_D,   4,   tableDMap },
    { TABLE_E,   5,   tableEMap },
    { TABLE_F,   6,   tableFMap },
    { TABLE_G,   7,   tableGMap },
}
// clang-format on
```
- **With clang-format**:
```c
const table_of_tables = 
{
    {TABLE_A, 1, tableAMap}, {TABLE_B, 2, tableBMap}, {TABLE_C, 3, tableCMap},
        {TABLE_D, 4, tableDMap}, {TABLE_E, 5, tableEMap}, {TABLE_F, 6, tableFMap},
        {TABLE_G, 7, tableGMap},
}
```

### Custom Scripts
In the repo we use custom scripts located in the [`scripts`](https://github.com/GBALATRO/balatro-gba/tree/main/scripts) directory.

🟡 **Note**: `python3` and `bash` are required for these scripts.

- **get_hash.py**: Get git hash from ROM.
- **generate_font.py**: Generate a font manually.
- **get_memory_map.sh**: Print the memory map of the pre-allocated pools.

## Debugging
It's recommended to use [mGBA](https://mgba.io/) for ROM testing and debugging. As it provides a [`gdbserver`](https://en.wikipedia.org/wiki/Gdbserver) via the `-g` flag `mgba -g build/balatro-gba.gba`. You can connect via `gdb` or here is a [great guide for vscode](https://felixjones.co.uk/mgba_gdb/vscode.html).
