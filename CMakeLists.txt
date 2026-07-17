cmake_minimum_required(VERSION 3.25)

project(MyChatty VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT IOS)
    set(QT_SKIP_AUTO_PLUGIN_INCLUSION ON)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Network Qml Quick QuickControls2 QuickDialogs2 Test WebView)

qt_standard_project_setup(REQUIRES 6.8)

add_library(mychatty_core STATIC
    src/core/ApiClient.cpp
    src/core/ApiClient.h
    src/core/ApiProtocolAdapter.cpp
    src/core/ApiProtocolAdapter.h
    src/core/ChatController.cpp
    src/core/ChatController.h
    src/core/ChatMessageModel.cpp
    src/core/ChatMessageModel.h
    src/core/ChatSerializer.cpp
    src/core/ChatSerializer.h
    src/core/ChatTypes.cpp
    src/core/ChatTypes.h
    src/core/ConversationStore.cpp
    src/core/ConversationStore.h
    src/core/ConversationFileStore.cpp
    src/core/ConversationFileStore.h
    src/core/MarkdownRenderer.cpp
    src/core/MarkdownRenderer.h
    src/core/ModelCatalog.cpp
    src/core/ModelCatalog.h
    src/core/OpenaiChatAPI.cpp
    src/core/OpenaiChatAPI.h
    src/core/OpenaiResponsesAPI.cpp
    src/core/OpenaiResponsesAPI.h
    src/core/SseParser.cpp
    src/core/SseParser.h
    src/core/SettingsStore.cpp
    src/core/SettingsStore.h
    src/core/SpeechService.cpp
    src/core/SpeechService.h
    src/core/ToolExecutor.cpp
    src/core/ToolExecutor.h
)
target_include_directories(mychatty_core PUBLIC src/core)
target_link_libraries(mychatty_core PUBLIC Qt6::Core Qt6::Gui Qt6::Network Qt6::Qml Qt6::WebView)

qt_add_resources(mychatty_core "reader_resources"
    PREFIX "/"
    FILES
        third_party/readability/Readability.js
)

qt_add_executable(MyChatty
    MACOSX_BUNDLE
    src/app/main.cpp
)
set_target_properties(MyChatty PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER de.ndurner.MyChatty
    MACOSX_BUNDLE_BUNDLE_NAME MyChatty
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_link_libraries(MyChatty PRIVATE mychatty_core Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::QuickDialogs2)

qt_add_qml_module(MyChatty
    URI MyChatty
    VERSION 1.0
    QML_FILES
        src/app/qml/Main.qml
        src/app/qml/AttachmentMenu.qml
        src/app/qml/AttachmentMenuIcon.qml
        src/app/qml/AttachmentMenuRow.qml
        src/app/qml/ChatBubble.qml
        src/app/qml/Composer.qml
        src/app/qml/Chevron.qml
        src/app/qml/IconButton.qml
        src/app/qml/LabeledSecretField.qml
        src/app/qml/MicrophoneIcon.qml
        src/app/qml/ModelEffortSelector.qml
        src/app/qml/SettingsSheet.qml
        src/app/qml/Sidebar.qml
)

qt_add_executable(mychatty-cli
    src/cli/main.cpp
)
set_target_properties(mychatty-cli PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries(mychatty-cli PRIVATE mychatty_core)

enable_testing()
qt_add_executable(mychatty_core_tests
    tests/test_core.cpp
)
set_target_properties(mychatty_core_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries(mychatty_core_tests PRIVATE mychatty_core Qt6::Test)
add_test(NAME mychatty_core_tests COMMAND mychatty_core_tests)

install(TARGETS MyChatty mychatty-cli
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
