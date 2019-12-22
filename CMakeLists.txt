cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(otp_server)

### Configure CMake to use our extra Find modules ###
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")


### Define extra windows defines ###
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)

    add_definitions(-D_WIN32_WINDOWS)
    add_definitions(-D_WIN32_WINNT=0x0600)
    add_definitions(-DNOMINMAX) # min() and max() macros conflict with <limits>
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
    if(MSVC)
        add_definitions("/wd4267") # 64-bit size_t complaints
    endif()
else()
    option(STATIC_OTP "Builds a self-contained OTP daemon binary (Non-Win32 only)" OFF)
endif()

if(STATIC_OTP)
    # !!! DO NOT SET THIS UNLESS YOU KNOW WHAT YOU'RE DOING !!!
    # Static OTP builds are still *very* experimental, and there are still issues with linking the C++ Mongo driver in some instances.
    set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)
    set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
    set(CMAKE_SHARED_LIBRARY_C_FLAGS)
    set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

### Set build type to Release with Debug Info by default ###
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type.")
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    RelWithDebInfo Release Debug)

### Define otp’s git aha for use in program ###
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    find_package(Git)
    if(GIT_FOUND)
        exec_program(
            "${GIT_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            ARGS "describe --abbrev=8 --dirty --always"
            OUTPUT_VARIABLE GIT_SHA_VERSION
        )

        add_definitions(-DGIT_SHA1="${GIT_SHA_VERSION}")
    endif()
endif()


### Add per-compiler flags ###
# Common flags:
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
   CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-error=deprecated-declarations -Wno-error=unused-parameter")
endif()

# Clang-specific flags:
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-register")
endif()

### Debug Flags -- compile in spam/debug log messages ###
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DOTP_DEBUG_MESSAGES")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DOTP_DEBUG_MESSAGES")

### OS Specific Settings ###
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")  ### Windows requires winsock
    list(APPEND EXTRA_LIBS wsock32 ws2_32)
    set(CMAKE_INSTALL_PREFIX "C:\\mongo-cxx-driver")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")        ### Linux requires pthreads
    list(APPEND EXTRA_LIBS pthread rt)
endif()

### YAML dependency -- required by config, used by yaml database ###
set(YAMLCPP_USE_STATIC_LIBS ON CACHE BOOL "If true, will try to find static YamlCpp first instead of dynamic.")
find_package(YamlCpp REQUIRED)
include_directories(${YAMLCPP_INCLUDE_DIR})
link_directories(${YAMLCPP_LIBRARY_DIR})
message(STATUS "Found the following YamlCpp library:")
message("  ${YAMLCPP_LIBRARY}\n")

set(BUILD_YAML ON) # ON CACHE BOOL "Stuff"
if(BUILD_YAML)
    add_definitions(-DOTP_WITH_YAML)
endif()

# libuv dependency -- required by networking
find_package(libuv REQUIRED)
include_directories(${LIBUV_INCLUDE_DIR})
link_directories(${LIBUV_LIBRARY_DIR})
message(STATUS "Found the libuv library:")
message("   ${LIBUV_LIBRARY}\n")

# We only really need this for Boost's ICL at this point, but until it's fully deprecated:
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

set(USE_32BIT_DATAGRAMS OFF CACHE BOOL
    "If on, datagrams will use 32-bit length tags instead of 16-bit.")
if(USE_32BIT_DATAGRAMS)
    add_definitions(-DOTP_32BIT_DATAGRAMS)
endif()
set(USE_32BIT_SIZETAGS OFF CACHE BOOL
    "If on, dclass fields will use 32-bit length tags instead of 16-bit.")
if(USE_32BIT_SIZETAGS)
    add_definitions(-DDCLASS_32BIT_SIZETAG)
endif()

# DB backends can add libraries here as needed:
set(DB_LIBRARY_NAMES)

set(BUILD_DBSERVER ON CACHE BOOL "If on, Database Server will be built into binary")
if(BUILD_DBSERVER)
    add_definitions(-DBUILD_DBSERVER)
    set(DBSERVER_FILES
        src/database/DatabaseServer.h
        src/database/DatabaseServer.cpp
        src/database/DatabaseBackend.h
        src/database/DatabaseBackend.cpp
        src/database/DBOperation.h
        src/database/DBOperation.cpp
        src/database/DBOperationQueue.h
        src/database/DBOperationQueue.cpp
        src/database/OldDatabaseBackend.h
        src/database/OldDatabaseBackend.cpp
        src/database/DBBackendFactory.h
        src/database/DBBackendFactory.cpp
    )

    set(BUILD_DB_YAML ON CACHE BOOL "If on, will support a YAML-based database (for development)")
    if(BUILD_DB_YAML)
        add_definitions(-DBUILD_DB_YAML)
        set(DBSERVER_FILES
            ${DBSERVER_FILES}
            src/database/YAMLDatabase.cpp
        )
    endif()

    ### Check for the presence of the MongoDB client library ###
    find_package(libmongocxx QUIET)
    find_package(libbsoncxx QUIET)
    find_package(libmongocxx-static QUIET)
    find_package(libbsoncxx-static QUIET)

    if(LIBMONGOCXX_LIBRARIES OR LIBMONGOCXX_STATIC_LIBRARIES)
        set(BUILD_DB_MONGO ON CACHE BOOL "If on, will support MongoDB.")
        message(STATUS "MongoDB client library found")
    else()
        unset(BUILD_DB_MONGO CACHE)
        message(STATUS "MongoDB client library not found")
    endif()
    if(BUILD_DB_MONGO)
        list(APPEND DBSERVER_FILES
            src/database/MongoDatabase.cpp)
        if(LIBMONGOCXX_LIBRARIES)
            include_directories(${LIBMONGOCXX_INCLUDE_DIRS} ${LIBBSONCXX_INCLUDE_DIRS})
            link_directories(${LIBMONGOCXX_LIBRARY_DIRS} ${LIBBSONCXX_LIBRARY_DIRS})
            list(APPEND DB_LIBRARY_NAMES ${LIBMONGOCXX_LIBRARIES} ${LIBBSONCXX_LIBRARIES})
        elseif(LIBMONGOCXX_STATIC_LIBRARIES)
            include_directories(${LIBMONGOCXX_STATIC_INCLUDE_DIRS} ${LIBBSONCXX_STATIC_INCLUDE_DIRS})
            link_directories(${LIBMONGOCXX_STATIC_LIBRARY_DIRS} ${LIBBSONCXX_STATIC_LIBRARY_DIRS})
            list(APPEND DB_LIBRARY_NAMES ${LIBMONGOCXX_STATIC_LIBRARIES} ${LIBBSONCXX_STATIC_LIBRARIES})
        else()
            message(FATAL_ERROR "BUILD_DB_MONGO is set but no libraries were found, this is never supposed to happen!")
        endif()
    endif()
else()
    unset(BUILD_DB_FILESYSTEM CACHE)
    unset(BUILD_DB_YAML CACHE)
    unset(BUILD_DB_MONGO CACHE)
endif()

set(BUILD_STATESERVER ON CACHE BOOL "If on, State Server will be built into binary")
if(BUILD_STATESERVER)
    set(STATESERVER_FILES
        src/stateserver/StateServer.cpp
        src/stateserver/StateServer.h
        src/stateserver/DistributedObject.cpp
        src/stateserver/DistributedObject.h
    )

    set(BUILD_STATESERVER_DBSS ON CACHE BOOL "If on, the Database-State Server will be built into binary")
    if(BUILD_STATESERVER_DBSS)
        set(STATESERVER_FILES
            ${STATESERVER_FILES}
            src/stateserver/DBStateServer.h
            src/stateserver/DBStateServer.cpp
            src/stateserver/LoadingObject.h
            src/stateserver/LoadingObject.cpp
        )
    endif()
else()
    unset(BUILD_STATESERVER_DBSS CACHE)
endif()

set(BUILD_EVENTLOGGER ON CACHE BOOL "If on, Event Logger will be built into binary")
if(BUILD_EVENTLOGGER)
    set(EVENTLOGGER_FILES
        src/eventlogger/EventLogger.cpp
        src/eventlogger/EventLogger.h
    )
endif()

set(BUILD_CLIENTAGENT ON CACHE BOOL "If on, Client Agent will be built into binary")
set(BUILD_DISNEYCLIENT OFF CACHE BOOL "If on, DisneyClient will be built into binary as an option")
if(BUILD_CLIENTAGENT)
    if(BUILD_DISNEYCLIENT)
        set(CLIENTAGENT_FILES
            src/json/json.hpp
            src/clientagent/Client.h
            src/clientagent/Client.cpp
            src/clientagent/ClientAgent.h
            src/clientagent/ClientAgent.cpp
            src/clientagent/ClientMessages.h
            src/clientagent/ClientFactory.cpp
            src/clientagent/ClientFactory.h
            src/clientagent/OTPClient.cpp
            src/clientagent/DisneyClient.cpp
            src/clientagent/OTPClientManager.h
            src/clientagent/OTPClientManager.cpp
            src/clientagent/ToontownClientManager.h
            src/clientagent/ToontownClientManager.cpp
        )
    else()
        set(CLIENTAGENT_FILES
            src/clientagent/Client.h
            src/clientagent/Client.cpp
            src/clientagent/ClientAgent.h
            src/clientagent/ClientAgent.cpp
            src/clientagent/ClientMessages.h
            src/clientagent/ClientFactory.cpp
            src/clientagent/ClientFactory.h
            src/clientagent/OTPClient.cpp
        )
    endif()
endif()

### Define core components ###
set(CORE_FILES
    src/core/global.h
    src/core/global.cpp
    src/core/main.cpp
    src/core/Logger.h
    src/core/Logger.cpp
    src/core/Role.h
    src/core/Role.cpp
    src/core/RoleFactory.h
    src/core/RoleFactory.cpp
    src/core/shutdown.h
    src/core/shutdown.cpp
    src/core/types.h
    src/core/msgtypes.h
)

set(CONFIG_FILES
    src/config/ConfigGroup.h
    src/config/ConfigGroup.cpp
    src/config/ConfigVariable.h
    src/config/ConfigVariable.cpp
    src/config/constraints.cpp
)

set(MESSAGEDIRECTOR_FILES
    src/messagedirector/ChannelMap.cpp
    src/messagedirector/ChannelMap.h
    src/messagedirector/MessageDirector.cpp
    src/messagedirector/MessageDirector.h
    src/messagedirector/MDNetworkParticipant.cpp
    src/messagedirector/MDNetworkParticipant.h
    src/messagedirector/MDNetworkUpstream.cpp
    src/messagedirector/MDNetworkUpstream.h
)

set(UTIL_FILES
    src/util/Datagram.h
    src/util/DatagramIterator.h
    src/util/EventSender.cpp
    src/util/EventSender.h
    src/util/Timeout.cpp
    src/util/Timeout.h
    src/util/TaskQueue.cpp
    src/util/TaskQueue.h
)

set(NET_FILES
    src/net/address_utils.cpp
    src/net/address_utils.h
    src/net/HAProxyHandler.cpp
    src/net/HAProxyHandler.h
    src/net/NetworkAcceptor.cpp
    src/net/NetworkAcceptor.h
    src/net/NetworkClient.cpp
    src/net/NetworkClient.h
    src/net/NetworkConnector.cpp
    src/net/NetworkConnector.h
    src/net/TcpAcceptor.cpp
    src/net/TcpAcceptor.h
)

include_directories(src)
add_executable(otp_server
    ${CORE_FILES}
    ${CONFIG_FILES}
    ${MESSAGEDIRECTOR_FILES}
    ${UTIL_FILES}
    ${NET_FILES}
    ${STATESERVER_FILES}
    ${EVENTLOGGER_FILES}
    ${CLIENTAGENT_FILES}
    ${DBSERVER_FILES}
)

### Setup source groups and defines for configurables ###
source_group("Core" FILES ${CORE_FILES})
source_group("Config" FILES ${CONFIG_FILES})
source_group("Util" FILES ${UTIL_FILES})
source_group("MessageDirector" FILES ${MESSAGEDIRECTOR_FILES})
source_group("Net" FILES ${NET_FILES})
if(STATESERVER_FILES)
    add_definitions(-DBUILD_STATESERVER)
    source_group("StateServer" FILES ${STATESERVER_FILES})
endif()
if(EVENTLOGGER_FILES)
    add_definitions(-DBUILD_EVENTLOGGER)
    source_group("EventLogger" FILES ${EVENTLOGGER_FILES})
endif()
if(CLIENTAGENT_FILES)
    add_definitions(-DBUILD_CLIENTAGENT)
    source_group("ClientAgent" FILES ${CLIENTAGENT_FILES})
endif()
if(DBSERVER_FILES)
    add_definitions(-DBUILD_STATESERVER_DBSS)
    source_group("DatabaseServer" FILES ${DBSERVER_FILES})
endif()

### Header source group ###
#foreach(sourcefile IN LISTS otp_server_SOURCES)
#   if(sourcefile MATCHES .*\\.h)
#       set(otp_server_HEADERS "${otp_server_HEADERS} ${sourcefile}")
#   endif()
#endforeach(sourcefile)
#source_group("Header Files" FILES ${otp_server_HEADERS})

add_library(dclass
    src/dclass/p3dcparser_composite1.cpp
    src/dclass/p3dcparser_composite2.cpp
    src/dclass/dcLexer.cpp
    src/dclass/dcParser.cpp
)
add_dependencies(otp_server dclass)
target_link_libraries(otp_server dclass ${YAMLCPP_LIBRARY} ${DB_LIBRARY_NAMES} ${LIBUV_LIBRARY} ${EXTRA_LIBS})
install(TARGETS otp_server DESTINATION bin)
