# Unknown Or Maniacs Patch Commands

This file prioritizes commands that are not normal RPG Maker 2000/2003 commands based on liblcf naming. No observed command_id was unknown in the liblcf enum snapshot used here.

| command_id | count | name | files | first_location | notes |
|---:|---:|---|---|---|---|
| 3007 | 21 | `Maniac_ShowStringPicture` | Map0001.emu;Map0002.emu;Map0007.emu;RPG_RT.edb | Map0001.emu:map=1:event=0001:page=0001:cmd=9 | liblcf name: Maniac_ShowStringPicture; EasyRPG Player local source has an ExecuteCommand case for this name; prioritized: liblcf classifies this as Maniacs Patch command |
| 3008 | 6 | `Maniac_GetPictureInfo` | RPG_RT.edb | RPG_RT.edb:map=database:event=common_event:3:page=-:cmd=1 | liblcf name: Maniac_GetPictureInfo; EasyRPG Player local source has an ExecuteCommand case for this name; prioritized: liblcf classifies this as Maniacs Patch command |
| 3015 | 11 | `Maniac_RewriteMap` | RPG_RT.edb | RPG_RT.edb:map=database:event=common_event:4:page=-:cmd=14 | liblcf name: Maniac_RewriteMap; EasyRPG Player local source has an ExecuteCommand case for this name; prioritized: liblcf classifies this as Maniacs Patch command |
| 3018 | 1 | `Maniac_SetGameOption` | Map0007.emu | Map0007.emu:map=7:event=0001:page=0001:cmd=1 | liblcf name: Maniac_SetGameOption; EasyRPG Player local source has an ExecuteCommand case for this name; prioritized: liblcf classifies this as Maniacs Patch command |
| 3020 | 10 | `Maniac_ControlStrings` | RPG_RT.edb | RPG_RT.edb:map=database:event=common_event:11:page=-:cmd=76 | liblcf name: Maniac_ControlStrings; EasyRPG Player local source has an ExecuteCommand case for this name; prioritized: liblcf classifies this as Maniacs Patch command |
| 3021 | 9 | `Maniac_GetGameInfo` | Map0002.emu;Map0007.emu;RPG_RT.edb | Map0002.emu:map=2:event=0001:page=0001:cmd=3 | liblcf name: Maniac_GetGameInfo; EasyRPG Player local source has an ExecuteCommand case for this name; prioritized: liblcf classifies this as Maniacs Patch command |
| 3025 | 1 | `Maniac_EditPicture` | RPG_RT.edb | RPG_RT.edb:map=database:event=common_event:20:page=-:cmd=10 | liblcf name: Maniac_EditPicture; no local ExecuteCommand case found by text search; prioritized: liblcf classifies this as Maniacs Patch command |
| 3026 | 1 | `Maniac_WritePicture` | RPG_RT.edb | RPG_RT.edb:map=database:event=common_event:20:page=-:cmd=25 | liblcf name: Maniac_WritePicture; EasyRPG Player local source has an ExecuteCommand case for this name; prioritized: liblcf classifies this as Maniacs Patch command |
| 3032 | 3 | `Maniac_Zoom` | Map0001.emu;RPG_RT.edb | Map0001.emu:map=1:event=0001:page=0001:cmd=5 | liblcf name: Maniac_Zoom; no local ExecuteCommand case found by text search; prioritized: liblcf classifies this as Maniacs Patch command |

## First Context Samples

### 3007 `Maniac_ShowStringPicture`

- First location: `Map0001.emu:map=1:event=0001:page=0001:cmd=9`
- Context: prev: 10220 ControlVars; params=1 73 74 0 0 0 / current: 3007 Maniac_ShowStringPicture; params=16 2 1 2 100 0 100 100 100 100 0 0 0 126976 65537 9 0 1 0 0 10; string=× \v[75]ms gothic / next: 10220 ControlVars; params=0 1 0 2 0 32

### 3008 `Maniac_GetPictureInfo`

- First location: `RPG_RT.edb:map=database:event=common_event:3:page=-:cmd=1`
- Context: prev: START / current: 3008 Maniac_GetPictureInfo; params=0 0 0 1 1 2 3 3 / next: 10220 ControlVars; params=0 1 0 0 21 3 16844848 17307698 1048937

### 3015 `Maniac_RewriteMap`

- First location: `RPG_RT.edb:map=database:event=common_event:4:page=-:cmd=14`
- Context: prev: 12010 ConditionalBranch; params=1 3 0 10 0 1 / current: 3015 Maniac_RewriteMap; params=272 0 0 0 11 4 1 1 0 / next: 3015 Maniac_RewriteMap; params=272 0 1 0 11 4 1 1 0

### 3018 `Maniac_SetGameOption`

- First location: `Map0007.emu:map=7:event=0001:page=0001:cmd=1`
- Context: prev: START / current: 3018 Maniac_SetGameOption; params=0 1 60 0 / next: 11330 MoveEvent; params=10001 8 1 0 23

### 3020 `Maniac_ControlStrings`

- First location: `RPG_RT.edb:map=database:event=common_event:11:page=-:cmd=76`
- Context: prev: 10220 ControlVars; params=0 3 0 0 0 0 / current: 3020 Maniac_ControlStrings; params=0 1 0 0 15 0 0; string=Let's move on ! / next: 3020 Maniac_ControlStrings; params=0 2 0 0 17 0 0; string=illust : lunetuki

### 3021 `Maniac_GetGameInfo`

- First location: `Map0002.emu:map=2:event=0001:page=0001:cmd=3`
- Context: prev: 11550 PlaySound; params=100 150 50; string=Start / current: 3021 Maniac_GetGameInfo; params=0 2 70 / next: 10220 ControlVars; params=0 1 0 0 19 4 70 2 1

### 3025 `Maniac_EditPicture`

- First location: `RPG_RT.edb:map=database:event=common_event:20:page=-:cmd=10`
- Context: prev: 3021 Maniac_GetGameInfo; params=4369 3 0 1 2 3 4 99999 / current: 3025 Maniac_EditPicture; params=0 999 0 0 320 240 99999 / next: 12010 ConditionalBranch; params=1 89 0 5 0 1

### 3026 `Maniac_WritePicture`

- First location: `RPG_RT.edb:map=database:event=common_event:20:page=-:cmd=25`
- Context: prev: 22011 EndBranch / current: 3026 Maniac_WritePicture; params=16 1 999 1 / next: 10220 ControlVars; params=0 1 0 0 19 4 70 2 1

### 3032 `Maniac_Zoom`

- First location: `Map0001.emu:map=1:event=0001:page=0001:cmd=5`
- Context: prev: 11110 ShowPicture; params=1 1 1 2 0 50 80 1 100 100 100 100 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 0; string=Logo / current: 3032 Maniac_Zoom; params=17 1 2 100 0 7 0 / next: 12010 ConditionalBranch; params=1 74 0 0 1 1
