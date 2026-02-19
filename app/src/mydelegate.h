#ifndef MYDELEGATE_H
#define MYDELEGATE_H

#include <QStyledItemDelegate>

class QComboBox;

/**
 * @brief Делегат для QTableView, редактирующий и отображающий поля таблицы rectangle.
 *
 * Назначение:
 *  - PenStyle (столбец kPenStyleColumn): редактирование через QComboBox (Qt::PenStyle),
 *    в модель записывается int (значение enum).
 *  - PenColor (столбец kPenColorColumn): выбор цвета через QColorDialog по двойному клику,
 *    в модель записывается QColor (или строка, если модель так настроена).
 *
 * Дополнительно:
 *  - paint(): для PenStyle рисует “ComboBox-подобное” отображение с текстом (SolidLine и т.п.),
 *    чтобы в таблице не показывались голые числа.
 *
 * @note Индексы столбцов должны соответствовать структуре таблицы:
 *       id=0, pencolor=1, penstyle=2, ...
 */
class MyDelegate final : public QStyledItemDelegate
{
    Q_OBJECT
public:
    /// Создаёт делегат. Обычно parent — это view (ui->tableView).
    explicit MyDelegate(QObject* parent = nullptr);

    /**
     * @brief Создаёт редактор для ячейки.
     *
     * Для PenStyle создаётся QComboBox.
     * Для остальных столбцов используется стандартный редактор базового класса.
     */
    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;

    /**
     * @brief Загружает значение из модели в редактор.
     *
     * Для PenStyle читает int из Qt::EditRole и выставляет текущий элемент ComboBox по userData().
     */
    void setEditorData(QWidget* editor,
                       const QModelIndex& index) const override;

    /**
     * @brief Сохраняет значение из редактора в модель.
     *
     * Для PenStyle берёт currentData() и пишет в модель как int (Qt::EditRole).
     */
    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      const QModelIndex& index) const override;

    /**
     * @brief Обрабатывает события редактирования без создания editor-виджета.
     *
     * Для PenColor по двойному клику ЛКМ открывает QColorDialog и пишет выбранный цвет в модель.
     */
    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

    /**
     * @brief Отрисовка ячейки.
     *
     * Для PenStyle рисует текстовый стиль (SolidLine / DashLine / ...) вместо числового значения,
     * имитируя отображение через ComboBox.
     * Для остальных столбцов — стандартная отрисовка базового класса.
     */
    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    /// Столбец pencolor в таблице rectangle (id=0, pencolor=1).
    static constexpr int kPenColorColumn = 1;

    /// Столбец penstyle в таблице rectangle (id=0, penstyle=2).
    static constexpr int kPenStyleColumn = 2;

private:
    /// Заполняет combo вариантами Qt::PenStyle. В userData хранится int(enum).
    static void fillPenStyleCombo(QComboBox* combo);
};

#endif // MYDELEGATE_H
