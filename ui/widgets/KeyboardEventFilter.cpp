#include "KeyboardEventFilter.h"
#include "ClavierNumerique.h"
#include "Clavier.h"
#include "MainWindow.h"
#include "FormeVisualization.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QMouseEvent>
#include <QMessageBox>
#include <QGraphicsView>

KeyboardEventFilter::KeyboardEventFilter(QObject *parent)
    : QObject(parent),
    m_keyboardActive(false)
{}

bool KeyboardEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    // On ne s'intéresse qu'aux clics gauche
    if (event->type() == QEvent::MouseButtonPress && !m_keyboardActive) {
        auto *me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton)
            return QObject::eventFilter(obj, event);

        // Blocage du déplacement manuel des formes pendant la découpe
        if (m_visu && obj == m_visu->getGraphicsView()->viewport()) {
            if (m_visu->isDecoupeEnCours()) {
                QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                                   "Découpe en cours",
                                                   "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                                   QMessageBox::Ok,
                                                   m_visu);
                msg->setModal(false);
                msg->show();
                return true; // empêche tout déplacement
            }
        }

        // Filtrer uniquement les QLineEdit (y compris ceux internes aux spinbox)
        if (auto *le = qobject_cast<QLineEdit*>(obj)) {
            // 1) si parent immédiat est un QSpinBox de MainWindow → pavé numérique
            if (auto *spin = qobject_cast<QSpinBox*>(le->parentWidget())) {
                if (qobject_cast<MainWindow*>(spin->window())) {
                    if (m_visu && m_visu->isDecoupeEnCours()) {
                        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                                           "Découpe en cours",
                                                           "Impossible d’ouvrir le clavier pendant la découpe.",
                                                           QMessageBox::Ok,
                                                           spin->window());
                        msg->setModal(false);
                        msg->show();
                        return true; // bloque uniquement l'ouverture du clavier numérique
                    }

                    m_keyboardActive = true;
                    ClavierNumerique numDlg(spin->window());
                    if (numDlg.exec() == QDialog::Accepted) {
                        bool ok;
                        int nv = numDlg.getText().toInt(&ok);
                        if (ok) spin->setValue(nv);
                    }
                    m_keyboardActive = false;
                    return true;
                }
            }
            // 2) sinon → clavier alphabétique
            else {
                m_keyboardActive = true;
                Clavier txtDlg(le->window());

                // 🔁 Mise à jour de la barre de recherche si on édite celle-ci
                connect(&txtDlg, &Clavier::textChangedExternally, le, [le](const QString &text){
                    if (le->text() != text) {
                        le->setText(text);  // 🟢 met à jour si différent
                    }
                    QMetaObject::invokeMethod(le, "textChanged", Qt::QueuedConnection, Q_ARG(QString, text));
                });


                if (txtDlg.exec() == QDialog::Accepted) {
                    le->setText(txtDlg.getText());
                }

                m_keyboardActive = false;
                return true;
            }

        }
    }

    // tout le reste on laisse passer
    return QObject::eventFilter(obj, event);
}

void KeyboardEventFilter::setFormeVisualization(FormeVisualization* visu)
{
    m_visu = visu;
}
