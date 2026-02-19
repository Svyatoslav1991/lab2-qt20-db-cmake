#include "mainwindow.h"
#include "ui_mainwindow.h"

// Реализация MainWindow: меню + операции с SQLite (QtSql) + отображение через QSqlTableModel.

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QMenuBar>

#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

#include "mydelegate.h"
#include "myrect.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupMenus_();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// -------------------- helpers --------------------

/**
 * @brief Проверяет, что соединение с БД создано и открыто.
 *
 * Используется как “guard” в публичных слотах, чтобы не падать и не выполнять SQL,
 * когда пользователь ещё не вызвал Create connection или уже закрыл соединение.
 */
bool MainWindow::ensureDbOpen_(const char* caller) const
{
    if (!m_db.isValid()) {
        qDebug() << caller << ": DB is not valid. Call BD -> Create connection.";
        return false;
    }
    if (!m_db.isOpen()) {
        qDebug() << caller << ": DB is not open. Call BD -> Create connection.";
        return false;
    }
    return true;
}

// -------------------- BD --------------------

/**
 * @brief Создаёт/переиспользует именованное соединение с SQLite и открывает его.
 *
 * @note Если соединение с таким именем уже зарегистрировано, берём его через database().
 *       Иначе создаём новое через addDatabase().
 */
void MainWindow::onCreateConnection()
{
    // Создаём/переиспользуем именованное соединение
    if (QSqlDatabase::contains(kConnName_)) {
        m_db = QSqlDatabase::database(kConnName_);
        qDebug() << "onCreateConnection: reuse connection" << m_db.connectionName();
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", kConnName_);
        qDebug() << "onCreateConnection: addDatabase QSQLITE conn=" << kConnName_;
    }

    if (!m_db.isValid()) {
        qDebug() << "onCreateConnection: invalid connection";
        qDebug() << "lastError:" << m_db.lastError().text();
        return;
    }

    m_db.setDatabaseName(kDbFile_);

    if (!m_db.open()) {
        qDebug() << "onCreateConnection: open() failed";
        qDebug() << "lastError:" << m_db.lastError().text();
        return;
    }

    qDebug() << "onCreateConnection: OK. db=" << m_db.databaseName();
    qDebug() << "tables:" << m_db.tables();
}

void MainWindow::onCloseConnection()
{
    if (!m_db.isValid()) {
        qDebug() << "onCloseConnection: DB is not valid";
        return;
    }

    if (m_db.isOpen()) {
        m_db.close();
        qDebug() << "onCloseConnection: closed";
    } else {
        qDebug() << "onCloseConnection: already closed";
    }
}

void MainWindow::onCreateTable()
{
    if (!ensureDbOpen_("onCreateTable")) return;

    // Если таблица уже была — удаляем (как требует задание)
    if (m_db.tables().contains(kTable_)) {
        qDebug() << "onCreateTable: table exists, dropping first...";
        onDropTable();
        if (!ensureDbOpen_("onCreateTable(after drop)")) return;
    }

    const QString sql =
            "CREATE TABLE rectangle ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " pencolor VARCHAR,"
            " penstyle INTEGER,"
            " penwidth INTEGER,"
            " left INTEGER,"
            " top INTEGER,"
            " width INTEGER,"
            " height INTEGER"
            ");";

    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        qDebug() << "onCreateTable: CREATE TABLE failed:" << q.lastError().text();
        return;
    }

    qDebug() << "onCreateTable: OK";
    qDebug() << "tables:" << m_db.tables();
}

void MainWindow::onDropTable()
{
    if (!ensureDbOpen_("onDropTable")) return;

    if (!m_db.tables().contains(kTable_)) {
        qDebug() << "onDropTable: table does not exist";
        return;
    }

    QSqlQuery q(m_db);
    if (!q.exec("DROP TABLE rectangle;")) {
        qDebug() << "onDropTable: DROP TABLE failed:" << q.lastError().text();
        return;
    }

    qDebug() << "onDropTable: OK";
    qDebug() << "tables:" << m_db.tables();
}

void MainWindow::onInsertInto()
{
    if (!ensureDbOpen_("onInsertInto")) return;

    if (!m_db.tables().contains(kTable_)) {
        qDebug() << "onInsertInto: table does not exist. Call BD -> Create table first.";
        return;
    }

    // 1) Один прямоугольник: INSERT ... VALUES
    {
        QSqlQuery q(m_db);
        const QString sql =
                "INSERT INTO rectangle (pencolor, penstyle, penwidth, left, top, width, height) "
                "VALUES ('#ff0000', 1, 3, 10, 20, 60, 60);";
        if (!q.exec(sql)) {
            qDebug() << "onInsertInto: simple INSERT failed:" << q.lastError().text();
            return;
        }
        qDebug() << "onInsertInto: simple INSERT OK";
    }

    // Данные (MyRect из ЛР1)
    const QVector<MyRect> rects = {
        MyRect(QColor("#00ff00"), Qt::SolidLine, 2,  0,  0, 200, 100),
        MyRect(QColor("#0000ff"), Qt::DashLine,  1, 10, 20,  60,  60),
        MyRect(QColor("#aaaaaa"), Qt::DotLine,   4, 50, 70,  30,  90),
    };

    // 2) prepare + bindValue(":name", ...)
    {
        QSqlQuery q(m_db);
        q.prepare(
                    "INSERT INTO rectangle (pencolor, penstyle, penwidth, left, top, width, height) "
                    "VALUES (:pencolor, :penstyle, :penwidth, :left, :top, :width, :height)"
                    );

        for (const auto& r : rects) {
            q.bindValue(":pencolor", r.penColor.name());
            q.bindValue(":penstyle", static_cast<int>(r.penStyle));
            q.bindValue(":penwidth", r.penWidth);
            q.bindValue(":left",     r.left);
            q.bindValue(":top",      r.top);
            q.bindValue(":width",    r.width);
            q.bindValue(":height",   r.height);

            if (!q.exec()) {
                qDebug() << "onInsertInto: named bindValue failed:" << q.lastError().text();
                return;
            }
        }
        qDebug() << "onInsertInto: named bindValue OK";
    }

    // 3) prepare + addBindValue (позиционные '?')
    {
        QSqlQuery q(m_db);
        q.prepare(
                    "INSERT INTO rectangle (pencolor, penstyle, penwidth, left, top, width, height) "
                    "VALUES (?,?,?,?,?,?,?)"
                    );

        for (const auto& r : rects) {
            q.addBindValue(r.penColor.name());
            q.addBindValue(static_cast<int>(r.penStyle));
            q.addBindValue(r.penWidth);
            q.addBindValue(r.left);
            q.addBindValue(r.top);
            q.addBindValue(r.width);
            q.addBindValue(r.height);

            if (!q.exec()) {
                qDebug() << "onInsertInto: addBindValue failed:" << q.lastError().text();
                return;
            }
        }
        qDebug() << "onInsertInto: addBindValue OK";
    }

    // 4) prepare + bindValue(pos, ...) (позиционный bindValue)
    {
        QSqlQuery q(m_db);
        q.prepare(
                    "INSERT INTO rectangle (pencolor, penstyle, penwidth, left, top, width, height) "
                    "VALUES (?,?,?,?,?,?,?)"
                    );

        for (const auto& r : rects) {
            q.bindValue(0, r.penColor.name());
            q.bindValue(1, static_cast<int>(r.penStyle));
            q.bindValue(2, r.penWidth);
            q.bindValue(3, r.left);
            q.bindValue(4, r.top);
            q.bindValue(5, r.width);
            q.bindValue(6, r.height);

            if (!q.exec()) {
                qDebug() << "onInsertInto: positional bindValue failed:" << q.lastError().text();
                return;
            }
        }
        qDebug() << "onInsertInto: positional bindValue OK";
    }

    qDebug() << "onInsertInto: DONE";
}

void MainWindow::onPrintTable()
{
    if (!ensureDbOpen_("onPrintTable")) return;

    if (!m_db.tables().contains(kTable_)) {
        qDebug() << "onPrintTable: table does not exist. Call BD -> Create table first.";
        return;
    }

    QSqlQuery q(m_db);
    if (!q.exec("SELECT * FROM rectangle;")) {
        qDebug() << "onPrintTable: SELECT failed:" << q.lastError().text();
        return;
    }

    // SELECT * -> индексы по именам через record().indexOf()
    const QSqlRecord rec = q.record();
    const int idCol       = rec.indexOf("id");
    const int colorCol    = rec.indexOf("pencolor");
    const int styleCol    = rec.indexOf("penstyle");
    const int penWidthCol = rec.indexOf("penwidth");
    const int leftCol     = rec.indexOf("left");
    const int topCol      = rec.indexOf("top");
    const int widthCol    = rec.indexOf("width");
    const int heightCol   = rec.indexOf("height");

    qDebug() << "onPrintTable: rows:";
    while (q.next()) {
        qDebug()
                << "id="    << q.value(idCol).toInt()
                << "color=" << q.value(colorCol).toString()
                << "style=" << q.value(styleCol).toInt()
                << "pW="    << q.value(penWidthCol).toInt()
                << "rect=(" << q.value(leftCol).toInt()
                << ","      << q.value(topCol).toInt()
                << ","      << q.value(widthCol).toInt()
                << ","      << q.value(heightCol).toInt()
                << ")";
    }
}

// -------------------- Model (пока заглушки) --------------------

void MainWindow::onInitTableModel() {
    if (!ensureDbOpen_("onInitTableModel")) return;

    if (!m_db.tables().contains(kTable_)) {
        qDebug() << "onInitTableModel: table does not exist. Call BD -> Create table first.";
        return;
    }

    if (!m_model) {
        m_model = new QSqlTableModel(this, m_db);
        ui->tableView->setModel(m_model);
    }

    m_model->setTable(kTable_);

    // стратегия редактирования (методичка: можно выбрать)
    // 0: OnFieldChange — сразу пишет в БД
    // 1: OnRowChange — пишет при уходе со строки (default)
    // 2: OnManualSubmit — по submitAll()
    m_model->setEditStrategy(QSqlTableModel::OnRowChange);

    if (!m_model->select()) {
        qDebug() << "onInitTableModel: select() failed:" << m_model->lastError().text();
        return;
    }

    // Заголовки (setHeaderData унаследован от QAbstractItemModel)
    m_model->setHeaderData(0, Qt::Horizontal, "ID");
    m_model->setHeaderData(1, Qt::Horizontal, "Color");
    m_model->setHeaderData(2, Qt::Horizontal, "Style");
    m_model->setHeaderData(3, Qt::Horizontal, "PenWidth");
    m_model->setHeaderData(4, Qt::Horizontal, "Left");
    m_model->setHeaderData(5, Qt::Horizontal, "Top");
    m_model->setHeaderData(6, Qt::Horizontal, "Width");
    m_model->setHeaderData(7, Qt::Horizontal, "Height");

    // временно скрыть id (как в методичке)
    ui->tableView->hideColumn(0);

    // делегаты (ВАЖНО: индексы смещены на +1 из-за id)
    ui->tableView->setItemDelegateForColumn(1, new MyDelegate(this));
    ui->tableView->setItemDelegateForColumn(2, new MyDelegate(this));

    ui->tableView->resizeColumnsToContents();

    qDebug() << "onInitTableModel: loaded rows=" << m_model->rowCount();
}
void MainWindow::onSelectTable()    { qDebug() << "Model: Select table"; }

void MainWindow::onInsertRow()      {
    if (!ensureDbOpen_("onInsertRow")) return;
    if (!m_model) {
        qDebug() << "onInsertRow: model not initialized. Use Model -> Table model first.";
        return;
    }

    const int row = m_model->rowCount(); // куда вставляем (в конец)
    if (!m_model->insertRows(row, 1)) {  // insertRows(row, count) — добавление строк в модель.
        qDebug() << "onInsertRow: insertRows failed:" << m_model->lastError().text();
        return;
    }

    // Поставим курсор на новую строку (удобно)
    ui->tableView->selectRow(row);
    ui->tableView->scrollTo(m_model->index(row, 1));

    qDebug() << "onInsertRow: inserted row=" << row;
}

void MainWindow::onRemoveRow()      {
    if (!ensureDbOpen_("onRemoveRow")) return;
    if (!m_model) {
        qDebug() << "onRemoveRow: model not initialized. Use Model -> Table model first.";
        return;
    }

    const QModelIndex cur = ui->tableView->currentIndex(); // currentIndex() ([doc.qt.io](https://doc.qt.io/qt-5/qabstractitemview.html?utm_source=chatgpt.com))
    if (!cur.isValid()) {
        qDebug() << "onRemoveRow: no current row selected";
        return;
    }

    const int row = cur.row();
    if (!m_model->removeRows(row, 1)) { // removeRows(row, count) ([doc.qt.io](https://doc.qt.io/qt-5/qsqltablemodel.html?utm_source=chatgpt.com))
        qDebug() << "onRemoveRow: removeRows failed:" << m_model->lastError().text();
        return;
    }

    qDebug() << "onRemoveRow: removed row=" << row;
}

// -------------------- Query (пока заглушка) --------------------

void MainWindow::onDoQuery()        { qDebug() << "Query: Do query"; }

// -------------------- menus --------------------

void MainWindow::setupMenus_()
{
    // --- BD ---
    QMenu* mBd = menuBar()->addMenu("BD");
    QAction* aCreateConn = mBd->addAction("Create connection");
    QAction* aCloseConn  = mBd->addAction("Close connection");
    mBd->addSeparator();
    QAction* aCreateTbl  = mBd->addAction("Create table");
    QAction* aInsertInto = mBd->addAction("Insert into");
    QAction* aPrintTbl   = mBd->addAction("Print table");
    QAction* aDropTbl    = mBd->addAction("Drop table");

    // --- Model ---
    QMenu* mModel = menuBar()->addMenu("Model");
    QAction* aInitModel   = mModel->addAction("Init table model");
    QAction* aSelectTable = mModel->addAction("Select table");
    QAction* aInsertRow   = mModel->addAction("Insert row");
    QAction* aRemoveRow   = mModel->addAction("Remove row");

    // --- Query ---
    QMenu* mQuery = menuBar()->addMenu("Query");
    QAction* aDoQuery = mQuery->addAction("Do query");

    // Связи: triggered -> слоты
    connect(aCreateConn, &QAction::triggered, this, &MainWindow::onCreateConnection);
    connect(aCloseConn,  &QAction::triggered, this, &MainWindow::onCloseConnection);
    connect(aCreateTbl,  &QAction::triggered, this, &MainWindow::onCreateTable);
    connect(aInsertInto, &QAction::triggered, this, &MainWindow::onInsertInto);
    connect(aPrintTbl,   &QAction::triggered, this, &MainWindow::onPrintTable);
    connect(aDropTbl,    &QAction::triggered, this, &MainWindow::onDropTable);

    connect(aInitModel,   &QAction::triggered, this, &MainWindow::onInitTableModel);
    connect(aSelectTable, &QAction::triggered, this, &MainWindow::onSelectTable);
    connect(aInsertRow,   &QAction::triggered, this, &MainWindow::onInsertRow);
    connect(aRemoveRow,   &QAction::triggered, this, &MainWindow::onRemoveRow);

    connect(aDoQuery, &QAction::triggered, this, &MainWindow::onDoQuery);
}
