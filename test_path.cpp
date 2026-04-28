#include <QCoreApplication>
#include <QPainterPath>
#include <QDebug>
#include <QPolygonF>

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    QPainterPath path;
    path.moveTo(0, 0);
    path.lineTo(10, 10);
    path.lineTo(20, 0);
    
    QList<QPolygonF> polys = path.toSubpathPolygons();
    for (const QPolygonF &poly : polys) {
        qDebug() << "Poly size:" << poly.size();
        for (int i = 0; i < poly.size(); ++i) {
            qDebug() << "  " << poly[i];
        }
    }
    return 0;
}
