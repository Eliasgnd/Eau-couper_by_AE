#ifndef IMAGEEDGEIMPORTER_H
#define IMAGEEDGEIMPORTER_H

#include <QPainterPath>
#include <QString>

/* ─── Paramètres utilisateurs ─────────────────────────── */
struct EdgeParams
{
    /* Pré‑filtrage */
    int  bilateral_d      = 25;   // >0 = bilatéral ; 0 ⇒ edgePreserving
    int  bilateral_sigma  = 100;

    /* CLAHE & seuillage adaptatif */
    int  clahe_clip       = 2;    // 2–4 selon le contraste
    int  adapt_block      = 51;   // doit rester impair
    int  adapt_C          = 1;

    /* Morphologie */
    bool morph_close      = true;
    int  morph_kernel     = 5;    // 3,5,7…

    /* Alternative Canny */
    bool useCanny         = true;
    int  canny_low        = 80;
    int  canny_high       = 200;

    /* Échelle & sortie */
    int    resize_max_w   = 1024; // 0 = pas de redimensionnement
    double epsilon_percent= 0.0001; // 1 % du périmètre (lissage fort)
    int    max_output_points = 4000;
    bool   final_simplify = true; // QPainterPath::simplified()
};

/* ─── Importeur principal ─────────────────────────────── */
class ImageEdgeImporter
{
public:
    explicit ImageEdgeImporter(const EdgeParams& p = EdgeParams()) : m_p(p) {}
    bool loadAndProcess(const QString& filePath, QPainterPath& edgePath);

    EdgeParams& params() { return m_p; }

private:
    EdgeParams m_p;
};

#endif // IMAGEEDGEIMPORTER_H
