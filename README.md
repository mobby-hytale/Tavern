![Forge Logo](banner.png)

Tavern
=============
![Discord](https://img.shields.io/discord/1480915716226945066.svg?color=%237289da&label=Discord&logo=discord&logoColor=%237289da) [![Support](https://img.shields.io/badge/Ko--fi-F16061?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/mobby_hytale)

Tavern is a Minecraft Dungeons Mod Loader based on [UE4SS-RE](https://github.com/UE4SS-RE/RE-UE4SS) and modified to be compatible with the game.

Tavern support lua, c++, and BluePrint (.pak) mods!

# How to install
- Download the .zip on the Realeases section of Tavern or [UE4SS-RE](https://github.com/UE4SS-RE/RE-UE4SS)
- Extract the archive on `/Dungeons/Binaries/Win64`

##### OR

- Compile [UE4SS-RE](https://github.com/UE4SS-RE/RE-UE4SS) yourself
- Place it on `/Dungeons/Binaries/Win64`
- Place on the files `VTableLayout.ini` and `MemberVariableLayout.ini` next to the `UE4SS.dll` file

# How to install Mods
- Place your mod on the `/Dungeons/Binaries/Win64/ue4ss/Mods/` folder
- Modify the `mods.txt` file by adding your mod (for exemple if you have a mod called "TavernAPI" you must write "TavernAPI : 1" in the `mods.txt` file
- You can disable a mod by replacing the `1` by a `0` in the `mods.txt` file

# Thanks to
- phoenixbulliner for the logo
- UE4SS maintainers for the help
