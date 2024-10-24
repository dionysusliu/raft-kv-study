cmake_minimum_required(VERSION 3.5)
project(raft-kv)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)

add_definitions(-Wall)
add_definitions(-DDEBUG)

include_directories(${CMAKE_SOURCE_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(dependencies
                  glib-2.0>=2.10 REQUIRED
                  msgpack REQUIRED
                  hiredis REQUIRED)
include_directories(${dependencies_INCLUDE_DIRS})
set(LIBS
    ${dependencies_LIBRARIES})

find_package(Boost REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS
    ${LIBS}
    pthread
    Boost::system
    Boost::filesystem
    )

add_subdirectory(raft-kv)

pkg_check_modules(GoogleTest gtest_main)
if (GoogleTest_FOUND)
    include_directories(${GoogleTest_INCLUDE_DIRS})
    set(LIBS
        ${LIBS}
        ${GoogleTest_LIBRARIES})
    add_subdirectory(tests)
    enable_testing()
endif (GoogleTest_FOUND)

configure_file(Procfile.in Procfile)

# rocksdb
pkg_check_modules(RocksDB rocksdb)
if (RocksDB_FOUND)
    include_directories(${RocksDB_INCLUDE_DIRS})
    # compression lib
    find_library(LIB_ZSTD zstd REQUIRED)
    find_library(LIB_LZ4 lz4 REQUIRED)
    find_library(LIB_BZIP2 bz2 REQUIRED)
    find_library(LIB_ZLIB z REQUIRED)
    find_library(LIB_SNAPPY snappy REQUIRED)
    set(LIBS
            ${LIBS}
            ${RocksDB_LIBRARIES}
            ${LIB_ZSTD} ${LIB_LZ4} ${LIB_BZIP2} ${LIB_ZLIB} ${LIB_SNAPPY}
    )
endif(RocksDB_FOUND)
