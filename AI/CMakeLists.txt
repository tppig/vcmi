#######################################
#        Add subdirectories           #
#######################################

# Combat AI's
if(ENABLE_STUPID_AI)
	add_subdirectory(StupidAI)
endif()

if(ENABLE_MYRULEBASED_AI)
	add_subdirectory(MyRuleBasedAI)
endif()

if(ENABLE_ARMAGEDDON_AI)
	add_subdirectory(ArmageddonAI)
endif()

if(ENABLE_BATTLE_AI)
	add_subdirectory(BattleAI)
endif()

if(ENABLE_MMAI)
	add_subdirectory(MMAI)
endif()

# Adventure AI's
add_subdirectory(EmptyAI)

if(ENABLE_NULLKILLER2_AI)
	add_subdirectory(Nullkiller2)
endif()
