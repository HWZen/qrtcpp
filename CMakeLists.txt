cmake_minimum_required(VERSION 3.18...3.28)
project(QRCodeStreamSender)

set(CMAKE_CXX_STANDARD 23)
include_directories(.)

set(Qt5_DIR "C:/Qt/Qt5.12.9/5.12.9/msvc2017_64/lib/cmake/Qt5")

find_package(Qt5 COMPONENTS Core Gui Widgets Svg REQUIRED)

set(CMAKE_AUTOMOC ON)

# 获取Qt的bin目录路径
get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)

# 设置windeployqt.exe的路径
set(WINDEPLOYQT_EXECUTABLE "${QT_BIN_DIR}/windeployqt.exe")
message(STATUS "windeployqt path: ${WINDEPLOYQT_EXECUTABLE}")

if(WIN32)
    # 生成包含兼容性声明的完整清单文件
    set(ADMIN_MANIFEST_CONTENT
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
            "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\n"
            "  <compatibility xmlns=\"urn:schemas-microsoft-com:compatibility.v1\">\n"
            "    <application>\n"
            "      <supportedOS Id=\"{8e0f7a12-bfb3-4fe8-b9a5-48fd0a9d0fb2}\"/>\n"  # Win10
            "    </application>\n"
            "  </compatibility>\n"
            "  <trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v3\">\n"
            "    <security>\n"
            "      <requestedPrivileges>\n"
            "        <requestedExecutionLevel level=\"requireAdministrator\" uiAccess=\"false\"/>\n"
            "      </requestedPrivileges>\n"
            "    </security>\n"
            "  </trustInfo>\n"
            "</assembly>"
    )
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/admin.manifest" "${ADMIN_MANIFEST_CONTENT}")

    # 生成RC文件（注意使用相对路径）
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/admin.rc" "1 RT_MANIFEST \"admin.manifest\"\n")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /utf-8")

add_definitions(-DUNICODE -D_UNICODE)

add_executable(qrcode_stream_sender 
    qrcode_stream_sender.cpp
    qrcodegen.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/admin.rc"
)
target_link_directories(qrcode_stream_sender PUBLIC .)

# 禁用自动清单生成
# set_target_properties(run PROPERTIES
# LINK_FLAGS "/MANIFEST:NO"
# )

# set_property(TARGET run PROPERTY
# MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_executable(qrcode_stream_receiver 
    qrcode_stream_receiver.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/admin.rc")

target_link_libraries(qrcode_stream_sender PRIVATE 
wirehair
Qt5::Core
Qt5::Gui
Qt5::Widgets
Qt5::Svg
)

target_link_libraries(qrcode_stream_receiver PRIVATE
        wirehair
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Svg
)

# 添加编译后运行windeployqt的命令
add_custom_command(TARGET qrcode_stream_sender POST_BUILD
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --verbose 0 --no-translations --no-system-d3d-compiler --no-opengl-sw "$<TARGET_FILE:qrcode_stream_sender>"
    COMMENT "Running windeployqt for qrcode_stream_sender"
)

add_custom_command(TARGET qrcode_stream_receiver POST_BUILD
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --verbose 0 --no-translations --no-system-d3d-compiler --no-opengl-sw "$<TARGET_FILE:qrcode_stream_receiver>"
    COMMENT "Running windeployqt for qrcode_stream_receiver"
)


