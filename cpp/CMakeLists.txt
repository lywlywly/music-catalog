cmake_minimum_required(VERSION 3.16)
project(music-catalog CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(TagLib CONFIG REQUIRED)
find_package(ICU REQUIRED COMPONENTS i18n uc data)

add_executable(music-catalog
    main.cpp
    song.cpp
    playlist.cpp
    yaml_io.cpp
)

target_include_directories(music-catalog PRIVATE ${ICU_INCLUDE_DIRS})

target_link_libraries(music-catalog PRIVATE
    yaml-cpp::yaml-cpp
    nlohmann_json::nlohmann_json
    TagLib::tag
    ${ICU_LIBRARIES}
)
