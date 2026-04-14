#pragma once

#include <QDialog>
#include "MachineViewModel.h"

namespace Ui { class StmTestDialog; }

// ============================================================
//  StmTestDialog — Interface de test moteurs STM32
//
//  Permet de :
//   - Connecter/déconnecter le port série
//   - Visualiser l'état machine et la position en temps réel
//   - Envoyer les commandes rapides (HOME, RÉARMER, VANNE, etc.)
//   - Envoyer des segments de mouvement manuels
//   - Gérer la recovery après arrêt d'urgence
//   - Consulter le journal UART complet
//
//  Parle UNIQUEMENT à MachineViewModel (jamais à StmUartService).
// ============================================================
class StmTestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StmTestDialog(MachineViewModel *vm, QWidget *parent = nullptr);
    ~StmTestDialog() override;

private slots:
    // --- Boutons connexion ---
    void onConnectClicked();
    void onRefreshPortsClicked();

    // --- Boutons commandes ---
    void onHomeClicked();
    void onRearmClicked();
    void onResetPosClicked();
    void onValveOnClicked();
    void onValveOffClicked();
    void onSendSegmentClicked();

    // --- Boutons recovery ---
    void onRecoverClicked();
    void onGoClicked();
    void onClearRecoveryClicked();

    // --- Log ---
    void onClearLogClicked();
    void onCopyLogClicked();

    // --- Réactions MachineViewModel ---
    void onStateChanged(MachineState state);
    void onConnectionChanged(bool connected);
    void onBufferLevelChanged(int level);
    void onPositionChanged(double x, double y, double z);
    void onStatusMessageChanged(const QString &msg);
    void onHomingProgress(const QString &msg);
    void onErrorOccurred(const QString &code);
    void onRecoveryAvailable(RecoveryData data);
    void onDoneReceived();

private:
    void setupConnections();
    void refreshPortList();
    void updateButtonStates(MachineState state, bool connected);
    void appendLog(const QString &text, const QString &color = QString());
    QString stateToString(MachineState state) const;
    QString stateToColor(MachineState state) const;

    Ui::StmTestDialog  *ui;
    MachineViewModel   *m_vm;
    bool                m_connected = false;
};
