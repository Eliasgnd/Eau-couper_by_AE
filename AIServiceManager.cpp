#include "AIServiceManager.h"

#include "AIImagePromptDialog.h"
#include "AIImageProcessDialog.h"

#include <QImage>

AIServiceManager::AIServiceManager(QObject *parent)
    : QObject(parent)
{
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

bool AIServiceManager::resolveImageProcessingOptions(QWidget *parent,
                                                     const QString &imagePath,
                                                     AiImageProcessingOptions &options,
                                                     QString &errorMessage) const
{
    QImage image(imagePath);
    if (image.isNull()) {
        errorMessage = tr("L'image générée est invalide.");
        return false;
    }

    AIImageProcessDialog dialog(image, parent);
    if (dialog.exec() != QDialog::Accepted)
        return false;

    options.internalContours = dialog.selectedMethod() == AIImageProcessDialog::LogoWithInternal;
    options.colorEdges = dialog.selectedMethod() == AIImageProcessDialog::ColorEdges;
    return true;
}
