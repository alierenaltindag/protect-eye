cmake_minimum_required(VERSION 3.20)

# Single Source of Truth: read version from root VERSION file
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROTECTEYE_RAW_VERSION)
string(STRIP "${PROTECTEYE_RAW_VERSION}" PROTECTEYE_RAW_VERSION)

project(ProtectEye
    VERSION ${PROTECTEYE_RAW_VERSION}
    DESCRIPTION "Cross-Platform Eye Health & Break Assistant"
    HOMEPAGE_URL "https://github.com/alierenaltindag/protect-eye"
    LANGUAGES CXX
)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    Svg
    Network
)

set(EXTRA_LIBS)
set(EXTRA_SOURCES)

if (UNIX AND NOT APPLE)
    find_package(Qt6 REQUIRED COMPONENTS DBus)
    list(APPEND EXTRA_LIBS Qt6::DBus)
endif()

if (WIN32)
    list(APPEND EXTRA_SOURCES resources/protecteye.rc)
    list(APPEND EXTRA_LIBS shell32 wtsapi32 user32 dwmapi)
endif()

set(SOURCES
    src/main.cpp
    src/core/Settings.cpp
    src/core/Localization.cpp
    src/core/BreakController.cpp
    src/services/DndMonitor.cpp
    src/services/ScreenLockMonitor.cpp
    src/services/SoundManager.cpp
    src/services/NotificationService.cpp
    src/services/UpdateChecker.cpp
    src/ui/EyeExerciseWidget.cpp
    src/ui/OverlayWindow.cpp
    src/ui/OverlayManager.cpp
    src/ui/TrayManager.cpp
    src/ui/SettingsDialog.cpp
    src/utils/AutostartHelper.cpp
    src/utils/SingleInstanceGuard.cpp
    resources/resources.qrc
    ${EXTRA_SOURCES}
)

set(HEADERS
    src/core/Settings.h
    src/core/Localization.h
    src/core/BreakController.h
    src/services/DndMonitor.h
    src/services/ScreenLockMonitor.h
    src/services/SoundManager.h
    src/services/NotificationService.h
    src/services/UpdateChecker.h
    src/ui/EyeExerciseWidget.h
    src/ui/OverlayWindow.h
    src/ui/OverlayManager.h
    src/ui/TrayManager.h
    src/ui/SettingsDialog.h
    src/utils/AutostartHelper.h
    src/utils/SingleInstanceGuard.h
)

if (WIN32)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/protecteye.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/resources/protecteye.rc
        @ONLY
    )
    add_executable(protecteye WIN32 ${SOURCES} ${HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/resources/protecteye.rc)
else()
    add_executable(protecteye ${SOURCES} ${HEADERS})
endif()

target_include_directories(protecteye PRIVATE src)

target_link_libraries(protecteye PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Svg
    Qt6::Network
    ${EXTRA_LIBS}
)

target_compile_definitions(protecteye PRIVATE
    PROTECTEYE_VERSION="${PROJECT_VERSION}"
)

include(GNUInstallDirs)

install(TARGETS protecteye
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (UNIX AND NOT APPLE)
    install(FILES protecteye.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(FILES resources/protecteye-autostart.desktop
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart
        RENAME protecteye.desktop
    )
    install(FILES resources/app_icon.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME protecteye.svg
    )
    install(FILES io.github.alierenaltindag.protecteye.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
    )
    install(PROGRAMS update.sh uninstall.sh
        DESTINATION ${CMAKE_INSTALL_DATADIR}/protecteye
    )
endif()

# CPack Packaging Configuration
set(CPACK_SET_DESTDIR "ON")
set(CPACK_PACKAGE_NAME "protecteye")
set(CPACK_PACKAGE_VENDOR "ProtectEye Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Göz Sağlığı ve Mola Asistanı (Cross-Platform)")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "ProtectEye Team <protecteye@example.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/alierenaltindag/protect-eye")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# Debian & Pardus Package (.deb) Configuration
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ProtectEye Team <protecteye@example.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libqt6core6 | libqt6core6t64, libqt6gui6 | libqt6gui6t64, libqt6widgets6 | libqt6widgets6t64, libqt6multimedia6 | libqt6multimedia6t64, libqt6svg6 | libqt6svg6t64, libqt6dbus6 | libqt6dbus6t64")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/alierenaltindag/protect-eye")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/packaging/ubuntu/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/ubuntu/prerm;${CMAKE_CURRENT_SOURCE_DIR}/packaging/ubuntu/postrm")

# RPM Package (.rpm) Configuration (Fedora, openSUSE, RHEL, AlmaLinux)
set(CPACK_RPM_PACKAGE_NAME "protecteye")
set(CPACK_RPM_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
set(CPACK_RPM_PACKAGE_VENDOR "ProtectEye Team")
set(CPACK_RPM_PACKAGE_URL "https://github.com/alierenaltindag/protect-eye")
set(CPACK_RPM_PACKAGE_SUMMARY "Cross-platform Eye Health and Break Assistant")
set(CPACK_RPM_PACKAGE_DESCRIPTION "ProtectEye is a modern cross-platform eye health assistant combating digital eye strain with 20-20-20 rule, interactive exercises, and posture reminders.")
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase-gui, qt6-qtmultimedia, qt6-qtsvg")
set(CPACK_RPM_PACKAGE_AUTOREQ OFF)

# NSIS (Windows) Configuration
set(CPACK_NSIS_DISPLAY_NAME "ProtectEye")
set(CPACK_NSIS_PACKAGE_NAME "ProtectEye")
set(CPACK_NSIS_MODIFY_PATH ON)

include(CPack)

# Testing
enable_testing()
find_package(Qt6 OPTIONAL_COMPONENTS Test)
if (Qt6Test_FOUND)
    set(TEST_SOURCES
        tests/test_protecteye.cpp
        src/core/Settings.cpp
        src/core/Localization.cpp
        src/core/BreakController.cpp
        src/services/DndMonitor.cpp
        src/services/ScreenLockMonitor.cpp
        src/services/UpdateChecker.cpp
        src/utils/AutostartHelper.cpp
        src/ui/OverlayWindow.cpp
        src/ui/OverlayManager.cpp
        src/ui/EyeExerciseWidget.cpp
        src/ui/SettingsDialog.cpp
        resources/resources.qrc
    )

    add_executable(test_protecteye ${TEST_SOURCES})
    target_include_directories(test_protecteye PRIVATE src)
    target_link_libraries(test_protecteye PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Test
        ${EXTRA_LIBS}
    )

    target_compile_definitions(test_protecteye PRIVATE
        PROTECTEYE_VERSION="${PROJECT_VERSION}"
    )

    add_test(NAME ProtectEyeUnitTests COMMAND test_protecteye)
endif()
