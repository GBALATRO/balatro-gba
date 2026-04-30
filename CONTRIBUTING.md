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

## Documentation 
Doxygen is used to build docs that can be opened in browser.

We host the docs for the main branch on github [here](https://gbalatro.github.io/balatro-gba)

You can also choose to build the docs yourself [Link in wiki](https://github.com/GBALATRO/balatro-gba/wiki/Documentation-for-Developers)

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
- **save_build.sh**: Save a timestamped copy of build outputs (.elf, .gba, .map).

## Debugging
It's recommended to use [mGBA](https://mgba.io/) for ROM testing and debugging. As it provides a [`gdbserver`](https://en.wikipedia.org/wiki/Gdbserver) via the `-g` flag `mgba -g build/balatro-gba.gba`. You can connect via `gdb` or here is a [great guide for vscode](https://felixjones.co.uk/mgba_gdb/vscode.html).

## **Build Instructions**

<details>
  <summary><b>Docker</b></summary>

### Docker
A docker compose file is provided to build this project. It provides a standard build environment for this projects CI/CD and can also be used to build the ROM locally.

_Some users may find this option to be the easiest way to build locally._
- _This option **avoids** setting up the development environment as described below._
- _No additional software besides **docker desktop** is required._

#### Step-by-Step

1.) Install [docker desktop](https://docs.docker.com/desktop/) for your operating system.

2.) Open a terminal to this project's directory:
- On **Linux** run `UID=$(id -u) GID=$(id -g) docker compose up`
- On **Windows** run `docker compose up`

<details>
  <summary><i>How do I open a terminal in windows?</i></summary>

---

From the file explorer, you can open a folder in **powershell** (_a modern windows terminal_):

- **hold 'Shift'** and **Right Click** on the folder. 

- Select **"Open PowerShell window here"** from the popup menu.

---

</details>

3.) Docker will build the project and the ROM will be in the same location as step 8 describes below.

</details>

<details>
  <summary><b>Windows</b></summary>

### Windows
Video Tutorial: https://youtu.be/72Zzo1VDYzQ?si=UDmEdbST1Cx1zZV2
### With `Git` (not required)
1.) Install `devkitPro` from https://github.com/devkitPro/installer by downloading the latest release, unzipping the archive, and then running the installer. You only need to install the tools for the GBA development so ensure you have it selected and install it to the default directory. You may need to create a temp folder for the project.

2.) Search for `MSys2` in the Start Menu and open it.

3.) Install `python-pillow` from the `mingw64` repo by copying the following script into your `MSys2` terminal and hitting **Enter** to run:

> ```sh
> pacman-key --populate devkitpro
> echo -e "[mingw64]\nServer = https://mirror.msys2.org/mingw/mingw64" >> /etc/pacman.conf
> pacman -Syu --noconfirm
> pacman -S --noconfirm mingw-w64-x86_64-python-pillow
> echo "export PATH="/mingw64/bin:$PATH"" >> /etc/bash.bashrc
> source /etc/bash.bashrc 
> ```

4.) Install `Git` by typing this command: `pacman -S git` if you don't have it already installed

5.) Clone the project by putting `git clone https://github.com/GBALATRO/balatro-gba.git` in the MSys2 window.

6.) Use `cd` and drag the new folder into the window to add the file path and press Enter.

7.) Type `make` into the window and press Enter to start building the rom.

8.) After it completes, navigate through the `build` directory in the project folder and look for `balatro-gba.gba` and load it on an emulator or flashcart.
#### Without `Git`
Disregard Steps 4-5 and instead click the green code button on the main repository page and press `Download Zip`. Unzip the folder and place it wherever you like. Then continue from Step 6.

</details>

<details>
  <summary><b>Linux</b></summary>

### Linux (Debian/Arch)

These instructions will be only for Debian (**Ubuntu**) and Arch based systems. 

1.) Follow the devkitPro Pacman installation guide for distro:   https://devkitpro.org/wiki/devkitPro_pacman
- On **Debian** systems it will be installed as **`dkp-pacman`**.
- On **Arch** systems you will use regular **`pacman`**.

2.) Install devkitPro by running 
> ```sh
> sudo (dkp-)pacman -S --noconfirm gba-dev
> ```

3.) Activate the devkitPro environment by running 
> ```sh
> source /etc/profile.d/devkit-env.sh
> ```

4.) Install `git`
- On **Debian**: **`sudo apt install -y git`**.
- On **Arch**: **`sudo pacman -S --noconfirm git`**.

5.) Clone this repository
> ```sh
> git clone https://github.com/GBALATRO/balatro-gba.git ~/balatro-gba
> ```

6.) Build the project
> ```sh
> cd ~/balatro-gba && make
> ```

</details>

<details>
  <summary><b>MacOS</b></summary>

### MacOS

🔴 **Requires python-pillow to be installed. Currently, no maintainer has a macOS machine to test on. If you have python and python-pillow tested installation instructions for MacOS, please let us know and we will add it here.** 🔴

1.) Install devkitPro installer using: https://github.com/devkitPro/installer and following https://devkitpro.org/wiki/devkitPro_pacman#macOS.
> Note: You may have to install the installers directly from their url in a browser, as the installer script may not install correctly due to Cloudflare checks on their server. You can use one of the following urls: 

> Apple Silicon: https://pkg.devkitpro.org/packages/macos-installers/devkitpro-pacman-installer.arm64.pkg

> Intel: https://pkg.devkitpro.org/packages/macos-installers/devkitpro-pacman-installer.x86_64.pkg

2.) Run `sudo dkp-pacman -S gba-dev`

3.) Verify that devkitPro is installed in '/opt/devkitpro'

4.) Add the following to your .bashrc or .zshrc (or export the variables in your shell session): 
- export DEVKITPRO=/opt/devkitpro
- export DEVKITARM=$DEVKITPRO/devkitARM
- export PATH=$PATH:$DEVKITPRO/tools/bin:$DEVKITPRO/pacman/bin

5.) Follow instructions from Windows tutorial step 5

</details>

### Common Issues:

#### 1. When I run `make` it errors out and won't compile!
- Move the project folder to your desktop and then `cd` to it by dragging the folder into the terminal window. This error could be caused by a bad file path, placing the folder on the desktop is a failsafe.

#### 2. I can't find the compiled rom!
- Look for a folder called `build` inside the project folder and then find `balatro-gba.gba`.

#### 3. The Game won't start!
- Try a different emulator or if you are using original hardware, make sure the rom is not corrupted and try a different flashcart or SD Card. If this does not work, open an issue on the Github page because a recent commit may have broke the game.

#### 4. It says I don't have `Git` or `Make` installed!
- Please verify you ran all the instructions required for your operating system. They will have the commands to install all required commands.
