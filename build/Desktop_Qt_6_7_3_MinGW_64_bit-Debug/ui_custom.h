/********************************************************************************
** Form generated from reading UI file 'custom.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
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
            custom->setObjectName("custom");
        custom->resize(1541, 930);
        custom->setStyleSheet(QString::fromUtf8("QWidget {\n"
"	background-color: rgb(189, 189, 189);\n"
"}\n"
""));
        buttonMenu = new QPushButton(custom);
        buttonMenu->setObjectName("buttonMenu");
        buttonMenu->setGeometry(QRect(0, 0, 141, 61));
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
        Appliquer = new QPushButton(custom);
        Appliquer->setObjectName("Appliquer");
        Appliquer->setGeometry(QRect(0, 80, 141, 61));
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
        Reset = new QPushButton(custom);
        Reset->setObjectName("Reset");
        Reset->setGeometry(QRect(0, 160, 141, 61));
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
        drawingWidget = new QWidget(custom);
        drawingWidget->setObjectName("drawingWidget");
        drawingWidget->setGeometry(QRect(140, 0, 1200, 800));
        drawingWidget->setMinimumSize(QSize(1000, 500));
        drawingWidget->setMaximumSize(QSize(1500, 800));
        drawingWidget->setStyleSheet(QString::fromUtf8("QWidget {\n"
"	background-color: rgb(255, 255, 255);\n"
"}\n"
""));
        lineEdit = new QLineEdit(drawingWidget);
        lineEdit->setObjectName("lineEdit");
        lineEdit->setGeometry(QRect(0, 0, 311, 51));
        buttonClavier = new QPushButton(custom);
        buttonClavier->setObjectName("buttonClavier");
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
        buttonLissage->setObjectName("buttonLissage");
        buttonLissage->setGeometry(QRect(0, 330, 141, 61));
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
        buttonSupprimer->setObjectName("buttonSupprimer");
        buttonSupprimer->setGeometry(QRect(1340, 0, 141, 61));
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
        buttonGomme = new QPushButton(custom);
        buttonGomme->setObjectName("buttonGomme");
        buttonGomme->setGeometry(QRect(1340, 80, 141, 61));
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
        buttonDeplacer = new QPushButton(custom);
        buttonDeplacer->setObjectName("buttonDeplacer");
        buttonDeplacer->setGeometry(QRect(1340, 160, 141, 61));
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
        tailleGomme = new QSlider(custom);
        tailleGomme->setObjectName("tailleGomme");
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
        buttonBouger->setObjectName("buttonBouger");
        buttonBouger->setGeometry(QRect(1340, 240, 141, 61));
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
        QIcon icon;
        icon.addFile(QString::fromUtf8("../../../../../../../../Downloads/main.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        buttonBouger->setIcon(icon);
        buttonBouger->setIconSize(QSize(50, 50));
        buttonForme = new QToolButton(custom);
        buttonForme->setObjectName("buttonForme");
        buttonForme->setGeometry(QRect(0, 410, 141, 61));
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
        buttonForme->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        buttonRetour = new QPushButton(custom);
        buttonRetour->setObjectName("buttonRetour");
        buttonRetour->setGeometry(QRect(0, 490, 141, 61));
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
        QIcon icon1(QIcon::fromTheme(QIcon::ThemeIcon::EditUndo));
        buttonRetour->setIcon(icon1);
        buttonImporter = new QToolButton(custom);
        buttonImporter->setObjectName("buttonImporter");
        buttonImporter->setGeometry(QRect(0, 570, 141, 61));
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
        buttonImporter->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        buttonSave = new QToolButton(custom);
        buttonSave->setObjectName("buttonSave");
        buttonSave->setGeometry(QRect(0, 650, 141, 61));
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
        buttonSave->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);

        retranslateUi(custom);

        QMetaObject::connectSlotsByName(custom);
    } // setupUi

    void retranslateUi(QWidget *custom)
    {
        custom->setWindowTitle(QCoreApplication::translate("custom", "Form", nullptr));
        buttonMenu->setText(QCoreApplication::translate("custom", "Main menu", nullptr));
        Appliquer->setText(QCoreApplication::translate("custom", "application", nullptr));
        Reset->setText(QCoreApplication::translate("custom", "reset", nullptr));
        buttonClavier->setText(QCoreApplication::translate("custom", "Clavier", nullptr));
        buttonLissage->setText(QCoreApplication::translate("custom", "Activer/D\303\251sactiver Lissage", nullptr));
        buttonSupprimer->setText(QCoreApplication::translate("custom", "Supprimer", nullptr));
        buttonGomme->setText(QCoreApplication::translate("custom", "Gomme", nullptr));
        buttonDeplacer->setText(QCoreApplication::translate("custom", "Deplacer", nullptr));
        buttonBouger->setText(QString());
        buttonForme->setText(QCoreApplication::translate("custom", "Forme", nullptr));
        buttonRetour->setText(QString());
        buttonImporter->setText(QCoreApplication::translate("custom", "Importer", nullptr));
        buttonSave->setText(QCoreApplication::translate("custom", "Save", nullptr));
    } // retranslateUi

};

namespace Ui {
    class custom: public Ui_custom {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CUSTOM_H
