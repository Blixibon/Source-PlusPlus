Ok, you maybe wondering why this is here. This is a folder with a vpk that includes more dev textures, tool textures, editor models and
sprites to help make hammer less antiquated. I combined what this branch of hammer can handle with entity sprites and models from future
editors such as L4D2 and Portal 2. This is why I included new fgds.

To install, please extract the entire sdk folder (NOT THE SDK.VPK) to your 

...steamapps\common\Source SDK Base 2013 Singleplayer

or

...steamapps\common\Source SDK Base 2013 Multiplayer

or

...steamapps\common\GAME

What you want to do is make it so the sdk folder is chilling with the hl2 and platform folders.

LAST, edit your gameinfo and add this BEFORE THE FIRST "game_lv" ENTRY!!!

			// Mount SDK content first!
			game+mod			sdk/sdk_pak.vpk

This is so that the mod(s) can read my edits before HL2's and platform. Why Valve did not seporate tool stuff from 
the actual game files, I will never know! 

If you need help, see gameinfo-example.txt



