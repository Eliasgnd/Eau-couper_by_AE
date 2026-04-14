#include "TrajetMotor.h"
#include "MachineViewModel.h"
#include "pathplanner.h"
#include <QSet>
#include <QLineF>
#include <QThread>
#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QStringList>            // Requis pour le Buffer G-Code
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
    steps += stepLen(cur, homePos); // Retour au point zéro

    return steps;
}

TrajetMotor::TrajetMotor(ShapeVisualization* visu, QWidget* parent)
    : QWidget(parent), m_visu(visu)
{}

// -----------------------------------------------------------------------------
// Lancement de la découpe
// -----------------------------------------------------------------------------
void TrajetMotor::executeTrajet()
{
    m_interrupted = false;

    if (m_running) {
        qWarning() << "Découpe déjà en cours.";
        return;
    }
    if (!m_visu || !m_visu->getScene()) return;

    m_running       = true;
    m_paused        = false;
    m_stopRequested = false;
    m_visu->setDecoupeEnCours(true);

    // --- DÉFINITION DU POINT ORIGINE (HOME) ---
    QPoint homePos(600, 400);

    // --- CRÉATION DU BUFFER  ---
    QStringList commandBuffer;
    commandBuffer << "G90 ; Mode de positionnement Absolu (mm)";
    commandBuffer << "G21 ; Unites en millimètres";
    commandBuffer << "M5  ; Assure que le jet d'eau est coupé au depart";

    // 1) Extraction des segments
    const QList<Segment> segs = PathPlanner::extractSegments(m_visu->getScene());
    if (segs.isEmpty()) {
        m_running = false;
        m_visu->setDecoupeEnCours(false);
        return;
    }

    // 2) Progression
    m_totalSteps      = estimateTotalSteps(segs, homePos);
    m_progressCounter = 0;
    emit decoupeProgress(m_totalSteps, m_totalSteps);

    // 3) Curseur vert
    auto *head = new QGraphicsEllipseItem(-3, -3, 6, 6);
    head->setBrush(Qt::green);
    head->setPen(Qt::NoPen);
    head->setZValue(60);
    m_visu->getScene()->addItem(head);

    // 4) Boucle principale
    QSet<quint64> done;
    QPoint cur = homePos;
    head->setPos(cur.x() - 3, cur.y() - 3);

    while (done.size() < segs.size()) {

        if (m_stopRequested) break;
        while (m_paused && !m_stopRequested) {
            QApplication::processEvents();
            QThread::msleep(30);
        }
        if (m_stopRequested) break;

        int    bestIdx = -1;
        double bestDi  = std::numeric_limits<double>::max();
        bool   reverseCut = false;

        for (int i = 0; i < segs.size(); ++i) {
            if (done.contains(keySeg(segs[i].a, segs[i].b))) continue;

            const double d_a = QLineF(cur, segs[i].a).length();
            const double d_b = QLineF(cur, segs[i].b).length();

            if (d_a < bestDi) { bestDi = d_a; bestIdx = i; reverseCut = false; }
            if (d_b < bestDi) { bestDi = d_b; bestIdx = i; reverseCut = true; }
        }
        if (bestIdx < 0) break;
        const auto s = segs[bestIdx];

        QPoint realStart = reverseCut ? s.b : s.a;
        QPoint realEnd   = reverseCut ? s.a : s.b;

        // ---------- Déplacement rapide (bleu) ------------------
        if (cur != realStart) {
            m_motor.stopJet();
            commandBuffer << "M5 ; Arrêt du jet";
            commandBuffer << QString("G0 X%1 Y%2").arg(realStart.x() * mmPerPx).arg(realStart.y() * mmPerPx);

            moveHeadProgressive(cur, realStart, head, false);
            m_motor.moveRapid(realStart.x() * mmPerPx, realStart.y() * mmPerPx);
            sendMoveToStm(cur, realStart, FLAG_VALVE_OFF, /*isLast=*/false, mmPerPx);
        }
        cur = realStart;

        // ---------- Coupe (rouge) ---------------------------------------------
        m_motor.startJet();
        commandBuffer << "M3 ; Démarrage du jet";
        // F1000 représente la vitesse d'avance (Feedrate) en mm/min
        commandBuffer << QString("G1 X%1 Y%2 F1000").arg(realEnd.x() * mmPerPx).arg(realEnd.y() * mmPerPx);

        moveHeadProgressive(realStart, realEnd, head, true);
        m_motor.moveCut(realEnd.x() * mmPerPx, realEnd.y() * mmPerPx);
        // FLAG_END_SEQ uniquement si ce segment coupe se termine déjà à l'origine
        // (pas de retour home nécessaire) — sinon c'est le home return qui le porte
        sendMoveToStm(realStart, realEnd, FLAG_VALVE_ON, /*isLast=*/false, mmPerPx);

        done.insert(keySeg(s.a, s.b));
        cur = realEnd;
    }

    // --- RETOUR AU POINT DE DÉPART (HOME) ---
    if (!m_stopRequested && cur != homePos) {
        m_motor.stopJet();
        commandBuffer << "M5 ; Arrêt du jet";
        commandBuffer << QString("G0 X%1 Y%2 ; Retour Origine").arg(homePos.x() * mmPerPx).arg(homePos.y() * mmPerPx);

        moveHeadProgressive(cur, homePos, head, false);
        m_motor.moveRapid(homePos.x() * mmPerPx, homePos.y() * mmPerPx);
        // Dernier mouvement de la séquence → FLAG_END_SEQ + fermeture vanne
        sendMoveToStm(cur, homePos, FLAG_VALVE_OFF, true, mmPerPx);
    }

    commandBuffer << "M30 ; Fin du programme de découpe";

    // --- AFFICHAGE DU BUFFER DANS LE TERMINAL ---
    qDebug() << "\n============= BUFFER  (G-CODE) =============";
    for (const QString& cmd : commandBuffer) {
        qDebug().noquote() << cmd; // .noquote() enlève les guillemets superflus dans le terminal
    }
    qDebug() << "=================================================\n";

    // Nettoyage Final
    m_motor.stopJet();
    m_visu->resetCutMarkers();
    m_visu->getScene()->removeItem(head);
    delete head;
    emit decoupeProgress(0, m_totalSteps);

    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "setSpinboxSliderEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    }

    if (m_visu) m_visu->setDecoupeEnCours(false);
    m_running = false;

    QMetaObject::invokeMethod(m_mainWindow, [this]() {
        QString titre = m_interrupted ? "Découpe interrompue" : "Découpe terminée";
        QString message = m_interrupted ? "La découpe a été arrêtée manuellement." : "La découpe s'est terminée avec succès.";
        QMessageBox* msg = new QMessageBox(QMessageBox::Information, titre, message, QMessageBox::Ok, m_mainWindow);
        msg->setModal(false);
        msg->show();
    }, Qt::QueuedConnection);
}

// -----------------------------------------------------------------------------
// Slots Pause / Reprendre / Stop
// -----------------------------------------------------------------------------
void TrajetMotor::pause() { if (m_running) { m_paused = true;  m_motor.stopJet(); } }
void TrajetMotor::resume() { if (m_running) { m_paused = false; } }
void TrajetMotor::stopCut() {
    if (m_visu) m_visu->setDecoupeEnCours(false);
    if (!m_running) return;
    m_stopRequested = true; m_paused = false; m_interrupted = true; m_motor.stopJet();
}
void TrajetMotor::setMainWindow(MainWindow* mainWindow) { m_mainWindow = mainWindow; }
void TrajetMotor::setMachineViewModel(MachineViewModel* vm) { m_machine = vm; }

// -----------------------------------------------------------------------------
// sendMoveToStm — envoie un déplacement en segments relatifs vers le STM.
// Découpe automatiquement si le déplacement dépasse la capacité int16_t (511 mm).
// -----------------------------------------------------------------------------
void TrajetMotor::sendMoveToStm(const QPoint& from, const QPoint& to,
                                 uint8_t flags, bool isLast, double mmPerPxScale)
{
    if (!m_machine) return;

    // Déplacement total en mm puis en pas
    const double dxMm = (to.x() - from.x()) * mmPerPxScale;
    const double dyMm = (to.y() - from.y()) * mmPerPxScale;
    const int    dxSteps = static_cast<int>(dxMm * STEPS_PER_MM);
    const int    dySteps = static_cast<int>(dyMm * STEPS_PER_MM);

    // Valeur ARR basée sur les vitesses de MotorControl
    const double vitesse = (flags & FLAG_VALVE_ON) ? m_motor.Vcut : m_motor.Vtrav;
    const uint16_t arr   = speedToArr(vitesse);

    // Découpage en sous-segments si le déplacement dépasse int16_t (±32767 pas ≈ 511 mm)
    static constexpr int MAX_STEPS = 30000; // marge de sécurité
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

        // FLAG_END_SEQ uniquement sur le dernier sous-segment du dernier mouvement
        if (isLast && i == numChunks - 1)
            seg.flags |= FLAG_END_SEQ;

        m_machine->sendSegment(seg);
    }
}

// -----------------------------------------------------------------------------
// moveHeadProgressive : Tracé par petits segments fixes (Effet "Trace" réel)
// -----------------------------------------------------------------------------
void TrajetMotor::moveHeadProgressive(const QPoint& start, const QPoint& end, QGraphicsEllipseItem* head, bool cut)
{
    double dist = QLineF(start, end).length();
    if (dist <= 0) return;

    QPen pen(cut ? Qt::red : Qt::blue, 1.5);
    double stepPx = cut ? 3.0 : 12.0;
    int delayMs = cut ? 2 : 4;

    QPointF currentPos = start;

    for (double d = stepPx; d < dist; d += stepPx) {
        double t = d / dist;
        QPointF nextPos(start.x() + t * (end.x() - start.x()), start.y() + t * (end.y() - start.y()));

        QGraphicsLineItem* stepLine = new QGraphicsLineItem(currentPos.x(), currentPos.y(), nextPos.x(), nextPos.y());
        stepLine->setPen(pen);
        m_visu->getScene()->addItem(stepLine);
        m_visu->addCutMarker(stepLine);

        head->setPos(nextPos.x() - 3, nextPos.y() - 3);
        currentPos = nextPos;

        QApplication::processEvents();
        QThread::msleep(delayMs);

        m_progressCounter += stepPx;
        if (m_progressCounter > m_totalSteps) m_progressCounter = m_totalSteps;
        emit decoupeProgress(m_totalSteps - m_progressCounter, m_totalSteps);
    }

    if (currentPos != end) {
        QGraphicsLineItem* finalLine = new QGraphicsLineItem(currentPos.x(), currentPos.y(), end.x(), end.y());
        finalLine->setPen(pen);
        m_visu->getScene()->addItem(finalLine);
        m_visu->addCutMarker(finalLine);

        head->setPos(end.x() - 3, end.y() - 3);
        QApplication::processEvents();
    }
}
