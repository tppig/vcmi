set(armageddonAI_SRCS
		ArmageddonAI.cpp
)

set(armageddonAI_HEADERS
		StdInc.h

		ArmageddonAI.h
)

assign_source_group(${armageddonAI_SRCS} ${armageddonAI_HEADERS})

add_library(ArmageddonAI OBJECT ${armageddonAI_SRCS} ${armageddonAI_HEADERS})
target_compile_definitions(ArmageddonAI PRIVATE VCMI_DLL=1)

target_link_libraries(ArmageddonAI PRIVATE vcmiMain)
target_include_directories(ArmageddonAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

vcmi_set_output_dir(ArmageddonAI "AI")
enable_pch(ArmageddonAI)
