cmake_minimum_required(VERSION 4.4)
project(frontend)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        REQUIRED)

add_executable(frontend
    main.cpp
        src/api/ApiClient.cpp
        src/api/ApiClientProgression.cpp
        src/api/ApiClientWork.cpp
        src/api/ApiClient.h
        src/ui/MainWindow.cpp
        src/ui/MainWindow.h
        src/ui/CalendarView.cpp
        src/ui/CalendarView.h
        src/ui/CalendarEntryDialog.cpp
        src/ui/CalendarEntryDialog.h
        src/ui/TaskView.cpp
        src/ui/TaskView.h
        src/models/Task.h
        src/models/DomainNode.h
        src/models/Document.h
        src/ui/LearningView.cpp
        src/ui/LearningView.h
        src/ui/DomainView.cpp
        src/ui/DomainView.h
        src/ui/DocumentEditor.cpp
        src/ui/DocumentEditor.h
        src/ui/RoutineDialog.cpp
        src/ui/RoutineDialog.h
        src/models/Flashcard.h
        src/models/Metadata.h
        src/models/Skill.h
        src/models/Pursuit.h
        src/models/Progression.h
        src/models/CalendarEntry.h
        src/models/Routine.h
)
target_link_libraries(frontend
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
)

if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt6::Core>"
            "$<TARGET_FILE:Qt6::Gui>"
            "$<TARGET_FILE:Qt6::Widgets>"
            "$<TARGET_FILE:Qt6::Network>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"

            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"
    )
endif()

qt_add_resources(frontend "icons"
        PREFIX "/"
        FILES
        assets/icons/task-white.png
        assets/icons/flashcards-white.png
)
