#include "TrajetMotor.h"
#include "MachineViewModel.h"
#include "MainWindow.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QMessageBox>
#include <QMutexLocker>
#include <QDebug>
#include <algorithm>
#include <utility>

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

// ----------------------------------------------------------
//  Lancement du thread de découpe
// ----------------------------------------------------------

void TrajetMotor::executeTrajet()
{
    if (m_running) return;
    if (!m_visu || !m_visu->getScene()) return;

    m_running       = true;
    m_paused        = false;
    m_stopRequested = false;
    m_interrupted   = false;
    m_doneReceived  = false;
    m_execCount     = 0;
    m_segIsCut.clear();
    m_visu->setDecoupeEnCours(true);

    m_workerThread = QThread::create([this]() { doExecuteTrajet(); });
    m_workerThread->setObjectName("TrajetWorker");

    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, this, [this]() { m_running = false; });

    m_workerThread->start();
}

// ----------------------------------------------------------
//  Corps du thread de découpe
// ----------------------------------------------------------

void TrajetMotor::doExecuteTrajet()
{
    const QPoint homePos(HOME_X, HOME_Y);

    // 1. Extraction et optimisation du trajet
    QList<ContinuousCut> optimizedCuts;
    QMetaObject::invokeMethod(this, [this, &optimizedCuts, homePos]() {
        optimizedCuts = PathPlanner::getOptimizedPaths(m_visu->getScene(), homePos);
    }, Qt::BlockingQueuedConnection);

    if (optimizedCuts.isEmpty()) {
        QMetaObject::invokeMethod(this, [this]() {
            m_visu->setDecoupeEnCours(false);
            m_running = false;
        }, Qt::QueuedConnection);
        return;
    }

    // 2. Pré-construction du plan de segments (isCut par chunk)
    //    Doit être fait avant l'envoi pour éviter les races avec onSegmentExecuted.
    {
        QVector<bool> plan;
        QPoint p = homePos;
        for (const auto& cut : std::as_const(optimizedCuts)) {
            QPoint start = cut.points.first().toPoint();
            if (p != start)
                appendSegPlan(p, start, false, plan); // voyage rapide
            p = start;
            for (int i = 1; i < cut.points.size(); ++i) {
                QPoint end = cut.points[i].toPoint();
                appendSegPlan(p, end, true, plan);    // coupe
                p = end;
            }
        }
        if (p != homePos)
            appendSegPlan(p, homePos, false, plan);   // retour origine

        m_segIsCut = plan;
        m_totalSteps = plan.size(); // on utilise le nb de segments comme base de progression
    }

    // 3. Création du curseur visuel (tête de buse) sur le thread principal
    QMetaObject::invokeMethod(this, [this, homePos]() {
        m_head = new QGraphicsEllipseItem(-3, -3, 6, 6);
        m_head->setBrush(Qt::green);
        m_head->setPen(Qt::NoPen);
        m_head->setZValue(60);
        m_visu->getScene()->addItem(m_head);
        m_head->setPos(homePos.x() - 3, homePos.y() - 3);
        m_lastHeadPos = homePos;
    }, Qt::BlockingQueuedConnection);

    // 4. Boucle d'envoi des segments au STM (sans animation temporelle)
    QPoint cur = homePos;

    for (const auto& cut : std::as_const(optimizedCuts)) {
        if (m_stopRequested) break;
        while (m_paused && !m_stopRequested) QThread::msleep(30);

        QPoint realStart = cut.points.first().toPoint();

        // Trajet rapide (vanne fermée)
        if (cur != realStart) {
            sendMoveToStm(cur, realStart, FLAG_VALVE_OFF, false, mmPerPx);
            cur = realStart;
        }

        // Découpe (vanne ouverte)
        for (int i = 1; i < cut.points.size(); ++i) {
            if (m_stopRequested) break;
            while (m_paused && !m_stopRequested) QThread::msleep(30);

            QPoint realEnd = cut.points[i].toPoint();
            bool isLastOfSequence = (i == cut.points.size() - 1 && &cut == &optimizedCuts.last());

            sendMoveToStm(cur, realEnd, FLAG_VALVE_ON, isLastOfSequence, mmPerPx);
            cur = realEnd;
        }
    }

    // Retour origine
    if (!m_stopRequested && cur != homePos) {
        sendMoveToStm(cur, homePos, FLAG_VALVE_OFF, true, mmPerPx);
    }

    // 5. Attente du signal DONE du STM (ou arrêt forcé)
    if (!m_stopRequested) {
        QMutexLocker lk(&m_doneMutex);
        while (!m_doneReceived.load() && !m_stopRequested.load()) {
            m_doneCond.wait(&m_doneMutex, 200);
        }
    }

    // 6. Nettoyage final sur le thread principal
    const bool success = !m_interrupted.load();
    QMetaObject::invokeMethod(this, [this, success]() {
        if (m_head) {
            m_visu->getScene()->removeItem(m_head);
            delete m_head;
            m_head = nullptr;
        }
        m_visu->setDecoupeEnCours(false);
        emit decoupeProgress(0, qMax(1, m_totalSteps));

        // Signal vers CuttingService pour réactiver les contrôles
        emit decoupeFinished(success);

        // Boîte de dialogue finale
        const QString titre = success ? tr("Découpe terminée") : tr("Découpe interrompue");
        const QString msg   = success ? tr("Découpe réussie !") : tr("L'opération a été stoppée.");
        QMessageBox::information(m_mainWindow, titre, msg);
    }, Qt::QueuedConnection);
}

// ----------------------------------------------------------
//  Pré-construction du plan de segments
// ----------------------------------------------------------

void TrajetMotor::appendSegPlan(const QPoint& from, const QPoint& to,
                                bool isCut, QVector<bool>& plan) const
{
    const int dxSteps = static_cast<int>((to.x() - from.x()) * mmPerPx * STEPS_PER_MM);
    const int dySteps = static_cast<int>((to.y() - from.y()) * mmPerPx * STEPS_PER_MM);
    const int total   = qMax(qAbs(dxSteps), qAbs(dySteps));
    if (total == 0) return;
    const int numChunks = (total + MAX_STEPS_CHUNK - 1) / MAX_STEPS_CHUNK;
    for (int i = 0; i < numChunks; ++i)
        plan.append(isCut);
}

// ----------------------------------------------------------
//  Slot : un segment physiquement exécuté par le STM (thread principal)
// ----------------------------------------------------------

void TrajetMotor::onSegmentExecuted(int /*seg*/, int x_steps, int y_steps)
{
    if (!m_head) return;
    if (m_execCount >= m_segIsCut.size()) return;

    const bool isCut = m_segIsCut[m_execCount++];

    // Conversion pas moteur → pixels canvas (homePos = origine STM)
    const QPointF newPos(HOME_X + x_steps / static_cast<double>(STEPS_PER_MM),
                         HOME_Y + y_steps / static_cast<double>(STEPS_PER_MM));

    // Tracé du sillage
    auto* line = new QGraphicsLineItem(
        m_lastHeadPos.x(), m_lastHeadPos.y(), newPos.x(), newPos.y());
    line->setPen(QPen(isCut ? Qt::red : Qt::blue, 1.5));
    m_visu->getScene()->addItem(line);
    m_visu->addCutMarker(line);

    // Déplacement de la tête
    m_head->setPos(newPos.x() - 3, newPos.y() - 3);
    m_lastHeadPos = newPos;

    // Mise à jour progression
    const int remaining = qMax(0, m_segIsCut.size() - m_execCount);
    emit decoupeProgress(remaining, qMax(1, m_segIsCut.size()));
}

// ----------------------------------------------------------
//  Slot : DONE reçu du STM (thread principal → débloque le worker)
// ----------------------------------------------------------

void TrajetMotor::onMachineDone()
{
    QMutexLocker lk(&m_doneMutex);
    m_doneReceived = true;
    m_doneCond.wakeAll();
}

// ----------------------------------------------------------
//  Envoi d'un déplacement au STM32
// ----------------------------------------------------------

int TrajetMotor::estimateTotalSteps(const QList<ContinuousCut>& cuts, const QPoint& homePos) {
    int steps = 0;
    QPoint p = homePos;
    for (const auto& cut : std::as_const(cuts)) {
        steps += std::max(std::abs(p.x() - cut.points.first().x()),
                          std::abs(p.y() - cut.points.first().y()));
        for (int i = 1; i < cut.points.size(); ++i) {
            steps += std::max(std::abs(cut.points[i-1].x() - cut.points[i].x()),
                              std::abs(cut.points[i-1].y() - cut.points[i].y()));
        }
        p = cut.points.last().toPoint();
    }
    return steps;
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

    const int totalAbs = qMax(qAbs(dxSteps), qAbs(dySteps));
    if (totalAbs == 0) return true;

    const int numChunks = (totalAbs + MAX_STEPS_CHUNK - 1) / MAX_STEPS_CHUNK;

    for (int i = 0; i < numChunks; ++i) {
        const double t0 = static_cast<double>(i)     / numChunks;
        const double t1 = static_cast<double>(i + 1) / numChunks;

        StmSegment seg;
        seg.dx    = static_cast<int16_t>(static_cast<int>(dxSteps * t1) - static_cast<int>(dxSteps * t0));
        seg.dy    = static_cast<int16_t>(static_cast<int>(dySteps * t1) - static_cast<int>(dySteps * t0));
        seg.dz    = 0;
        seg.v_max = arr;
        seg.flags = flags;

        if (isLast && i == numChunks - 1) seg.flags |= FLAG_END_SEQ;

        bool accepted = false;

        while (!m_stopRequested) {
            QMetaObject::invokeMethod(this, [this, seg, &accepted]() {
                if (m_machine)
                    accepted = m_machine->sendSegment(seg);
            }, Qt::BlockingQueuedConnection);

            if (accepted) break;
            QThread::msleep(10);
        }
    }
    return true;
}

// ----------------------------------------------------------
//  Contrôle pause / stop
// ----------------------------------------------------------

void TrajetMotor::pause()
{
    m_paused = true;
}

void TrajetMotor::resume()
{
    m_paused = false;
}

void TrajetMotor::stopCut()
{
    m_stopRequested = true;
    m_paused        = false;
    m_interrupted   = true;
    QMutexLocker lk(&m_doneMutex);
    m_doneCond.wakeAll(); // débloque l'attente DONE dans doExecuteTrajet
}
