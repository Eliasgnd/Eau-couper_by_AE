#include "TrajetMotor.h"
#include "MachineViewModel.h"
#include "PathOptimizer.h"
#include "MainWindow.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QMessageBox>
#include <QDebug>
#include <algorithm>

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

    // 1. Extraction des formes depuis la scène
    QList<ContinuousCut> rawCuts;
    QMetaObject::invokeMethod(this, [this, &rawCuts]() {
        rawCuts = PathPlanner::extractContinuousPaths(m_visu->getScene());
    }, Qt::BlockingQueuedConnection);

    if (rawCuts.isEmpty()) {
        QMetaObject::invokeMethod(this, [this]() {
            m_visu->setDecoupeEnCours(false);
            m_running = false;
        }, Qt::QueuedConnection);
        return;
    }

    // 2. OPTIMISATION (Intelligence de parcours : Inside-Out, Lead-in, Trajet court)
    QList<ContinuousCut> optimizedCuts = PathOptimizer::optimize(rawCuts, homePos);

    // 3. Préparation progression
    m_totalSteps = estimateTotalSteps(optimizedCuts, homePos);
    m_progressCounter = 0;
    emit decoupeProgress(m_totalSteps, m_totalSteps);

    // 4. Création du curseur visuel (tête de buse)
    QGraphicsEllipseItem *head = nullptr;
    QMetaObject::invokeMethod(this, [this, &head, homePos]() {
        head = new QGraphicsEllipseItem(-3, -3, 6, 6);
        head->setBrush(Qt::green);
        head->setPen(Qt::NoPen);
        head->setZValue(60);
        m_visu->getScene()->addItem(head);
        head->setPos(homePos.x() - 3, homePos.y() - 3);
    }, Qt::BlockingQueuedConnection);

    // 5. BOUCLE DE DÉCOUPE
    QPoint cur = homePos;

    for (const auto& cut : optimizedCuts) {
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
        // On parcourt tous les points de la forme continue
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

        // Affichage message fin
        QString titre = m_interrupted ? tr("Découpe interrompue") : tr("Découpe terminée");
        QString msg = m_interrupted ? tr("L'opération a été stoppée.") : tr("Découpe réussie !");
        QMessageBox::information(m_mainWindow, titre, msg);
    }, Qt::QueuedConnection);
}

int TrajetMotor::estimateTotalSteps(const QList<ContinuousCut>& cuts, const QPoint& homePos) {
    int steps = 0;
    QPoint p = homePos;
    for (const auto& cut : cuts) {
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

    for (double d = stepPx; d < dist; d += stepPx) {
        if (m_stopRequested) return;

        double t = d / dist;
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
}

bool TrajetMotor::sendMoveToStm(const QPoint& from, const QPoint& to,
                                uint8_t flags, bool isLast, double mmPerPxScale)
{
    if (!m_machine) return true;

    const double dxMm = (to.x() - from.x()) * mmPerPxScale;
    const double dyMm = (to.y() - from.y()) * mmPerPxScale;
    const int dxSteps = static_cast<int>(dxMm * STEPS_PER_MM);
    const int dySteps = static_cast<int>(dyMm * STEPS_PER_MM);

    // Choix de la vitesse selon le flag (Coupe ou Rapide)
    const double vitesse = (flags & FLAG_VALVE_ON) ? m_vCut : m_vTrav;
    const uint16_t arr = speedToArr(vitesse); // Assurez-vous que cette fonction est accessible

    // Segmentation pour ne pas dépasser les limites du protocole (int16)
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

        // On attend que le buffer du STM32 ait de la place (géré dans le ViewModel)
        while (!m_machine->sendSegment(seg) && !m_stopRequested) {
            QThread::msleep(10);
        }
    }
    return true;
}

void TrajetMotor::pause() { m_paused = true; if (m_machine) m_machine->sendValveOff(); }
void TrajetMotor::resume() { m_paused = false; }
void TrajetMotor::stopCut() { m_stopRequested = true; m_paused = false; m_interrupted = true; if (m_machine) m_machine->sendValveOff(); }
