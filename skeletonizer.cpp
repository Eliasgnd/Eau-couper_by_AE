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

    QVector<uchar> done(w*h, 0);              // marquage des pixels déjà traités
    QPainterPath path;

    /* --- balayage horizontal -------------------------------------- */
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ) {
            if (!black(x,y) || done[y*w+x]) { ++x; continue; }
            int x2 = x;
            while (x2+1 < w && black(x2+1,y)) ++x2;            // run noir
            path.moveTo(x   +0.5, y+0.5);
            path.lineTo(x2+0.5, y+0.5);
            while (x <= x2) done[y*w + x++] = 1;               // marque la run
        }
    }

    /* --- balayage vertical ---------------------------------------- */
    done.fill(0);
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ) {
            if (!black(x,y) || done[y*w+x]) { ++y; continue; }
            int y2 = y;
            while (y2+1 < h && black(x,y2+1)) ++y2;
            path.moveTo(x+0.5, y +0.5);
            path.lineTo(x+0.5, y2+0.5);
            while (y <= y2) done[y++*w + x] = 1;
        }
    }
    return path;
}
