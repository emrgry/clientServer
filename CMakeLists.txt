cmake_minimum_required(VERSION 3.10)

project(SocketProgrammingC)

set(CMAKE_C_STANDARD 11)

# Server executable
add_executable(server
    src/server.c
)

# Client executable
#add_executable(client
#    src/client.c
#)

# Include directories
target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

#target_include_directories(client PRIVATE
#    ${CMAKE_CURRENT_SOURCE_DIR}/include
#)

