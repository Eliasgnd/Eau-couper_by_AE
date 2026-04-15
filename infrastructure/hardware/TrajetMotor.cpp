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
    m_animQueue.clear();
    m_visu->setDecoupeEnCours(true);

    // Timer single-shot qui pilote l'animation frame par frame
    if (!m_animTimer) {
        m_animTimer = new QTimer(this);
        m_animTimer->setSingleShot(true);
        connect(m_animTimer, &QTimer::timeout,
                this, &TrajetMotor::onAnimStep);
    }

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
        m_animCurrentPos = homePos;   // position initiale de l'animation
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
        if (m_animTimer) m_animTimer->stop();
        m_animQueue.clear();

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
//  Slot : correction de position depuis le STM réel (SEG_DONE)
//
//  Le STM envoie SEG_DONE toutes les 100 exécutions.
//  L'animation est pilotée localement par onAnimStep() — ce slot
//  sert uniquement à corriger la dérive de position éventuelle.
// ----------------------------------------------------------

void TrajetMotor::onSegmentExecuted(int /*seg*/, int x_steps, int y_steps)
{
    if (!m_head) return;
    const QPointF realPos(HOME_X + x_steps / static_cast<double>(STEPS_PER_MM),
                          HOME_Y + y_steps / static_cast<double>(STEPS_PER_MM));
    // Correction de la position de référence
    m_animCurrentPos = realPos;
    m_head->setPos(realPos.x() - 3, realPos.y() - 3);
}

// ----------------------------------------------------------
//  Slot : avance l'animation d'un frame (pilote Pi-side)
//
//  Appelé par m_animTimer (single-shot) — s'enchaîne seul
//  frame par frame, avec un délai proportionnel au temps
//  d'exécution simulé de chaque chunk STM.
// ----------------------------------------------------------

void TrajetMotor::onAnimStep()
{
    if (m_animQueue.isEmpty() || !m_head) return;

    const SegAnimFrame frame = m_animQueue.dequeue();

    // Type de tracé (utilise le plan pré-construit si disponible)
    const bool isCut = (m_execCount < m_segIsCut.size())
                       ? m_segIsCut[m_execCount] : frame.isCut;

    // Tracé du sillage depuis la position courante vers la nouvelle
    auto* line = new QGraphicsLineItem(
        m_animCurrentPos.x(), m_animCurrentPos.y(),
        frame.canvasPos.x(),  frame.canvasPos.y());
    line->setPen(QPen(isCut ? Qt::red : Qt::blue, 1.5));
    m_visu->getScene()->addItem(line);
    m_visu->addCutMarker(line);

    // Déplacement de la tête
    m_head->setPos(frame.canvasPos.x() - 3, frame.canvasPos.y() - 3);
    m_animCurrentPos = frame.canvasPos;
    ++m_execCount;

    // Mise à jour progression
    const int remaining = qMax(0, m_segIsCut.size() - m_execCount);
    emit decoupeProgress(remaining, qMax(1, m_segIsCut.size()));

    // Planifier le prochain frame si la file n'est pas vide
    if (!m_animQueue.isEmpty())
        m_animTimer->start(m_animQueue.head().durationMs);
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

// -----------------------------------------------------------------------------
// sendMoveToStm — envoie un déplacement en segments relatifs vers le STM.
// Cette fonction DOIT être appelée directement depuis le thread worker (doExecuteTrajet).
// Elle gère elle-même la mise en pause si le STM32 attend un ACK.
// -----------------------------------------------------------------------------
bool TrajetMotor::sendMoveToStm(const QPoint& from, const QPoint& to,
                                uint8_t flags, bool isLast, double mmPerPxScale)
{
    if (!m_machine) return true; // Pas de machine = on ne bloque pas la simulation

    const double dxMm    = (to.x() - from.x()) * mmPerPxScale;
    const double dyMm    = (to.y() - from.y()) * mmPerPxScale;
    const int    dxSteps = static_cast<int>(dxMm * STEPS_PER_MM);
    const int    dySteps = static_cast<int>(dyMm * STEPS_PER_MM);

    const double vitesse = (flags & FLAG_VALVE_ON) ? m_vCut : m_vTrav;
    const uint16_t arr     = speedToArr(vitesse);

    qDebug() << "[sendMoveToStm]" << (flags & FLAG_VALVE_ON ? "COUPE" : "DÉPLACEMENT")
             << "— vitesse =" << vitesse << "mm/s, ARR =" << arr;

    static constexpr int MAX_STEPS = 30000;
    const int totalAbs = qMax(qAbs(dxSteps), qAbs(dySteps));

    if (totalAbs == 0) return true;

    const int numChunks = (totalAbs + MAX_STEPS - 1) / MAX_STEPS;

    // On divise le grand mouvement en "chunks" (morceaux)
    for (int i = 0; i < numChunks; ++i) {
        const double t0 = static_cast<double>(i)     / numChunks;
        const double t1 = static_cast<double>(i + 1) / numChunks;

        // ==========================================================
        // 🎨 MISE À JOUR VISUELLE EN TEMPS RÉEL
        // ==========================================================
        // On calcule les positions de dessin sur le canevas (en pixels)
        QPointF p0(from.x() + (to.x() - from.x()) * t0,
                   from.y() + (to.y() - from.y()) * t0);
        QPointF p1(from.x() + (to.x() - from.x()) * t1,
                   from.y() + (to.y() - from.y()) * t1);

        bool isCut = (flags & FLAG_VALVE_ON);

        // On dessine sur le thread principal pour éviter que l'UI plante
        QMetaObject::invokeMethod(this, [this, p0, p1, isCut]() {
            if (m_visu && m_head) {
                // Dessine le trait (Rouge = coupe, Bleu = déplacement rapide)
                auto* line = new QGraphicsLineItem(p0.x(), p0.y(), p1.x(), p1.y());
                line->setPen(QPen(isCut ? Qt::red : Qt::blue, 1.5));
                m_visu->getScene()->addItem(line);
                m_visu->addCutMarker(line); // Ajoute à la liste pour pouvoir nettoyer l'écran à la fin

                // Déplace la tête (le point vert)
                m_head->setPos(p1.x() - 3, p1.y() - 3);
            }
        }, Qt::BlockingQueuedConnection);
        // ==========================================================

        const int chunkDx = static_cast<int>(dxSteps * t1) - static_cast<int>(dxSteps * t0);
        const int chunkDy = static_cast<int>(dySteps * t1) - static_cast<int>(dySteps * t0);

        StmSegment seg;
        seg.dx    = static_cast<int16_t>(chunkDx);
        seg.dy    = static_cast<int16_t>(chunkDy);
        seg.dz    = 0;
        seg.v_max = arr;
        seg.flags = flags;

        // On ajoute le flag de fin uniquement sur le tout dernier chunk du dernier segment
        if (isLast && i == numChunks - 1)
            seg.flags |= FLAG_END_SEQ;

        bool accepted = false;
        int waited = 0;
        constexpr int MAX_WAIT_MS = 5000; // Timeout de sécurité (5 secondes)

        // =================================================================
        // BOUCLE D'ATTENTE VITALE (La Contre-pression)
        // Si l'UART attend un ACK, MachineViewModel refusera la trajectoire.
        // On endort le thread 10ms et on réessaie CE chunk, sans le perdre.
        // =================================================================
        while (!m_stopRequested) {

            // On s'assure d'exécuter l'envoi sur le thread principal pour la synchronisation
            QMetaObject::invokeMethod(this, [this, seg, &accepted]() {
                if (m_machine) {
                    accepted = m_machine->sendSegment(seg);
                }
            }, Qt::BlockingQueuedConnection);

            // Si le chunk a été accepté, on casse la boucle while et on passe au chunk i+1
            if (accepted) {
                break;
            }

            // Sinon, l'UART est bloqué (attente d'ACK). On patiente sans bloquer l'interface graphique.
            QThread::msleep(10);
            waited += 10;

            if (waited >= MAX_WAIT_MS) {
                qWarning() << "TrajetMotor: timeout (5s) attente ACK du STM32, machine bloquée.";
                return false;
            }
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
    if (m_animTimer) m_animTimer->stop();
    m_animQueue.clear();

    m_stopRequested = true;
    m_paused        = false;
    m_interrupted   = true;
    QMutexLocker lk(&m_doneMutex);
    m_doneCond.wakeAll(); // débloque l'attente DONE dans doExecuteTrajet
}
