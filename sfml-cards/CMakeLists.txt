cmake_minimum_required(VERSION 3.16)
project(sfml_cards)

set(CMAKE_CXX_STANDARD 17)

# 本地SFML库路径
set(SFML_DIR "${CMAKE_SOURCE_DIR}/SFML-3.0.2")

# 创建可执行文件 (编译src下所有cpp)
file(GLOB SOURCES "src/*.cpp")
add_executable(main ${SOURCES})

# 手动指定SFML的包含和库目录
target_include_directories(main PRIVATE "${SFML_DIR}/include")
target_link_directories(main PRIVATE "${SFML_DIR}/lib")

# 静态链接所有运行时库
target_link_options(main PRIVATE -static)

# 使用SFML静态库 (完全静态链接，不需要DLL)
add_compile_definitions(SFML_STATIC)
target_link_libraries(main PRIVATE
    sfml-graphics-s sfml-window-s sfml-system-s sfml-audio-s sfml-network-s
    flac freetype ogg vorbis vorbisenc vorbisfile
    opengl32 winmm gdi32 ws2_32
)

# 复制图片和字体资源到构建目录 (使双击exe也能正常运行)
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/images" "$<TARGET_FILE_DIR:main>/images"
)
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:main>/resources"
)
