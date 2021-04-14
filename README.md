# VITA-Homebrew-Sorter

A basic PS VITA homebrew application that sorts the application database in your LiveArea. The application sorts apps and games that are inside folders as well. This applications also allows you to backup your current "loadout" that you can switch into as you wish. A backup will be made before any changes are applied to the application database. This backup is overwritten each time you use the sort option. You can find the backup in `ux0:/data/VITAHomebrewSorter/backups/app.db`. 

<p align="center">
<img src="https://i.imgur.com/iPiawoM.png" alt="VITA Homebrew Sorter Screenshot" width="640" height="362"/>
</p>

# Disclaimer
I am not responsible for anything that happens to your device after the use of this software. I always make sure to test my software(s) thoroughly before release. If you do encounter any problems please submit an issue with a copy of your app.db (`ur0:/shell/db/app.db`).

# Features:
- Sort app list alphabetically (ascending)
- Sort app list alphabetically (descending)
- Display app list after sorting is applied using ImGui's tables API.
- Backup application database before sorting is applied.
- Custom loadouts to backup/restore. (Do note: If you install a new application after you've already backed up your loadout and then attempt to restore this loadout, the new application will not appear on LiveArea and a warning message will be displayed. You can work around this by overwriting your load out backups each time an app is installed or simple re-install the VPK. Although the new application's icon will not appear on LiveArea, its data should not be lost.)

# Credits:
- Rinnegatamante for [vitaGL](https://github.com/Rinnegatamante/vitaGL)
- Rinnegatamante for [imgui-vita](https://github.com/Rinnegatamante/imgui-vita) (Based on imgui-vita with touch code removed + controller changes and official font usage)
- ocornut and contributors for [upstream imgui](https://github.com/ocornut/imgui)
- [vitasdk](https://github.com/vitasdk)
- VitaSmith for [libsqlite](https://github.com/VitaSmith/libsqlite)
- PreetiSketch for the LiveArea assets
