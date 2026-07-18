cmake_minimum_required(VERSION 3.5)
project(seajeweled_project)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11 -O3")
endif()

file(GLOB SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

if(EMSCRIPTEN)
    # No wasm build of jsoncpp is available via pkg-config, so it's vendored
    # under third_party/ and compiled alongside the game.
    file(GLOB JSONCPP_SOURCE_FILES ${PROJECT_SOURCE_DIR}/third_party/jsoncpp/src/*.cpp)
    list(APPEND SOURCE_FILES ${JSONCPP_SOURCE_FILES})
endif()

if(WIN32)
	add_executable (seajeweled WIN32
		${SOURCE_FILES}
		${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/appicon.rc
	)
else()
	add_executable (seajeweled
		${SOURCE_FILES}
	)
endif()

set(BUILD_STATIC_DEFAULT OFF)
if(WIN32)
    set(BUILD_STATIC_DEFAULT ON)
endif()
option(BUILD_STATIC "Build a static binary" ${BUILD_STATIC_DEFAULT})

include_directories(${PROJECT_SOURCE_DIR}/include)

if(EMSCRIPTEN)
    # SDL2/_image/_mixer/_ttf ship as built-in Emscripten ports, enabled via
    # -s flags applied to both compile and link steps.
    set(EMSCRIPTEN_SDL_FLAGS
        "-sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sUSE_SDL_MIXER=2 -sUSE_SDL_TTF=2 -sSDL2_IMAGE_FORMATS=[\"png\"] -sSDL2_MIXER_FORMATS=[\"ogg\"]"
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_SDL_FLAGS}")

    target_include_directories(seajeweled PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/jsoncpp/include
    )

    set_target_properties(seajeweled PROPERTIES SUFFIX ".html")

    separate_arguments(EMSCRIPTEN_SDL_LINK_FLAGS UNIX_COMMAND "${EMSCRIPTEN_SDL_FLAGS}")
    target_link_options(seajeweled PRIVATE
        ${EMSCRIPTEN_SDL_LINK_FLAGS}
        # Fixed-size memory: growable wasm memory backs onto a resizable
        # ArrayBuffer, which Chrome's TextDecoder currently can't decode from.
        -sINITIAL_MEMORY=134217728
        --preload-file ${PROJECT_SOURCE_DIR}/media@media
        # MiabSDK::RequestHighscore calls back from JS into a C++ export (miab_sdk_on_highscore_data)
        # via Module.ccall, which isn't attached to Module by default.
        -sEXPORTED_RUNTIME_METHODS=["ccall"]
    )
else()
    # Find libraries
    include(FindPkgConfig)
    pkg_search_module(SDL2 REQUIRED sdl2)
    pkg_search_module(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_search_module(SDL2_MIXER REQUIRED SDL2_mixer)
    pkg_search_module(SDL2_TTF REQUIRED SDL2_ttf)
    pkg_search_module(JSONCPP REQUIRED jsoncpp)

    # Link libraries
    if(BUILD_STATIC)
        set(CMAKE_CXX_FLAGS "-static ${CMAKE_CXX_FLAGS}")
        target_link_libraries(seajeweled PRIVATE
            ${SDL2_STATIC_LIBRARIES}
            ${SDL2_IMAGE_STATIC_LIBRARIES}
            ${SDL2_MIXER_STATIC_LIBRARIES}
            ${SDL2_TTF_STATIC_LIBRARIES}
            ${JSONCPP_STATIC_LIBRARIES}
        )
    else()
        target_link_libraries(seajeweled PRIVATE
            ${SDL2_LIBRARIES}
            ${SDL2_IMAGE_LIBRARIES}
            ${SDL2_MIXER_LIBRARIES}
            ${SDL2_TTF_LIBRARIES}
            ${JSONCPP_LIBRARIES}
        )
    endif()

    include_directories(
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${JSONCPP_INCLUDE_DIRS}
    )
endif()

IF(WIN32)
	# Nothing to do
ELSEIF(EMSCRIPTEN)
	# Nothing to do: assets are bundled via --preload-file above, and the
	# build output (seajeweled.html/.js/.wasm/.data) is ready to serve as-is.
ELSE()
	add_custom_target(
		link_media_folder
		COMMAND ln -nfs ${PROJECT_SOURCE_DIR}/media ${PROJECT_BINARY_DIR}/media
	)

	add_dependencies(
		seajeweled
		link_media_folder
	)

	install(TARGETS seajeweled DESTINATION bin)
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/media DESTINATION share/seajeweled)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/platform/unix/seajeweled.desktop DESTINATION share/applications)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/platform/unix/icons/64x64/seajeweled.png DESTINATION share/icons/hicolor/64x64/apps)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md DESTINATION share/doc/seajeweled)
ENDIF()
