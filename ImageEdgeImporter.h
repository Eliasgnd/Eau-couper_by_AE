#ifndef IMAGEEDGEIMPORTER_H
#define IMAGEEDGEIMPORTER_H

#include <QPainterPath>
#include <QString>

/* ─────────────  Paramètres réglables  ───────────── */
struct EdgeParams
{
    int   bilateral_d       = 25;     // diamètre du filtre bilatéral
    int   bilateral_sigma   = 100;    // sigma couleur & espace
    int   clahe_clip        = 2;     // 1-5 ; > = + contraste
    int   adapt_block       = 51;    // toujours impair (17-51)
    int   adapt_C           = 1;     // 1-10
    bool  morph_close       = true;  // ferme petits trous
    int   morph_kernel      = 5;     // 3 ou 5
    double epsilon_percent  = 0.0001;   // 0 pour aucun approx. (garde ergots)
    bool  final_simplify    = true;     // QPainterPath::simplified()
};

class ImageEdgeImporter
{
public:
    /* règle les paramètres (optionnel) */
    void  setParams(const EdgeParams &p) { m_p = p; }

    /* extrait le contour principal → edgePath */
    bool  loadAndProcess(const QString &filePath,
                        QPainterPath   &edgePath);

private:
    EdgeParams m_p;
};

#endif // IMAGEEDGEIMPORTER_H
