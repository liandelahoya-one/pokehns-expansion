GoldenrodCity_DepartmentStore_5F_hns_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, GoldenrodDeptElevator_OnFrame
	.byte 0


GoldenrodDeptStore5_EventScript_Lass::
	lock
	faceplayer
	msgbox GoldenrodDeptStore_Text_5FLass, MSGBOX_DEFAULT
	closemessage
	release
	end

GoldenrodDeptStore5_EventScript_PokefanM::
	lock
	faceplayer
	msgbox GoldenrodDeptStore_Text_5FPokefanM, MSGBOX_DEFAULT
	closemessage
	release
	end

GoldenrodDeptStore5_EventScript_Girl::
	lock
	faceplayer
	msgbox GoldenrodDeptStore_Text_Girl, MSGBOX_DEFAULT
	closemessage
	release
	end

GoldenrodDeptStore5_EventScript_Sign::
	lock
	faceplayer
	msgbox GoldenrodDeptStore_Text_5FDirectory, MSGBOX_DEFAULT
	closemessage
	release
	end

GoldenrodDeptStore5_EventScript_Receptionist::
	lock
	faceplayer
	goto_if_set FLAG_GOT_RETURN, GoldenrodDeptStore5_EventScript_Receptionist_Already
	msgbox GoldenrodDeptStore_Text_5FReceptionistOhYourMon, MSGBOX_DEFAULT
	closemessage
	specialvar VAR_RESULT, GetLeadMonFriendshipScore
	goto_if_ge VAR_RESULT, FRIENDSHIP_150_TO_199, GoldenrodDeptStore5_EventScript_Receptionist_GiveReturn
	msgbox GoldenrodDeptStore_Text_5FReceptionistPutInSomeMoreEffort, MSGBOX_DEFAULT
	closemessage
	release
	end

GoldenrodDeptStore5_EventScript_Receptionist_GiveReturn::
	giveitem ITEM_TM_RETURN
	setflag FLAG_GOT_RETURN
	msgbox GoldenrodDeptStore_Text_5FReceptionistThisMoveShouldBePerfect, MSGBOX_DEFAULT
	closemessage
	release
	end

GoldenrodDeptStore5_EventScript_Receptionist_Already::
	msgbox GoldenrodDeptStore_Text_5FReceptionistWait, MSGBOX_DEFAULT
	closemessage
	release
	end

GoldenrodDeptStore_Text_5FReceptionistOhYourMon:
    .string "Hello. Oh, your POKéMON…$"

GoldenrodDeptStore_Text_5FReceptionistWait:
    .string "Wait, I already gave you that TM!$"

GoldenrodDeptStore_Text_5FReceptionistPutInSomeMoreEffort:
	.string "Hmm…\n"
	.string "It's not bad, but it's also not good.\p"
	.string "You, as the TRAINER, need to put in\n"
	.string "some more effort.$"

GoldenrodDeptStore_Text_5FReceptionistThisMoveShouldBePerfect:
    .string "It's very attached to you.\p"
    .string "This move should be perfect for a\n"
    .string "pair like you.$"

GoldenrodDeptStore_Text_5FLass:
    .string "During the day, a lady comes to check\n"
    .string "out POKéMON.\p"
    .string "She even gives away TMs!$"

GoldenrodDeptStore_Text_5FPokefanM:
    .string "You can't rename a POKéMON you get\n"
    .string "in a trade.\p"
    .string "The name is a reflection of the\n"
    .string "original trainer's feelings for it.$"

GoldenrodDeptStore_Text_Girl:
	.string "The lady gave me RETURN!\n"
    .string "My POKéMON must really like me!$"

GoldenrodDeptStore5_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart GoldenrodDeptStore5_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
GoldenrodDeptStore5_Pokemart:
	.2byte ITEM_TM_DRAGON_CLAW
	.2byte ITEM_TM_DRAGON_PULSE
	.2byte ITEM_TM_DRAGON_DANCE
	.2byte ITEM_TM_DRACO_METEOR
	.2byte ITEM_TM_CALM_MIND
	.2byte ITEM_TM_BULK_UP
	.2byte ITEM_TM_SWORDS_DANCE
	.2byte ITEM_TM_STEALTH_ROCK
	.2byte ITEM_TM_ROOST
	.2byte ITEM_TM_SUBSTITUTE
	.2byte ITEM_TM_EARTHQUAKE
	.2byte ITEM_TM_ICE_BEAM
	.2byte ITEM_TM_BLIZZARD
	.2byte ITEM_TM_THUNDERBOLT
	.2byte ITEM_TM_THUNDER
	.2byte ITEM_TM_FLAMETHROWER
	.2byte ITEM_TM_FIRE_BLAST
	.2byte ITEM_TM_PSYCHIC
	.2byte ITEM_TM_SHADOW_BALL
	.2byte ITEM_TM_DARK_PULSE
	.2byte ITEM_TM_FLASH_CANNON
	.2byte ITEM_TM_ENERGY_BALL
	.2byte ITEM_TM_FOCUS_BLAST
	.2byte ITEM_TM_DRAIN_PUNCH
	.2byte ITEM_TM_STONE_EDGE
	.2byte ITEM_TM_ROCK_SLIDE
	.2byte ITEM_TM_WILL_O_WISP
	.2byte ITEM_TM_THUNDER_WAVE
	.2byte ITEM_TM_U_TURN
	.2byte ITEM_TM_TRICK_ROOM
	.2byte ITEM_TM_PROTECT
	.2byte ITEM_TM_LIGHT_SCREEN
	.2byte ITEM_TM_REFLECT
	.2byte ITEM_TM_HYPER_BEAM
	pokemartlistend

GoldenrodDeptStore_Text_5FDirectory:
    .string "Customize Your POKéMON\p"
    .string "5F TM CORNER$"
