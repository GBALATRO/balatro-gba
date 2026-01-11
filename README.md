# Balatro-GBA

[![Build Status](https://img.shields.io/github/actions/workflow/status/cellos51/balatro-gba/build_ci_workflow.yml?style=flat&logo=github&branch=main&label=Builds&labelColor=gray&color=default&maxAge=7200)](https://github.com/cellos51/balatro-gba/actions)
[![Open Issues](https://img.shields.io/github/issues/cellos51/balatro-gba?style=flat&color=red&label=Issues&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxNiAxNiIgd2lkdGg9IjE2IiBoZWlnaHQ9IjE2Ij48cGF0aCBmaWxsPSJ3aGl0ZSIgZD0iTTggOS41YTEuNSAxLjUgMCAxIDAgMC0zIDEuNSAxLjUgMCAwIDAgMCAzWiI+PC9wYXRoPjxwYXRoIGZpbGw9IndoaXRlIiBkPSJNOCAwYTggOCAwIDEgMSAwIDE2QTggOCAwIDAgMSA4IDBaTTEuNSA4YTYuNSA2LjUgMCAxIDAgMTMgMCA2LjUgNi41IDAgMCAwLTEzIDBaIj48L3BhdGg+PC9zdmc+)](https://github.com/cellos51/balatro-gba/issues)
[![Pull Requests](https://img.shields.io/github/issues-pr/cellos51/balatro-gba?style=flat&color=indigo&label=Pull%20Requests&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxNiAxNiIgd2lkdGg9IjE2IiBoZWlnaHQ9IjE2Ij48cGF0aCBmaWxsPSJ3aGl0ZSIgZD0iTTEuNSAzLjI1YTIuMjUgMi4yNSAwIDEgMSAzIDIuMTIydjUuMjU2YTIuMjUxIDIuMjUxIDAgMSAxLTEuNSAwVjUuMzcyQTIuMjUgMi4yNSAwIDAgMSAxLjUgMy4yNVptNS42NzctLjE3N0w5LjU3My42NzdBLjI1LjI1IDAgMCAxIDEwIC44NTRWMi41aDFBMi41IDIuNSAwIDAgMSAxMy41IDV2NS42MjhhMi4yNTEgMi4yNTEgMCAxIDEtMS41IDBWNWExIDEgMCAwIDAtMS0xaC0xdjEuNjQ2YS4yNS4yNSAwIDAgMS0uNDI3LjE3N0w3LjE3NyAzLjQyN2EuMjUuMjUgMCAwIDEgMC0uMzU0Wk0zLjc1IDIuNWEuNzUuNzUgMCAxIDAgMCAxLjUuNzUuNzUgMCAwIDAgMC0xLjVabTAgOS41YS43NS43NSAwIDEgMCAwIDEuNS43NS43NSAwIDAgMCAwLTEuNVptOC4yNS43NWEuNzUuNzUgMCAxIDAgMS41IDAgLjc1Ljc1IDAgMCAwLTEuNSAwWiI+PC9wYXRoPjwvc3ZnPg==)](https://github.com/cellos51/balatro-gba/pulls)
[![Discussions](https://img.shields.io/github/discussions/cellos51/balatro-gba?style=flat&color=blue&label=Discussions&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxNiAxNiIgd2lkdGg9IjE2IiBoZWlnaHQ9IjE2Ij48cGF0aCBmaWxsPSJ3aGl0ZSIgZD0iTTEuNzUgMWg4LjVjLjk2NiAwIDEuNzUuNzg0IDEuNzUgMS43NXY1LjVBMS43NSAxLjc1IDAgMCAxIDEwLjI1IDEwSDcuMDYxbC0yLjU3NCAyLjU3M0ExLjQ1OCAxLjQ1OCAwIDAgMSAyIDExLjU0M1YxMGgtLjI1QTEuNzUgMS43NSAwIDAgMSAwIDguMjV2LTUuNUMwIDEuNzg0Ljc4NCAxIDEuNzUgMVpNMS41IDIuNzV2NS41YzAgLjEzOC4xMTIuMjUuMjUuMjVoMWEuNzUuNzUgMCAwIDEgLjc1Ljc1djIuMTlsMi43Mi0yLjcyYS43NDkuNzQ5IDAgMCAxIC41My0uMjJoMy41YS4yNS4yNSAwIDAgMCAuMjUtLjI1di01LjVhLjI1LjI1IDAgMCAwLS4yNS0uMjVoLTguNWEuMjUuMjUgMCAwIDAtLjI1LjI1Wm0xMyAyYS4yNS4yNSAwIDAgMC0uMjUtLjI1aC0uNWEuNzUuNzUgMCAwIDEgMC0xLjVoLjVjLjk2NiAwIDEuNzUuNzg0IDEuNzUgMS43NXY1LjVBMS43NSAxLjc1IDAgMCAxIDE0LjI1IDEySDE0djEuNTQzYTEuNDU4IDEuNDU4IDAgMCAxLTIuNDg3IDEuMDNMOS4yMiAxMi4yOGEuNzQ5Ljc0OSAwIDAgMSAuMzI2LTEuMjc1Ljc0OS43NDkgMCAwIDEgLjczNC4yMTVsMi4yMiAyLjIydi0yLjE5YS43NS43NSAwIDAgMSAuNzUtLjc1aDFhLjI1LjI1IDAgMCAwIC4yNS0uMjVaIj48L3BhdGg+PC9zdmc+)](https://github.com/cellos51/balatro-gba/discussions)

This is an attempt to recreate the game **'Balatro'** as accurately as possible, including all of the visual effects that make Balatro feel satisfying to play.
This **tech-demo/proof of concept** is strictly limited in content to a minimal version of Balatro and will **NOT** recreate the full game. **This version is intended for people who already own and know how the official full game works.** Please refer to the Balatro Wiki if you need help understanding certain mechanics or abilities.

<a href="https://balatrowiki.org/">
  <img src="https://custom-icon-badges.demolab.com/badge/Balatro%20Wiki-194c84?logo=bigjoker&logoColor=fff" alt="Balatro Wiki" width="155">
</a>

### Disclaimer: This project is NOT endorsed by or affiliated with Playstack or LocalThunk
#### This is a non-profit community fan project solely aimed to recreate a minimal version of Balatro on the Game Boy Advance as a tribute to the full Balatro and is not intended to infringe or draw sales away from the full game's release or any of the established works by Playstack and LocalThunk.
#### All rights are reserved to their respective holders. 

### Please buy the official full version from these sources below:
[![Balatro on Steam](https://custom-icon-badges.demolab.com/badge/Balatro%20on%20Steam-194c84?logo=steam&logoColor=fff)](https://store.steampowered.com/app/2379780/Balatro/)
[![Balatro on Google Play](https://custom-icon-badges.demolab.com/badge/Balatro%20on%20Google%20Play-414141?logo=Google-play&logoColor=fff)](https://play.google.com/store/apps/details?id=com.playstack.balatro.android)
[![Balatro on Apple App Store](https://custom-icon-badges.demolab.com/badge/Balatro%20on%20Apple%20App%20Store-0D96F6?logo=app-store&logoColor=fff)](https://apps.apple.com/us/app/balatro/id6502453075)
[![Balatro on Nintendo eShop](https://custom-icon-badges.demolab.com/badge/Balatro%20on%20Nintendo%20eShop-e60012?logo=nintendo&logoColor=fff)](https://www.nintendo.com/us/store/products/balatro-switch/)
[![Balatro on PlayStation Store](https://custom-icon-badges.demolab.com/badge/Balatro%20on%20PlayStation%20Store-006FCD?logo=PlayStation&logoColor=fff)](https://store.playstation.com/en-us/concept/10010334)
[![Balatro on Xbox](https://custom-icon-badges.demolab.com/badge/Balatro%20on%20Xbox-107C10.svg?logo=xbox&logoColor=white)](https://www.xbox.com/en-US/games/store/balatro/9PK087LNGJC5)
[![Balatro on Humble Bundle](https://img.shields.io/badge/Balatro%20on%20Humble%20Bundle-%23494F5C.svg?logo=HumbleBundle&logoColor=white)](https://www.humblebundle.com/store/balatro?srsltid=AfmBOoqS2De8T4kizzWxJS1pbvQosJ_bYCl4qvC6LA1YLPAh4sZ8vJqO)

---

<a href="https://github.com/cellos51/balatro-gba/releases/">
  <img src="https://img.shields.io/badge/Download_ROMs_from_the_Releases_tab-8A2BE2?&logo=github" alt="Download ROM" width="500">
</a>

---

<!-- The Gif is a little blurry but I think it looks fine -->
<img src="example.gif" alt="Example GIF" width="800">

### Controls: 
(A: Pick Card/Make Selections)

(B: Deselect All Cards) 

(L: Sell Joker)

(SELECT: Toggle Sort Suit/Rank)

(D-Pad: Navigation) 

(Hold A: Swap Owned Jokers in the Shop)

# Contributing

If you would like to contribute, please read CONTRIBUTING.md.

# **Build Instructions:**

## **-Docker-**
A docker compose file is provided to build this project. It provides a standard build environment for this projects CI/CD and can also be used to build the ROM locally.

_Some users may find this option to be the easiest way to build locally._
- _This option **avoids** setting up the development environment as described below._
- _No additional software besides **docker desktop** is required._

### Step-by-Step

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

3.) Docker will build the project and the ROM will be in the same location as step 7 describes below.

## **-Windows-**
Video Tutorial: https://youtu.be/72Zzo1VDYzQ?si=UDmEdbST1Cx1zZV2
### With `Git` (not required)
1.) Install `devkitPro` from https://github.com/devkitPro/installer by downloading the latest release, unzipping the archive, and then running the installer. You only need to install the tools for the GBA development so ensure you have it selected and install it to the default directory. You may need to create a temp folder for the project.

2.) Search for `MSys2` in the Start Menu and open it.

3.) Install `Git` by typing this command: `pacman -S git` if you don't have it already installed

4.) Clone the project by putting `git clone https://github.com/cellos51/balatro-gba.git` in the MSys2 window.

5.) Use `cd` and drag the new folder into the window to add the file path and press Enter.

6.) Type `make` into the window and press Enter to start building the rom.

7.) After it completes, navigate through the `build` directory in the project folder and look for `balatro-gba.gba` and load it on an emulator or flashcart.
### Without `Git`
Disregard Steps 3-4 and instead click the green code button on the main repository page and press `Download Zip`. Unzip the folder and place it wherever you like. Then continue from Step 5.

## **-Linux-**

1.) Add the devkitPro repository using these instructions https://devkitpro.org/wiki/devkitPro_pacman

2.) Install devkitPro by running `sudo pacman -S gba-dev` and accepting all packages.

3.) Activate the devkitPro environment by running `source /etc/profile.d/devkit-env.sh` or opening a new shell.

4.) Follow instructions from the Windows tutorial starting from Step 3

## **-macOS-**
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

5.) Follow instructions from Windows tutorial step 4

## **Common Issues:**

#### 1. **When I run `make` it errors out and won't compile!**
- Move the project folder to your desktop and then `cd` to it by dragging the folder into the terminal window. This error could be caused by a bad file path, placing the folder on the desktop is a failsafe.

#### 2. **I can't find the compiled rom!**
- Look for a folder called `build` inside the project folder and then find `balatro-gba.gba`.

#### 3. **The Game won't start!**
- Try a different emulator or if you are using original hardware, make sure the rom is not corrupted and try a different flashcart or SD Card. If this does not work, open an issue on the Github page because a recent commit may have broke the game.

#### 4. **It says I don't have `Git` or `Make` installed!**
- Use `pacman -S git` (not required) or `pacman -S make` although make should already be installed if you followed the instructions correctly.

# **Credits:**
## **Game**
This GBA implementation is based on Balatro which is designed and created by LocalThunk and published by Playstack.
See repository contributors list for code contribution credits to this GBA implementation.
## **Music**
Music arrangement is made by @cellos51 based on original Balatro soundtrack by [LouisF](https://louisfmusic.com/) and [transcription by MrCrimit](https://musescore.com/user/8237321/scores/14590660).
## **Imagery**
Sprites and backgrounds are based on original Balatro imagery created by LocalThunk.
See [Joker Art Discussion](https://github.com/cellos51/balatro-gba/discussions/69) for full credits for each joker sprite.
## **Sounds**
For the mult and xmult sound effects: [Toy records#06-E3-02.wav by poissonmort](https://freesound.org/s/253249/) used under License: Attribution 4.0

All other sound effects were created by LocalThunk and are used under Creative Commons - CC0 license. 
