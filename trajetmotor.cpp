#include "trajetmotor.h"
#include "pathplanner.h"          // définitions de PathPlanner / Segment
#include <QSet>
#include <QLineF>
#include <QThread>
#include <QApplication>
#include <QGraphicsEllipseItem>
#include <algorithm>
#include <limits>

// --- décommentez si Segment est imbriqué dans la classe PathPlanner ---------
// using Segment = PathPlanner::Segment;
// ---------------------------------------------------------------------------

static constexpr int    VIS_DELAY_MS  = 15;   // délai visualisation (ms)
static constexpr double mmPerPx       = 1.0;  // calibration plateau
static constexpr int    VIS_SAMPLE_PX = 3;    // pas entre deux points affichés

TrajetMotor::TrajetMotor(FormeVisualization* visu, QWidget* parent)
    : QWidget(parent), m_visu(visu)
{}

// -----------------------------------------------------------------------------
// Helper pour colorier un segment
// -----------------------------------------------------------------------------
static void drawSegment(FormeVisualization* v,
                        const QPoint& a, const QPoint& b,
                        bool cut)
{
    const int dx    = b.x() - a.x();
    const int dy    = b.y() - a.y();
    const int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) return;

    for (int s = 1; s <= steps; s += VIS_SAMPLE_PX) {
        const double t = static_cast<double>(s) / steps;
        const QPoint p(qRound(a.x() + t * dx),
                       qRound(a.y() + t * dy));
        cut ? v->colorPositionRed(p) : v->colorPositionBlue(p);
    }
}

// -----------------------------------------------------------------------------
// Lancement de la découpe
// -----------------------------------------------------------------------------
void TrajetMotor::executeTrajet()
{
    if (m_running) {
        qWarning() << "Découpe déjà en cours (pause ou non).";
        return;
    }
    if (!m_visu || !m_visu->getScene()) return;

    m_running       = true;
    m_paused        = false;
    m_stopRequested = false;

    // 1) Extraction des segments ----------------------------------------------
    const QList<Segment> segs = PathPlanner::extractSegments(m_visu->getScene());
    if (segs.isEmpty()) {
        qWarning() << "Aucun segment à découper";
        m_running = false;
        return;
    }

    // 2) Progression -----------------------------------------------------------
    const int totalSegments = segs.size();
    int remaining           = totalSegments;
    emit decoupeProgress(remaining, totalSegments);

    // 3) Curseur vert ----------------------------------------------------------
    auto *head = new QGraphicsEllipseItem(-3, -3, 6, 6);
    head->setBrush(Qt::green);
    head->setPen(Qt::NoPen);
    head->setZValue(60);
    m_visu->getScene()->addItem(head);

    // 4) Boucle principale -----------------------------------------------------
    const auto key = [](const QPoint& a, const QPoint& b) {
        const quint32 k1 = (static_cast<quint32>(a.x()) << 16) | quint16(a.y());
        const quint32 k2 = (static_cast<quint32>(b.x()) << 16) | quint16(b.y());
        return (static_cast<quint64>(qMin(k1, k2)) << 32) | qMax(k1, k2);
    };
    QSet<quint64> done;

    QPoint cur = segs.first().a;
    head->setPos(cur.x() - 3, cur.y() - 3);
    m_visu->colorPositionBlue(cur);

    while (done.size() < segs.size()) {

        // ---------- STOP -------------------------------------------------------
        if (m_stopRequested) break;

        // ---------- PAUSE ------------------------------------------------------
        while (m_paused && !m_stopRequested) {
            QApplication::processEvents();
            QThread::msleep(30);
        }
        if (m_stopRequested) break;

        // ---------- Cherche segment le plus proche ----------------------------
        int    bestIdx = -1;
        double bestDi  = std::numeric_limits<double>::max();
        for (int i = 0; i < segs.size(); ++i) {
            if (done.contains(key(segs[i].a, segs[i].b))) continue;
            const double d = QLineF(cur, segs[i].a).length();
            if (d < bestDi) { bestDi = d; bestIdx = i; }
        }
        if (bestIdx < 0) break;
        const auto s = segs[bestIdx];

        // ---------- Déplacement rapide (bleu) ---------------------------------
        if (cur != s.a) {
            drawSegment(m_visu, cur, s.a, false);
            m_motor.stopJet();
            m_motor.moveRapid(s.a.x() * mmPerPx, s.a.y() * mmPerPx);
            head->setPos(s.a.x() - 3, s.a.y() - 3);
            QApplication::processEvents();
            QThread::msleep(VIS_DELAY_MS);
        }
        cur = s.a;

        // ---------- Coupe (rouge) ---------------------------------------------
        m_motor.startJet();
        drawSegment(m_visu, s.a, s.b, true);
        m_motor.moveCut(s.b.x() * mmPerPx, s.b.y() * mmPerPx);
        head->setPos(s.b.x() - 3, s.b.y() - 3);
        QApplication::processEvents();
        QThread::msleep(VIS_DELAY_MS);

        // ---------- Progression ------------------------------------------------
        done.insert(key(s.a, s.b));
        remaining = totalSegments - done.size();
        emit decoupeProgress(remaining, totalSegments);

        cur = s.b;
    }

    // 5) Nettoyage -------------------------------------------------------------
    m_motor.stopJet();
    m_visu->resetCutMarkers();          // suppression points/curseur
    m_visu->getScene()->removeItem(head);
    delete head;

    emit decoupeProgress(0, totalSegments);
    qDebug() << "Découpe terminée – Pas X=" << m_motor.getStepsX()
             << " Y=" << m_motor.getStepsY();

    m_running = false;
}

// -----------------------------------------------------------------------------
// Slots Pause / Reprendre / Stop
// -----------------------------------------------------------------------------
void TrajetMotor::pause()
{
    if (m_running) { m_paused = true;  m_motor.stopJet(); }
}

void TrajetMotor::resume()
{
    if (m_running) { m_paused = false; }
}

void TrajetMotor::stopCut()
{
    if (!m_running) return;
    m_stopRequested = true;
    m_paused        = false;
    m_motor.stopJet();
}
