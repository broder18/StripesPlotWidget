// stripes_plot.h
#ifndef STRIPES_PLOT_H
#define STRIPES_PLOT_H

#include <QWidget>
#include <QMap>
#include <QVector>
#include <QColor>
#include <QMouseEvent>
#include <QToolTip>
#include "qcustomplot.h"

/*!
 * \brief The Stripe struct
 * Структура с информацией выводимой по полосе
 */
struct Stripe {
    int id;
    QString label;
    double start;
    double end;
    QString category;
    QString description;  // текст описания
    QVariantMap metadata; // Дополнительные метаданные
};

class StripesPlot : public QCustomPlot
{
    Q_OBJECT

public:
    explicit StripesPlot(QWidget *parent = nullptr);

    void addStripe(int id, const QString& label, double start, double end,
                   const QString& category = "", const QString& description = "",
                   const QVariantMap& metadata = QVariantMap());

    void updateStripe(int id, double start, double end,
                      const QString& label = "", const QString& category = "",
                      const QString& description = "", const QVariantMap& metadata = QVariantMap());

    void removeStripe(int id);
    void clearStripes();

    void highlightStripe(int id);
    void clearHighlight();

    Stripe getStripe(int id) const;
    QList<int> getStripeIds() const;
    int getVisibleStripeCount() const;

    void setBaseOpacity(double opacity);
    void setHighlightOpacity(double opacity);

    // Методы для управления информационным окном
    void setShowInfoOnClick(bool show);
    bool getShowInfoOnClick() const;

signals:
    void stripeSelected(int id);
    void stripeDoubleClicked(int id);
    void stripeInfoRequested(int id, const Stripe& stripe); // Новый сигнал с информацией

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    enum PatternStyle {
        SolidPattern,
        DiagonalPattern,
        CrossPattern,
        DensePattern,
        VerticalPattern,
        HorizontalPattern
    };

    struct StripeVisual {
        QColor baseColor;
        PatternStyle pattern;
        Qt::BrushStyle brushStyle;
        double opacity;
    };

    QMap<int, Stripe> m_stripes;
    QMap<int, QCPItemRect*> m_rectItems;
    QMap<int, QCPItemText*> m_textItems;
    QMap<int, StripeVisual> m_visualStyles;

    int m_highlightedStripeId = -1;
    bool m_showInfoOnClick = true; // Показывать ли информацию при клике

    double m_baseOpacity = 0.5;
    double m_highlightOpacity = 0.9;

    QVector<QColor> m_availableColors = {
        QColor(0, 255, 0),    // Зеленый
        QColor(255, 255, 0),  // Желтый
        QColor(255, 0, 0)     // Красный
    };

    QVector<PatternStyle> m_availablePatterns = {
        SolidPattern, DiagonalPattern, CrossPattern,
        DensePattern, VerticalPattern, HorizontalPattern
    };

private:
    int m_pressedStripeId = -1;
    void setupPlot();
    void assignVisualStyle(int id);
    Qt::BrushStyle patternToBrushStyle(PatternStyle pattern);
    void updateStripeVisual(int id);
    void updateStripeAppearance(int id);
    int findStripeAtPos(const QPointF& pos);

    QVector<int> findOverlappingStripes(int stripeId);
    void updateStripesOpacity();

    // Методы для работы с информацией
    void showStripeInfo(int id);
    QString generateStripeInfoText(int id);

    QString colorToString(const QColor& color) const;
    QString patternToString(PatternStyle pattern) const;
};

#endif // STRIPES_PLOT_H
