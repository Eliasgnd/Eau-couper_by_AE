#include "TrajetMotor.h"
#include "MachineViewModel.h"
#include "pathplanner.h"
#include <QSet>
#include <QLineF>
#include <QThread>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QStringList>
#include <algorithm>
#include <limits>
#include <QMessageBox>
#include "MainWindow.h"

// --- Constantes de calibration ---
static constexpr double mmPerPx = 1.0;  // Calibration plateau

// -----------------------------------------------------------------------------
// Clé unique pour identifier un segment indépendamment de son sens
// -----------------------------------------------------------------------------
static inline quint64 keySeg(const QPoint &p1, const QPoint &p2) {
    quint32 k1 = (static_cast<quint32>(p1.x()) << 16) | (static_cast<quint16>(p1.y()) & 0xFFFF);
    quint32 k2 = (static_cast<quint32>(p2.x()) << 16) | (static_cast<quint16>(p2.y()) & 0xFFFF);
    return (static_cast<quint64>(qMin(k1, k2)) << 32) | qMax(k1, k2);
}

// -----------------------------------------------------------------------------
// Estimation du nombre total de pas (incluant le départ et retour à l'origine)
// -----------------------------------------------------------------------------
static int estimateTotalSteps(const QList<Segment>& segs, const QPoint& homePos)
{
    if (segs.isEmpty()) return 0;

    const auto stepLen = [](const QPoint& a, const QPoint& b) {
        return std::max(std::abs(a.x() - b.x()), std::abs(a.y() - b.y()));
    };

    QSet<quint64> done;
    QPoint cur = homePos;
    int steps = 0;

    while (done.size() < segs.size()) {
        int bestIdx = -1;
        double bestDi = std::numeric_limits<double>::max();
        bool reverseCut = false;

        for (int i = 0; i < segs.size(); ++i) {
            if (done.contains(keySeg(segs[i].a, segs[i].b))) continue;

            double d_a = QLineF(cur, segs[i].a).length();
            double d_b = QLineF(cur, segs[i].b).length();

            if (d_a < bestDi) { bestDi = d_a; bestIdx = i; reverseCut = false; }
            if (d_b < bestDi) { bestDi = d_b; bestIdx = i; reverseCut = true; }
        }
        if (bestIdx < 0) break;
        const auto s = segs[bestIdx];

        QPoint realStart = reverseCut ? s.b : s.a;
        QPoint realEnd   = reverseCut ? s.a : s.b;

        steps += stepLen(cur, realStart);
        steps += stepLen(realStart, realEnd);

        done.insert(keySeg(s.a, s.b));
        cur = realEnd;
    }
    steps += stepLen(cur, homePos);

    return steps;
}

// -----------------------------------------------------------------------------
// Constructeur / Destructeur
// -----------------------------------------------------------------------------
TrajetMotor::TrajetMotor(ShapeVisualization* visu, QWidget* parent)
    : QWidget(parent), m_visu(visu)
{}

TrajetMotor::~TrajetMotor()
{
    if (m_workerThread && m_workerThread->isRunning()) {
        m_stopRequested = true;
        m_paused        = false;
        m_workerThread->wait(5000);
    }
}

// -----------------------------------------------------------------------------
// Lancement de la découpe — démarre le thread worker
// -----------------------------------------------------------------------------
void TrajetMotor::executeTrajet()
{
    if (m_running) {
        qWarning() << "Découpe déjà en cours.";
        return;
    }
    if (!m_visu || !m_visu->getScene()) return;

    m_running       = true;
    m_paused        = false;
    m_stopRequested = false;
    m_interrupted   = false;
    m_visu->setDecoupeEnCours(true);

    m_workerThread = QThread::create([this]() { doExecuteTrajet(); });
    m_workerThread->setObjectName("TrajetWorker");

    // Nettoyage automatique après fin du thread
    connect(m_workerThread, &QThread::finished,
            m_workerThread, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, this, [this]() {
        m_workerThread = nullptr;
    }, Qt::QueuedConnection);

    m_workerThread->start();
}

// -----------------------------------------------------------------------------
// doExecuteTrajet — tourne dans le thread worker.
// Toutes les opérations sur QGraphicsScene et UART sont dispatched sur le
// thread principal via QMetaObject::invokeMethod (BlockingQueuedConnection).
// Les QThread::msleep() n'endorment QUE le worker → l'UI reste réactive.
// -----------------------------------------------------------------------------
void TrajetMotor::doExecuteTrajet()
{
    const QPoint homePos(600, 400);

    QStringList commandBuffer;
    commandBuffer << "G90 ; Mode de positionnement Absolu (mm)";
    commandBuffer << "G21 ; Unites en millimètres";
    commandBuffer << "M5  ; Assure que le jet d'eau est coupé au depart";

    // 1) Extraction des segments (accès scène → thread principal)
    QList<Segment> segs;
    QMetaObject::invokeMethod(this, [this, &segs]() {
        segs = PathPlanner::extractSegments(m_visu->getScene());
    }, Qt::BlockingQueuedConnection);

    if (segs.isEmpty()) {
        QMetaObject::invokeMethod(this, [this]() {
            m_visu->setDecoupeEnCours(false);
            m_running = false;
        }, Qt::QueuedConnection);
        return;
    }

    // 2) Estimation progression
    m_totalSteps      = estimateTotalSteps(segs, homePos);
    m_progressCounter = 0;
    emit decoupeProgress(m_totalSteps, m_totalSteps);

    // 3) Créer le curseur vert sur le thread principal
    QGraphicsEllipseItem *head = nullptr;
    QMetaObject::invokeMethod(this, [this, &head, homePos]() {
        head = new QGraphicsEllipseItem(-3, -3, 6, 6);
        head->setBrush(Qt::green);
        head->setPen(Qt::NoPen);
        head->setZValue(60);
        m_visu->getScene()->addItem(head);
        head->setPos(homePos.x() - 3, homePos.y() - 3);
    }, Qt::BlockingQueuedConnection);

    // 4) Boucle principale (worker thread — chemin critique sans bloc UI)
    QSet<quint64> done;
    QPoint cur = homePos;

    while (done.size() < segs.size()) {

        if (m_stopRequested) break;
        while (m_paused && !m_stopRequested)
            QThread::msleep(30);
        if (m_stopRequested) break;

        int    bestIdx    = -1;
        double bestDi     = std::numeric_limits<double>::max();
        bool   reverseCut = false;

        for (int i = 0; i < segs.size(); ++i) {
            if (done.contains(keySeg(segs[i].a, segs[i].b))) continue;

            const double d_a = QLineF(cur, segs[i].a).length();
            const double d_b = QLineF(cur, segs[i].b).length();

            if (d_a < bestDi) { bestDi = d_a; bestIdx = i; reverseCut = false; }
            if (d_b < bestDi) { bestDi = d_b; bestIdx = i; reverseCut = true; }
        }
        if (bestIdx < 0) break;

        const auto   s         = segs[bestIdx];
        const QPoint realStart = reverseCut ? s.b : s.a;
        const QPoint realEnd   = reverseCut ? s.a : s.b;

        // ---------- Déplacement rapide (bleu) ----------
        if (cur != realStart) {
            m_motor.stopJet();
            commandBuffer << "M5 ; Arrêt du jet";
            commandBuffer << QString("G0 X%1 Y%2").arg(realStart.x() * mmPerPx)
                                                   .arg(realStart.y() * mmPerPx);
            moveHeadProgressive(cur, realStart, head, false);
            m_motor.moveRapid(realStart.x() * mmPerPx, realStart.y() * mmPerPx);
            const QPoint from = cur, to = realStart;
            QMetaObject::invokeMethod(this, [this, from, to]() {
                sendMoveToStm(from, to, FLAG_VALVE_OFF, false, mmPerPx);
            }, Qt::BlockingQueuedConnection);
        }
        cur = realStart;

        // ---------- Coupe (rouge) ----------
        m_motor.startJet();
        commandBuffer << "M3 ; Démarrage du jet";
        commandBuffer << QString("G1 X%1 Y%2 F1000").arg(realEnd.x() * mmPerPx)
                                                     .arg(realEnd.y() * mmPerPx);
        moveHeadProgressive(realStart, realEnd, head, true);
        m_motor.moveCut(realEnd.x() * mmPerPx, realEnd.y() * mmPerPx);
        {
            const QPoint from = realStart, to = realEnd;
            QMetaObject::invokeMethod(this, [this, from, to]() {
                sendMoveToStm(from, to, FLAG_VALVE_ON, false, mmPerPx);
            }, Qt::BlockingQueuedConnection);
        }

        done.insert(keySeg(s.a, s.b));
        cur = realEnd;
    }

    // --- Retour au point de départ (HOME) ---
    if (!m_stopRequested && cur != homePos) {
        m_motor.stopJet();
        commandBuffer << "M5 ; Arrêt du jet";
        commandBuffer << QString("G0 X%1 Y%2 ; Retour Origine").arg(homePos.x() * mmPerPx)
                                                                .arg(homePos.y() * mmPerPx);
        moveHeadProgressive(cur, homePos, head, false);
        m_motor.moveRapid(homePos.x() * mmPerPx, homePos.y() * mmPerPx);
        const QPoint from = cur, to = homePos;
        QMetaObject::invokeMethod(this, [this, from, to]() {
            sendMoveToStm(from, to, FLAG_VALVE_OFF, true, mmPerPx);
        }, Qt::BlockingQueuedConnection);
    }

    commandBuffer << "M30 ; Fin du programme de découpe";

    qDebug() << "\n============= BUFFER  (G-CODE) =============";
    for (const QString& cmd : commandBuffer)
        qDebug().noquote() << cmd;
    qDebug() << "=================================================\n";

    m_motor.stopJet();
    const bool wasInterrupted = m_interrupted.load();

    // Nettoyage final sur le thread principal
    QMetaObject::invokeMethod(this, [this, head, wasInterrupted]() {
        m_visu->resetCutMarkers();
        m_visu->getScene()->removeItem(head);
        delete head;
        emit decoupeProgress(0, m_totalSteps);

        if (m_mainWindow)
            QMetaObject::invokeMethod(m_mainWindow, "setSpinboxSliderEnabled",
                                      Qt::QueuedConnection, Q_ARG(bool, true));

        m_visu->setDecoupeEnCours(false);
        m_running = false;

        const QString titre   = wasInterrupted ? tr("Découpe interrompue") : tr("Découpe terminée");
        const QString message = wasInterrupted ? tr("La découpe a été arrêtée manuellement.")
                                               : tr("La découpe s'est terminée avec succès.");
        auto *msg = new QMessageBox(QMessageBox::Information, titre, message,
                                    QMessageBox::Ok, m_mainWindow);
        msg->setModal(false);
        msg->show();
    }, Qt::QueuedConnection);
}

// -----------------------------------------------------------------------------
// Slots Pause / Reprendre / Stop
// -----------------------------------------------------------------------------
void TrajetMotor::pause()  { if (m_running) { m_paused = true;  m_motor.stopJet(); } }
void TrajetMotor::resume() { if (m_running) { m_paused = false; } }
void TrajetMotor::stopCut()
{
    if (m_visu) m_visu->setDecoupeEnCours(false);
    if (!m_running) return;
    m_stopRequested = true;
    m_paused        = false;
    m_interrupted   = true;
    m_motor.stopJet();
}

void TrajetMotor::setMainWindow(MainWindow* mainWindow) { m_mainWindow = mainWindow; }
void TrajetMotor::setMachineViewModel(MachineViewModel* vm) { m_machine = vm; }

void TrajetMotor::setVcut(double vitesse_mm_s)
{
    m_motor.Vcut = qBound(1.0, vitesse_mm_s, 200.0);
}

// -----------------------------------------------------------------------------
// moveHeadProgressive — appelée depuis le thread worker.
// Les ajouts dans la scène sont dispatched sur le thread principal (BlockingQueued),
// puis le worker fait son msleep → l'UI est libre pendant toute la pause.
// -----------------------------------------------------------------------------
void TrajetMotor::moveHeadProgressive(const QPoint& start, const QPoint& end,
                                      QGraphicsEllipseItem* head, bool cut)
{
    const double dist = QLineF(start, end).length();
    if (dist <= 0) return;

    const QPen   pen(cut ? Qt::red : Qt::blue, 1.5);
    const double stepPx   = cut ? 3.0 : 12.0;
    // Délai synchronisé avec la vitesse réelle : t = d/v × 1000 ms
    const double vitesse  = cut ? m_motor.Vcut : m_motor.Vtrav;
    const int    delayMs  = qBound(1, (int)(stepPx * mmPerPx * 1000.0 / vitesse), 500);
    QPointF      currentPos = start;

    for (double d = stepPx; d < dist; d += stepPx) {
        if (m_stopRequested) return;

        const double  t       = d / dist;
        const QPointF nextPos(start.x() + t * (end.x() - start.x()),
                              start.y() + t * (end.y() - start.y()));
        const QPointF cp = currentPos;
        const QPointF np = nextPos;

        // Opérations scène sur le thread principal (rapide < 0.5ms)
        QMetaObject::invokeMethod(this, [this, pen, cp, np, head]() {
            auto *line = new QGraphicsLineItem(cp.x(), cp.y(), np.x(), np.y());
            line->setPen(pen);
            m_visu->getScene()->addItem(line);
            m_visu->addCutMarker(line);
            head->setPos(np.x() - 3, np.y() - 3);
        }, Qt::BlockingQueuedConnection);

        currentPos = nextPos;

        // Pause du worker uniquement — le thread UI est libre pendant ce délai
        QThread::msleep(delayMs);

        m_progressCounter += static_cast<int>(stepPx);
        if (m_progressCounter > m_totalSteps) m_progressCounter = m_totalSteps;
        emit decoupeProgress(m_totalSteps - m_progressCounter, m_totalSteps);
    }

    // Dernier tronçon résiduel
    if (currentPos.toPoint() != end) {
        const QPointF cp = currentPos;
        QMetaObject::invokeMethod(this, [this, pen, cp, end, head]() {
            auto *line = new QGraphicsLineItem(cp.x(), cp.y(), end.x(), end.y());
            line->setPen(pen);
            m_visu->getScene()->addItem(line);
            m_visu->addCutMarker(line);
            head->setPos(end.x() - 3, end.y() - 3);
        }, Qt::BlockingQueuedConnection);
    }
}

// -----------------------------------------------------------------------------
// sendMoveToStm — envoie un déplacement en segments relatifs vers le STM.
// Doit être appelé depuis le thread principal (via invokeMethod depuis le worker).
// -----------------------------------------------------------------------------
void TrajetMotor::sendMoveToStm(const QPoint& from, const QPoint& to,
                                 uint8_t flags, bool isLast, double mmPerPxScale)
{
    if (!m_machine) return;

    const double dxMm    = (to.x() - from.x()) * mmPerPxScale;
    const double dyMm    = (to.y() - from.y()) * mmPerPxScale;
    const int    dxSteps = static_cast<int>(dxMm * STEPS_PER_MM);
    const int    dySteps = static_cast<int>(dyMm * STEPS_PER_MM);

    const double   vitesse = (flags & FLAG_VALVE_ON) ? m_motor.Vcut : m_motor.Vtrav;
    const uint16_t arr     = speedToArr(vitesse);

    static constexpr int MAX_STEPS = 30000;
    const int totalAbs = qMax(qAbs(dxSteps), qAbs(dySteps));

    if (totalAbs == 0) return;

    const int numChunks = (totalAbs + MAX_STEPS - 1) / MAX_STEPS;

    for (int i = 0; i < numChunks; ++i) {
        const double t0 = static_cast<double>(i)     / numChunks;
        const double t1 = static_cast<double>(i + 1) / numChunks;

        const int chunkDx = static_cast<int>(dxSteps * t1) - static_cast<int>(dxSteps * t0);
        const int chunkDy = static_cast<int>(dySteps * t1) - static_cast<int>(dySteps * t0);

        StmSegment seg;
        seg.dx    = static_cast<int16_t>(chunkDx);
        seg.dy    = static_cast<int16_t>(chunkDy);
        seg.dz    = 0;
        seg.v_max = arr;
        seg.flags = flags;

        if (isLast && i == numChunks - 1)
            seg.flags |= FLAG_END_SEQ;

        m_machine->sendSegment(seg);
    }
}
