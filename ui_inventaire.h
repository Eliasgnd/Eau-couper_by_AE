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
#include <QtGui/QIcon>
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
        Inventaire->resize(1301, 677);
        Inventaire->setBaseSize(QSize(1280, 720));
        Inventaire->setStyleSheet(QString::fromUtf8("QWidget {\n"
"	background-color: rgb(85, 170, 255);\n"
"}\n"
"\n"
""));
        buttonMenu = new QPushButton(Inventaire);
        buttonMenu->setObjectName("buttonMenu");
        buttonMenu->setGeometry(QRect(1220, 0, 61, 51));
        buttonMenu->setAutoFillBackground(false);
        buttonMenu->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #F44336;  /* Rouge primaire */\n"
"    color: white; \n"
"    font-size: 14px;\n"
"    border: 2px solid #D32F2F;  /* Bordure l\303\251g\303\250rement plus fonc\303\251e */\n"
"    border-radius: 5px;\n"
"    padding: 6px 12px;\n"
"    text-align: left;           /* texte \303\240 gauche */\n"
"    padding-left: 8px;          /* espace avant le texte */\n"
"    padding-right: 8px;         /* espace apr\303\250s l\342\200\231ic\303\264ne */\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background-color: #E57373;  /* Rouge plus clair au survol */\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background-color: #C62828;  /* Rouge plus fonc\303\251 lorsqu'on clique */\n"
"}\n"
""));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/cross.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        buttonMenu->setIcon(icon);
        buttonMenu->setIconSize(QSize(40, 40));
        scrollAreaInventaire = new QScrollArea(Inventaire);
        scrollAreaInventaire->setObjectName("scrollAreaInventaire");
        scrollAreaInventaire->setGeometry(QRect(10, 70, 1221, 591));
        scrollAreaInventaire->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 1219, 589));
        containerWidget = new QWidget(scrollAreaWidgetContents);
        containerWidget->setObjectName("containerWidget");
        containerWidget->setGeometry(QRect(0, 10, 1221, 581));
        gridLayoutWidget = new QWidget(containerWidget);
        gridLayoutWidget->setObjectName("gridLayoutWidget");
        gridLayoutWidget->setGeometry(QRect(-10, -10, 1241, 591));
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
        buttonMenu->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class Inventaire: public Ui_Inventaire {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INVENTAIRE_H
