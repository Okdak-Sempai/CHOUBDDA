cmake_minimum_required(VERSION 3.16)
project(SHINBDDA LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_library(DBConfig STATIC DBConfig.cpp DBConfig.h)

add_library(LowLevelDatabase STATIC
        Tools_L.c
        Tools_L.h
        PageId.c
        PageId.h
        DiskManager.c
        DiskManager.h
        TestsProcedures.c
        TestsProcedures.h
        Structures.h
        BufferManager.c
        BufferManager.h
        Relation.c
        Relation.h
        Record.c
        Record.h
        HeapFile.c
        HeapFile.h
)
target_link_libraries(LowLevelDatabase PUBLIC DBConfig)

add_library(DatabaseManagement STATIC
        DBManager.cpp
        DBManager.h
        SGBD.cpp
        SGBD.h
        SelectCommand.cpp
        SelectCommand.h
)
target_link_libraries(DatabaseManagement PUBLIC DBConfig PUBLIC LowLevelDatabase)

add_executable(SHINBDDA main.c)
target_link_libraries(SHINBDDA PRIVATE DBConfig PRIVATE DatabaseManagement PRIVATE LowLevelDatabase)

add_executable(SGDB main.cpp)
target_link_libraries(SGDB PRIVATE DBConfig PRIVATE DatabaseManagement PRIVATE LowLevelDatabase)