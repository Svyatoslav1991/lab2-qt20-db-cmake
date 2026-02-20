#include <QtTest/QtTest>

#include <QComboBox>
#include <QImage>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTableView>
#include <QMouseEvent>

#include "mydelegate.h"

/**
 * @brief Тесты для MyDelegate.
 *
 * Проверяем:
 *  - создание редактора для столбца penstyle (QComboBox)
 *  - загрузку данных из модели в редактор
 *  - сохранение данных из редактора в модель
 *  - обработку "не наших" событий в editorEvent()
 *  - отрисовку (paint) без падений
 *
 * @note Полные ветки editorEvent() с QColorDialog::getColor() (выбор/отмена цвета)
 *       без рефакторинга стабильно не покрываются, потому что это статический модальный диалог.
 */
class TestMyDelegate : public QObject
{
    Q_OBJECT

private:
    /**
     * @brief Заполняет переданную модель тестовыми данными (1 строка, 3 столбца).
     *
     * @details
     * Формат колонок:
     *  - 0: id (условное целочисленное значение)
     *  - 1: penColor (QColor)
     *  - 2: penStyle (int, значение Qt::PenStyle)
     *
     * @param model Модель, которую нужно подготовить для тестов делегата.
     */
    static void fillModel(QStandardItemModel& model)
    {
        model.setRowCount(1);
        model.setColumnCount(3);

        // col 0: id / обычное поле
        model.setData(model.index(0, 0), 123, Qt::EditRole);

        // col 1: pencolor
        model.setData(model.index(0, 1), QColor(Qt::red), Qt::EditRole);

        // col 2: penstyle
        model.setData(model.index(0, 2), static_cast<int>(Qt::SolidLine), Qt::EditRole);
    }

private slots:
    /**
     * @brief Smoke-проверка конструктора делегата.
     *
     * @details
     * Проверяем, что объект делегата создаётся и не падает.
     */
    void test_ctor_constructs()
    {
        MyDelegate delegate;
        QVERIFY(true);
    }

    /**
     * @brief createEditor(): для столбца penstyle создаётся QComboBox.
     *
     * @details
     * Ожидаем:
     *  - не nullptr
     *  - тип QComboBox
     *  - combo не editable
     *  - в combo 6 значений Qt::PenStyle
     */
    void test_createEditor_penStyleColumn_returnsComboBox()
    {
        QStandardItemModel model;
        fillModel(model);
        MyDelegate delegate;
        QWidget parentWidget;
        QStyleOptionViewItem option;

        const QModelIndex penStyleIndex = model.index(0, 2); // kPenStyleColumn = 2
        QWidget* editor = delegate.createEditor(&parentWidget, option, penStyleIndex);

        QVERIFY(editor != nullptr);

        auto* combo = qobject_cast<QComboBox*>(editor);
        QVERIFY(combo != nullptr);

        QCOMPARE(combo->isEditable(), false);
        QCOMPARE(combo->count(), 6);

        // Проверяем, что userData реально хранит int-ы Qt::PenStyle
        QCOMPARE(combo->itemData(0).toInt(), static_cast<int>(Qt::NoPen));
        QCOMPARE(combo->itemData(1).toInt(), static_cast<int>(Qt::SolidLine));
        QCOMPARE(combo->itemData(2).toInt(), static_cast<int>(Qt::DashLine));
        QCOMPARE(combo->itemData(3).toInt(), static_cast<int>(Qt::DotLine));
        QCOMPARE(combo->itemData(4).toInt(), static_cast<int>(Qt::DashDotLine));
        QCOMPARE(combo->itemData(5).toInt(), static_cast<int>(Qt::DashDotDotLine));

        delete editor;
    }

    /**
     * @brief createEditor(): для "не penstyle" колонки используется базовый редактор.
     *
     * @details
     * Мы не фиксируем конкретный тип (это может зависеть от Qt/editor factory),
     * но проверяем, что это НЕ наш QComboBox для penstyle.
     */
    void test_createEditor_otherColumn_notPenStyleEditor()
    {
        QStandardItemModel model;
        fillModel(model);
        MyDelegate delegate;
        QWidget parentWidget;
        QStyleOptionViewItem option;

        const QModelIndex otherIndex = model.index(0, 0); // не penstyle
        QWidget* editor = delegate.createEditor(&parentWidget, option, otherIndex);

        // Базовый delegate может вернуть nullptr для некоторых случаев, это допустимо.
        if (editor != nullptr) {
            auto* combo = qobject_cast<QComboBox*>(editor);
            QVERIFY(combo == nullptr); // не наш combobox для penstyle
            delete editor;
        } else {
            QVERIFY(true);
        }
    }

    /**
     * @brief setEditorData(): для penstyle загружает значение из модели в QComboBox.
     *
     * @details
     * Ставим в модель DashLine и проверяем, что combo выбрала элемент с тем же userData.
     */
    void test_setEditorData_penStyle_setsCurrentComboItem()
    {
        QStandardItemModel model;
        fillModel(model);
        model.setData(model.index(0, 2), static_cast<int>(Qt::DashLine), Qt::EditRole);

        MyDelegate delegate;
        QWidget parentWidget;
        QStyleOptionViewItem option;
        const QModelIndex penStyleIndex = model.index(0, 2);

        QWidget* editor = delegate.createEditor(&parentWidget, option, penStyleIndex);
        QVERIFY(editor != nullptr);

        auto* combo = qobject_cast<QComboBox*>(editor);
        QVERIFY(combo != nullptr);

        delegate.setEditorData(editor, penStyleIndex);

        QCOMPARE(combo->currentData().toInt(), static_cast<int>(Qt::DashLine));

        delete editor;
    }

    /**
     * @brief setEditorData(): если в модели неизвестное значение penstyle, выбирается первый элемент.
     *
     * @details
     * В коде это ветка:
     *  pos >= 0 ? pos : 0
     */
    void test_setEditorData_penStyle_unknownValue_fallsBackToFirstItem()
    {
        QStandardItemModel model;
        fillModel(model);
        model.setData(model.index(0, 2), 9999, Qt::EditRole); // неизвестный стиль

        MyDelegate delegate;
        QWidget parentWidget;
        QStyleOptionViewItem option;
        const QModelIndex penStyleIndex = model.index(0, 2);

        QWidget* editor = delegate.createEditor(&parentWidget, option, penStyleIndex);
        QVERIFY(editor != nullptr);

        auto* combo = qobject_cast<QComboBox*>(editor);
        QVERIFY(combo != nullptr);

        delegate.setEditorData(editor, penStyleIndex);

        QCOMPARE(combo->currentIndex(), 0);
        QCOMPARE(combo->currentData().toInt(), static_cast<int>(Qt::NoPen));

        delete editor;
    }

    /**
     * @brief setModelData(): для penstyle сохраняет currentData() из QComboBox в модель.
     */
    void test_setModelData_penStyle_writesEnumIntToModel()
    {
        QStandardItemModel model;
        fillModel(model);

        MyDelegate delegate;
        QWidget parentWidget;
        QStyleOptionViewItem option;
        const QModelIndex penStyleIndex = model.index(0, 2);

        QWidget* editor = delegate.createEditor(&parentWidget, option, penStyleIndex);
        QVERIFY(editor != nullptr);

        auto* combo = qobject_cast<QComboBox*>(editor);
        QVERIFY(combo != nullptr);

        // Выбираем DotLine
        const int pos = combo->findData(static_cast<int>(Qt::DotLine));
        QVERIFY(pos >= 0);
        combo->setCurrentIndex(pos);

        delegate.setModelData(editor, &model, penStyleIndex);

        QCOMPARE(model.data(penStyleIndex, Qt::EditRole).toInt(), static_cast<int>(Qt::DotLine));

        delete editor;
    }

    /**
     * @brief editorEvent(): для невалидного индекса делегат отдаёт обработку базовому классу.
     *
     * @details
     * На практике ожидаем false и отсутствие изменений.
     */
    void test_editorEvent_invalidIndex_returnsBaseBehavior()
    {
        QStandardItemModel model;
        fillModel(model);
        MyDelegate delegate;
        QStyleOptionViewItem option;

        QMouseEvent event(QEvent::MouseButtonDblClick,
                          QPointF(5, 5),
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);

        const QModelIndex invalidIndex; // invalid
        const bool handled = delegate.editorEvent(&event, &model, option, invalidIndex);

        QCOMPARE(handled, false);
    }

    /**
     * @brief editorEvent(): для "не color" колонки делегат не открывает QColorDialog.
     *
     * @details
     * Это ветка index.column() != kPenColorColumn.
     */
    void test_editorEvent_wrongColumn_returnsBaseBehavior()
    {
        QStandardItemModel model;
        fillModel(model);
        MyDelegate delegate;
        QStyleOptionViewItem option;

        QMouseEvent event(QEvent::MouseButtonDblClick,
                          QPointF(5, 5),
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);

        const QModelIndex notColorIndex = model.index(0, 2); // penstyle, а не pencolor
        const bool handled = delegate.editorEvent(&event, &model, option, notColorIndex);

        QCOMPARE(handled, false);
    }

    /**
     * @brief editorEvent(): для color-колонки, но не double click — отдаёт базовому классу.
     *
     * @details
     * Покрывает ветку:
     *   if (event->type() != QEvent::MouseButtonDblClick)
     */
    void test_editorEvent_colorColumn_notDoubleClick_returnsBaseBehavior()
    {
        QStandardItemModel model;
        fillModel(model);
        MyDelegate delegate;
        QStyleOptionViewItem option;

        QMouseEvent event(QEvent::MouseButtonPress,
                          QPointF(5, 5),
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);

        const QModelIndex colorIndex = model.index(0, 1); // kPenColorColumn = 1
        const bool handled = delegate.editorEvent(&event, &model, option, colorIndex);

        QCOMPARE(handled, false);
    }

    /**
     * @brief editorEvent(): для color-колонки и double click, но не ЛКМ — отдаёт базовому классу.
     *
     * @details
     * Покрывает ветку:
     *   if (me->button() != Qt::LeftButton)
     */
    void test_editorEvent_colorColumn_doubleClickRightButton_returnsBaseBehavior()
    {
        QStandardItemModel model;
        fillModel(model);
        MyDelegate delegate;
        QStyleOptionViewItem option;

        QMouseEvent event(QEvent::MouseButtonDblClick,
                          QPointF(5, 5),
                          Qt::RightButton,
                          Qt::RightButton,
                          Qt::NoModifier);

        const QModelIndex colorIndex = model.index(0, 1); // kPenColorColumn = 1
        const bool handled = delegate.editorEvent(&event, &model, option, colorIndex);

        QCOMPARE(handled, false);
    }

    /**
     * @brief paint(): отрисовка penstyle-колонки выполняется без падения.
     *
     * @details
     * Мы не проверяем "пиксель-в-пиксель", но убеждаемся, что ветка paint() для penstyle
     * исполняется и что рисование проходит штатно.
     */
    void test_paint_penStyleColumn_noCrash()
    {
        QStandardItemModel model;
        fillModel(model);
        model.setData(model.index(0, 2), static_cast<int>(Qt::DashDotLine), Qt::EditRole);

        MyDelegate delegate;

        QImage image(200, 40, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);

        QPainter painter(&image);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 200, 40);
        option.state = QStyle::State_Enabled;

        delegate.paint(&painter, option, model.index(0, 2));

        painter.end();

        QVERIFY(true);
    }

    /**
     * @brief paint(): для не penstyle-колонки используется базовая отрисовка без падения.
     */
    void test_paint_otherColumn_noCrash()
    {
        QStandardItemModel model;
        fillModel(model);
        model.setData(model.index(0, 0), QString("123"), Qt::EditRole);

        MyDelegate delegate;

        QImage image(200, 40, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);

        QPainter painter(&image);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 200, 40);
        option.state = QStyle::State_Enabled;

        delegate.paint(&painter, option, model.index(0, 0));

        painter.end();

        QVERIFY(true);
    }

    /**
     * @brief paint(): неизвестный стиль (например 9999) должен обрабатываться без падения.
     *
     * @details
     * Это косвенно покрывает ветку penStyleToText(...)-> default: "Style(N)".
     */
    void test_paint_penStyleUnknownValue_noCrash()
    {
        QStandardItemModel model;
        fillModel(model);
        model.setData(model.index(0, 2), 9999, Qt::EditRole);

        MyDelegate delegate;

        QImage image(200, 40, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);

        QPainter painter(&image);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 200, 40);
        option.state = QStyle::State_Enabled;

        delegate.paint(&painter, option, model.index(0, 2));

        painter.end();

        QVERIFY(true);
    }
};

QTEST_MAIN(TestMyDelegate)
#include "test_mydelegate.moc"
