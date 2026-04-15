Cinderalla::
# 2 "data/maps/PalletTown_Frlg_2/scripts.pory"
	lock
# 3 "data/maps/PalletTown_Frlg_2/scripts.pory"
	faceplayer
# 4 "data/maps/PalletTown_Frlg_2/scripts.pory"
	goto_if_unset FLAG_PALLET_TOWN_CINDERELLA_GIFTED_ENTEI, Cinderalla_2
# 11 "data/maps/PalletTown_Frlg_2/scripts.pory"
	msgbox Cinderalla_Text_1
Cinderalla_1:
# 13 "data/maps/PalletTown_Frlg_2/scripts.pory"
	release
	end

Cinderalla_2:
# 5 "data/maps/PalletTown_Frlg_2/scripts.pory"
	msgbox Cinderalla_Text_0
# 7 "data/maps/PalletTown_Frlg_2/scripts.pory"
	givemon SPECIES_ENTEI, 5
# 8 "data/maps/PalletTown_Frlg_2/scripts.pory"
	setflag FLAG_PALLET_TOWN_CINDERELLA_GIFTED_ENTEI
	goto Cinderalla_1


Cinderalla_Text_0:
# 5 "data/maps/PalletTown_Frlg_2/scripts.pory"
	.string "Hi, I'm Cinderella! I have a gift for you!\n"
	.string "Please accept my Entei!$"

Cinderalla_Text_1:
# 11 "data/maps/PalletTown_Frlg_2/scripts.pory"
	.string "Good luck with your Entei!$"
