include(ExternalProject)

ExternalProject_Add(ParaView
	GIT_REPOSITORY https://gitlab.kitware.com/paraview/paraview.git
	GIT_TAG v5.2.0
    BINARY_DIR ${CMAKE_BINARY_DIR}/ParaView-build
    INSTALL_COMMAND ""
	CMAKE_ARGS 
		-Wno-dev
		-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
		-DBUILD_EXAMPLES:BOOL=OFF
		-DBUILD_TESTING:BOOL=OFF
)

ExternalPRoject_Add(BioGears
	GIT_REPOSITORY https://gitlab.kitware.com/biogears/biogears.git
	BINARY_DIR ${CMAKE_BINARY_DIR}/BioGears-build
	CMAKE_ARGS
		-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
)

ExternalProject_Add(BioGearsDemo
	DEPENDS ParaView
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
	BINARY_DIR ${CMAKE_BINARY_DIR}/BioGearsDemo-build
	INSTALL_COMMAND ""
	CMAKE_ARGS
		-DDO_SUPERBUILD:BOOL=OFF
		-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
		-DParaView_DIR:PATH=${CMAKE_BINARY_DIR}/ParaView-build
)