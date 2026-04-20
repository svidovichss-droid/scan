# DataMatrix Quality Scanner

Сканер качества печати DataMatrix кодов для промышленных конвейерных систем с поддержкой ГОСТ Р 57302-2016 и ISO/IEC 16022.

## Возможности

- **Поддержка промышленных камер**: USB, GigE, Camera Link
- **Анализ качества по стандартам**: ГОСТ Р 57302-2016 и ISO/IEC 16022
- **Параметры контроля**:
  - Контрастность (мин. 20%)
  - Дефектность печати (мин. 70%)
  - Геометрия символов
  - Модуляция
  - Размер модуля (0.254-1.016 мм)
  - Тихая зона (мин. 1 модуль)
  - Угол поворота (макс. отклонение 5°)
- **Непрерывный режим работы** на конвейере
- **Генерация отчетов**: JSON, CSV, XML, HTML
- **Режим отладки** с сохранением изображений

## Требования

### Для Windows:
- Windows 10/11
- Visual Studio 2019 или новее
- CMake 3.15+
- vcpkg

### Для Linux:
- Ubuntu 18.04+ / Debian 10+
- g++ 7+
- CMake 3.15+
- OpenCV 4.x

**Примечание:** Библиотека libzbar удалена из vcpkg и некоторых репозиториев Linux. Для декодирования DataMatrix в промышленном решении рекомендуется использовать:
1. SDK промышленной камеры (Basler pylon, Cognex, Keyence) - имеют встроенные декодеры
2. Библиотеку libdmtx: `sudo apt-get install libdmtx-dev` или сборка из исходников
3. OpenCV WeChat QRCode detector (экспериментальная поддержка DataMatrix в OpenCV 4.+)

## Установка зависимостей

### Windows (через vcpkg):
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
# Примечание: libzbar удален из vcpkg. Используем только OpenCV.
# Для декодирования DataMatrix используйте SDK камеры или libdmtx.
.\vcpkg install opencv:x64-windows
```

### Linux (Ubuntu/Debian):
```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libopencv-dev libpthread-stubs0-dev
# Опционально: установите libdmtx для декодирования DataMatrix
# sudo apt-get install libdmtx-dev
```

## Сборка

### Windows:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release
```

### Linux:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```

## Использование

### Базовый запуск (тестовый режим):
```bash
DataMatrixScanner.exe
```

### Запуск с камерой:
```bash
DataMatrixScanner.exe --mode 1 --camera 0 --camera-type 0
```

### Режим конвейера с GigE камерой:
```bash
DataMatrixScanner.exe --mode 1 --camera-type 1 --camera-ip 192.168.1.100
```

### Все параметры:
```
--mode <0|1>       Режим: 0=тестовый, 1=конвейер (по умолчанию: 0)
--camera <id>      ID камеры (по умолчанию: 0)
--camera-type <t>  Тип камеры: 0=USB, 1=GigE, 2=CameraLink (по умолчанию: 0)
--camera-ip <ip>   IP адрес для GigE камеры
--report-path <p>  Путь для отчетов (по умолчанию: ./reports/)
--debug            Включить режим отладки
--width <w>        Ширина кадра (по умолчанию: 1920)
--height <h>       Высота кадра (по умолчанию: 1080)
--fps <f>          Частота кадров (по умолчанию: 30)
--help, -h         Показать справку
```

## Структура проекта

```
DataMatrixScanner/
├── CMakeLists.txt              # Конфигурация CMake
├── .github/workflows/build.yml # GitHub Actions для сборки
├── include/
│   ├── gost_standards.h        # Стандарты ГОСТ и ISO
│   ├── camera_capture.h        # Захват с камеры
│   ├── datamatrix_scanner.h    # Основной сканер
│   ├── quality_analyzer.h      # Анализ качества
│   └── report_generator.h      # Генерация отчетов
├── src/
│   ├── main.cpp                # Точка входа
│   ├── datamatrix_scanner.cpp
│   ├── camera_capture.cpp
│   ├── quality_analyzer.cpp
│   └── report_generator.cpp
└── config/                     # Конфигурационные файлы
```

## Автоматическая сборка через GitHub Actions

Проект включает workflow для автоматической сборки:

1. При пуше в main/master ветку собирается Windows и Linux версии
2. При создании тега релиза автоматически создается GitHub Release с бинарниками

Для запуска сборки:
```bash
git tag v1.0.0
git push origin v1.0.0
```

## Интеграция с промышленными камерами

### Basler (GigE):
```cpp
camera::CameraConfig config;
config.camera_type = 1; // GigE
config.camera_ip = "192.168.1.100";
```

### FLIR Blackfly S:
Требуется установка Spinnaker SDK и модификация `camera_capture.cpp`

### Camera Link:
Требуется frame grabber SDK (National Instruments, Matrox и т.д.)

## Форматы отчетов

### JSON:
```json
{
  "report_info": {
    "generated_at": "2024-01-15 10:30:00",
    "total_entries": 100
  },
  "entries": [...]
}
```

### CSV:
```csv
Timestamp,Data,Quality Score,Passed,Scan Time (ms),Error Message
2024-01-15 10:30:00,"ABC123",85.5,PASS,12.3,""
```

### HTML:
Интерактивный отчет с таблицей результатов и сводной статистикой.

## Лицензия

MIT License

## Поддержка стандартов

Приложение соответствует требованиям:
- **ГОСТ Р 57302-2016** - Контроль качества маркировки продукции
- **ISO/IEC 16022** - International specification for Data Matrix

## Контакты

Для вопросов и предложений создавайте Issues на GitHub.
