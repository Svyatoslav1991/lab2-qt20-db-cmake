# lab2_qt20_db_cmake

Учебный проект на **C++17 / Qt5 / SQLite / CMake** по лабораторной работе №2 (Qt20):
работа с базой данных через `QtSql`, архитектура **Model/View**, отображение и редактирование данных через `QSqlTableModel` + `QTableView`, а также покрытие логики тестами (`QtTest`, `CTest`).

---

## Что реализовано

### Работа с БД (SQLite, QtSql)

* создание/переиспользование соединения с БД (`QSqlDatabase`)
* закрытие соединения
* создание таблицы `rectangle`
* удаление таблицы
* заполнение таблицы тестовыми данными:

  * `INSERT ... VALUES`
  * `prepare + bindValue(":name", ...)`
  * `prepare + addBindValue(...)`
  * `prepare + bindValue(pos, ...)`
* выборка данных (`SELECT *`) и вывод в `qDebug()`

### Model/View (Qt Widgets)

* отображение таблицы БД через `QSqlTableModel` в `QTableView`
* настройка заголовков столбцов
* скрытие `id`-столбца
* редактирование через делегат `MyDelegate`

  * `PenStyle` редактируется через `QComboBox`
  * `PenColor` редактируется через `QColorDialog`
  * для `PenStyle` реализован `paint()` с текстовым отображением стиля

### Тесты (QtTest + CTest)

* `test_smoke` — базовая проверка сборки/запуска QtTest
* `test_mydelegate` — тесты делегата `MyDelegate`
* `test_mainwindow` — тесты логики `MainWindow` (БД, модель, действия меню/слотов)

### CI

* GitHub Actions workflow для автоматической:

  * конфигурации CMake
  * сборки проекта
  * запуска тестов (`ctest`)

---

## Стек технологий

* **C++17**
* **Qt5** (`Core`, `Widgets`, `Sql`, `Test`)
* **SQLite** (через `QSQLITE`)
* **CMake**
* **CTest / QtTest**
* **GitHub Actions** (CI)

---

## Структура проекта

```text
lab2_qt20_db_cmake/
├─ CMakeLists.txt
├─ app/
│  ├─ CMakeLists.txt
│  ├─ include/
│  │  └─ myrect.h
│  └─ src/
│     ├─ main.cpp
│     ├─ mainwindow.h
│     ├─ mainwindow.cpp
│     ├─ mainwindow.ui
│     ├─ mydelegate.h
│     └─ mydelegate.cpp
├─ tests/
│  ├─ CMakeLists.txt
│  ├─ test_smoke.cpp
│  ├─ test_mydelegate.cpp
│  └─ test_mainwindow.cpp
└─ .github/
   └─ workflows/
      └─ ci.yml
```

---

## Сборка (локально)

### Windows (Qt + CMake)

> Нужны установленный Qt5 (модули `Widgets`, `Sql`, `Test`) и CMake.

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
```

### Запуск приложения

```bash
./build/app/lab2_app
```

> На Windows путь к `lab2_app.exe` может отличаться в зависимости от генератора CMake (Ninja / MinGW / MSVC).

### Запуск тестов

```bash
ctest --test-dir build --output-on-failure
```

---

## CMake-архитектура

### Корневой `CMakeLists.txt`

* подключает `CTest`
* собирает `app`
* при `BUILD_TESTING=ON` подключает `tests`

### `app/CMakeLists.txt`

* `lab2_core` — интерфейсная библиотека с include-путями и общими зависимостями Qt
* `lab2_ui` — библиотека с UI-логикой (`MainWindow`, `MyDelegate`)
* `lab2_app` — исполняемый файл (`main.cpp`)

### `tests/CMakeLists.txt`

* функция `add_qt_test(...)` для быстрого добавления QtTest-таргетов
* автоматическое подключение `lab2_core`, `lab2_ui`, `Qt5::Test`
* регистрация тестов в `CTest` через `add_test(...)`

---

## Основные классы

### `MainWindow`

Главное окно приложения. Содержит:

* меню `BD`, `Model`, `Query`
* слоты для работы с БД и моделью
* `QSqlDatabase` (именованное соединение)
* `QSqlTableModel` для таблицы `rectangle`

### `MyDelegate`

Кастомный делегат для `QTableView`:

* `createEditor()` — `QComboBox` для колонки `penstyle`
* `setEditorData()` / `setModelData()` — работа со значением `Qt::PenStyle`
* `editorEvent()` — открытие `QColorDialog` для `pencolor`
* `paint()` — отображение текстового имени стиля пера вместо числа

### `MyRect`

Структура данных прямоугольника (цвет, стиль, толщина, координаты, размеры), используемая при заполнении таблицы БД.

---

## Таблица БД `rectangle`

Используется таблица `rectangle` со столбцами:

* `id` — ключ (`INTEGER PRIMARY KEY AUTOINCREMENT`)
* `pencolor` — цвет пера
* `penstyle` — стиль пера (`Qt::PenStyle`)
* `penwidth` — толщина пера
* `left`, `top`, `width`, `height` — геометрия прямоугольника

---

## Тестовое покрытие

### `test_smoke`

Проверяет, что QtTest-таргет корректно собирается и запускается.

### `test_mydelegate`

Покрывает:

* создание редактора для нужных/ненужных колонок
* загрузку данных в `QComboBox`
* запись данных обратно в модель
* обработку `editorEvent()` для цвета
* отрисовку `paint()` для `PenStyle` и базовых колонок

### `test_mainwindow`

Покрывает:

* создание/закрытие соединения
* создание/удаление таблицы
* вставку данных и проверку количества строк
* инициализацию `QSqlTableModel`
* вставку/удаление строк через модель

---

## CI (GitHub Actions)

Workflow `.github/workflows/ci.yml` автоматически запускает:

1. конфигурацию проекта (`cmake`)
2. сборку
3. тесты (`ctest`)

Это позволяет быстро проверять, что изменения не ломают сборку и тесты.

---

## Цель проекта

Учебная цель проекта — отработать:

* работу с БД в Qt (`QSqlDatabase`, `QSqlQuery`)
* связывание БД и модели (`QSqlTableModel`)
* отображение данных в `QTableView`
* кастомизацию редактирования через делегаты
* базовую инфраструктуру тестирования и CI

---

## Автор

Святослав Мышковский (Svyatoslav Myshkovskiy)
