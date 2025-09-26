<h1 align="center">C-Tale: Meneghetti VS Python</h1>
<p align="center">
  <img alt="Static Badge" src="https://img.shields.io/badge/COMPLETE-2ecc71?style=for-the-badge">
  <img alt="GitHub License" src="https://img.shields.io/github/license/danilocb21/projeto-rpg?style=for-the-badge&logo=github&color=purple">
  <img alt="Static Badge" src="https://img.shields.io/badge/Language-blue?style=for-the-badge&logo=c">
</p>
<p align="center">
🐍 A 2D RPG based off Undertale, made with <a href="https://wiki.libsdl.org/SDL2">SDL2</a>. A short humoristic rip-off for an university project. 🐍
</p>

## 👾 Table of Contents
- [Project Status](#project-status)
- [Roadmap](#roadmap)
- [Screenshots](#screenshots)
- [Setup](#setup)
- [Usage](#usage)
- [Authors](#authors)
- [Acknowledgements](#acknowledgements)
- [License](#license)

## 🔋 Project Status
Project is: _complete_.

## 🗺️ Roadmap
- [x] 🎨 Sprites
- [x] 🏃 Character movement
- [x] 💬 NPC interaction
- [x] ⚔️ Battle system
- [x] 🎒 Items
- [x] 🚩 Ending sequence
## 💡 Future Goals
- [ ] 🌍 Multiple scenarios
- [ ] ⚔️ Different battles
- [ ] 🚂 Different game modes
- [ ] 🪙 Different endings
- [ ] 👾 Debug mode

## 📸 Screenshots
![PLACEHOLDER]

## 💾 Setup
- __Windows:__ For this program to work, you will need a C/C++ compiler. We recommend [_MinGW_](https://sourceforge.net/projects/mingw/), which is the one we used. Keep your eyes on the said architeture, 64-bits version is needed.
Install the compiler on the official page, run the executable, download via MingGW Installation Manager the C compatibility, and don't forget to add MinGW directory to PATH in Windows system variables.<br> Then, you need to install and open [_MSYS2_](https://www.msys2.org/) (64-bits, alright?) terminal. Once your MSYS2 is installed, just search "MSYS2 MSYS" up in your search bar. You'll need the one with that exact name (the purple one).<br> Now, just execute the following commands in there:

__Commands:__

```
pacman -Syu
```

If you get some error here, try:

```
pacman -Syuu
```

Now, for the dependencies (if the terminal stops asking for a input, just hit Return):

```
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-pkg-config
```

Got any problems with package conflict in the last step? Try this line:

```
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake
```

If everything's good so far, here goes the last step:

```
pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_image mingw-w64-ucrt-x86_64-SDL2_ttf mingw-w64-ucrt-x86_64-SDL2_mixer
```

<hr>

- __Linux:__ First, install/update the SDL2 library on the console. Then, just compile the code with the makefile command, execute the .exe file, and have fun. (May be redundant, but you should also have GCC and Make support installed).

__Commands:__

> sudo apt update && sudo apt install -y build-essential cmake pkg-config libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev

If you don't have GCC:

> sudo apt update

> sudo apt install gcc

<hr>

- __Extras:__

__Fedora, Oracle, Rocky, etc.:__

> sudo dnf install SDL2-devel gcc make

__Arch:__

> sudo pacman -S sdl2 gcc make

## 🎮 Usage
WASD to walk, Spacebar to interact.

Walk up to the NPC, press Space to interact, defeat it in battle mode, reach the final object.

## 🖋️ Authors
[@danilocb21](https://github.com/danilocb21): main programmer.

[@williamdants62](https://github.com/williamdants62): artist, programmer.

[@RicardinhoGplays](https://github.com/RicardinhoGplays): creative director, artist.

## ©️ Acknowledgements
The project was highly inspired on famous media.
- [Undertale](https://undertale.com/) (2015), by Toby Fox.

## 📙 License
This project is open source and available under the [MIT License](./LICENSE).
