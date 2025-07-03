/********************************************************************************
** Form generated from reading UI file 'inventaire.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_INVENTAIRE_H
#define UI_INVENTAIRE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Inventaire
{
public:
    QPushButton *buttonMenu;
    QScrollArea *scrollAreaInventaire;
    QWidget *scrollAreaWidgetContents;
    QWidget *containerWidget;
    QWidget *gridLayoutWidget;
    QGridLayout *gridLayout;

    void setupUi(QWidget *Inventaire)
    {
        if (Inventaire->objectName().isEmpty())
            Inventaire->setObjectName("Inventaire");
        Inventaire->resize(1013, 631);
        Inventaire->setStyleSheet(QString::fromUtf8("QWidget {\n"
"	background-color: rgb(85, 170, 255);\n"
"}\n"
"\n"
""));
        buttonMenu = new QPushButton(Inventaire);
        buttonMenu->setObjectName("buttonMenu");
        buttonMenu->setGeometry(QRect(0, 0, 191, 61));
        buttonMenu->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #00BCD4;  /* Cyan primaire */\n"
"    color: white; \n"
"    font-size: 14px;\n"
"    border: 2px solid #008C9E;  /* Bordure l\303\251g\303\250rement plus fonc\303\251e */\n"
"    border-radius: 5px;\n"
"    padding: 6px 12px;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background-color: #26C6DA;  /* Bleu plus clair au survol */\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background-color: #008C9E;  /* Bleu plus fonc\303\251 lorsqu'on clique */\n"
"}\n"
""));
        scrollAreaInventaire = new QScrollArea(Inventaire);
        scrollAreaInventaire->setObjectName("scrollAreaInventaire");
        scrollAreaInventaire->setGeometry(QRect(10, 70, 981, 531));
        scrollAreaInventaire->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 979, 529));
        containerWidget = new QWidget(scrollAreaWidgetContents);
        containerWidget->setObjectName("containerWidget");
        containerWidget->setGeometry(QRect(10, 10, 951, 501));
        gridLayoutWidget = new QWidget(containerWidget);
        gridLayoutWidget->setObjectName("gridLayoutWidget");
        gridLayoutWidget->setGeometry(QRect(-10, -10, 981, 531));
        gridLayout = new QGridLayout(gridLayoutWidget);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(0, 0, 0, 0);
        scrollAreaInventaire->setWidget(scrollAreaWidgetContents);

        retranslateUi(Inventaire);

        QMetaObject::connectSlotsByName(Inventaire);
    } // setupUi

    void retranslateUi(QWidget *Inventaire)
    {
        Inventaire->setWindowTitle(QCoreApplication::translate("Inventaire", "Form", nullptr));
        buttonMenu->setText(QCoreApplication::translate("Inventaire", "buttonMenu", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Inventaire: public Ui_Inventaire {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INVENTAIRE_H
