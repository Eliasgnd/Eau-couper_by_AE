#include "keyboardeventfilter.h"
#include "claviernumerique.h"
#include "clavier.h"
#include "MainWindow.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QMouseEvent>

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

        // Filtrer uniquement les QLineEdit (y compris ceux internes aux spinbox)
        if (auto *le = qobject_cast<QLineEdit*>(obj)) {
            // 1) si parent immédiat est un QSpinBox de MainWindow → pavé numérique
            if (auto *spin = qobject_cast<QSpinBox*>(le->parentWidget())) {
                if (qobject_cast<MainWindow*>(spin->window())) {
                    m_keyboardActive = true;
                    //int v = spin->value();
                    ClavierNumerique numDlg(spin->window());
                    if (numDlg.exec() == QDialog::Accepted) {
                        bool ok;
                        int nv = numDlg.getText().toInt(&ok);
                        if (ok) spin->setValue(nv);
                    }
                    m_keyboardActive = false;
                    return true; // consommation : pas de clavier système ni récursion
                }
            }
            // 2) sinon → clavier alphabétique
            else {
                m_keyboardActive = true;
                Clavier txtDlg(le->window());
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

