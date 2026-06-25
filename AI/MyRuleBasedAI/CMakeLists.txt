set(myRuleBasedAI_SRCS
		MyRuleBasedAI.cpp
)

set(myRuleBasedAI_HEADERS
		StdInc.h

		MyRuleBasedAI.h
)

assign_source_group(${myRuleBasedAI_SRCS} ${myRuleBasedAI_HEADERS})

add_library(MyRuleBasedAI OBJECT ${myRuleBasedAI_SRCS} ${myRuleBasedAI_HEADERS})
target_compile_definitions(MyRuleBasedAI PRIVATE VCMI_DLL=1)

target_link_libraries(MyRuleBasedAI PRIVATE vcmiMain)
target_include_directories(MyRuleBasedAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

vcmi_set_output_dir(MyRuleBasedAI "AI")
enable_pch(MyRuleBasedAI)
