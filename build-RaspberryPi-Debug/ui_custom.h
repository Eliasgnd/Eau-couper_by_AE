/********************************************************************************
** Form generated from reading UI file 'custom.ui'
**
** Created by: Qt User Interface Compiler version 5.15.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CUSTOM_H
#define UI_CUSTOM_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_custom
{
public:
    QPushButton *buttonMenu;
    QPushButton *Appliquer;
    QPushButton *Reset;
    QWidget *drawingWidget;
    QLineEdit *lineEdit;
    QPushButton *buttonClavier;
    QPushButton *buttonLissage;
    QPushButton *buttonSupprimer;
    QPushButton *buttonGomme;
    QPushButton *buttonDeplacer;
    QSlider *tailleGomme;
    QPushButton *buttonBouger;
    QToolButton *buttonForme;
    QPushButton *buttonRetour;
    QToolButton *buttonImporter;
    QToolButton *buttonSave;

    void setupUi(QWidget *custom)
    {
        if (custom->objectName().isEmpty())
            custom->setObjectName(QString::fromUtf8("custom"));
        custom->resize(1541, 930);
        custom->setStyleSheet(QString::fromUtf8("QWidget {\n"
"	background-color: rgb(189, 189, 189);\n"
"}\n"
""));
        buttonMenu = new QPushButton(custom);
        buttonMenu->setObjectName(QString::fromUtf8("buttonMenu"));
        buttonMenu->setGeometry(QRect(0, 0, 141, 61));
        buttonMenu->setCursor(QCursor(Qt::PointingHandCursor));
        buttonMenu->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
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
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/house.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonMenu->setIcon(icon);
        Appliquer = new QPushButton(custom);
        Appliquer->setObjectName(QString::fromUtf8("Appliquer"));
        Appliquer->setGeometry(QRect(0, 80, 141, 61));
        Appliquer->setCursor(QCursor(Qt::PointingHandCursor));
        Appliquer->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
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
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/check.svg"), QSize(), QIcon::Normal, QIcon::Off);
        Appliquer->setIcon(icon1);
        Reset = new QPushButton(custom);
        Reset->setObjectName(QString::fromUtf8("Reset"));
        Reset->setGeometry(QRect(0, 160, 141, 61));
        Reset->setCursor(QCursor(Qt::PointingHandCursor));
        Reset->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
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
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/reload.svg"), QSize(), QIcon::Normal, QIcon::Off);
        Reset->setIcon(icon2);
        drawingWidget = new QWidget(custom);
        drawingWidget->setObjectName(QString::fromUtf8("drawingWidget"));
        drawingWidget->setGeometry(QRect(140, 0, 1200, 800));
        drawingWidget->setMinimumSize(QSize(1000, 500));
        drawingWidget->setMaximumSize(QSize(1500, 800));
        drawingWidget->setStyleSheet(QString::fromUtf8("QWidget {\n"
"	background-color: rgb(255, 255, 255);\n"
"}\n"
""));
        lineEdit = new QLineEdit(drawingWidget);
        lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
        lineEdit->setGeometry(QRect(0, 0, 311, 51));
        buttonClavier = new QPushButton(custom);
        buttonClavier->setObjectName(QString::fromUtf8("buttonClavier"));
        buttonClavier->setGeometry(QRect(0, 250, 141, 61));
        buttonClavier->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        buttonLissage = new QPushButton(custom);
        buttonLissage->setObjectName(QString::fromUtf8("buttonLissage"));
        buttonLissage->setGeometry(QRect(0, 330, 141, 61));
        buttonLissage->setCursor(QCursor(Qt::PointingHandCursor));
        buttonLissage->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        buttonSupprimer = new QPushButton(custom);
        buttonSupprimer->setObjectName(QString::fromUtf8("buttonSupprimer"));
        buttonSupprimer->setGeometry(QRect(1340, 0, 141, 61));
        buttonSupprimer->setCursor(QCursor(Qt::PointingHandCursor));
        buttonSupprimer->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
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
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/cross.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonSupprimer->setIcon(icon3);
        buttonSupprimer->setIconSize(QSize(20, 20));
        buttonGomme = new QPushButton(custom);
        buttonGomme->setObjectName(QString::fromUtf8("buttonGomme"));
        buttonGomme->setGeometry(QRect(1340, 80, 141, 61));
        buttonGomme->setCursor(QCursor(Qt::PointingHandCursor));
        buttonGomme->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
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
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/eraser.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonGomme->setIcon(icon4);
        buttonDeplacer = new QPushButton(custom);
        buttonDeplacer->setObjectName(QString::fromUtf8("buttonDeplacer"));
        buttonDeplacer->setGeometry(QRect(1340, 160, 141, 61));
        buttonDeplacer->setCursor(QCursor(Qt::PointingHandCursor));
        buttonDeplacer->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
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
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/cursor.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonDeplacer->setIcon(icon5);
        tailleGomme = new QSlider(custom);
        tailleGomme->setObjectName(QString::fromUtf8("tailleGomme"));
        tailleGomme->setGeometry(QRect(1340, 140, 141, 22));
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
        tailleGomme->setMinimum(5);
        tailleGomme->setMaximum(100);
        tailleGomme->setValue(20);
        tailleGomme->setOrientation(Qt::Orientation::Horizontal);
        buttonBouger = new QPushButton(custom);
        buttonBouger->setObjectName(QString::fromUtf8("buttonBouger"));
        buttonBouger->setGeometry(QRect(1340, 240, 141, 61));
        buttonBouger->setCursor(QCursor(Qt::PointingHandCursor));
        buttonBouger->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        QIcon icon6;
        icon6.addFile(QString::fromUtf8("../../../../../../../../Downloads/main.png"), QSize(), QIcon::Normal, QIcon::Off);
        buttonBouger->setIcon(icon6);
        buttonBouger->setIconSize(QSize(50, 50));
        buttonForme = new QToolButton(custom);
        buttonForme->setObjectName(QString::fromUtf8("buttonForme"));
        buttonForme->setGeometry(QRect(0, 410, 141, 61));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(buttonForme->sizePolicy().hasHeightForWidth());
        buttonForme->setSizePolicy(sizePolicy);
        buttonForme->setCursor(QCursor(Qt::PointingHandCursor));
        buttonForme->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
        buttonForme->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
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
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/pencil.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonForme->setIcon(icon7);
        buttonForme->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        buttonForme->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
        buttonForme->setArrowType(Qt::ArrowType::NoArrow);
        buttonRetour = new QPushButton(custom);
        buttonRetour->setObjectName(QString::fromUtf8("buttonRetour"));
        buttonRetour->setGeometry(QRect(0, 490, 141, 61));
        buttonRetour->setCursor(QCursor(Qt::PointingHandCursor));
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
        QIcon icon8(QIcon::fromTheme(QString::fromUtf8("QIcon::ThemeIcon::EditUndo")));
        buttonRetour->setIcon(icon8);
        buttonImporter = new QToolButton(custom);
        buttonImporter->setObjectName(QString::fromUtf8("buttonImporter"));
        buttonImporter->setGeometry(QRect(0, 570, 141, 61));
        sizePolicy.setHeightForWidth(buttonImporter->sizePolicy().hasHeightForWidth());
        buttonImporter->setSizePolicy(sizePolicy);
        buttonImporter->setMinimumSize(QSize(0, 0));
        buttonImporter->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
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
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/icons/download.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonImporter->setIcon(icon9);
        buttonImporter->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        buttonImporter->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
        buttonSave = new QToolButton(custom);
        buttonSave->setObjectName(QString::fromUtf8("buttonSave"));
        buttonSave->setGeometry(QRect(0, 650, 141, 61));
        buttonSave->setCursor(QCursor(Qt::PointingHandCursor));
        buttonSave->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
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
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/icons/save.svg"), QSize(), QIcon::Normal, QIcon::Off);
        buttonSave->setIcon(icon10);
        buttonSave->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        buttonSave->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);

        retranslateUi(custom);

        QMetaObject::connectSlotsByName(custom);
    } // setupUi

    void retranslateUi(QWidget *custom)
    {
        custom->setWindowTitle(QCoreApplication::translate("custom", "Form", nullptr));
        buttonMenu->setText(QCoreApplication::translate("custom", "Main menu       ", nullptr));
        Appliquer->setText(QCoreApplication::translate("custom", "Application       ", nullptr));
        Reset->setText(QCoreApplication::translate("custom", "Reset                 ", nullptr));
        buttonClavier->setText(QCoreApplication::translate("custom", "Clavier", nullptr));
        buttonLissage->setText(QCoreApplication::translate("custom", "Activer/D\303\251sactiver Lissage", nullptr));
        buttonSupprimer->setText(QCoreApplication::translate("custom", "       Supprimer", nullptr));
        buttonGomme->setText(QCoreApplication::translate("custom", "            Gomme", nullptr));
        buttonDeplacer->setText(QCoreApplication::translate("custom", "           Deplacer", nullptr));
        buttonBouger->setText(QString());
        buttonForme->setText(QCoreApplication::translate("custom", "Forme ", nullptr));
        buttonRetour->setText(QString());
        buttonImporter->setText(QCoreApplication::translate("custom", "Importer ", nullptr));
        buttonSave->setText(QCoreApplication::translate("custom", "Save ", nullptr));
    } // retranslateUi

};

namespace Ui {
    class custom: public Ui_custom {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CUSTOM_H
