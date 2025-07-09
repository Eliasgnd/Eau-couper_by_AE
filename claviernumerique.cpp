// ClavierNumerique.cpp
#include "claviernumerique.h"
#include <QVBoxLayout>

ClavierNumerique::ClavierNumerique(QWidget *parent)
    : QDialog(parent), hasDecimal(false)
{
    setWindowTitle("Pavé Numérique");
    setFixedSize(300, 490);
    setWindowState(Qt::WindowFullScreen);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    lineEdit = new QLineEdit(this);
    lineEdit->setReadOnly(true);
    lineEdit->setAlignment(Qt::AlignRight);
    lineEdit->setFixedHeight(50);
    mainLayout->addWidget(lineEdit);

    gridLayout = new QGridLayout;
    mainLayout->addLayout(gridLayout);

    // Création des boutons 1-9
    int number = 1;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            QPushButton *btn = new QPushButton(QString::number(number++), this);
            btn->setFixedSize(80, 80);
            connect(btn, &QPushButton::clicked, this, &ClavierNumerique::handleButton);
            digitButtons.append(btn);
            gridLayout->addWidget(btn, row, col);
        }
    }

    // Bouton 0
    QPushButton *btn0 = new QPushButton("0", this);
    btn0->setFixedSize(80, 80);
    connect(btn0, &QPushButton::clicked, this, &ClavierNumerique::handleButton);
    digitButtons.append(btn0);
    gridLayout->addWidget(btn0, 3, 1);

    // Bouton décimal
    decimalButton = new QPushButton(",", this);
    decimalButton->setFixedSize(80, 80);
    connect(decimalButton, &QPushButton::clicked, this, &ClavierNumerique::handleButton);
    gridLayout->addWidget(decimalButton, 3, 0);

    // Bouton effacer
    deleteButton = new QPushButton("⌫", this);
    deleteButton->setFixedSize(80, 80);
    connect(deleteButton, &QPushButton::clicked, this, &ClavierNumerique::deleteChar);
    gridLayout->addWidget(deleteButton, 3, 2);

    // Bouton valider
    validateButton = new QPushButton("✔", this);
    validateButton->setFixedSize(80, 80);
    connect(validateButton, &QPushButton::clicked, this, &ClavierNumerique::validateText);
    mainLayout->addWidget(validateButton, 0, Qt::AlignCenter);
}

QString ClavierNumerique::getText() const
{
    return lineEdit->text();
}

void ClavierNumerique::handleButton()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;

    QString text = button->text();
    if (text == ",") {
        // Un seul séparateur décimal autorisé
        if (!hasDecimal) {
            lineEdit->insert(text);
            hasDecimal = true;
        }
    } else {
        lineEdit->insert(text);
    }
}

void ClavierNumerique::deleteChar()
{
    QString txt = lineEdit->text();
    if (!txt.isEmpty()) {
        if (txt.endsWith(","))
            hasDecimal = false;
        txt.chop(1);
        lineEdit->setText(txt);
    }
}

void ClavierNumerique::validateText()
{
    accept();
}


bool ClavierNumerique::ouvrirClavierNum(QWidget *parent, int &out)
{
    ClavierNumerique dlg(parent);
    if (dlg.exec() == QDialog::Accepted) {
        bool ok = false;
        int v = dlg.getText().toInt(&ok);
        if (ok) {
            out = v;
            return true;
        }
    }
    return false;
}
