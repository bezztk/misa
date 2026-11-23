cmake_minimum_required(VERSION 3.30)
project(frontend LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(sodium
        GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
        GIT_TAG 9b2848dfc1b917a9410f0de9d81059b26cbfaa8d
        GIT_SHALLOW TRUE
)
set(SODIUM_DISABLE_TESTS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(sodium)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (WIN32 AND NOT MSVC)
    message(FATAL_ERROR "The Misa frontend must be built with the MSVC toolchain.")
endif()

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        WebEngineWidgets
        WebChannel
        REQUIRED)

find_program(NPM_EXECUTABLE NAMES npm.cmd npm REQUIRED)

set(EDITOR_WEB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/editor")
set(EDITOR_DIST_DIR "${EDITOR_WEB_DIR}/dist")
set(EDITOR_NODE_MODULES_MARKER "${EDITOR_WEB_DIR}/node_modules/.package-lock.json")
set(EDITOR_WEB_OUTPUTS
        "${EDITOR_DIST_DIR}/index.html"
        "${EDITOR_DIST_DIR}/editor.js"
        "${EDITOR_DIST_DIR}/editor.css"
)

add_custom_command(
        OUTPUT "${EDITOR_NODE_MODULES_MARKER}"
        COMMAND "${NPM_EXECUTABLE}" ci
        WORKING_DIRECTORY "${EDITOR_WEB_DIR}"
        DEPENDS
        "${EDITOR_WEB_DIR}/package.json"
        "${EDITOR_WEB_DIR}/package-lock.json"
        COMMENT "Installing Milkdown editor dependencies"
        VERBATIM
)

add_custom_command(
        OUTPUT ${EDITOR_WEB_OUTPUTS}
        COMMAND "${NPM_EXECUTABLE}" run build
        WORKING_DIRECTORY "${EDITOR_WEB_DIR}"
        DEPENDS
        "${EDITOR_NODE_MODULES_MARKER}"
        "${EDITOR_WEB_DIR}/index.html"
        "${EDITOR_WEB_DIR}/tsconfig.json"
        "${EDITOR_WEB_DIR}/vite.config.ts"
        "${EDITOR_WEB_DIR}/src/main.ts"
        "${EDITOR_WEB_DIR}/src/style.css"
        COMMENT "Building Milkdown editor"
        VERBATIM
)

add_custom_target(editor_web DEPENDS ${EDITOR_WEB_OUTPUTS})
set_source_files_properties(${EDITOR_WEB_OUTPUTS} PROPERTIES GENERATED TRUE)

set(CALENDAR_WEB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/calendar")
set(CALENDAR_DIST_DIR "${CALENDAR_WEB_DIR}/dist")
set(CALENDAR_NODE_MODULES_MARKER "${CALENDAR_WEB_DIR}/node_modules/.package-lock.json")
set(CALENDAR_WEB_OUTPUTS
        "${CALENDAR_DIST_DIR}/index.html"
        "${CALENDAR_DIST_DIR}/calendar.js"
        "${CALENDAR_DIST_DIR}/calendar.css"
)

add_custom_command(
        OUTPUT "${CALENDAR_NODE_MODULES_MARKER}"
        COMMAND "${NPM_EXECUTABLE}" ci
        WORKING_DIRECTORY "${CALENDAR_WEB_DIR}"
        DEPENDS
        "${CALENDAR_WEB_DIR}/package.json"
        "${CALENDAR_WEB_DIR}/package-lock.json"
        COMMENT "Installing FullCalendar dependencies"
        VERBATIM
)

add_custom_command(
        OUTPUT ${CALENDAR_WEB_OUTPUTS}
        COMMAND "${NPM_EXECUTABLE}" run build
        WORKING_DIRECTORY "${CALENDAR_WEB_DIR}"
        DEPENDS
        "${CALENDAR_NODE_MODULES_MARKER}"
        "${CALENDAR_WEB_DIR}/index.html"
        "${CALENDAR_WEB_DIR}/vite.config.js"
        "${CALENDAR_WEB_DIR}/src/main.js"
        "${CALENDAR_WEB_DIR}/src/style.css"
        COMMENT "Building FullCalendar view"
        VERBATIM
)

add_custom_target(calendar_web DEPENDS ${CALENDAR_WEB_OUTPUTS})
set_source_files_properties(${CALENDAR_WEB_OUTPUTS} PROPERTIES GENERATED TRUE)

set(KANBAN_WEB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/kanban")
set(KANBAN_DIST_DIR "${KANBAN_WEB_DIR}/dist")
set(KANBAN_NODE_MODULES_MARKER "${KANBAN_WEB_DIR}/node_modules/.package-lock.json")
set(KANBAN_WEB_OUTPUTS
        "${KANBAN_DIST_DIR}/index.html"
        "${KANBAN_DIST_DIR}/kanban.js"
        "${KANBAN_DIST_DIR}/kanban.css"
)

add_custom_command(
        OUTPUT "${KANBAN_NODE_MODULES_MARKER}"
        COMMAND "${NPM_EXECUTABLE}" ci
        WORKING_DIRECTORY "${KANBAN_WEB_DIR}"
        DEPENDS
        "${KANBAN_WEB_DIR}/package.json"
        "${KANBAN_WEB_DIR}/package-lock.json"
        COMMENT "Installing SVAR Kanban dependencies"
        VERBATIM
)

add_custom_command(
        OUTPUT ${KANBAN_WEB_OUTPUTS}
        COMMAND "${NPM_EXECUTABLE}" run build
        WORKING_DIRECTORY "${KANBAN_WEB_DIR}"
        DEPENDS
        "${KANBAN_NODE_MODULES_MARKER}"
        "${KANBAN_WEB_DIR}/index.html"
        "${KANBAN_WEB_DIR}/vite.config.js"
        "${KANBAN_WEB_DIR}/src/main.jsx"
        "${KANBAN_WEB_DIR}/src/style.css"
        COMMENT "Building SVAR Kanban view"
        VERBATIM
)

add_custom_target(kanban_web DEPENDS ${KANBAN_WEB_OUTPUTS})
set_source_files_properties(${KANBAN_WEB_OUTPUTS} PROPERTIES GENERATED TRUE)

if (WIN32)
    set(APP_ICON_RESOURCE frontend.rc)
endif()

add_executable(frontend
        main.cpp
        ${APP_ICON_RESOURCE}
        src/api/ApiClient.cpp
        src/api/ApiClientProgression.cpp
        src/api/ApiClientIdeas.cpp
        src/api/ApiClientRss.cpp
        src/api/ApiClientPassword.cpp
        src/api/ApiClientWork.cpp
        src/api/ApiClient.h
        src/ui/MainWindow.cpp
        src/ui/MainWindow.h
        src/ui/NavigationSidebar.cpp
        src/ui/NavigationSidebar.h
        src/ui/WorkspaceManager.cpp
        src/ui/WorkspaceManager.h
        src/ui/WorkspaceSplitter.cpp
        src/ui/WorkspaceSplitter.h
        src/ui/TabGroup.cpp
        src/ui/TabGroup.h
        src/ui/CalendarView.cpp
        src/ui/CalendarView.h
        src/ui/CalendarEntryDialog.cpp
        src/ui/CalendarEntryDialog.h
        src/ui/DialogUtils.cpp
        src/ui/DialogUtils.h
        src/ui/TaskEditDialog.cpp
        src/ui/TaskEditDialog.h
        src/ui/VersionHistoryDialog.cpp
        src/ui/VersionHistoryDialog.h
        src/ui/CommentUi.cpp
        src/ui/CommentUi.h
        src/ui/TaskView.cpp
        src/ui/TaskView.h
        src/ui/IdeaView.cpp
        src/ui/IdeaView.h
        src/ui/RssView.cpp
        src/ui/RssView.h
        src/ui/PasswordManagerView.cpp
        src/ui/PasswordManagerView.h
        src/models/Task.h
        src/models/Idea.h
        src/models/Rss.h
        src/models/Password.h
        src/models/DomainNode.h
        src/models/Document.h
        src/ui/LearningView.cpp
        src/ui/LearningView.h
        src/ui/DomainView.cpp
        src/ui/DomainView.h
        src/ui/DocumentEditor.cpp
        src/ui/DocumentEditor.h
        src/models/Flashcard.h
        src/models/Attachment.h
        src/models/Metadata.h
        src/models/Skill.h
        src/models/Pursuit.h
        src/models/Progression.h
        src/models/CalendarEntry.h
        src/models/EntityFieldVersion.h
        src/models/Comment.cpp
        src/models/Comment.h
)
set_target_properties(frontend PROPERTIES
        WIN32_EXECUTABLE "$<CONFIG:Release>"
)
target_link_libraries(frontend
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::WebEngineWidgets
        Qt6::WebChannel
        sodium
)

if (WIN32)
    target_link_libraries(frontend Shell32)
endif()

target_compile_options(frontend PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
add_dependencies(frontend editor_web calendar_web kanban_web)

if (WIN32)
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    add_custom_target(deploy_frontend
            COMMAND "${QT_BIN_DIR}/windeployqt.exe" "$<TARGET_FILE:${PROJECT_NAME}>"
            DEPENDS ${PROJECT_NAME}
            COMMENT "Deploying Qt and Qt WebEngine runtime files"
            VERBATIM
    )
endif()

qt_add_resources(frontend "document_editor"
        PREFIX "/editor"
        BASE "${EDITOR_DIST_DIR}"
        FILES ${EDITOR_WEB_OUTPUTS}
)

qt_add_resources(frontend "calendar"
        PREFIX "/calendar"
        BASE "${CALENDAR_DIST_DIR}"
        FILES ${CALENDAR_WEB_OUTPUTS}
)

qt_add_resources(frontend "kanban"
        PREFIX "/kanban"
        BASE "${KANBAN_DIST_DIR}"
        FILES ${KANBAN_WEB_OUTPUTS}
)

qt_add_resources(frontend "icons"
        PREFIX "/"
        FILES
        assets/icons/task-white.png
        assets/icons/attach-file.png
        assets/icons/star.png
        assets/icons/lock.png
        assets/icons/incubator.png
        assets/icons/pursuit.png
        assets/icons/flashcards-white.png
        assets/icons/calendar.png
        assets/icons/folder.png
        assets/icons/file.png
        assets/icons/rss.png
        assets/icons/open.png
        assets/icons/clock.png
        assets/icons/add.png
        assets/icons/edit.png
        assets/icons/delete.png
        assets/icons/misa.png
        assets/icons/node.png
        assets/icons/knowledge.png
        assets/icons/version_history.png
        assets/icons/speech-bubble.png
        assets/icons/books.png
        assets/icons/infinity.png
        assets/icons/idea.png
        assets/icons/close.png
        assets/icons/split-right.png
        assets/icons/split-down.png
)
