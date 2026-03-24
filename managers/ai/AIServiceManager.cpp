#include "AIServiceManager.h"

#include "AIImagePromptDialog.h"
#include "AIImageProcessDialog.h"

#include <QImage>
#include <QMessageBox>
#include <QTimer>

AIServiceManager::AIServiceManager(QObject *parent)
    : QObject(parent)
{
}

void AIServiceManager::setDialogParent(QWidget *parent)
{
    m_dialogParent = parent;
}

bool AIServiceManager::openGenerationPrompt(QWidget *parent, AiGenerationRequest &request) const
{
    AIImagePromptDialog dialog(parent);
    if (dialog.exec() != QDialog::Accepted)
        return false;

    request.prompt = dialog.getPrompt();
    request.model = dialog.getModel();
    request.quality = dialog.getQuality();
    request.size = dialog.getSize();
    request.colorPrompt = dialog.isColor();
    return true;
}

void AIServiceManager::onGenerationStatus(const QString &msg)
{
    emit generationStatusChanged(msg);
}

void AIServiceManager::onAiImageReady(const QString &imagePath)
{
    QImage image(imagePath);
    if (image.isNull()) {
        if (m_dialogParent)
            QMessageBox::critical(m_dialogParent, tr("Erreur image"), tr("L'image générée est invalide."));
        return;
    }

    AIImageProcessDialog dialog(image, m_dialogParent);
    if (dialog.exec() != QDialog::Accepted) {
        if (m_dialogParent) {
            QTimer::singleShot(3000, m_dialogParent, [this]() {
                emit generationStatusChanged(QString());
            });
        }
        return;
    }

    const bool internalContours = dialog.selectedMethod() == AIImageProcessDialog::LogoWithInternal;
    const bool colorEdges = dialog.selectedMethod() == AIImageProcessDialog::ColorEdges;
    emit generationStatusChanged(QString());
    emit imageReadyForImport(imagePath, internalContours, colorEdges);
}
