/********************************************************************************
** Form generated from reading UI file 'custom.ui'
**
** Created by: Qt User Interface Compiler version 5.15.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CUSTOM_H
#define UI_CUSTOM_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_custom
{
public:
    QWidget *drawingWidget;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QPushButton *buttonMenu;
    QPushButton *Reset;
    QPushButton *Appliquer;
    QPushButton *buttonLissage;
    QToolButton *buttonForme;
    QPushButton *buttonRetour;
    QToolButton *buttonImporter;
    QToolButton *buttonSave;
    QWidget *verticalLayoutWidget_2;
    QVBoxLayout *verticalLayout_2;
    QPushButton *buttonGomme;
    QSlider *tailleGomme;
    QPushButton *buttonSupprimer;
    QPushButton *buttonDeplacer;
    QPushButton *buttonConnect;
    QPushButton *buttonSnapGrid;
    QSlider *sliderGridSpacing;
    QLabel *labelGridSpacing;
    QPushButton *buttonCloseShape;

    void setupUi(QWidget *custom)
    {
        if (custom->objectName().isEmpty())
            custom->setObjectName(QString::fromUtf8("custom"));
        custom->resize(1497, 842);
        custom->setStyleSheet(QString::fromUtf8("QWidget {\n"
"	background-color: rgb(189, 189, 189);\n"
"}\n"
""));
        drawingWidget = new QWidget(custom);
        drawingWidget->setObjectName(QString::fromUtf8("drawingWidget"));
        drawingWidget->setGeometry(QRect(140, 0, 1000, 671));
        drawingWidget->setMinimumSize(QSize(1000, 500));
        drawingWidget->setMaximumSize(QSize(1500, 800));
        drawingWidget->setStyleSheet(QString::fromUtf8("QWidget {\n"
"	background-color: rgb(255, 255, 255);\n"
"}\n"
""));
        verticalLayoutWidget = new QWidget(custom);
        verticalLayoutWidget->setObjectName(QString::fromUtf8("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(0, 0, 142, 511));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        buttonMenu = new QPushButton(verticalLayoutWidget);
        buttonMenu->setObjectName(QString::fromUtf8("buttonMenu"));
        buttonMenu->setMinimumSize(QSize(140, 0));
        buttonMenu->setMaximumSize(QSize(140, 16777215));
        buttonMenu->setCursor(QCursor(Qt::PointingHandCursor));
        buttonMenu->setLayoutDirection(Qt::RightToLeft);
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
        buttonMenu->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/house.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonMenu->setIcon(icon);

        verticalLayout->addWidget(buttonMenu);

        Reset = new QPushButton(verticalLayoutWidget);
        Reset->setObjectName(QString::fromUtf8("Reset"));
        Reset->setMinimumSize(QSize(140, 0));
        Reset->setMaximumSize(QSize(140, 16777215));
        Reset->setCursor(QCursor(Qt::PointingHandCursor));
        Reset->setLayoutDirection(Qt::RightToLeft);
        Reset->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        Reset->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/reload.svg"), QSize(), QIcon::Normal, QIcon::Off);
        Reset->setIcon(icon1);

        verticalLayout->addWidget(Reset);

        Appliquer = new QPushButton(verticalLayoutWidget);
        Appliquer->setObjectName(QString::fromUtf8("Appliquer"));
        Appliquer->setMinimumSize(QSize(140, 0));
        Appliquer->setMaximumSize(QSize(140, 16777215));
        Appliquer->setCursor(QCursor(Qt::PointingHandCursor));
        Appliquer->setLayoutDirection(Qt::RightToLeft);
        Appliquer->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        Appliquer->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/check.svg"), QSize(), QIcon::Normal, QIcon::Off);
        Appliquer->setIcon(icon2);

        verticalLayout->addWidget(Appliquer);

        buttonLissage = new QPushButton(verticalLayoutWidget);
        buttonLissage->setObjectName(QString::fromUtf8("buttonLissage"));
        buttonLissage->setMinimumSize(QSize(140, 65));
        buttonLissage->setMaximumSize(QSize(140, 16777215));
        buttonLissage->setBaseSize(QSize(0, 0));
        buttonLissage->setCursor(QCursor(Qt::PointingHandCursor));
        buttonLissage->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #00BCD4;  /* Cyan primaire */\n"
"    color: white; \n"
"    font-size: 14px;\n"
"    border: 2px solid #008C9E;  /* Bordure l\303\251g\303\250rement plus fonc\303\251e */\n"
"    border-radius: 5px;\n"
"    padding: 0px 4px;\n"
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
        buttonLissage->setLocale(QLocale(QLocale::French, QLocale::France));

        verticalLayout->addWidget(buttonLissage);

        buttonForme = new QToolButton(verticalLayoutWidget);
        buttonForme->setObjectName(QString::fromUtf8("buttonForme"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(buttonForme->sizePolicy().hasHeightForWidth());
        buttonForme->setSizePolicy(sizePolicy);
        buttonForme->setMinimumSize(QSize(140, 0));
        buttonForme->setMaximumSize(QSize(140, 16777215));
        buttonForme->setCursor(QCursor(Qt::PointingHandCursor));
        buttonForme->setFocusPolicy(Qt::NoFocus);
        buttonForme->setLayoutDirection(Qt::RightToLeft);
        buttonForme->setStyleSheet(QString::fromUtf8("QToolButton {\n"
"    background-color: #00BCD4;  /* Cyan primaire */\n"
"    color: white; \n"
"    font-size: 14px;\n"
"    border: 2px solid #008C9E;  /* Bordure l\303\251g\303\250rement plus fonc\303\251e */\n"
"    border-radius: 5px;\n"
"    padding: 6px 12px;\n"
"}\n"
"\n"
"QToolButton:hover {\n"
"    background-color: #26C6DA;  /* Bleu plus clair au survol */\n"
"}\n"
"\n"
"QToolButton:pressed {\n"
"    background-color: #008C9E;  /* Bleu plus fonc\303\251 lorsqu'on clique */\n"
"}\n"
""));
        buttonForme->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/pencil.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonForme->setIcon(icon3);
        buttonForme->setPopupMode(QToolButton::DelayedPopup);
        buttonForme->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        buttonForme->setArrowType(Qt::NoArrow);

        verticalLayout->addWidget(buttonForme);

        buttonRetour = new QPushButton(verticalLayoutWidget);
        buttonRetour->setObjectName(QString::fromUtf8("buttonRetour"));
        buttonRetour->setMinimumSize(QSize(140, 0));
        buttonRetour->setMaximumSize(QSize(140, 16777215));
        buttonRetour->setCursor(QCursor(Qt::PointingHandCursor));
        buttonRetour->setLayoutDirection(Qt::RightToLeft);
        buttonRetour->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        buttonRetour->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/back-arrow.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonRetour->setIcon(icon4);

        verticalLayout->addWidget(buttonRetour);

        buttonImporter = new QToolButton(verticalLayoutWidget);
        buttonImporter->setObjectName(QString::fromUtf8("buttonImporter"));
        sizePolicy.setHeightForWidth(buttonImporter->sizePolicy().hasHeightForWidth());
        buttonImporter->setSizePolicy(sizePolicy);
        buttonImporter->setMinimumSize(QSize(140, 0));
        buttonImporter->setMaximumSize(QSize(140, 16777215));
        buttonImporter->setLayoutDirection(Qt::RightToLeft);
        buttonImporter->setStyleSheet(QString::fromUtf8("QToolButton {\n"
"    background-color: #00BCD4;  /* Cyan primaire */\n"
"    color: white; \n"
"    font-size: 14px;\n"
"    border: 2px solid #008C9E;  /* Bordure l\303\251g\303\250rement plus fonc\303\251e */\n"
"    border-radius: 5px;\n"
"    padding: 6px 12px;\n"
"}\n"
"\n"
"QToolButton:hover {\n"
"    background-color: #26C6DA;  /* Bleu plus clair au survol */\n"
"}\n"
"\n"
"QToolButton:pressed {\n"
"    background-color: #008C9E;  /* Bleu plus fonc\303\251 lorsqu'on clique */\n"
"}\n"
""));
        buttonImporter->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/download.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonImporter->setIcon(icon5);
        buttonImporter->setPopupMode(QToolButton::DelayedPopup);
        buttonImporter->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        verticalLayout->addWidget(buttonImporter);

        buttonSave = new QToolButton(verticalLayoutWidget);
        buttonSave->setObjectName(QString::fromUtf8("buttonSave"));
        buttonSave->setMinimumSize(QSize(140, 0));
        buttonSave->setMaximumSize(QSize(140, 16777215));
        buttonSave->setCursor(QCursor(Qt::PointingHandCursor));
        buttonSave->setLayoutDirection(Qt::RightToLeft);
        buttonSave->setStyleSheet(QString::fromUtf8("QToolButton {\n"
"    background-color: #00BCD4;  /* Cyan primaire */\n"
"    color: white; \n"
"    font-size: 14px;\n"
"    border: 2px solid #008C9E;  /* Bordure l\303\251g\303\250rement plus fonc\303\251e */\n"
"    border-radius: 5px;\n"
"    padding: 6px 12px;\n"
"}\n"
"\n"
"QToolButton:hover {\n"
"    background-color: #26C6DA;  /* Bleu plus clair au survol */\n"
"}\n"
"\n"
"QToolButton:pressed {\n"
"    background-color: #008C9E;  /* Bleu plus fonc\303\251 lorsqu'on clique */\n"
"}\n"
""));
        buttonSave->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/save.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonSave->setIcon(icon6);
        buttonSave->setPopupMode(QToolButton::DelayedPopup);
        buttonSave->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        verticalLayout->addWidget(buttonSave);

        verticalLayoutWidget_2 = new QWidget(custom);
        verticalLayoutWidget_2->setObjectName(QString::fromUtf8("verticalLayoutWidget_2"));
        verticalLayoutWidget_2->setGeometry(QRect(1140, 0, 142, 421));
        verticalLayout_2 = new QVBoxLayout(verticalLayoutWidget_2);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        buttonGomme = new QPushButton(verticalLayoutWidget_2);
        buttonGomme->setObjectName(QString::fromUtf8("buttonGomme"));
        buttonGomme->setMinimumSize(QSize(140, 0));
        buttonGomme->setMaximumSize(QSize(140, 16777215));
        buttonGomme->setCursor(QCursor(Qt::PointingHandCursor));
        buttonGomme->setLayoutDirection(Qt::LeftToRight);
        buttonGomme->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        buttonGomme->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/eraser.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonGomme->setIcon(icon7);

        verticalLayout_2->addWidget(buttonGomme);

        tailleGomme = new QSlider(verticalLayoutWidget_2);
        tailleGomme->setObjectName(QString::fromUtf8("tailleGomme"));
        tailleGomme->setStyleSheet(QString::fromUtf8("QSlider::groove:horizontal {\n"
"    height: 6px;\n"
"    background: #d6d6d6;\n"
"    border: 1px solid #bbb;\n"
"    border-radius: 3px;\n"
"}\n"
"\n"
"QSlider::handle:horizontal {\n"
"    width: 20px;\n"
"    background: #00BCD4;\n"
"    border: 1px solid #008C9E;\n"
"    border-radius: 10px;\n"
"    margin: -5px 0;\n"
"}\n"
"\n"
"QSlider::handle:horizontal:hover {\n"
"    background: #26C6DA;\n"
"}\n"
""));
        tailleGomme->setLocale(QLocale(QLocale::French, QLocale::France));
        tailleGomme->setMinimum(5);
        tailleGomme->setMaximum(100);
        tailleGomme->setValue(20);
        tailleGomme->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(tailleGomme);

        buttonSupprimer = new QPushButton(verticalLayoutWidget_2);
        buttonSupprimer->setObjectName(QString::fromUtf8("buttonSupprimer"));
        buttonSupprimer->setMinimumSize(QSize(140, 0));
        buttonSupprimer->setMaximumSize(QSize(140, 16777215));
        buttonSupprimer->setCursor(QCursor(Qt::PointingHandCursor));
        buttonSupprimer->setLayoutDirection(Qt::LeftToRight);
        buttonSupprimer->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        buttonSupprimer->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/icons/cross.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonSupprimer->setIcon(icon8);
        buttonSupprimer->setIconSize(QSize(20, 20));

        verticalLayout_2->addWidget(buttonSupprimer);

        buttonDeplacer = new QPushButton(verticalLayoutWidget_2);
        buttonDeplacer->setObjectName(QString::fromUtf8("buttonDeplacer"));
        buttonDeplacer->setMinimumSize(QSize(140, 0));
        buttonDeplacer->setMaximumSize(QSize(140, 16777215));
        buttonDeplacer->setCursor(QCursor(Qt::PointingHandCursor));
        buttonDeplacer->setLayoutDirection(Qt::LeftToRight);
        buttonDeplacer->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        buttonDeplacer->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/icons/cursor.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonDeplacer->setIcon(icon9);

        verticalLayout_2->addWidget(buttonDeplacer);

        buttonConnect = new QPushButton(verticalLayoutWidget_2);
        buttonConnect->setObjectName(QString::fromUtf8("buttonConnect"));
        buttonConnect->setMinimumSize(QSize(140, 0));
        buttonConnect->setMaximumSize(QSize(140, 16777215));
        buttonConnect->setStyleSheet(QString::fromUtf8("/* ton style de base */\n"
"QPushButton {\n"
"    background-color: #00BCD4;\n"
"    color: white;\n"
"    font-size: 14px;\n"
"    border: 2px solid #008C9E;\n"
"    border-radius: 5px;\n"
"    padding: 6px 12px;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #26C6DA;\n"
"}\n"
"QPushButton:pressed {\n"
"    background-color: #008C9E;\n"
"}\n"
"\n"
"/* nouveau : quand la propri\303\251t\303\251 closeMode vaut true */\n"
"QPushButton[closeMode=\"true\"] {\n"
"    background-color: #a0c4ff;\n"
"    border: 2px solid #5F7FFF;\n"
"}\n"
"QPushButton[closeMode=\"true\"]:hover {\n"
"    background-color: #b5d5ff;\n"
"}\n"
"QPushButton[closeMode=\"true\"]:pressed {\n"
"    background-color: #8fb1ff;\n"
"}\n"
""));
        buttonConnect->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/icons/link.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonConnect->setIcon(icon10);

        verticalLayout_2->addWidget(buttonConnect);

        buttonSnapGrid = new QPushButton(verticalLayoutWidget_2);
        buttonSnapGrid->setObjectName(QString::fromUtf8("buttonSnapGrid"));
        buttonSnapGrid->setMinimumSize(QSize(140, 0));
        buttonSnapGrid->setMaximumSize(QSize(140, 16777215));
        buttonSnapGrid->setStyleSheet(QString::fromUtf8("/* ton style de base */\n"
"QPushButton {\n"
"    background-color: #00BCD4;\n"
"    color: white;\n"
"    font-size: 14px;\n"
"    border: 2px solid #008C9E;\n"
"    border-radius: 5px;\n"
"    padding: 6px 12px;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #26C6DA;\n"
"}\n"
"QPushButton:pressed {\n"
"    background-color: #008C9E;\n"
"}\n"
"\n"
"/* nouveau : quand la propri\303\251t\303\251 closeMode vaut true */\n"
"QPushButton[closeMode=\"true\"] {\n"
"    background-color: #a0c4ff;\n"
"    border: 2px solid #5F7FFF;\n"
"}\n"
"QPushButton[closeMode=\"true\"]:hover {\n"
"    background-color: #b5d5ff;\n"
"}\n"
"QPushButton[closeMode=\"true\"]:pressed {\n"
"    background-color: #8fb1ff;\n"
"}\n"
""));
        buttonSnapGrid->setLocale(QLocale(QLocale::French, QLocale::France));
        buttonSnapGrid->setIcon(icon10);

        verticalLayout_2->addWidget(buttonSnapGrid);

        sliderGridSpacing = new QSlider(verticalLayoutWidget_2);
        sliderGridSpacing->setObjectName(QString::fromUtf8("sliderGridSpacing"));
        sliderGridSpacing->setStyleSheet(QString::fromUtf8("QSlider::groove:horizontal {\n"
"    height: 6px;\n"
"    background: #d6d6d6;\n"
"    border: 1px solid #bbb;\n"
"    border-radius: 3px;\n"
"}\n"
"\n"
"QSlider::handle:horizontal {\n"
"    width: 20px;\n"
"    background: #00BCD4;\n"
"    border: 1px solid #008C9E;\n"
"    border-radius: 10px;\n"
"    margin: -5px 0;\n"
"}\n"
"\n"
"QSlider::handle:horizontal:hover {\n"
"    background: #26C6DA;\n"
"}\n"
""));
        sliderGridSpacing->setLocale(QLocale(QLocale::French, QLocale::France));
        sliderGridSpacing->setMinimum(5);
        sliderGridSpacing->setMaximum(100);
        sliderGridSpacing->setValue(20);
        sliderGridSpacing->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(sliderGridSpacing);

        labelGridSpacing = new QLabel(verticalLayoutWidget_2);
        labelGridSpacing->setObjectName(QString::fromUtf8("labelGridSpacing"));
        labelGridSpacing->setMaximumSize(QSize(140, 20));

        verticalLayout_2->addWidget(labelGridSpacing);

        buttonCloseShape = new QPushButton(verticalLayoutWidget_2);
        buttonCloseShape->setObjectName(QString::fromUtf8("buttonCloseShape"));
        buttonCloseShape->setMinimumSize(QSize(140, 0));
        buttonCloseShape->setMaximumSize(QSize(140, 16777215));
        buttonCloseShape->setStyleSheet(QString::fromUtf8("/* ton style de base */\n"
"QPushButton {\n"
"    background-color: #00BCD4;\n"
"    color: white;\n"
"    font-size: 14px;\n"
"    border: 2px solid #008C9E;\n"
"    border-radius: 5px;\n"
"    padding: 6px 12px;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #26C6DA;\n"
"}\n"
"QPushButton:pressed {\n"
"    background-color: #008C9E;\n"
"}\n"
"\n"
"/* nouveau : quand la propri\303\251t\303\251 closeMode vaut true */\n"
"QPushButton[closeMode=\"true\"] {\n"
"    background-color: #a0c4ff;\n"
"    border: 2px solid #5F7FFF;\n"
"}\n"
"QPushButton[closeMode=\"true\"]:hover {\n"
"    background-color: #b5d5ff;\n"
"}\n"
"QPushButton[closeMode=\"true\"]:pressed {\n"
"    background-color: #8fb1ff;\n"
"}\n"
""));
        buttonCloseShape->setLocale(QLocale(QLocale::French, QLocale::France));
        QIcon icon11;
        icon11.addFile(QString::fromUtf8(":/icons/fence.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonCloseShape->setIcon(icon11);

        verticalLayout_2->addWidget(buttonCloseShape);


        retranslateUi(custom);

        QMetaObject::connectSlotsByName(custom);
    } // setupUi

    void retranslateUi(QWidget *custom)
    {
        custom->setWindowTitle(QCoreApplication::translate("custom", "Form", nullptr));
        buttonMenu->setText(QCoreApplication::translate("custom", "Main menu     ", nullptr));
        Reset->setText(QCoreApplication::translate("custom", "Reset              ", nullptr));
        Appliquer->setText(QCoreApplication::translate("custom", "Application    ", nullptr));
        buttonLissage->setText(QCoreApplication::translate("custom", "Activer/D\303\251sactiver \n"
"Lissage", nullptr));
        buttonForme->setText(QCoreApplication::translate("custom", "Forme ", nullptr));
        buttonRetour->setText(QCoreApplication::translate("custom", "Retour            ", nullptr));
        buttonImporter->setText(QCoreApplication::translate("custom", "Importer ", nullptr));
        buttonSave->setText(QCoreApplication::translate("custom", "Sauvegarder", nullptr));
        buttonGomme->setText(QCoreApplication::translate("custom", "          Gomme", nullptr));
        buttonSupprimer->setText(QCoreApplication::translate("custom", "     Supprimer", nullptr));
        buttonDeplacer->setText(QCoreApplication::translate("custom", "       D\303\251placer", nullptr));
        buttonConnect->setText(QCoreApplication::translate("custom", "            Relier", nullptr));
        buttonSnapGrid->setText(QCoreApplication::translate("custom", "aiment", nullptr));
        labelGridSpacing->setText(QString());
        buttonCloseShape->setText(QCoreApplication::translate("custom", "          Fermer", nullptr));
    } // retranslateUi

};

namespace Ui {
    class custom: public Ui_custom {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CUSTOM_H
