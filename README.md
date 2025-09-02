<h1 align="center">C-Tale: Meneghetti VS Python</h1>
<p align="center">
  <img alt="Static Badge" src="https://img.shields.io/badge/In%20Progress-yellow?style=for-the-badge">
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
Project is: _in progress_.

## 🗺️ Roadmap
- [ ] 🎨 Sprites
- [ ] 🏃 Character movement
- [ ] 💬 NPC interaction
- [ ] ⚔️ Battle system
- [ ] 🎒 Items
- [ ] 🚩 Ending sequence

## 📸 Screenshots
![PLACEHOLDER](./img/placeholder.png)

## 💾 Setup
- __Windows:__ For this program to work, you will need a C/C++ compiler. We recommend [_MinGW_](https://sourceforge.net/projects/mingw/), which is the one we used. Keep your eyes on the said architeture, 32-bits version is needed.
Install the compiler on the official page, run the executable, and download via MingGW Installation Manager the C compatibility.
Then, you just need to open CMD/PowerShell, and type the makefile command on the downloaded repository root file.

__Commands:__

> mingw32-make run

Or (if you changed the mingw32-make.exe to make.exe, which makes it easier to type):

> make run

<hr>

- __Linux:__ First, install/update the SDL2 library on the console. Then, just compile the code with the makefile command, execute the .exe file, and have fun. (May be redundant, but you should also have GCC and Make support installed).

__Commands:__

> sudo apt update && sudo apt install -y libsdl2-dev libsdl2-image-dev pkg-config

If you don't have GCC and Make:

> sudo apt update

> sudo apt install gcc

> sudo apt install make

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

[@RicardinhoGplays](https://github.com/RicardinhoGplays): artist, programming assistant.

## ©️ Acknowledgements
The project was highly inspired on famous media.
- [Undertale](https://undertale.com/) (2015), by Toby Fox.

## 📙 License
This project is open source and available under the [MIT License](./LICENSE).
