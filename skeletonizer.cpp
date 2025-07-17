#include "skeletonizer.h"
#include <QPainter>
#include <QVector>

/* Helpers ------------------------------------------------------------ */
static inline bool isBlack(const QImage &im, int x, int y)
{
    return qGray(im.pixel(x, y)) < 128;          // noir si < 50 %
}
static inline void setWhite(QImage &im, int x, int y)
{
    im.setPixel(x, y, qRgb(255, 255, 255));
}

/* ------------------------------------------------------------------ */
/*  Zhang‑Suen thinning                                               */
/* ------------------------------------------------------------------ */
QImage Skeletonizer::thin(const QImage &srcGray)
{
    QImage img = srcGray.convertToFormat(QImage::Format_Grayscale8);
    const int w = img.width(), h = img.height();

    bool changed;
    do {
        changed = false;

        for (int pass = 0; pass < 2; ++pass)
        {
            QVector<QPoint> toErase;

            for (int y = 1; y < h - 1; ++y)
                for (int x = 1; x < w - 1; ++x)
                {
                    if (!isBlack(img, x, y)) continue;

                    /* 8‑voisins p2…p9 (sens horaire) */
                    bool p2 = isBlack(img, x,     y-1);
                    bool p3 = isBlack(img, x+1,   y-1);
                    bool p4 = isBlack(img, x+1,   y  );
                    bool p5 = isBlack(img, x+1,   y+1);
                    bool p6 = isBlack(img, x,     y+1);
                    bool p7 = isBlack(img, x-1,   y+1);
                    bool p8 = isBlack(img, x-1,   y  );
                    bool p9 = isBlack(img, x-1,   y-1);

                    int  B = p2+p3+p4+p5+p6+p7+p8+p9;                // nb. voisins noirs
                    int  A = (!p2 && p3) + (!p3 && p4) + (!p4 && p5)
                            + (!p5 && p6) + (!p6 && p7) + (!p7 && p8)
                            + (!p8 && p9) + (!p9 && p2);              // transitions 0→1

                    if (B < 2 || B > 6 || A != 1) continue;

                    bool cond = (pass == 0)
                                    ? !(p2 && p4 && p6) && !(p4 && p6 && p8)
                                    : !(p2 && p4 && p8) && !(p2 && p6 && p8);

                    if (cond) toErase.append(QPoint(x, y));
                }

            if (!toErase.isEmpty()) changed = true;
            for (const QPoint &pt : toErase)
                setWhite(img, pt.x(), pt.y());
        }
    } while (changed);

    return img;
}

/* ------------------------------------------------------------------ */
/*  Conversion d’un squelette binaire (1 px) en QPainterPath          */
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/*  Conversion bitmap 1 px  ->  QPainterPath (compressé)              */
/* ------------------------------------------------------------------ */
QPainterPath Skeletonizer::bitmapToPath(const QImage &img)
{
    const int w = img.width(), h = img.height();
    auto black = [&](int x,int y){ return qGray(img.pixel(x,y)) < 128; };

    QVector<uchar> done(w*h, 0);
    QPainterPath path;

    /* --- passe horizontale ----------------------------------------- */
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; )
        {
            if (!black(x,y) || done[y*w+x]) { ++x; continue; }
            int x2 = x;  while (x2+1 < w && black(x2+1,y)) ++x2;
            path.moveTo(x +0.5, y+0.5);
            path.lineTo(x2+0.5, y+0.5);
            while (x <= x2) done[y*w + x++] = 1;
        }

    /* --- passe verticale ------------------------------------------- */
    done.fill(0);
    for (int x = 0; x < w; ++x)
        for (int y = 0; y < h; )
        {
            if (!black(x,y) || done[y*w+x]) { ++y; continue; }
            int y2 = y;  while (y2+1 < h && black(x,y2+1)) ++y2;
            path.moveTo(x+0.5, y +0.5);
            path.lineTo(x+0.5, y2+0.5);
            while (y <= y2) done[y++*w + x] = 1;
        }

    /* --- passe diagonale ↘ (x+,y+) -------------------------------- */
    done.fill(0);
    for (int y0 = 0; y0 < h; ++y0)
        for (int x0 = 0; x0 < w; ++x0)
        {
            int x = x0, y = y0;
            if (x>=w || y>=h || !black(x,y) || done[y*w+x]) continue;

            int len = 0;
            while (x<w && y<h && black(x,y)) { ++x; ++y; ++len; }

            if (len >= 1) {
                path.moveTo(x0+0.5, y0+0.5);
                path.lineTo(x-1+0.5, y-1+0.5);
                for (int i=0;i<len;++i) done[(y0+i)*w + (x0+i)] = 1;
            }
        }

    /* --- passe diagonale ↗ (x+,y−) -------------------------------- */
    done.fill(0);
    for (int y0 = h-1; y0 >= 0; --y0)
        for (int x0 = 0; x0 < w; ++x0)
        {
            int x = x0, y = y0;
            if (x>=w || y<0 || !black(x,y) || done[y*w+x]) continue;

            int len = 0;
            while (x<w && y>=0 && black(x,y)) { ++x; --y; ++len; }

            if (len >= 1) {
                path.moveTo(x0+0.5, y0+0.5);
                path.lineTo(x-1+0.5, y+1+0.5);
                for (int i=0;i<len;++i) done[(y0-i)*w + (x0+i)] = 1;
            }
        }

    return path;
}

/* ------------------------------------------------------------------ */
/*  Lissage et simplification d'un QPainterPath                       */
/* ------------------------------------------------------------------ */

static double perpDist(const QPointF &p, const QPointF &a, const QPointF &b)
{
    QLineF l(a, b);
    if (l.length() == 0.0)
        return QLineF(p, a).length();
    const double t = qMax(0.0, qMin(1.0, QPointF::dotProduct(p - a, b - a) / (l.length()*l.length())));
    QPointF proj = a + t * (b - a);
    return QLineF(p, proj).length();
}

static QList<QPointF> douglasPeucker(const QList<QPointF> &pts, double epsilon)
{
    if (pts.size() < 3)
        return pts;

    double maxDist = 0.0;
    int index = 0;
    for (int i = 1; i < pts.size() - 1; ++i) {
        double d = perpDist(pts[i], pts.first(), pts.last());
        if (d > maxDist) { maxDist = d; index = i; }
    }

    if (maxDist > epsilon) {
        QList<QPointF> res1 = douglasPeucker(pts.mid(0, index + 1), epsilon);
        QList<QPointF> res2 = douglasPeucker(pts.mid(index), epsilon);
        res1.pop_back();
        res1.append(res2);
        return res1;
    }

    return { pts.first(), pts.last() };
}

static QList<QPointF> chaikinSmooth(const QList<QPointF> &pts, int passes)
{
    QList<QPointF> out = pts;
    for (int pass = 0; pass < passes; ++pass) {
        QList<QPointF> smth;
        smth.reserve(out.size() * 2);
        smth.append(out.first());
        for (int i = 0; i < out.size() - 1; ++i) {
            const QPointF &p0 = out[i];
            const QPointF &p1 = out[i + 1];
            smth.append(p0 * 0.75 + p1 * 0.25);
            smth.append(p0 * 0.25 + p1 * 0.75);
        }
        smth.append(out.last());
        out = smth;
    }
    return out;
}

QPainterPath Skeletonizer::smoothPath(const QPainterPath &path,
                                      int chaikinPasses,
                                      double epsilon)
{
    QPainterPath result;
    const int n = path.elementCount();
    if (n == 0)
        return result;

    int start = 0;
    while (start < n)
    {
        /* --- extrait un sous‑chemin -------------------------------- */
        int end = start + 1;
        while (end < n && !path.elementAt(end).isMoveTo())
            ++end;

        QList<QPointF> pts;
        for (int i = start; i < end; ++i)
            pts.append({ path.elementAt(i).x, path.elementAt(i).y });

        /* --- simplification + lissage ------------------------------ */
        pts = douglasPeucker(pts, epsilon);
        pts = chaikinSmooth (pts, chaikinPasses);

        /* --- reconstruction ---------------------------------------- */
        if (!pts.isEmpty()) {
            result.moveTo(pts.first());            // toujours garder l’extrémité
            for (int i = 1; i < pts.size(); ++i)   // s’il n’y en a qu’une,
                result.lineTo(pts[i]);             // la boucle ne s’exécute pas
        }

        start = end;
    }
    return result;
}
