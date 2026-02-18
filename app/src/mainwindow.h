#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

    // Model
    void onInitTableModel();
    void onSelectTable();
    void onInsertRow();
    void onRemoveRow();

    // Query
    void onDoQuery();

private:
    void setupMenus_();

private:
    Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
