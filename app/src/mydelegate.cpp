#include "mydelegate.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionComboBox>
#include <QVariant>

/**
 * @brief Преобразует числовое значение Qt::PenStyle в строку для отображения.
 * @param v Значение стиля (как хранится в БД / Qt::EditRole).
 * @return Текст (например "SolidLine"). Для неизвестных значений: "Style(N)".
 */
static QString penStyleToText(int v)
{
    switch (static_cast<Qt::PenStyle>(v)) {
    case Qt::NoPen:          return "NoPen";
    case Qt::SolidLine:      return "SolidLine";
    case Qt::DashLine:       return "DashLine";
    case Qt::DotLine:        return "DotLine";
    case Qt::DashDotLine:    return "DashDotLine";
    case Qt::DashDotDotLine: return "DashDotDotLine";
    default:                 return QString("Style(%1)").arg(v);
    }
}

MyDelegate::MyDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void MyDelegate::fillPenStyleCombo(QComboBox* combo)
{
    combo->addItem("Qt::NoPen",          static_cast<int>(Qt::NoPen));
    combo->addItem("Qt::SolidLine",      static_cast<int>(Qt::SolidLine));
    combo->addItem("Qt::DashLine",       static_cast<int>(Qt::DashLine));
    combo->addItem("Qt::DotLine",        static_cast<int>(Qt::DotLine));
    combo->addItem("Qt::DashDotLine",    static_cast<int>(Qt::DashDotLine));
    combo->addItem("Qt::DashDotDotLine", static_cast<int>(Qt::DashDotDotLine));
}

QWidget* MyDelegate::createEditor(QWidget* parent,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
{
    if (!index.isValid() || index.column() != kPenStyleColumn)
        return QStyledItemDelegate::createEditor(parent, option, index);

    auto* combo = new QComboBox(parent);
    fillPenStyleCombo(combo);
    combo->setEditable(false);
    return combo;
}

void MyDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (!index.isValid() || index.column() != kPenStyleColumn) {
        QStyledItemDelegate::setEditorData(editor, index);
        return;
    }

    auto* combo = static_cast<QComboBox*>(editor);
    const int styleInt = index.model()->data(index, Qt::EditRole).toInt();

    const int pos = combo->findData(styleInt);
    combo->setCurrentIndex(pos >= 0 ? pos : 0);
}

void MyDelegate::setModelData(QWidget* editor,
                              QAbstractItemModel* model,
                              const QModelIndex& index) const
{
    if (!index.isValid() || index.column() != kPenStyleColumn) {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    auto* combo = static_cast<QComboBox*>(editor);
    model->setData(index, combo->currentData(), Qt::EditRole);
}

bool MyDelegate::editorEvent(QEvent* event,
                             QAbstractItemModel* model,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index)
{
    if (!index.isValid() || !model || index.column() != kPenColorColumn)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    if (event->type() != QEvent::MouseButtonDblClick)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    auto* me = static_cast<QMouseEvent*>(event);
    if (me->button() != Qt::LeftButton)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    const QVariant cur = model->data(index, Qt::EditRole);

    QColor currentColor;
    if (cur.canConvert<QColor>())
        currentColor = cur.value<QColor>();
    else
        currentColor = QColor(cur.toString().trimmed());

    QWidget* parentWidget = option.widget ? const_cast<QWidget*>(option.widget) : nullptr;

    const QColor selectedColor =
        QColorDialog::getColor(currentColor, parentWidget, "Select pen color");

    if (!selectedColor.isValid())
        return true;

    model->setData(index, selectedColor, Qt::EditRole);
    return true;
}

void MyDelegate::paint(QPainter* painter,
                       const QStyleOptionViewItem& option,
                       const QModelIndex& index) const
{
    if (index.column() == kPenStyleColumn) {
        const int style = index.data(Qt::EditRole).toInt();
        const QString text = penStyleToText(style);

        QStyleOptionComboBox comboOpt;
        comboOpt.rect = option.rect;
        comboOpt.currentText = text;

        QApplication::style()->drawComplexControl(QStyle::CC_ComboBox, &comboOpt, painter);
        QApplication::style()->drawItemText(painter,
                                            comboOpt.rect,
                                            Qt::AlignCenter,
                                            QApplication::palette(),
                                            true,
                                            comboOpt.currentText);
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);
}
