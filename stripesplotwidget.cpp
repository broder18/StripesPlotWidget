// stripes_plot.cpp
#include "stripesplotwidget.h"
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

StripesPlot::StripesPlot(QWidget *parent)
    : QCustomPlot(parent)
{
    setupPlot();
}

/*!
 * \brief StripesPlot::setupPlot
 * Установка настроек для всего виджета
 */
void StripesPlot::setupPlot()
{
    setBackground(Qt::transparent);
    axisRect()->setBackground(Qt::transparent);

    xAxis->setVisible(true);
    xAxis->setLabel("Частота, МГц");
    xAxis->setTickLabels(true);
    xAxis->setTickLabelFont(QFont("Arial", 9));
    xAxis->setLabelFont(QFont("Arial", 10, QFont::Bold));

    yAxis->setVisible(false);

    setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    axisRect()->setRangeDrag(Qt::Horizontal);
    axisRect()->setRangeZoom(Qt::Horizontal);

    yAxis->setRange(0, 1);
    xAxis->setRange(0, 8000);

    axisRect()->setAutoMargins(QCP::msLeft | QCP::msBottom);
    axisRect()->setMargins(QMargins(40, 0, 0, 30));
}

/*!
 * \brief StripesPlot::addStripe
 * \param id
 * \param label - текст
 * \param start - начальная частота
 * \param end - конечная частота
 * \param category - тип
 * \param description - описание
 * \param metadata - полезная штука на будуще, не придумал где юзать
 * Добавление полосы
 */
void StripesPlot::addStripe(int id, const QString& label, double start, double end,
                           const QString& category, const QString& description,
                           const QVariantMap& metadata)
{
    if (id < 0 || start >= end) {
        qWarning() << "Invalid stripe data:" << id << start << end;
        return;
    }

    if (m_stripes.contains(id)) {
        updateStripe(id, start, end, label, category, description, metadata);
        return;
    }

    Stripe stripe{id, label, start, end, category, description, metadata};
    m_stripes[id] = stripe;

    assignVisualStyle(id);
    updateStripeVisual(id);
    updateStripesOpacity();
    this->replot();
}

/*!
 * \brief StripesPlot::updateStripe
 * \param id
 * \param start
 * \param end
 * \param label
 * \param category
 * \param description
 * \param metadata
 * Обновление полосы
 */
void StripesPlot::updateStripe(int id, double start, double end,
                              const QString& label, const QString& category,
                              const QString& description, const QVariantMap& metadata)
{
    if (!m_stripes.contains(id)) {
        qWarning() << "Stripe not found:" << id;
        return;
    }

    Stripe& stripe = m_stripes[id];
    stripe.start = start;
    stripe.end = end;
    if (!label.isEmpty()) stripe.label = label;
    if (!category.isEmpty()) stripe.category = category;
    if (!description.isEmpty()) stripe.description = description;
    if (!metadata.isEmpty()) stripe.metadata = metadata;

    updateStripeVisual(id);
    updateStripesOpacity();
    this->replot();
}

/*!
 * \brief StripesPlot::setShowInfoOnClick
 * \param show
 * Включение/выключение всплывающих подсказок
 */
void StripesPlot::setShowInfoOnClick(bool show)
{
    m_showInfoOnClick = show;
}

/*!
 * \brief StripesPlot::getShowInfoOnClick
 * Геттер на показ иформации о полосе
 * \return
 */
bool StripesPlot::getShowInfoOnClick() const
{
    return m_showInfoOnClick;
}

void StripesPlot::mousePressEvent(QMouseEvent *event)
{
    QCustomPlot::mousePressEvent(event);

    if (event->button() == Qt::LeftButton) {
        double x = xAxis->pixelToCoord(event->pos().x());
        int stripeId = findStripeAtPos(QPointF(x, 0.5));

        if (stripeId != -1) {
            highlightStripe(stripeId);

            // Сохраняем ID полосы для показа информации после отпускания кнопки
            m_pressedStripeId = stripeId;

            emit stripeSelected(stripeId);
        } else {
            clearHighlight();
        }
    }

}

void StripesPlot::mouseReleaseEvent(QMouseEvent *event)
{
    QCustomPlot::mouseReleaseEvent(event);

   if (event->button() == Qt::LeftButton && m_pressedStripeId != -1) {
       // Показываем информацию после отпускания кнопки
       if (m_showInfoOnClick) {
           showStripeInfo(m_pressedStripeId);
       }

       if (event->type() == QMouseEvent::MouseButtonDblClick) {
           emit stripeDoubleClicked(m_pressedStripeId);
       }

       m_pressedStripeId = -1;
   }

}

/*!
 * \brief StripesPlot::showStripeInfo
 * \param id
 * Показать информационные подсказки для определённой полосы
 */
void StripesPlot::showStripeInfo(int id)
{
    if (!m_stripes.contains(id)) return;

    QString infoText = generateStripeInfoText(id);

    // Показываем tooltip после отпускания кнопки
    QToolTip::showText(QCursor::pos(), infoText, this, QRect(), 15000);
}

/*!
 * \brief StripesPlot::generateStripeInfoText
 * \param id
 * Создание текста для полос
 * \return
 */
QString StripesPlot::generateStripeInfoText(int id)
{
    const Stripe& stripe = m_stripes[id];
   const StripeVisual& visual = m_visualStyles[id];

   QString info;
   info += QString("🎯 <b>Основная информация</b>\n");
   info += QString("ID: <b>%1</b>\n").arg(stripe.id);
   info += QString("Название: <b>%1</b>\n").arg(stripe.label);
   info += QString("Категория: <b>%1</b>\n").arg(stripe.category.isEmpty() ? "Не указана" : stripe.category);

   info += QString("\n📊 <b>Временные параметры</b>\n");
   info += QString("Начало: <b>%1</b>\n").arg(stripe.start, 0, 'f', 2);
   info += QString("Конец: <b>%1</b>\n").arg(stripe.end, 0, 'f', 2);
   info += QString("Длина: <b>%1</b>\n").arg(stripe.end - stripe.start, 0, 'f', 2);

   info += QString("\n🎨 <b>Визуальное оформление</b>\n");
   info += QString("Цвет: <b>%1</b>\n").arg(colorToString(visual.baseColor));
   info += QString("Тип заливки: <b>%1</b>\n").arg(patternToString(visual.pattern));

   if (!stripe.description.isEmpty()) {
       info += QString("\n📝 <b>Описание</b>\n");
       info += QString("%1\n").arg(stripe.description);
   }

   return info;
}

/*!
 * \brief StripesPlot::removeStripe
 * \param id
 * Удаление полосы
 */
void StripesPlot::removeStripe(int id)
{
    if (m_rectItems.contains(id)) {
        removeItem(m_rectItems[id]);
        m_rectItems.remove(id);
    }
    if (m_textItems.contains(id)) {
        removeItem(m_textItems[id]);
        m_textItems.remove(id);
    }

    m_stripes.remove(id);
    m_visualStyles.remove(id);

    if (m_highlightedStripeId == id) {
        m_highlightedStripeId = -1;
    }

    updateStripesOpacity();
    replot();
}

/*!
 * \brief StripesPlot::clearStripes
 * Очистка всех полос
 */
void StripesPlot::clearStripes()
{
    for (int id : m_rectItems.keys()) {
        removeItem(m_rectItems[id]);
    }
    for (int id : m_textItems.keys()) {
        removeItem(m_textItems[id]);
    }

    m_stripes.clear();
    m_rectItems.clear();
    m_textItems.clear();
    m_visualStyles.clear();

    m_highlightedStripeId = -1;
    replot();
}

/*!
 * \brief StripesPlot::highlightStripe
 * \param id
 * Подсветить полосу по id
 */
void StripesPlot::highlightStripe(int id)
{
    if (!m_stripes.contains(id)) return;

    clearHighlight();
    m_highlightedStripeId = id;
    updateStripesOpacity();
    replot();

    emit stripeSelected(id);
}

/*!
 * \brief StripesPlot::clearHighlight
 * Очистить подсветку
 */
void StripesPlot::clearHighlight()
{
    if (m_highlightedStripeId == -1) return;

    m_highlightedStripeId = -1;
    updateStripesOpacity();
    replot();
}

/*!
 * \brief StripesPlot::setBaseOpacity
 * \param opacity
 * Базовая прозрачность для полос
 */
void StripesPlot::setBaseOpacity(double opacity)
{
    m_baseOpacity = qBound(0.1, opacity, 1.0);
    updateStripesOpacity();
    replot();
}

/*!
 * \brief StripesPlot::setHighlightOpacity
 * \param opacity
 * Прозрачность подсветки для полос
 */
void StripesPlot::setHighlightOpacity(double opacity)
{
    m_highlightOpacity = qBound(0.1, opacity, 1.0);
    updateStripesOpacity();
    replot();
}

/*!
 * \brief StripesPlot::getStripe
 * \param id
 * Полоучить полосу
 * \return
 */
Stripe StripesPlot::getStripe(int id) const
{
    return m_stripes.value(id);
}

/*!
 * \brief StripesPlot::getStripeIds
 * Получение id всех полос
 * \return
 */
QList<int> StripesPlot::getStripeIds() const
{
    return m_stripes.keys();
}

/*!
 * \brief StripesPlot::getVisibleStripeCount
 * Геттер на получение количества видимых полос
 * \return
 */
int StripesPlot::getVisibleStripeCount() const
{
    return m_stripes.size();
}

/*!
 * \brief StripesPlot::assignVisualStyle
 * \param id
 * Рандоманая штриховка + автоудаление старых ( на данный момент вызвается из main.cpp)
 */
void StripesPlot::assignVisualStyle(int id)
{
    int colorIndex = id % m_availableColors.size();
    int patternIndex = QRandomGenerator::global()->bounded(m_availablePatterns.size()); // Рандом
    StripeVisual visual;
    visual.baseColor = m_availableColors[colorIndex];
    visual.pattern = m_availablePatterns[patternIndex];
    visual.brushStyle = patternToBrushStyle(visual.pattern);
    visual.opacity = m_baseOpacity;
    m_visualStyles[id] = visual;
}

/*!
 * \brief StripesPlot::patternToBrushStyle
 * \param pattern
 * Выбор типа штриховки
 * \return
 */
Qt::BrushStyle StripesPlot::patternToBrushStyle(PatternStyle pattern)
{
    switch (pattern) {
    case SolidPattern: return Qt::SolidPattern;
    case DiagonalPattern: return Qt::DiagCrossPattern;
    case CrossPattern: return Qt::CrossPattern;
    case DensePattern: return Qt::Dense4Pattern;
    case VerticalPattern: return Qt::VerPattern;
    case HorizontalPattern: return Qt::HorPattern;
    default: return Qt::SolidPattern;
    }
}

/*!
 * \brief StripesPlot::updateStripeVisual
 * Обновление визуализации полосы
 * \param id
 */
void StripesPlot::updateStripeVisual(int id)
{
    if (!m_stripes.contains(id) || !m_visualStyles.contains(id)) return;

    const Stripe& stripe = m_stripes[id];

    QCPItemRect* rectItem = m_rectItems.value(id, nullptr);
    if (!rectItem) {
        rectItem = new QCPItemRect(this);
        m_rectItems[id] = rectItem;
        rectItem->setSelectable(false); // Отключаем selection QCustomPlot
    }

    // Устанавливаем позицию на всю высоту
    rectItem->topLeft->setCoords(stripe.start, 1);
    rectItem->bottomRight->setCoords(stripe.end, 0);

    updateStripeAppearance(id);
}

/*!
 * \brief StripesPlot::updateStripeAppearance
 * \param id
 * Обновление цвета, штриховки, текста
 */
void StripesPlot::updateStripeAppearance(int id)
{
    if (!m_rectItems.contains(id) || !m_visualStyles.contains(id)) return;

    QCPItemRect* rectItem = m_rectItems[id];
    StripeVisual& visual = m_visualStyles[id];

    QColor color = visual.baseColor;
    color.setAlphaF(visual.opacity);

    QBrush brush(color, visual.brushStyle);
    rectItem->setBrush(brush);

    // Рамка для выделенной полосы
    if (id == m_highlightedStripeId) {
        rectItem->setPen(QPen(QColor(0, 0, 255, 200), 3));
    } else {
        rectItem->setPen(QPen(QColor(0, 0, 0, 150), 1));
    }

    // Текст
    const Stripe& stripe = m_stripes[id];
    if (!stripe.label.isEmpty()) {
        QCPItemText* textItem = m_textItems.value(id, nullptr);
        if (!textItem) {
            textItem = new QCPItemText(this);
            m_textItems[id] = textItem;

            textItem->setPositionAlignment(Qt::AlignCenter);
            textItem->setTextAlignment(Qt::AlignCenter);
            textItem->setPadding(QMargins(3, 2, 3, 2));
            textItem->setBrush(QBrush(QColor(255, 255, 255, 220)));
            textItem->setPen(QPen(Qt::black, 1));
            textItem->setFont(QFont("Arial", 8, QFont::Bold));
        }

        QString displayText = stripe.label;
        /*if (stripe.label.length() > 20) {
            displayText = stripe.label.left(17) + "...";
        }*/
        displayText = QString("ID:%1").arg(id);

        textItem->setText(displayText);
        double centerX = (stripe.start + stripe.end) / 2;
        textItem->position->setCoords(centerX, 0.5);
    } else if (m_textItems.contains(id)) {
        removeItem(m_textItems[id]);
        m_textItems.remove(id);
    }
}

/*!
 * \brief StripesPlot::findStripeAtPos
 * \param pos
 * Поиск полосы содержащей координату X
 * \return
 */
int StripesPlot::findStripeAtPos(const QPointF& pos)
{
    for (auto it = m_stripes.begin(); it != m_stripes.end(); ++it) {
        const Stripe& stripe = it.value();
        if (pos.x() >= stripe.start && pos.x() <= stripe.end) {
            return it.key();
        }
    }
    return -1;
}

/*!
 * \brief StripesPlot::findOverlappingStripes
 * \param stripeId
 * Возврат списка ID полос, которые пересекаются с данной
 * \return
 */
QVector<int> StripesPlot::findOverlappingStripes(int stripeId)
{
    if (!m_stripes.contains(stripeId)) return QVector<int>();

    QVector<int> overlapping;
    const Stripe& targetStripe = m_stripes[stripeId];

    for (auto it = m_stripes.begin(); it != m_stripes.end(); ++it) {
        const Stripe& stripe = it.value();
        if (stripe.start < targetStripe.end && stripe.end > targetStripe.start) {
            overlapping.append(it.key());
        }
    }

    return overlapping;
}

/*!
 * \brief StripesPlot::updateStripesOpacity
 * Пересчёт прозрачности для всех полос с учётом наложений
 */
void StripesPlot::updateStripesOpacity()
{
    for (auto it = m_stripes.begin(); it != m_stripes.end(); ++it) {
        int id = it.key();
        StripeVisual& visual = m_visualStyles[id];

        if (id == m_highlightedStripeId) {
            visual.opacity = m_highlightOpacity;
        } else {
            QVector<int> overlapping = findOverlappingStripes(id);
            int overlapCount = overlapping.size();
            double opacity = m_baseOpacity / std::max(1, overlapCount);
            visual.opacity = qMax(0.1, opacity);
        }

        updateStripeAppearance(id);
    }
}

/*!
 * \brief StripesPlot::colorToString
 * \param color
 * Конвертация цветов в текст (только для трёх цветов на данный момент)
 * \return
 */
QString StripesPlot::colorToString(const QColor& color) const
{
    const int r = color.red();
    const int g = color.green();
    const int b = color.blue();

    if (r == 0 && g == 255 && b == 0) return "Зеленый";
    if (r == 255 && g == 255 && b == 0) return "Желтый";
    if (r == 255 && g == 0 && b == 0) return "Красный";

    return "Неизвестный";
}

/*!
 * \brief StripesPlot::patternToString
 * \param pattern
 * Конвертация паттернов отрисовки в текст
 * \return
 */
QString StripesPlot::patternToString(PatternStyle pattern) const
{
    switch (pattern) {
    case SolidPattern:
        return "Сплошной";
    case DiagonalPattern:
        return "Диагональный";
    case CrossPattern:
        return "Крестовый";
    case DensePattern:
        return "Густой";
    case VerticalPattern:
        return "Вертикальный";
    case HorizontalPattern:
        return "Горизонтальный";
    default:
        return "Неизвестный";
    }
}
