cmake_minimum_required(VERSION 3.15)
project(DataMatrixScanner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки для Windows
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)
    add_definitions(-DNOMINMAX)
endif()

# Поиск пакетов
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio highgui)
# Примечание: libzbar удален из vcpkg. Используем встроенные возможности OpenCV (WeChat QRCode) для детекции и анализа.
# Для декодирования DataMatrix в промышленных решениях часто используют отдельные SDK камер.
find_package(Threads REQUIRED)

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/datamatrix_scanner.cpp
    src/camera_capture.cpp
    src/quality_analyzer.cpp
    src/report_generator.cpp
)

set(HEADERS
    include/datamatrix_scanner.h
    include/camera_capture.h
    include/quality_analyzer.h
    include/report_generator.h
    include/gost_standards.h
)

# Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Подключение заголовочных файлов
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

# Подключение библиотек
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
    Threads::Threads
)

# Для Windows - дополнительные настройки
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
        VS_DPI_AWARE "PerMonitor"
    )
endif()

# Установка
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/config DESTINATION .)
