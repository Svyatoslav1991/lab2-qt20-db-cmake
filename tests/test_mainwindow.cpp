#include <QtTest/QtTest>

#include <QAction>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QMenuBar>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QTableView>
#include <QTemporaryDir>
#include <QVariant>

#include "mainwindow.h"

/**
 * @brief Интеграционные тесты для MainWindow (Qt Widgets + QtSql + SQLite).
 *
 * @details
 * Тесты проверяют:
 *  - инициализацию окна и меню,
 *  - операции с БД (создание/закрытие соединения, создание/удаление таблицы, вставка данных),
 *  - инициализацию QSqlTableModel и работу с QTableView,
 *  - добавление/удаление строк через модель,
 *  - безопасное поведение "guard"-веток при неготовой БД/модели.
 *
 * @note Внутренние приватные методы setupMenus_() и ensureDbOpen_() напрямую недоступны,
 *       поэтому покрываются косвенно через конструктор и публичные/slot-методы.
 *
 * @note MainWindow использует фиксированные:
 *       - имя соединения: "rectangles_conn"
 *       - файл БД: "rectangle_data.sqlite"
 *       Поэтому каждый тест запускается в отдельной временной рабочей папке.
 */
class TestMainWindow : public QObject
{
    Q_OBJECT

private:
    /// Исходная рабочая директория процесса (восстанавливаем после каждого теста).
    QString m_originalCwd;

    /// Временная директория для текущего теста (чтобы файл SQLite не конфликтовал между тестами).
    QTemporaryDir* m_tempDir = nullptr;

private:
    /**
     * @brief Вызывает private slot MainWindow через метаобъект.
     *
     * @param w Экземпляр MainWindow.
     * @param slotName Имя слота (например, "onCreateConnection").
     * @return true если слот найден и вызван успешно.
     */
    static bool invokeSlot(MainWindow& w, const char* slotName)
    {
        return QMetaObject::invokeMethod(&w, slotName, Qt::DirectConnection);
    }

    /**
     * @brief Возвращает именованное SQL-соединение приложения без автооткрытия.
     *
     * @details
     * Второй параметр false важен: QSqlDatabase::database(name, false)
     * не пытается автоматически открыть закрытое соединение.
     */
    static QSqlDatabase appDb()
    {
        if (!QSqlDatabase::contains("rectangles_conn")) {
            return QSqlDatabase();
        }
        return QSqlDatabase::database("rectangles_conn", false);
    }

    /**
     * @brief Выполняет SQL-запрос COUNT(*) по таблице rectangle.
     *
     * @param db Открытая БД.
     * @return Количество строк в таблице или -1 при ошибке.
     */
    static int countRowsInRectangle(QSqlDatabase& db)
    {
        QSqlQuery q(db);

        if (!q.exec("SELECT COUNT(*) FROM rectangle;")) {
            qWarning() << "COUNT(*) failed:" << q.lastError().text();
            return -1;
        }

        if (!q.next()) {
            qWarning() << "COUNT(*) query returned no rows";
            return -1;
        }

        return q.value(0).toInt();
    }


    /**
     * @brief Возвращает указатель на QTableView из UI главного окна.
     *
     * @details
     * Ожидается, что в .ui файл виджет называется "tableView".
     */
    static QTableView* findTableView(MainWindow& w)
    {
        return w.findChild<QTableView*>("tableView");
    }

    /**
     * @brief Проверяет наличие пункта меню по заголовку.
     *
     * @param bar menuBar окна.
     * @param title Текст меню (например, "BD").
     * @return Указатель на меню или nullptr, если не найдено.
     */
    static QMenu* findMenuByTitle(QMenuBar* bar, const QString& title)
    {
        if (!bar) return nullptr;

        for (QAction* a : bar->actions()) {
            if (!a) continue;
            QMenu* m = a->menu();
            if (m && m->title() == title) {
                return m;
            }
        }
        return nullptr;
    }

    /**
     * @brief Возвращает множество текстов actions меню.
     */
    static QSet<QString> actionTexts(QMenu* menu)
    {
        QSet<QString> result;
        if (!menu) return result;

        for (QAction* a : menu->actions()) {
            if (!a) continue;
            if (a->isSeparator()) continue;
            result.insert(a->text());
        }
        return result;
    }

private slots:
    /**
     * @brief Подготовка окружения перед каждым тестом.
     *
     * @details
     * 1) Сохраняем текущую рабочую директорию.
     * 2) Создаём уникальную временную директорию.
     * 3) Переключаем cwd в неё, чтобы SQLite-файл создавался изолированно.
     */
    void init()
    {
        m_originalCwd = QDir::currentPath();

        m_tempDir = new QTemporaryDir();
        QVERIFY2(m_tempDir->isValid(), "QTemporaryDir is invalid");

        const bool changed = QDir::setCurrent(m_tempDir->path());
        QVERIFY(changed);
    }

    /**
     * @brief Очистка окружения после каждого теста.
     *
     * @details
     * 1) Если именованное соединение осталось зарегистрировано — закрываем и удаляем.
     * 2) Возвращаем исходную рабочую директорию.
     * 3) Удаляем временную директорию.
     *
     * @note removeDatabase(...) требует, чтобы не было живых QSqlDatabase/QSqlQuery,
     *       поэтому используем локальную область видимости.
     */
    void cleanup()
    {
        {
            if (QSqlDatabase::contains("rectangles_conn")) {
                QSqlDatabase db = QSqlDatabase::database("rectangles_conn");
                if (db.isOpen()) {
                    db.close();
                }
            }
        }
        QSqlDatabase::removeDatabase("rectangles_conn");

        if (!m_originalCwd.isEmpty()) {
            QDir::setCurrent(m_originalCwd);
        }

        delete m_tempDir;
        m_tempDir = nullptr;
    }

    /**
     * @brief Конструктор создаёт окно, задаёт заголовок и поднимает menuBar.
     *
     * @details
     * Косвенно покрывает setupMenus_() вызовом из конструктора.
     */
    void test_construct_windowTitle_and_menuBar_exists()
    {
        MainWindow w;

        QCOMPARE(w.windowTitle(), QString("lab2-qt20-db"));
        QVERIFY(w.menuBar() != nullptr);

        // tableView должен существовать в UI
        QTableView* tv = findTableView(w);
        QVERIFY2(tv != nullptr, "tableView not found by objectName='tableView'");
    }

    /**
     * @brief Проверка структуры меню и набора actions.
     *
     * @details
     * Проверяем, что setupMenus_() создал меню:
     *  - BD
     *  - Model
     *  - Query
     * И что внутри присутствуют ожидаемые действия.
     */
    void test_setupMenus_expectedMenusAndActions_exist()
    {
        MainWindow w;
        QMenuBar* bar = w.menuBar();
        QVERIFY(bar != nullptr);

        QMenu* mBd = findMenuByTitle(bar, "BD");
        QMenu* mModel = findMenuByTitle(bar, "Model");
        QMenu* mQuery = findMenuByTitle(bar, "Query");

        QVERIFY(mBd != nullptr);
        QVERIFY(mModel != nullptr);
        QVERIFY(mQuery != nullptr);

        const QSet<QString> bdActions = actionTexts(mBd);
        QVERIFY(bdActions.contains("Create connection"));
        QVERIFY(bdActions.contains("Close connection"));
        QVERIFY(bdActions.contains("Create table"));
        QVERIFY(bdActions.contains("Insert into"));
        QVERIFY(bdActions.contains("Print table"));
        QVERIFY(bdActions.contains("Drop table"));

        const QSet<QString> modelActions = actionTexts(mModel);
        QVERIFY(modelActions.contains("Init table model"));
        QVERIFY(modelActions.contains("Select table"));
        QVERIFY(modelActions.contains("Insert row"));
        QVERIFY(modelActions.contains("Remove row"));

        const QSet<QString> queryActions = actionTexts(mQuery);
        QVERIFY(queryActions.contains("Do query"));
    }

    /**
     * @brief onCloseConnection() безопасен до создания соединения.
     *
     * @details
     * Покрывает guard-ветку:
     *  - m_db невалиден -> ранний выход.
     */
    void test_onCloseConnection_beforeCreate_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCloseConnection"));

        // Именованное соединение не должно появиться само по себе
        QVERIFY(!QSqlDatabase::contains("rectangles_conn"));
    }

    /**
     * @brief onCreateConnection() создаёт и открывает именованное SQLite-соединение.
     *
     * @details
     * Проверяем:
     *  - соединение зарегистрировано в QSqlDatabase
     *  - соединение валидно и открыто
     *  - файл БД создан в текущей (временной) директории
     */
    void test_onCreateConnection_createsAndOpensNamedConnection()
    {
        MainWindow w;

        QVERIFY(invokeSlot(w, "onCreateConnection"));

        QVERIFY(QSqlDatabase::contains("rectangles_conn"));

        QSqlDatabase db = appDb();
        QVERIFY(db.isValid());
        QVERIFY(db.isOpen());

        QCOMPARE(db.connectionName(), QString("rectangles_conn"));
        QCOMPARE(db.databaseName(), QString("rectangle_data.sqlite"));

        QFile dbFile(QDir::current().filePath("rectangle_data.sqlite"));
        QVERIFY(dbFile.exists());
    }

    /**
     * @brief Повторный onCreateConnection() переиспользует уже зарегистрированное соединение.
     *
     * @details
     * Проверяем, что после второго вызова соединение остаётся валидным/открытым.
     * Это косвенно покрывает ветку QSqlDatabase::contains(kConnName_).
     */
    void test_onCreateConnection_reuseExistingConnection()
    {
        MainWindow w;

        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateConnection"));

        QSqlDatabase db = appDb();
        QVERIFY(db.isValid());
        QVERIFY(db.isOpen());
        QCOMPARE(db.connectionName(), QString("rectangles_conn"));
    }

    /**
     * @brief onCloseConnection() закрывает открытое соединение.
     */
    void test_onCloseConnection_afterCreate_closesDb()
    {
        MainWindow w;

        QVERIFY(invokeSlot(w, "onCreateConnection"));

        QSqlDatabase dbBefore = appDb();
        QVERIFY(dbBefore.isOpen());

        QVERIFY(invokeSlot(w, "onCloseConnection"));

        QSqlDatabase dbAfter = appDb();
        QVERIFY(dbAfter.isValid());
        QVERIFY(!dbAfter.isOpen());
    }

    /**
     * @brief onCreateTable() без открытой БД безопасно завершает работу.
     *
     * @details
     * Покрывает guard через ensureDbOpen_().
     */
    void test_onCreateTable_withoutConnection_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateTable"));

        // Соединение не должно появиться и таблица тоже
        QVERIFY(!QSqlDatabase::contains("rectangles_conn"));
    }

    /**
     * @brief onCreateTable() создаёт таблицу rectangle.
     */
    void test_onCreateTable_createsRectangleTable()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));

        QSqlDatabase db = appDb();
        QVERIFY(db.isOpen());

        const QStringList tables = db.tables();
        QVERIFY(tables.contains("rectangle"));
    }

    /**
     * @brief Повторный onCreateTable() удаляет старую таблицу и создаёт заново.
     *
     * @details
     * Проверяем ветку:
     *  - table exists -> onDropTable() -> create again
     */
    void test_onCreateTable_whenExists_recreatesTable()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));

        // Вставим одну строку вручную, чтобы убедиться, что таблица действительно пересоздалась.
        {
            QSqlDatabase db = appDb();
            QSqlQuery q(db);
            QVERIFY2(q.exec("INSERT INTO rectangle (pencolor, penstyle, penwidth, left, top, width, height) "
                            "VALUES ('#ffffff', 1, 1, 1, 1, 1, 1);"),
                     qPrintable(q.lastError().text()));
        }

        {
            QSqlDatabase db = appDb();
            QCOMPARE(countRowsInRectangle(db), 1);
        }

        // Повторный create должен дропнуть и создать пустую таблицу
        QVERIFY(invokeSlot(w, "onCreateTable"));

        {
            QSqlDatabase db = appDb();
            QVERIFY(db.tables().contains("rectangle"));
            QCOMPARE(countRowsInRectangle(db), 0);
        }
    }

    /**
     * @brief onDropTable() без открытой БД безопасен.
     */
    void test_onDropTable_withoutConnection_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onDropTable"));
        QVERIFY(!QSqlDatabase::contains("rectangles_conn"));
    }

    /**
     * @brief onDropTable() при отсутствии таблицы безопасен.
     *
     * @details
     * Покрывает ветку:
     *  - table does not exist -> return
     */
    void test_onDropTable_whenTableMissing_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));

        QSqlDatabase db = appDb();
        QVERIFY(!db.tables().contains("rectangle"));

        QVERIFY(invokeSlot(w, "onDropTable"));

        QVERIFY(!db.tables().contains("rectangle"));
    }

    /**
     * @brief onDropTable() удаляет существующую таблицу rectangle.
     */
    void test_onDropTable_removesTable()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));

        QSqlDatabase db = appDb();
        QVERIFY(db.tables().contains("rectangle"));

        QVERIFY(invokeSlot(w, "onDropTable"));

        QSqlDatabase db2 = appDb();
        QVERIFY(!db2.tables().contains("rectangle"));
    }

    /**
     * @brief onInsertInto() без открытой БД безопасен.
     */
    void test_onInsertInto_withoutConnection_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onInsertInto"));
        QVERIFY(!QSqlDatabase::contains("rectangles_conn"));
    }

    /**
     * @brief onInsertInto() без таблицы безопасен.
     *
     * @details
     * Покрывает ветку:
     *  - table does not exist -> return
     */
    void test_onInsertInto_withoutTable_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));

        QVERIFY(invokeSlot(w, "onInsertInto"));

        QSqlDatabase db = appDb();
        QVERIFY(!db.tables().contains("rectangle"));
    }

    /**
     * @brief onInsertInto() вставляет все тестовые записи.
     *
     * @details
     * В коде вставляется:
     *  1) один простой INSERT
     *  2) 3 записи через named bindValue
     *  3) 3 записи через addBindValue
     *  4) 3 записи через positional bindValue
     *
     * Итого ожидаем 10 строк.
     */
    void test_onInsertInto_insertsTenRows()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));
        QVERIFY(invokeSlot(w, "onInsertInto"));

        QSqlDatabase db = appDb();
        QCOMPARE(countRowsInRectangle(db), 10);
    }

    /**
     * @brief onPrintTable() без открытой БД безопасен.
     */
    void test_onPrintTable_withoutConnection_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onPrintTable"));
        QVERIFY(!QSqlDatabase::contains("rectangles_conn"));
    }

    /**
     * @brief onPrintTable() без таблицы безопасен.
     */
    void test_onPrintTable_withoutTable_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onPrintTable"));
    }

    /**
     * @brief onPrintTable() после вставки данных выполняется без падений.
     *
     * @details
     * qDebug-вывод отдельно не перехватываем; функционально проверяем, что
     * SELECT уже корректен (через countRowsInRectangle) и сам слот исполняется.
     */
    void test_onPrintTable_withData_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));
        QVERIFY(invokeSlot(w, "onInsertInto"));

        QSqlDatabase db = appDb();
        QCOMPARE(countRowsInRectangle(db), 10);

        QVERIFY(invokeSlot(w, "onPrintTable"));
    }

    /**
     * @brief onInitTableModel() без БД безопасен.
     */
    void test_onInitTableModel_withoutConnection_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onInitTableModel"));

        QTableView* tv = findTableView(w);
        QVERIFY(tv != nullptr);
        QVERIFY(tv->model() == nullptr);
    }

    /**
     * @brief onInitTableModel() без таблицы безопасен.
     */
    void test_onInitTableModel_withoutTable_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onInitTableModel"));

        QTableView* tv = findTableView(w);
        QVERIFY(tv != nullptr);
        QVERIFY(tv->model() == nullptr);
    }

    /**
     * @brief onInitTableModel() создаёт QSqlTableModel, подключает к QTableView и загружает данные.
     *
     * @details
     * Проверяем:
     *  - модель создана и установлена в tableView
     *  - таблица "rectangle" выбрана
     *  - количество строк совпадает с данными в БД
     *  - заголовки установлены
     *  - колонка ID скрыта
     *  - делегаты для колонок 1 и 2 установлены
     */
    void test_onInitTableModel_initializesViewAndModel()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));
        QVERIFY(invokeSlot(w, "onInsertInto"));

        QVERIFY(invokeSlot(w, "onInitTableModel"));

        QTableView* tv = findTableView(w);
        QVERIFY(tv != nullptr);
        QVERIFY(tv->model() != nullptr);

        auto* model = qobject_cast<QSqlTableModel*>(tv->model());
        QVERIFY(model != nullptr);

        QCOMPARE(model->tableName(), QString("rectangle"));
        QCOMPARE(model->rowCount(), 10);

        QCOMPARE(model->headerData(0, Qt::Horizontal).toString(), QString("ID"));
        QCOMPARE(model->headerData(1, Qt::Horizontal).toString(), QString("Color"));
        QCOMPARE(model->headerData(2, Qt::Horizontal).toString(), QString("Style"));
        QCOMPARE(model->headerData(3, Qt::Horizontal).toString(), QString("PenWidth"));
        QCOMPARE(model->headerData(4, Qt::Horizontal).toString(), QString("Left"));
        QCOMPARE(model->headerData(5, Qt::Horizontal).toString(), QString("Top"));
        QCOMPARE(model->headerData(6, Qt::Horizontal).toString(), QString("Width"));
        QCOMPARE(model->headerData(7, Qt::Horizontal).toString(), QString("Height"));

        QVERIFY(tv->isColumnHidden(0));

        QVERIFY(tv->itemDelegateForColumn(1) != nullptr);
        QVERIFY(tv->itemDelegateForColumn(2) != nullptr);
    }

    /**
     * @brief Повторный onInitTableModel() переиспользует уже созданную модель.
     *
     * @details
     * Покрывает ветку:
     *  - if (!m_model) { ... } // false
     */
    void test_onInitTableModel_secondCall_reusesExistingModel()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));
        QVERIFY(invokeSlot(w, "onInsertInto"));

        QVERIFY(invokeSlot(w, "onInitTableModel"));

        QTableView* tv = findTableView(w);
        QVERIFY(tv != nullptr);

        QObject* firstModel = tv->model();
        QVERIFY(firstModel != nullptr);

        QVERIFY(invokeSlot(w, "onInitTableModel"));

        QObject* secondModel = tv->model();
        QCOMPARE(secondModel, firstModel);
    }

    /**
     * @brief onInsertRow() без открытой БД безопасен.
     */
    void test_onInsertRow_withoutConnection_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onInsertRow"));
    }

    /**
     * @brief onInsertRow() без инициализированной модели безопасен.
     *
     * @details
     * Покрывает ветку:
     *  - if (!m_model) return;
     */
    void test_onInsertRow_withoutModel_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));

        QVERIFY(invokeSlot(w, "onInsertRow"));

        // База ещё пустая, модель не инициализирована
        QSqlDatabase db = appDb();
        QCOMPARE(countRowsInRectangle(db), 0);
    }

    /**
     * @brief onInsertRow() добавляет строку в модель.
     *
     * @details
     * Проверяем изменение rowCount() модели.
     * При стратегии OnRowChange запись в БД может быть отложена до ухода со строки,
     * поэтому здесь проверяем именно модель.
     */
    void test_onInsertRow_withModel_increasesModelRowCount()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));
        QVERIFY(invokeSlot(w, "onInsertInto"));
        QVERIFY(invokeSlot(w, "onInitTableModel"));

        QTableView* tv = findTableView(w);
        auto* model = qobject_cast<QSqlTableModel*>(tv->model());
        QVERIFY(model != nullptr);

        const int before = model->rowCount();

        QVERIFY(invokeSlot(w, "onInsertRow"));

        const int after = model->rowCount();
        QCOMPARE(after, before + 1);

        QVERIFY(tv->currentIndex().isValid());
        QCOMPARE(tv->currentIndex().row(), before); // выбрана новая строка
    }

    /**
     * @brief onRemoveRow() без открытой БД безопасен.
     */
    void test_onRemoveRow_withoutConnection_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onRemoveRow"));
    }

    /**
     * @brief onRemoveRow() без модели безопасен.
     */
    void test_onRemoveRow_withoutModel_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));

        QVERIFY(invokeSlot(w, "onRemoveRow"));
    }

    /**
     * @brief onRemoveRow() без выбранной строки безопасен.
     *
     * @details
     * Покрывает ветку:
     *  - currentIndex invalid -> return
     */
    void test_onRemoveRow_noSelection_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));
        QVERIFY(invokeSlot(w, "onInsertInto"));
        QVERIFY(invokeSlot(w, "onInitTableModel"));

        QTableView* tv = findTableView(w);
        QVERIFY(tv != nullptr);

        tv->clearSelection();
        tv->setCurrentIndex(QModelIndex()); // явно снимаем текущий индекс

        auto* model = qobject_cast<QSqlTableModel*>(tv->model());
        QVERIFY(model != nullptr);
        const int before = model->rowCount();

        QVERIFY(invokeSlot(w, "onRemoveRow"));

        QCOMPARE(model->rowCount(), before);
    }

    /**
     * @brief onRemoveRow() удаляет выбранную строку из модели/БД.
     *
     * @details
     * Для QSqlTableModel при стратегии OnRowChange визуальное/модельное количество строк
     * может обновляться не мгновенно. Поэтому после удаления делаем select() и проверяем:
     *  - количество строк в модели
     *  - количество строк в БД
     */
    void test_onRemoveRow_withSelection_decreasesModelRowCount()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onCreateConnection"));
        QVERIFY(invokeSlot(w, "onCreateTable"));
        QVERIFY(invokeSlot(w, "onInsertInto"));
        QVERIFY(invokeSlot(w, "onInitTableModel"));

        QTableView* tv = findTableView(w);
        QVERIFY(tv != nullptr);

        auto* model = qobject_cast<QSqlTableModel*>(tv->model());
        QVERIFY(model != nullptr);

        const int beforeModel = model->rowCount();
        QVERIFY(beforeModel > 0);

        QSqlDatabase db = appDb();
        const int beforeDb = countRowsInRectangle(db);
        QVERIFY(beforeDb >= 0);
        QCOMPARE(beforeDb, beforeModel);

        // Выбираем первую строку
        tv->selectRow(0);
        QVERIFY(tv->currentIndex().isValid());
        QCOMPARE(tv->currentIndex().row(), 0);

        QVERIFY(invokeSlot(w, "onRemoveRow"));

        // Принудительно обновим модель из БД
        QVERIFY(model->select());

        const int afterModel = model->rowCount();
        const int afterDb = countRowsInRectangle(db);
        QVERIFY(afterDb >= 0);

        QCOMPARE(afterModel, beforeModel - 1);
        QCOMPARE(afterDb, beforeDb - 1);
    }


    /**
     * @brief onSelectTable() (заглушка) вызывается без падений.
     */
    void test_onSelectTable_stub_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onSelectTable"));
    }

    /**
     * @brief onDoQuery() (заглушка) вызывается без падений.
     */
    void test_onDoQuery_stub_safe()
    {
        MainWindow w;
        QVERIFY(invokeSlot(w, "onDoQuery"));
    }
};

QTEST_MAIN(TestMainWindow)
#include "test_mainwindow.moc"
