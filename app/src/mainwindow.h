#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql/QSqlTableModel>

// QSqlDatabase нужен в полях/методах класса, поэтому подключаем в .h
#include <QtSql/QSqlDatabase>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // BD
    void onCreateConnection();
    void onCloseConnection();
    void onCreateTable();
    void onInsertInto();
    void onPrintTable();
    void onDropTable();

    // Model (пока заглушки)
    void onInitTableModel();
    void onSelectTable();
    void onInsertRow();
    void onRemoveRow();

    // Query (пока заглушка)
    void onDoQuery();

private:
    void setupMenus_();

    // Проверка соединения (валидность + открытость)
    bool ensureDbOpen_(const char* caller) const;

private:
    Ui::MainWindow *ui;

    // Храним соединение (именованное, чтобы было проще контролировать)
    QSqlDatabase m_db;
    QSqlTableModel* m_model = nullptr;

    // Константы проекта
    static constexpr const char* kConnName_ = "rectangles_conn";
    static constexpr const char* kDbFile_   = "rectangle_data.sqlite";
    static constexpr const char* kTable_    = "rectangle";
};

#endif // MAINWINDOW_H
