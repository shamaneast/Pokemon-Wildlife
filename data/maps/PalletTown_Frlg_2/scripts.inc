PalletTown_Frlg_2_MapScripts::
	.byte 0


Cinderella::
# 5 "data/maps/PalletTown_Frlg_2/scripts.pory"
	lock
# 6 "data/maps/PalletTown_Frlg_2/scripts.pory"
	faceplayer
# 7 "data/maps/PalletTown_Frlg_2/scripts.pory"
	goto_if_unset FLAG_VISITED_OAKS_LAB, Cinderella_2
# 14 "data/maps/PalletTown_Frlg_2/scripts.pory"
	goto_if_unset FLAG_PALLET_TOWN_CINDERELLA_GIFTED_ENTEI, Cinderella_5
# 23 "data/maps/PalletTown_Frlg_2/scripts.pory"
	msgbox Cinderella_Text_2
Cinderella_1:
# 26 "data/maps/PalletTown_Frlg_2/scripts.pory"
	release
	end

Cinderella_2:
# 8 "data/maps/PalletTown_Frlg_2/scripts.pory"
	msgbox Cinderella_Text_0
	goto Cinderella_1

Cinderella_5:
# 15 "data/maps/PalletTown_Frlg_2/scripts.pory"
	msgbox Cinderella_Text_1
# 19 "data/maps/PalletTown_Frlg_2/scripts.pory"
	givemon SPECIES_ENTEI, 5
# 20 "data/maps/PalletTown_Frlg_2/scripts.pory"
	setflag FLAG_PALLET_TOWN_CINDERELLA_GIFTED_ENTEI
	goto Cinderella_1


Cinderella_Text_0:
# 8 "data/maps/PalletTown_Frlg_2/scripts.pory"
	.string "Hi, I'm Cinderella!\n"
	.string "I'm going to give you something overpowered.\p"
	.string "Come talk to me when you have a Pokedex!$"

Cinderella_Text_1:
# 15 "data/maps/PalletTown_Frlg_2/scripts.pory"
	.string "Hi, I'm Cinderella! \n"
	.string "I have a gift for you!\p"
	.string "Please accept my Entei!$"

Cinderella_Text_2:
# 23 "data/maps/PalletTown_Frlg_2/scripts.pory"
	.string "Good luck with your Entei!$"
