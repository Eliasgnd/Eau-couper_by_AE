#include "TrajetMotor.h"
#include "MachineViewModel.h"
#include "MainWindow.h"

#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QPainter>
#include <QMessageBox>
#include <QMutexLocker>
#include <algorithm>
#include <cmath>
#include <utility>

static constexpr double mmPerPx = 1.0;

// Facteur de résolution du pixmap de tracé.
// Modifier UNIQUEMENT cette ligne pour ajuster la qualité (3.0 = excellent, O(1) garanti).
static constexpr qreal RENDER_SCALE = 3.0;

namespace {
qint64 estimateSegmentDurationMs(const StmSegment& seg, bool isCut, double cutSpeedMmS, double travelSpeedMmS)
{
    const double speedMmS = isCut ? cutSpeedMmS : travelSpeedMmS;
    if (speedMmS <= 0.0)
        return 0;

    const int dominantSteps = qMax(qAbs(seg.dx), qAbs(seg.dy));
    if (dominantSteps <= 0)
        return 0;

    const double durationMs = (static_cast<double>(dominantSteps) * 1000.0)
                              / (speedMmS * STEPS_PER_MM);
    return qMax<qint64>(1, static_cast<qint64>(std::llround(durationMs)));
}
}

TrajetMotor::TrajetMotor(ShapeVisualization* visu, QWidget* parent)
    : QWidget(parent), m_visu(visu)
{}

TrajetMotor::~TrajetMotor()
{
    if (m_workerThread && m_workerThread->isRunning()) {
        m_stopRequested = true;
        m_paused = false;
        {
            QMutexLocker lk(&m_doneMutex);
            m_doneCond.wakeAll();
        }
        m_workerThread->wait(3000);
    }
}

void TrajetMotor::setMainWindow(MainWindow* mainWindow) { m_mainWindow = mainWindow; }
void TrajetMotor::setMachineViewModel(MachineViewModel* vm) { m_machine = vm; }
void TrajetMotor::setVcut(double vitesse_mm_s)    { m_vCut  = qBound(1.0, vitesse_mm_s, 200.0); }
void TrajetMotor::setVtravel(double vitesse_mm_s) { m_vTrav = qBound(1.0, vitesse_mm_s, 300.0); }

bool TrajetMotor::prepareTrajet()
{
    if (m_running || !m_visu || !m_visu->getScene())
        return false;
    return buildPlannedSegments();
}

void TrajetMotor::executeTrajet()
{
    if (m_running)
        return;
    if (!m_visu || !m_visu->getScene())
        return;
    if (m_plannedSegments.isEmpty() && !buildPlannedSegments())
        return;

    m_running       = true;
    m_paused        = false;
    m_stopRequested = false;
    m_interrupted   = false;
    m_doneReceived  = false;
    m_visu->setDecoupeEnCours(true);

    m_workerThread = QThread::create([this]() { doExecuteTrajet(); });
    m_workerThread->setObjectName("TrajetWorker");

    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, this, [this]() { m_running = false; });

    m_workerThread->start();
}

bool TrajetMotor::buildPlannedSegments()
{
    const QPoint homePos(HOME_X, HOME_Y);

    const QList<ContinuousCut> rawCuts = PathPlanner::extractRawPaths(m_visu->getScene());

    QList<ContinuousCut> optimizedCuts = PathPlanner::computeOptimizedPaths(rawCuts, homePos);

    if (optimizedCuts.isEmpty())
        return false;

    m_plannedSegments.clear();
    m_remainingMsByCompletedSegments.clear();
    QPoint cur = homePos;

    auto planMove = [&](const QPoint& from, const QPoint& to, uint8_t flags, bool isLast) {
        const double dxMm = (to.x() - from.x()) * mmPerPx;
        const double dyMm = (to.y() - from.y()) * mmPerPx;
        const int dxSteps = static_cast<int>(dxMm * STEPS_PER_MM);
        const int dySteps = static_cast<int>(dyMm * STEPS_PER_MM);
        const double vitesse = (flags & FLAG_VALVE_ON) ? m_vCut : m_vTrav;
        const uint16_t arr = speedToArr(vitesse);

        static constexpr int MAX_STEPS = 30000;
        const int totalAbs = qMax(qAbs(dxSteps), qAbs(dySteps));
        if (totalAbs == 0)
            return;

        const int numChunks = (totalAbs + MAX_STEPS - 1) / MAX_STEPS;
        for (int i = 0; i < numChunks; ++i) {
            const double t0 = static_cast<double>(i) / numChunks;
            const double t1 = static_cast<double>(i + 1) / numChunks;

            StmSegment seg;
            seg.dx = static_cast<int32_t>(static_cast<int>(dxSteps * t1) - static_cast<int>(dxSteps * t0));
            seg.dy = static_cast<int32_t>(static_cast<int>(dySteps * t1) - static_cast<int>(dySteps * t0));
            seg.dz = 0;
            seg.v_max = arr;
            seg.flags = flags;
            if (isLast && i == numChunks - 1)
                seg.flags |= FLAG_END_SEQ;

            const bool isCut = (flags & FLAG_VALVE_ON) != 0;
            m_plannedSegments.append({seg, isCut, estimateSegmentDurationMs(seg, isCut, m_vCut, m_vTrav)});
        }
    };

    for (int c = 0; c < optimizedCuts.size(); ++c) {
        const ContinuousCut& cut = optimizedCuts[c];
        QPoint realStart = cut.points.first().toPoint();

        qDebug() << "MOUVEMENT: de" << cur << "vers" << realStart;

        if (cur != realStart)
            planMove(cur, realStart, FLAG_VALVE_OFF, false);
        cur = realStart;

        for (int i = 1; i < cut.points.size(); ++i) {
            QPoint realEnd = cut.points[i].toPoint();

            const bool isLastShapePoint = (i == cut.points.size() - 1 && c == optimizedCuts.size() - 1);
            const bool isLast = isLastShapePoint && (realEnd == homePos);

            planMove(cur, realEnd, FLAG_VALVE_ON, isLast);
            cur = realEnd;
        }
    }

    if (cur != homePos)
        planMove(cur, homePos, FLAG_VALVE_OFF, true);

    m_remainingMsByCompletedSegments.resize(m_plannedSegments.size() + 1);
    qint64 remainingMs = 0;
    for (int i = m_plannedSegments.size() - 1; i >= 0; --i) {
        remainingMs += m_plannedSegments[i].estimatedDurationMs;
        m_remainingMsByCompletedSegments[i] = remainingMs;
    }
    m_remainingMsByCompletedSegments[m_plannedSegments.size()] = 0;
    return !m_plannedSegments.isEmpty();
}

void TrajetMotor::doExecuteTrajet()
{
    const QPoint homePos(HOME_X, HOME_Y);

    if (m_plannedSegments.isEmpty()) {
        QMetaObject::invokeMethod(this, [this]() {
            m_visu->setDecoupeEnCours(false);
            m_running = false;
        }, Qt::QueuedConnection);
        return;
    }

    m_isCurrentlyCutting.store(m_plannedSegments.isEmpty() ? false : m_plannedSegments[0].isCut);

    QMetaObject::invokeMethod(this, [this, homePos]() {
        m_head = new QGraphicsEllipseItem(-3, -3, 6, 6);
        m_head->setBrush(Qt::green);
        m_head->setPen(Qt::NoPen);
        m_head->setZValue(60);
        m_visu->getScene()->addItem(m_head);
        m_head->setPos(homePos.x() - 3, homePos.y() - 3);
        m_lastHeadPos = homePos;

        // --- Rendu offscreen O(1) : un seul QPixmap, peint segment par segment ---
        const QRectF sr = m_visu->getScene()->sceneRect();
        const int pw = qMax(1, qCeil(sr.width()  * RENDER_SCALE));
        const int ph = qMax(1, qCeil(sr.height() * RENDER_SCALE));
        m_cutPixmap = QPixmap(pw, ph);
        m_cutPixmap.fill(Qt::transparent);

        m_cutPixmapItem = new QGraphicsPixmapItem();
        m_cutPixmapItem->setPixmap(m_cutPixmap);
        m_cutPixmapItem->setPos(sr.topLeft());
        m_cutPixmapItem->setScale(1.0 / RENDER_SCALE);          // compense l'agrandissement
        m_cutPixmapItem->setTransformationMode(Qt::SmoothTransformation);
        m_cutPixmapItem->setZValue(54);
        m_visu->getScene()->addItem(m_cutPixmapItem);
    }, Qt::BlockingQueuedConnection);

    // IMPORTANT : Réinitialise le STM (seg_executed=0, last_received_seq_id=0xFFFF).
    // Sans ce CLEAR, l'anti-doublon du STM peut rejeter les premières trames binaires
    // si leur seq_id correspond à celui de la session précédente → pas d'ACK → timeout UART.
    QMetaObject::invokeMethod(this, [this]() {
        if (m_machine) m_machine->sendClear();
    }, Qt::BlockingQueuedConnection);
    QThread::msleep(50); // laisse le STM traiter CLEAR avant PRESTART

    // PRESTART : annonce la session et le nombre de segments au STM.
    // MachineViewModel exige m_prestartAccepted=true avant d'accepter sendSegment().
    // requestPrestart() → STM répond PRESTART_OK → onPrestartAccepted() → m_prestartAccepted=true.
    const int totalSegments = m_plannedSegments.size();
    QMetaObject::invokeMethod(this, [this, totalSegments]() {
        if (m_machine) m_machine->requestPrestart(totalSegments);
    }, Qt::BlockingQueuedConnection);

    // Attendre que le STM confirme PRESTART_OK (max 3 s)
    const int maxWaitMs = 3000;
    const int pollMs    = 20;
    for (int waited = 0; waited < maxWaitMs && !m_stopRequested; waited += pollMs) {
        bool accepted = false;
        QMetaObject::invokeMethod(this, [this, &accepted]() {
            accepted = m_machine && m_machine->isPrestartAccepted();
        }, Qt::BlockingQueuedConnection);
        if (accepted) break;
        QThread::msleep(pollMs);
    }

    emit decoupeProgress(0, qMax(1, totalSegments), m_remainingMsByCompletedSegments.value(0));

    for (int i = 0; i < totalSegments; ++i) {
        if (m_stopRequested)
            break;
        while (m_paused && !m_stopRequested)
            QThread::msleep(30);

        bool accepted = false;
        while (!m_stopRequested) {
            QMetaObject::invokeMethod(this, [&]() {
                if (m_machine)
                    accepted = m_machine->sendSegment(m_plannedSegments[i].stmSeg);
            }, Qt::BlockingQueuedConnection);

            if (accepted)
                break;
            QThread::msleep(10);
        }
    }

    if (!m_stopRequested) {
        QMutexLocker lk(&m_doneMutex);
        while (!m_doneReceived.load() && !m_stopRequested.load())
            m_doneCond.wait(&m_doneMutex, 200);
    }

    const bool success = !m_interrupted.load();

    QMetaObject::invokeMethod(this, [this, success, totalSegments]() {
        if (m_head) {
            m_visu->getScene()->removeItem(m_head);
            delete m_head;
            m_head = nullptr;
        }
        m_visu->setDecoupeEnCours(false);
        emit decoupeProgress(totalSegments, qMax(1, totalSegments), 0);
        emit decoupeFinished(success);

    }, Qt::QueuedConnection);
}

void TrajetMotor::onPositionUpdated(int x_steps, int y_steps)
{
    if (!m_head)
        return;

    const double realX_mm = x_steps / static_cast<double>(STEPS_PER_MM);
    const double realY_mm = y_steps / static_cast<double>(STEPS_PER_MM);
    const QPointF realPos(HOME_X + realX_mm, HOME_Y + realY_mm);

    const bool isCut = m_isCurrentlyCutting.load();

    QMetaObject::invokeMethod(this, [this, realPos, isCut]() {
        if (m_lastHeadPos != realPos && m_lastHeadPos != QPointF(0,0) && m_cutPixmapItem) {
            // Rendu O(1) : on peint directement sur le QPixmap, pas de nouvel item Qt
            QPainter painter(&m_cutPixmap);
            painter.setRenderHint(QPainter::Antialiasing, true);

            const QRectF sr = m_visu->getScene()->sceneRect();
            const qreal ox = sr.left();
            const qreal oy = sr.top();

            const QPointF from((m_lastHeadPos.x() - ox) * RENDER_SCALE,
                               (m_lastHeadPos.y() - oy) * RENDER_SCALE);
            const QPointF to((realPos.x() - ox) * RENDER_SCALE,
                             (realPos.y() - oy) * RENDER_SCALE);

            // Trait rouge = découpe, bleu = trajet à vide — épaisseur échelonnée
            painter.setPen(QPen(isCut ? Qt::red : Qt::blue,
                                (isCut ? 1.5 : 1.0) * RENDER_SCALE,
                                Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(from, to);
            painter.end();

            m_cutPixmapItem->setPixmap(m_cutPixmap);
        }
        m_head->setPos(realPos.x() - 3, realPos.y() - 3);
        m_lastHeadPos = realPos;
        m_visu->updateHeadLogicalPositionFromScene(realPos);
    }, Qt::QueuedConnection);
}

void TrajetMotor::onSegmentExecuted(int seg, int /*x_steps*/, int /*y_steps*/)
{
    if (seg >= 0 && seg < m_plannedSegments.size())
        m_isCurrentlyCutting.store(m_plannedSegments[seg].isCut);

    const int completedSegments = qBound(0, seg, m_plannedSegments.size());
    const int totalSegments = qMax(1, m_plannedSegments.size());
    const qint64 remainingMs = m_remainingMsByCompletedSegments.value(completedSegments, 0);
    emit decoupeProgress(completedSegments, totalSegments, remainingMs);
}

void TrajetMotor::onMachineDone()
{
    QMutexLocker lk(&m_doneMutex);
    m_doneReceived = true;
    m_doneCond.wakeAll();
}

void TrajetMotor::pause() { m_paused = true; }
void TrajetMotor::resume() { m_paused = false; }
void TrajetMotor::stopCut()
{
    m_stopRequested = true;
    m_paused = false;
    m_interrupted = true;
    QMutexLocker lk(&m_doneMutex);
    m_doneCond.wakeAll();
}

int TrajetMotor::estimateTotalSteps(const QList<ContinuousCut>& cuts, const QPoint& homePos)
{
    int steps = 0;
    QPoint p = homePos;

    for (int c = 0; c < cuts.size(); ++c) {
        const ContinuousCut& cut = cuts[c];

        steps += static_cast<int>(std::max(std::abs(static_cast<double>(p.x() - cut.points.first().x())),
                                           std::abs(static_cast<double>(p.y() - cut.points.first().y()))));
        for (int i = 1; i < cut.points.size(); ++i) {
            steps += static_cast<int>(std::max(std::abs(static_cast<double>(cut.points[i - 1].x() - cut.points[i].x())),
                                               std::abs(static_cast<double>(cut.points[i - 1].y() - cut.points[i].y()))));
        }
        p = cut.points.last().toPoint();
    }
    return steps;
}

void TrajetMotor::onValveOn()
{
    m_isCurrentlyCutting.store(true);
}

void TrajetMotor::onValveOff()
{
    m_isCurrentlyCutting.store(false);
}

void TrajetMotor::appendSegPlan(const QPoint&, const QPoint&, bool, QVector<bool>&) const {}
void TrajetMotor::onAnimStep() {}
bool TrajetMotor::sendMoveToStm(const QPoint&, const QPoint&, uint8_t, bool, double) { return true; }
