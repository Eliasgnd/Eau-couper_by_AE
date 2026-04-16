#include "TrajetMotor.h"
#include "MachineViewModel.h"
#include "MainWindow.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QMessageBox>
#include <QDebug>
#include <algorithm>
#include <utility> // Pour std::as_const

// Calibration : 1 pixel sur l'écran = 1 mm sur la machine
static constexpr double mmPerPx = 1.0;

TrajetMotor::TrajetMotor(ShapeVisualization* visu, QWidget* parent)
    : QWidget(parent), m_visu(visu)
{}

TrajetMotor::~TrajetMotor()
{
    if (m_workerThread && m_workerThread->isRunning()) {
        m_stopRequested = true;
        m_paused = false;
        m_workerThread->wait(3000);
    }
}

void TrajetMotor::setMainWindow(MainWindow* mainWindow) { m_mainWindow = mainWindow; }
void TrajetMotor::setMachineViewModel(MachineViewModel* vm) { m_machine = vm; }

void TrajetMotor::setVcut(double vitesse_mm_s) { m_vCut = qBound(1.0, vitesse_mm_s, 200.0); }
void TrajetMotor::setVtravel(double vitesse_mm_s) { m_vTrav = qBound(1.0, vitesse_mm_s, 300.0); }

void TrajetMotor::executeTrajet()
{
    if (m_running) return;
    if (!m_visu || !m_visu->getScene()) return;

    m_running       = true;
    m_paused        = false;
    m_stopRequested = false;
    m_interrupted   = false;
    m_visu->setDecoupeEnCours(true);

    // Création du thread worker pour ne pas figer l'IHM
    m_workerThread = QThread::create([this]() { doExecuteTrajet(); });
    m_workerThread->setObjectName("TrajetWorker");

    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, this, [this]() { m_running = false; });

    m_workerThread->start();
}

void TrajetMotor::doExecuteTrajet()
{
    const QPoint homePos(600, 400); // Point de repos de la machine

    // 1. Extraction et Optimisation directes via le Planner
    QList<ContinuousCut> optimizedCuts;
    QMetaObject::invokeMethod(this, [this, &optimizedCuts, homePos]() {
        // C'est ICI que l'erreur est corrigée !
        optimizedCuts = PathPlanner::getOptimizedPaths(m_visu->getScene(), homePos);
    }, Qt::BlockingQueuedConnection);

    if (optimizedCuts.isEmpty()) {
        QMetaObject::invokeMethod(this, [this]() {
            m_visu->setDecoupeEnCours(false);
            m_running = false;
        }, Qt::QueuedConnection);
        return;
    }

    // 2. Préparation progression
    m_totalSteps = estimateTotalSteps(optimizedCuts, homePos);
    m_progressCounter = 0;
    emit decoupeProgress(m_totalSteps, m_totalSteps);

    // 3. Création du curseur visuel (tête de buse)
    QGraphicsEllipseItem *head = nullptr;
    QMetaObject::invokeMethod(this, [this, &head, homePos]() {
        head = new QGraphicsEllipseItem(-3, -3, 6, 6);
        head->setBrush(Qt::green);
        head->setPen(Qt::NoPen);
        head->setZValue(60);
        m_visu->getScene()->addItem(head);
        head->setPos(homePos.x() - 3, homePos.y() - 3);
    }, Qt::BlockingQueuedConnection);

    // 4. BOUCLE DE DÉCOUPE
    QPoint cur = homePos;

    // Utilisation de std::as_const pour corriger l'avertissement [clazy-range-loop-detach]
    for (const auto& cut : std::as_const(optimizedCuts)) {
        if (m_stopRequested) break;
        while (m_paused && !m_stopRequested) QThread::msleep(30);

        QPoint realStart = cut.points.first().toPoint();

        // --- TRAJET RAPIDE (Vanne fermée) ---
        if (cur != realStart) {
            moveHeadProgressive(cur, realStart, head, false);
            sendMoveToStm(cur, realStart, FLAG_VALVE_OFF, false, mmPerPx);
            cur = realStart;
        }

        // --- DÉCOUPE DE LA FORME (Vanne ouverte) ---
        for (int i = 1; i < cut.points.size(); ++i) {
            if (m_stopRequested) break;
            while (m_paused && !m_stopRequested) QThread::msleep(30);

            QPoint realEnd = cut.points[i].toPoint();
            bool isLastOfSequence = (i == cut.points.size() - 1 && &cut == &optimizedCuts.last());

            moveHeadProgressive(cur, realEnd, head, true);
            sendMoveToStm(cur, realEnd, FLAG_VALVE_ON, isLastOfSequence, mmPerPx);

            cur = realEnd;
        }
    }

    // --- RETOUR ORIGINE ---
    if (!m_stopRequested && cur != homePos) {
        moveHeadProgressive(cur, homePos, head, false);
        sendMoveToStm(cur, homePos, FLAG_VALVE_OFF, true, mmPerPx);
    }

    // Nettoyage final
    QMetaObject::invokeMethod(this, [this, head]() {
        m_visu->getScene()->removeItem(head);
        delete head;
        m_visu->setDecoupeEnCours(false);
        emit decoupeProgress(0, m_totalSteps);

        QString titre = m_interrupted ? tr("Découpe interrompue") : tr("Découpe terminée");
        QString msg = m_interrupted ? tr("L'opération a été stoppée.") : tr("Découpe réussie !");
        QMessageBox::information(m_mainWindow, titre, msg);
    }, Qt::QueuedConnection);
}

int TrajetMotor::estimateTotalSteps(const QList<ContinuousCut>& cuts, const QPoint& homePos) {
    int steps = 0;
    QPoint p = homePos;
    // std::as_const ici aussi
    for (const auto& cut : std::as_const(cuts)) {
        steps += std::max(std::abs(p.x() - cut.points.first().x()), std::abs(p.y() - cut.points.first().y()));
        for (int i = 1; i < cut.points.size(); ++i) {
            steps += std::max(std::abs(cut.points[i-1].x() - cut.points[i].x()),
                              std::abs(cut.points[i-1].y() - cut.points[i].y()));
        }
        p = cut.points.last().toPoint();
    }
    return steps;
}

void TrajetMotor::moveHeadProgressive(const QPoint& start, const QPoint& end,
                                      QGraphicsEllipseItem* head, bool cut)
{
    const double dist = QLineF(start, end).length();
    if (dist <= 0) return;

    const QPen pen(cut ? Qt::red : Qt::blue, 1.5);
    const double stepPx = cut ? 3.0 : 12.0;
    const int delayMs = cut ? 2 : 1;
    QPointF currentPos = start;

    // Correction de l'avertissement [FloatLoopCounter] en utilisant un entier (int)
    int numSteps = static_cast<int>(dist / stepPx);

    for (int i = 1; i <= numSteps; ++i) {
        if (m_stopRequested) return;

        double t = (i * stepPx) / dist;
        QPointF nextPos(start.x() + t * (end.x() - start.x()), start.y() + t * (end.y() - start.y()));

        QMetaObject::invokeMethod(this, [this, pen, currentPos, nextPos, head]() {
            auto *line = new QGraphicsLineItem(currentPos.x(), currentPos.y(), nextPos.x(), nextPos.y());
            line->setPen(pen);
            m_visu->getScene()->addItem(line);
            m_visu->addCutMarker(line);
            head->setPos(nextPos.x() - 3, nextPos.y() - 3);
        }, Qt::BlockingQueuedConnection);

        currentPos = nextPos;
        QThread::msleep(delayMs);
        m_progressCounter += static_cast<int>(stepPx);
        emit decoupeProgress(m_totalSteps - m_progressCounter, m_totalSteps);
    }

    // Tracer le dernier résidu de ligne si besoin
    if (currentPos.toPoint() != end) {
        QMetaObject::invokeMethod(this, [this, pen, currentPos, end, head]() {
            auto *line = new QGraphicsLineItem(currentPos.x(), currentPos.y(), end.x(), end.y());
            line->setPen(pen);
            m_visu->getScene()->addItem(line);
            m_visu->addCutMarker(line);
            head->setPos(end.x() - 3, end.y() - 3);
        }, Qt::BlockingQueuedConnection);
    }
}

bool TrajetMotor::sendMoveToStm(const QPoint& from, const QPoint& to,
                                uint8_t flags, bool isLast, double mmPerPxScale)
{
    if (!m_machine) return true;

    const double dxMm = (to.x() - from.x()) * mmPerPxScale;
    const double dyMm = (to.y() - from.y()) * mmPerPxScale;
    const int dxSteps = static_cast<int>(dxMm * STEPS_PER_MM);
    const int dySteps = static_cast<int>(dyMm * STEPS_PER_MM);

    const double vitesse = (flags & FLAG_VALVE_ON) ? m_vCut : m_vTrav;
    const uint16_t arr = speedToArr(vitesse);

    static constexpr int MAX_STEPS = 30000;
    const int totalAbs = qMax(qAbs(dxSteps), qAbs(dySteps));
    if (totalAbs == 0) return true;

    const int numChunks = (totalAbs + MAX_STEPS - 1) / MAX_STEPS;

    for (int i = 0; i < numChunks; ++i) {
        const double t0 = static_cast<double>(i) / numChunks;
        const double t1 = static_cast<double>(i + 1) / numChunks;

        StmSegment seg;
        seg.dx = static_cast<int16_t>(static_cast<int>(dxSteps * t1) - static_cast<int>(dxSteps * t0));
        seg.dy = static_cast<int16_t>(static_cast<int>(dySteps * t1) - static_cast<int>(dySteps * t0));
        seg.dz = 0;
        seg.v_max = arr;
        seg.flags = flags;

        if (isLast && i == numChunks - 1) seg.flags |= FLAG_END_SEQ;

        bool accepted = false;

        while (!m_stopRequested) {
            QMetaObject::invokeMethod(this, [this, seg, &accepted]() {
                if (m_machine) {
                    accepted = m_machine->sendSegment(seg);
                }
            }, Qt::BlockingQueuedConnection);

            if (accepted) break;
            QThread::msleep(10);
        }
    }
    return true;
}

void TrajetMotor::pause() { m_paused = true; if (m_machine) m_machine->sendValveOff(); }
void TrajetMotor::resume() { m_paused = false; }
void TrajetMotor::stopCut() { m_stopRequested = true; m_paused = false; m_interrupted = true; if (m_machine) m_machine->sendValveOff(); }
