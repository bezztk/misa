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
        src/api/ApiClient.h
        src/ui/MainWindow.cpp
        src/ui/MainWindow.h
        src/ui/TaskView.cpp
        src/ui/TaskView.h
        src/models/Task.h
        src/ui/LearningView.cpp
        src/ui/LearningView.h
        src/models/Flashcard.h
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
        assets/icons/task.png
        assets/icons/clipboard.png
        assets/icons/flash-cards.png
)