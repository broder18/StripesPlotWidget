#include <QApplication>
#include <QMainWindow>
#include "stripesplotwidget.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    StripesPlot *plot = new StripesPlot;
    window.setCentralWidget(plot);

    QTimer *timer = new QTimer(&window);

    // Авто-генерация каждые 0.2 секунды с проверкой пересечений
    QObject::connect(timer, &QTimer::timeout, [plot]() {
        int id = QRandomGenerator::global()->bounded(10);
        double length = QRandomGenerator::global()->bounded(10, 31);

        // Получаем все существующие полосы
        auto stripeIds = plot->getStripeIds();
        QVector<QPair<double, double>> existingRanges;

        for (int existingId : stripeIds) {
            Stripe stripe = plot->getStripe(existingId);
            existingRanges.append(qMakePair(stripe.start, stripe.end));
        }

        auto ids = plot->getStripeIds();
        if (ids.size() >= 10) {
            plot->removeStripe(ids.first()); // Удаляем самую старую
        }

        // Ищем свободное место
        double start = 300;
        bool found = false;

        while (start + length <= 500 && !found) {
            found = true;

            // Проверяем пересечение с существующими полосами
            for (const auto& range : existingRanges) {
                if (start < range.second && start + length > range.first) {
                    // Есть пересечение - сдвигаемся за эту полосу
                    start = range.second + 1;
                    found = false;
                    break;
                }
            }
        }

        if (found) {
            double end = start + length;
            plot->addStripe(id, QString("Авто-полоса %1").arg(id), start, end);
            qDebug() << "Авто-генерация: ID" << id << "позиция" << start << "-" << end;
        }
    });

    timer->start(200);

    window.resize(1000, 400);
    window.setWindowTitle("Минимальный генератор полос");
    window.show();

    return app.exec();
}
