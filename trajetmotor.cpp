#include "trajetmotor.h"
#include "pathplanner.h"          // définitions de PathPlanner / Segment
#include <QSet>
#include <QThread>
#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QMessageBox>
#include "MainWindow.h"
#include <utility>

namespace {
template <typename F>
inline void invokeLater(QObject *obj, F &&func)
{
    // QMetaObject::invokeMethod n'accepte pas un pointeur nul. Si aucun objet n'est
    // fourni, on se rabat sur l'instance de l'application afin d'éviter un crash.
    QMetaObject::invokeMethod(obj ? obj : QApplication::instance(),
                              std::forward<F>(func),
                              Qt::QueuedConnection);
}
}

// --- décommentez si Segment est imbriqué dans la classe PathPlanner ---------
// using Segment = PathPlanner::Segment;
// ---------------------------------------------------------------------------

// Le délai artificiel de visualisation ralentissait fortement la découpe.
// On le supprime et on limite les mises à jour graphiques pour plus de fluidité.
static constexpr int    VIS_DELAY_MS       = 0;   // délai visualisation (ms)
static constexpr int    VIS_UPDATE_INTERVAL = 20; // fréquence des rafraîchissements
static constexpr double mmPerPx            = 1.0; // calibration plateau
static constexpr int    VIS_SAMPLE_PX      = 1;   // pas entre deux points affichés

TrajetMotor::TrajetMotor(FormeVisualization* visu, QWidget* parent)
    : QWidget(parent), m_visu(visu)
{}

// -----------------------------------------------------------------------------
// Estimation du nombre total de pas (déplacements + coupes)
// -----------------------------------------------------------------------------
static int estimateTotalSteps(const QList<Segment>& segs)
{
    if (segs.isEmpty())
        return 0;

    const auto stepLen = [](const QPoint& a, const QPoint& b) {
        return std::max(std::abs(a.x() - b.x()), std::abs(a.y() - b.y()));
    };

    // On construit un chemin eulérien qui visite chaque segment une fois.
    const QVector<QPoint> path = PathPlanner::buildEulerPath(segs);
    if (path.size() < 2)
        return 0;

    int steps = 0;
    for (int i = 0; i + 1 < path.size(); ++i)
        steps += stepLen(path[i], path[i + 1]);

    return steps;
}

// -----------------------------------------------------------------------------
// Lancement de la découpe
// -----------------------------------------------------------------------------
void TrajetMotor::executeTrajet()
{
    m_interrupted = false;
    qDebug() << "[DEBUG] executeTrajet() exécuté dans instance" << this;

    if (m_running) {
        qWarning() << "Découpe déjà en cours (pause ou non).";
        return;
    }
    if (!m_visu || !m_visu->getScene()) return;

    m_running       = true;
    m_paused        = false;
    m_stopRequested = false;
    m_visu->setDecoupeEnCours(true);

    // 1) Extraction des segments ----------------------------------------------
    const QList<Segment> segs = PathPlanner::extractSegments(m_visu->getScene());
    if (segs.isEmpty()) {
        qWarning() << "Aucun segment à découper";
        m_running = false;
        return;
    }

    // 2) Construction du chemin optimisé --------------------------------------
    const QVector<QPoint> path = PathPlanner::buildEulerPath(segs);
    if (path.size() < 2) {
        qWarning() << "Chemin vide";
        m_running = false;
        return;
    }

    // Ensemble des segments réels pour distinguer déplacement et coupe
    const auto key = [](const QPoint& a, const QPoint& b) {
        const quint32 k1 = (static_cast<quint32>(a.x()) << 16) | quint16(a.y());
        const quint32 k2 = (static_cast<quint32>(b.x()) << 16) | quint16(b.y());
        return (static_cast<quint64>(qMin(k1, k2)) << 32) | qMax(k1, k2);
    };
    QSet<quint64> realSegs;
    for (const Segment &s : segs)
        realSegs.insert(key(s.a, s.b));

    // 3) Progression -----------------------------------------------------------
    m_totalSteps      = estimateTotalSteps(segs);
    m_progressCounter = 0;
    emit decoupeProgress(m_totalSteps, m_totalSteps);

    // 4) Curseur vert ----------------------------------------------------------
    auto *head = new QGraphicsEllipseItem(-3, -3, 6, 6);
    head->setBrush(Qt::green);
    head->setPen(Qt::NoPen);
    head->setZValue(60);
    m_visu->getScene()->addItem(head);

    // 5) Parcours du chemin ----------------------------------------------------
    QPoint cur = path.first();
    head->setPos(cur.x() - 3, cur.y() - 3);
    m_visu->colorPositionBlue(cur);

    for (int idx = 1; idx < path.size(); ++idx) {
        if (m_stopRequested)
            break;

        while (m_paused && !m_stopRequested) {
            QApplication::processEvents();
            QThread::msleep(30);
        }
        if (m_stopRequested)
            break;

        const QPoint next = path[idx];
        const bool   cut  = realSegs.contains(key(cur, next));

        if (cut)
            m_motor.startJet();
        else
            m_motor.stopJet();

        moveHeadProgressive(cur, next, head, cut);

        if (cut)
            m_motor.moveCut(next.x() * mmPerPx, next.y() * mmPerPx);
        else
            m_motor.moveRapid(next.x() * mmPerPx, next.y() * mmPerPx);

        cur = next;
    }

    // 5) Nettoyage -------------------------------------------------------------
    m_motor.stopJet();
    m_visu->resetCutMarkers();          // suppression points/curseur
    m_visu->getScene()->removeItem(head);
    delete head;
    emit decoupeProgress(0, m_totalSteps);

    qDebug() << "Découpe terminée – Pas X=" << m_motor.getStepsX()
             << " Y=" << m_motor.getStepsY();
    qDebug() << "[DEBUG] m_mainWindow == nullptr ?" << (m_mainWindow == nullptr);

    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "setParamWidgetsEnabled",
                                  Qt::QueuedConnection, Q_ARG(bool, true));
        qDebug() << "[DEBUG] invokeMethod vers setSpinboxSliderEnabled(true)";

        QMetaObject::invokeMethod(m_mainWindow, "setSpinboxSliderEnabled",
                                  Qt::QueuedConnection, Q_ARG(bool, true));
    }

    if (m_visu)
        m_visu->setDecoupeEnCours(false);

    m_running = false;
    invokeLater(m_mainWindow, [this]() {
        QString titre;
        QString message;

        if (m_interrupted) {
            titre = "Découpe interrompue";
            message = "La découpe a été arrêtée manuellement.";
        } else {
            titre = "Découpe terminée";
            message = "La découpe s'est terminée avec succès.";
        }

        QMessageBox* msg = new QMessageBox(QMessageBox::Information,
                                           titre,
                                           message,
                                           QMessageBox::Ok,
                                           m_mainWindow);
        msg->setModal(false);
        msg->show();
    });


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
    if (m_visu)
        m_visu->setDecoupeEnCours(false);
    if (!m_running) return;
    m_stopRequested = true;
    m_paused        = false;
    m_interrupted = true;
    m_motor.stopJet();
}

void TrajetMotor::setMainWindow(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
    qDebug() << "[DEBUG] m_mainWindow défini dans TrajetMotor";
    qDebug() << "[DEBUG] setMainWindow appelé pour instance" << this;
}

void TrajetMotor::moveHeadProgressive(const QPoint& start, const QPoint& end,QGraphicsEllipseItem* head, bool cut)
{
    int dx = end.x() - start.x();
    int dy = end.y() - start.y();
    int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) return;

    for (int step = 1; step <= steps; ++step) {
        double t = static_cast<double>(step) / steps;
        int x = qRound(start.x() + t * dx);
        int y = qRound(start.y() + t * dy);

        QPoint pos(x, y);
        head->setPos(x - 3, y - 3);

        // Coloration dynamique seulement toutes les VIS_SAMPLE_PX étapes
        if (step % VIS_SAMPLE_PX == 0 || step == steps) {
            cut ? m_visu->colorPositionRed(pos)
                : m_visu->colorPositionBlue(pos);
        }

        if (step % VIS_UPDATE_INTERVAL == 0) {
            QApplication::processEvents();
            if (VIS_DELAY_MS > 0)
                QThread::msleep(VIS_DELAY_MS);
        }

        ++m_progressCounter;
        emit decoupeProgress(m_totalSteps - m_progressCounter, m_totalSteps);
    }
    QApplication::processEvents();
}
