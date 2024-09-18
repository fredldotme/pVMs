set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(PLUGIN "PocketVMs")

set(
    SRC
    plugin.cpp
    vmmanager.cpp
    machine.cpp
    scaler.cpp
    vnc_client.cpp
    vnc_output.cpp
)

set(CMAKE_AUTOMOC ON)

execute_process(
    COMMAND dpkg-architecture -qDEB_HOST_MULTIARCH
    OUTPUT_VARIABLE ARCH_TRIPLET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_library(${PLUGIN} MODULE ${SRC})
set_target_properties(${PLUGIN} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGIN})
qt5_use_modules(${PLUGIN} Gui Qml Quick DBus Widgets)
target_link_libraries(${PLUGIN} vncclient ${CMAKE_INSTALL_PREFIX}/usr/lib/${ARCH_TRIPLET}/qt5/qml/QMLTermWidget/libqmltermwidget.so)

set(QT_IMPORTS_DIR "${CMAKE_INSTALL_PREFIX}/lib/${ARCH_TRIPLET}")

install(TARGETS ${PLUGIN} DESTINATION ${QT_IMPORTS_DIR}/${PLUGIN}/)
install(FILES qmldir DESTINATION ${QT_IMPORTS_DIR}/${PLUGIN}/)
