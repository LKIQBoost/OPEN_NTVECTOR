add_executable(nbt_roundtrip_test
    nbt_roundtrip_test.cpp
    "${CMAKE_SOURCE_DIR}/application/NBT/NbtIo.cpp"
    "${CMAKE_SOURCE_DIR}/application/NBT/Tag.cpp"
)
target_include_directories(nbt_roundtrip_test PRIVATE
    "${CMAKE_SOURCE_DIR}/application"
)

add_executable(command_request_test
    command_request_test.cpp
    "${CMAKE_SOURCE_DIR}/application/Packets/CommandRequest.cpp"
    "${CMAKE_SOURCE_DIR}/application/Packets/PacketBase.cpp"
    "${CMAKE_SOURCE_DIR}/application/Crypto/BinaryReader.cpp"
    "${CMAKE_SOURCE_DIR}/application/Crypto/BinaryWriter.cpp"
)
target_include_directories(command_request_test PRIVATE
    "${CMAKE_SOURCE_DIR}/application"
    "${CMAKE_SOURCE_DIR}/application/Core"
    "${CMAKE_SOURCE_DIR}/application/Crypto"
    "${CMAKE_SOURCE_DIR}/application/Packets"
)

add_test(NAME nbt_roundtrip COMMAND nbt_roundtrip_test)
add_test(NAME command_request COMMAND command_request_test)

add_executable(player_list_test
    player_list_test.cpp
    "${CMAKE_SOURCE_DIR}/application/Packets/PlayerList.cpp"
    "${CMAKE_SOURCE_DIR}/application/Packets/PacketBase.cpp"
    "${CMAKE_SOURCE_DIR}/application/Crypto/BinaryReader.cpp"
    "${CMAKE_SOURCE_DIR}/application/Crypto/BinaryWriter.cpp"
)
target_include_directories(player_list_test PRIVATE
    "${CMAKE_SOURCE_DIR}/application"
    "${CMAKE_SOURCE_DIR}/application/Core"
    "${CMAKE_SOURCE_DIR}/application/Crypto"
    "${CMAKE_SOURCE_DIR}/application/Packets"
)

add_test(NAME player_list COMMAND player_list_test)
