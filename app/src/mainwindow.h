#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlTableModel>

namespace Ui {
class MainWindow;
}

/**
 * @brief Главное окно приложения (ЛР2): работа с SQLite через QtSql.
 *
 * Окно предоставляет меню и набор слотов для пошагового выполнения операций с БД:
 *  - создание/закрытие соединения (QSqlDatabase),
 *  - создание/удаление таблицы,
 *  - вставка тестовых данных разными способами (QSqlQuery),
 *  - выборка и печать таблицы в qDebug(),
 *  - отображение таблицы через QSqlTableModel + QTableView,
 *  - добавление/удаление строк через модель.
 *
 * @note Соединение используется именованное (kConnName_), чтобы:
 *  - можно было переиспользовать его при повторных вызовах createConnection,
 *  - было проще контролировать жизненный цикл соединения.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор главного окна.
     * @param parent Родительский виджет.
     *
     * Создаёт UI, настраивает меню и привязывает действия меню к слотам.
     * Логику БД в конструкторе не выполняет (по методичке — всё по команде пользователя).
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Деструктор.
     *
     * Освобождает UI.
     * @note m_model имеет parent = this (создаётся как QObject), поэтому удалится автоматически.
     */
    ~MainWindow();

private slots:
    // -------------------- BD --------------------

    /**
     * @brief Создаёт (или переиспользует) соединение с SQLite и открывает его.
     *
     * Шаги:
     *  - addDatabase("QSQLITE", kConnName_) или database(kConnName_) если уже существует,
     *  - setDatabaseName(kDbFile_),
     *  - open().
     *
     * В случае ошибок пишет диагностику через qDebug().
     */
    void onCreateConnection();

    /**
     * @brief Закрывает соединение с БД (m_db.close()).
     *
     * @note Соединение остаётся зарегистрированным в QSqlDatabase, но физически закрыто.
     */
    void onCloseConnection();

    /**
     * @brief Создаёт таблицу kTable_ ("rectangle") командой CREATE TABLE.
     *
     * Если таблица уже существует — удаляет её (onDropTable()) и создаёт заново.
     */
    void onCreateTable();

    /**
     * @brief Заполняет таблицу тестовыми данными.
     *
     * Демонстрирует варианты:
     *  1) INSERT ... VALUES (простой запрос),
     *  2) prepare + bindValue(":name", ...),
     *  3) prepare + addBindValue(...) (позиционные "?"),
     *  4) prepare + bindValue(pos, ...) (позиционный bindValue).
     *
     * @note Цвет сохраняется в БД строкой "#rrggbb" (через QColor::name()).
     */
    void onInsertInto();

    /**
     * @brief Делает SELECT * FROM kTable_ и печатает строки в qDebug().
     *
     * Для SELECT * порядок колонок не фиксирован — используется QSqlRecord::indexOf().
     */
    void onPrintTable();

    /**
     * @brief Удаляет таблицу kTable_ командой DROP TABLE.
     */
    void onDropTable();

    // -------------------- Model --------------------

    /**
     * @brief Инициализирует QSqlTableModel и показывает таблицу в QTableView.
     *
     * Шаги:
     *  - создаёт модель (если ещё не создана),
     *  - setTable(kTable_),
     *  - setEditStrategy(...),
     *  - select(),
     *  - задаёт заголовки колонок,
     *  - (опционально) скрывает ID,
     *  - ставит делегаты на колонки цвета и стиля,
     *  - приводит размеры столбцов.
     */
    void onInitTableModel();

    /**
     * @brief Заглушка/точка расширения: выбор другой таблицы или повторный select().
     */
    void onSelectTable();

    /**
     * @brief Добавляет одну строку в конец модели (insertRows(rowCount, 1)).
     *
     * @note В зависимости от стратегии редактирования изменения попадут в БД:
     *  - OnFieldChange — сразу,
     *  - OnRowChange — при уходе со строки,
     *  - OnManualSubmit — после submitAll().
     */
    void onInsertRow();

    /**
     * @brief Удаляет текущую выбранную строку из модели (removeRows(row, 1)).
     */
    void onRemoveRow();

    // -------------------- Query --------------------

    /**
     * @brief Заглушка/точка расширения: выполнение произвольного SQL-запроса.
     */
    void onDoQuery();

private:
    /**
     * @brief Создаёт меню (BD/Model/Query) и соединяет QAction::triggered со слотами.
     */
    void setupMenus_();

    /**
     * @brief Проверяет готовность соединения перед SQL-операциями.
     *
     * Проверяет:
     *  - m_db.isValid()
     *  - m_db.isOpen()
     *
     * @param caller Имя вызывающей функции (для диагностики).
     * @return true если соединение валидно и открыто, иначе false.
     */
    bool ensureDbOpen_(const char* caller) const;

private:
    Ui::MainWindow *ui = nullptr;

    /**
     * @brief Текущее соединение с БД (именованное).
     */
    QSqlDatabase m_db;

    /**
     * @brief Табличная модель для отображения/редактирования одной таблицы.
     *
     * Создаётся лениво в onInitTableModel(). Владелец — QObject parent = this.
     */
    QSqlTableModel* m_model = nullptr;

    // -------------------- constants --------------------

    /// Имя соединения (именованное), используемое в QSqlDatabase.
    static constexpr const char* kConnName_ = "rectangles_conn";
    /// Имя файла SQLite (создастся рядом с исполняемым файлом, если пути не указаны явно).
    static constexpr const char* kDbFile_   = "rectangle_data.sqlite";
    /// Имя таблицы с прямоугольниками.
    static constexpr const char* kTable_    = "rectangle";
};

#endif // MAINWINDOW_H
