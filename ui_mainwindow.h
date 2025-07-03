/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <FormeVisualization.h>
#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QPushButton *Cercle;
    QPushButton *Rectangle;
    QPushButton *Triangle;
    QPushButton *Coeur;
    QPushButton *Etoile;
    QPushButton *optimizePlacementButton;
    QPushButton *optimizePlacementButton2;
    QPushButton *buttonInventaire;
    QPushButton *buttonCustom;
    QPushButton *buttonFileReceiver;
    QSlider *Slider_longueur;
    QLabel *Vitesse_txt;
    QSlider *Slider_largeur;
    QLabel *Pression_txt;
    QLabel *Reglages_txt;
    QLabel *Taille_txt;
    QLabel *x_txt;
    QLabel *Hauteur_txt;
    QLabel *mm_txt;
    QSpinBox *Largeur;
    QSpinBox *Longueur;
    QLabel *x_txt_2;
    QSpinBox *Hauteur;
    QLabel *Titre;
    QSpinBox *SpinBox_vitesse;
    QSpinBox *SpinBox_pression;
    QPushButton *pushButton;
    QSpinBox *shapeCountSpinBox;
    QLabel *Taille_txt_2;
    QProgressBar *progressBar;
    QLabel *shapeCountLabel;
    QSpinBox *spaceSpinBox;
    QLabel *Taille_txt_3;
    QWidget *verticalLayoutWidget_2;
    QVBoxLayout *verticalLayout_2;
    QPushButton *ButtonUp;
    QSpacerItem *verticalSpacer;
    QPushButton *ButtonDown;
    QSpacerItem *verticalSpacer_2;
    QPushButton *ButtonRight;
    QSpacerItem *verticalSpacer_3;
    QPushButton *ButtonLeft;
    QSpacerItem *verticalSpacer_4;
    QPushButton *ButtonRotationLeft;
    QSpacerItem *verticalSpacer_5;
    QPushButton *ButtonRotationRight;
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout_5;
    QPushButton *Stop;
    QPushButton *Pause;
    QPushButton *Play;
    FormeVisualization *formeVisualizationWidget;
    QPushButton *pushButton_2;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1920, 1080);
        MainWindow->setStyleSheet(QString::fromUtf8("QWidget {\n"
"	background-color: rgb(189, 189, 189);\n"
"	border-color: rgb(85, 170, 255);\n"
"\n"
"}\n"
""));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        centralwidget->setLocale(QLocale(QLocale::French, QLocale::France));
        verticalLayoutWidget = new QWidget(centralwidget);
        verticalLayoutWidget->setObjectName("verticalLayoutWidget");
        verticalLayoutWidget->setGeometry(QRect(0, 0, 223, 502));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        Cercle = new QPushButton(verticalLayoutWidget);
        Cercle->setObjectName("Cercle");
        Cercle->setMinimumSize(QSize(0, 40));
        Cercle->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        Cercle->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        icon.addFile(QString::fromUtf8(":/icons/circle.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        Cercle->setIcon(icon);

        verticalLayout->addWidget(Cercle);

        Rectangle = new QPushButton(verticalLayoutWidget);
        Rectangle->setObjectName("Rectangle");
        Rectangle->setMinimumSize(QSize(0, 40));
        Rectangle->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        Rectangle->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        icon1.addFile(QString::fromUtf8(":/icons/square.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        Rectangle->setIcon(icon1);

        verticalLayout->addWidget(Rectangle);

        Triangle = new QPushButton(verticalLayoutWidget);
        Triangle->setObjectName("Triangle");
        Triangle->setEnabled(true);
        Triangle->setMinimumSize(QSize(0, 40));
        Triangle->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        Triangle->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        icon2.addFile(QString::fromUtf8(":/icons/triangle.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        Triangle->setIcon(icon2);

        verticalLayout->addWidget(Triangle);

        Coeur = new QPushButton(verticalLayoutWidget);
        Coeur->setObjectName("Coeur");
        Coeur->setMinimumSize(QSize(0, 40));
        Coeur->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        Coeur->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        icon3.addFile(QString::fromUtf8(":/icons/heart.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        Coeur->setIcon(icon3);

        verticalLayout->addWidget(Coeur);

        Etoile = new QPushButton(verticalLayoutWidget);
        Etoile->setObjectName("Etoile");
        Etoile->setMinimumSize(QSize(0, 40));
        Etoile->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        Etoile->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        icon4.addFile(QString::fromUtf8(":/icons/star.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        Etoile->setIcon(icon4);

        verticalLayout->addWidget(Etoile);

        optimizePlacementButton = new QPushButton(verticalLayoutWidget);
        optimizePlacementButton->setObjectName("optimizePlacementButton");
        optimizePlacementButton->setMinimumSize(QSize(0, 40));
        optimizePlacementButton->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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

        verticalLayout->addWidget(optimizePlacementButton);

        optimizePlacementButton2 = new QPushButton(verticalLayoutWidget);
        optimizePlacementButton2->setObjectName("optimizePlacementButton2");
        optimizePlacementButton2->setMinimumSize(QSize(0, 40));
        optimizePlacementButton2->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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

        verticalLayout->addWidget(optimizePlacementButton2);

        buttonInventaire = new QPushButton(verticalLayoutWidget);
        buttonInventaire->setObjectName("buttonInventaire");
        buttonInventaire->setMinimumSize(QSize(0, 40));
        buttonInventaire->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        buttonInventaire->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        icon5.addFile(QString::fromUtf8(":/icons/book.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        buttonInventaire->setIcon(icon5);

        verticalLayout->addWidget(buttonInventaire);

        buttonCustom = new QPushButton(verticalLayoutWidget);
        buttonCustom->setObjectName("buttonCustom");
        buttonCustom->setMinimumSize(QSize(0, 40));
        buttonCustom->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        buttonCustom->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        icon6.addFile(QString::fromUtf8(":/icons/pencil.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        buttonCustom->setIcon(icon6);

        verticalLayout->addWidget(buttonCustom);

        buttonFileReceiver = new QPushButton(verticalLayoutWidget);
        buttonFileReceiver->setObjectName("buttonFileReceiver");
        buttonFileReceiver->setMinimumSize(QSize(0, 40));
        buttonFileReceiver->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        buttonFileReceiver->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/download.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        buttonFileReceiver->setIcon(icon7);

        verticalLayout->addWidget(buttonFileReceiver);

        Slider_longueur = new QSlider(centralwidget);
        Slider_longueur->setObjectName("Slider_longueur");
        Slider_longueur->setGeometry(QRect(610, 470, 111, 22));
        Slider_longueur->setStyleSheet(QString::fromUtf8("QSlider::groove:horizontal {\n"
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
        Slider_longueur->setMaximum(400);
        Slider_longueur->setOrientation(Qt::Orientation::Horizontal);
        Vitesse_txt = new QLabel(centralwidget);
        Vitesse_txt->setObjectName("Vitesse_txt");
        Vitesse_txt->setGeometry(QRect(520, 509, 61, 31));
        Slider_largeur = new QSlider(centralwidget);
        Slider_largeur->setObjectName("Slider_largeur");
        Slider_largeur->setGeometry(QRect(750, 470, 111, 22));
        Slider_largeur->setStyleSheet(QString::fromUtf8("QSlider::groove:horizontal {\n"
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
        Slider_largeur->setMinimum(0);
        Slider_largeur->setMaximum(600);
        Slider_largeur->setSingleStep(1);
        Slider_largeur->setPageStep(10);
        Slider_largeur->setValue(0);
        Slider_largeur->setOrientation(Qt::Orientation::Horizontal);
        Pression_txt = new QLabel(centralwidget);
        Pression_txt->setObjectName("Pression_txt");
        Pression_txt->setGeometry(QRect(670, 515, 61, 21));
        Reglages_txt = new QLabel(centralwidget);
        Reglages_txt->setObjectName("Reglages_txt");
        Reglages_txt->setGeometry(QRect(520, 410, 71, 21));
        Taille_txt = new QLabel(centralwidget);
        Taille_txt->setObjectName("Taille_txt");
        Taille_txt->setGeometry(QRect(520, 430, 91, 31));
        x_txt = new QLabel(centralwidget);
        x_txt->setObjectName("x_txt");
        x_txt->setGeometry(QRect(730, 430, 20, 31));
        Hauteur_txt = new QLabel(centralwidget);
        Hauteur_txt->setObjectName("Hauteur_txt");
        Hauteur_txt->setGeometry(QRect(830, 510, 61, 31));
        mm_txt = new QLabel(centralwidget);
        mm_txt->setObjectName("mm_txt");
        mm_txt->setGeometry(QRect(960, 510, 31, 31));
        Largeur = new QSpinBox(centralwidget);
        Largeur->setObjectName("Largeur");
        Largeur->setGeometry(QRect(750, 430, 111, 26));
        Largeur->setMaximumSize(QSize(700, 700));
        Largeur->setStyleSheet(QString::fromUtf8("QSpinBox{\n"
"	background-color: rgb(255, 255, 255);\n"
"}"));
        Largeur->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::UpDownArrows);
        Largeur->setMaximum(600);
        Largeur->setValue(0);
        Longueur = new QSpinBox(centralwidget);
        Longueur->setObjectName("Longueur");
        Longueur->setGeometry(QRect(610, 430, 111, 26));
        Longueur->setMaximumSize(QSize(700, 400));
        Longueur->setStyleSheet(QString::fromUtf8("QSpinBox{\n"
"	background-color: rgb(255, 255, 255);\n"
"}"));
        Longueur->setMaximum(600);
        Longueur->setValue(0);
        x_txt_2 = new QLabel(centralwidget);
        x_txt_2->setObjectName("x_txt_2");
        x_txt_2->setGeometry(QRect(870, 430, 31, 31));
        Hauteur = new QSpinBox(centralwidget);
        Hauteur->setObjectName("Hauteur");
        Hauteur->setGeometry(QRect(900, 510, 51, 26));
        Hauteur->setMaximumSize(QSize(60, 50));
        Hauteur->setStyleSheet(QString::fromUtf8("QSpinBox{\n"
"	background-color: rgb(255, 255, 255);\n"
"}"));
        Hauteur->setMaximum(100);
        Titre = new QLabel(centralwidget);
        Titre->setObjectName("Titre");
        Titre->setGeometry(QRect(240, 0, 281, 51));
        QFont font;
        font.setFamilies({QString::fromUtf8("Arial")});
        font.setPointSize(20);
        font.setBold(true);
        font.setUnderline(false);
        font.setStrikeOut(false);
        font.setKerning(true);
        Titre->setFont(font);
        Titre->setStyleSheet(QString::fromUtf8("Label {\n"
"    color: #00BCD4;  /* Cyan primaire */\n"
"   }\n"
"\n"
""));
        SpinBox_vitesse = new QSpinBox(centralwidget);
        SpinBox_vitesse->setObjectName("SpinBox_vitesse");
        SpinBox_vitesse->setGeometry(QRect(580, 510, 81, 26));
        SpinBox_vitesse->setStyleSheet(QString::fromUtf8("QSpinBox{\n"
"	background-color: rgb(255, 255, 255);\n"
"}"));
        SpinBox_vitesse->setMaximum(200);
        SpinBox_pression = new QSpinBox(centralwidget);
        SpinBox_pression->setObjectName("SpinBox_pression");
        SpinBox_pression->setGeometry(QRect(740, 510, 81, 26));
        SpinBox_pression->setStyleSheet(QString::fromUtf8("QSpinBox{\n"
"	background-color: rgb(255, 255, 255);\n"
"}"));
        SpinBox_pression->setMinimum(1500);
        SpinBox_pression->setMaximum(3500);
        pushButton = new QPushButton(centralwidget);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(260, 120, 151, 100));
        pushButton->setMinimumSize(QSize(100, 100));
        pushButton->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #D32F2F;  /* Rouge fonc\303\251 */\n"
"    color: white;\n"
"    font-size: 18px;\n"
"    font-weight: bold;\n"
"    border: none;\n"
"    border-radius: 50px;  /* Rend le bouton rond */\n"
"    min-width: 100px;  /* Taille minimale */\n"
"    min-height: 100px;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background-color: #F44336;  /* Rouge plus clair au survol */\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background-color: #B71C1C;  /* Rouge encore plus fonc\303\251 quand cliqu\303\251 */\n"
"}\n"
""));
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/icons/on-off.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        pushButton->setIcon(icon8);
        pushButton->setIconSize(QSize(80, 80));
        shapeCountSpinBox = new QSpinBox(centralwidget);
        shapeCountSpinBox->setObjectName("shapeCountSpinBox");
        shapeCountSpinBox->setGeometry(QRect(1000, 430, 111, 26));
        shapeCountSpinBox->setStyleSheet(QString::fromUtf8("QSpinBox{\n"
"	background-color: rgb(255, 255, 255);\n"
"}"));
        shapeCountSpinBox->setMaximum(1000000);
        shapeCountSpinBox->setValue(0);
        Taille_txt_2 = new QLabel(centralwidget);
        Taille_txt_2->setObjectName("Taille_txt_2");
        Taille_txt_2->setGeometry(QRect(920, 430, 71, 31));
        progressBar = new QProgressBar(centralwidget);
        progressBar->setObjectName("progressBar");
        progressBar->setEnabled(true);
        progressBar->setGeometry(QRect(0, 520, 221, 41));
        QFont font1;
        font1.setBold(true);
        progressBar->setFont(font1);
        progressBar->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        progressBar->setStyleSheet(QString::fromUtf8("QProgressBar {\n"
"    border: 2px solid #00BCD4;\n"
"    border-radius: 5px;\n"
"    background: #f3f3f3;\n"
"    text-align: center;\n"
"    font-size: 12px;\n"
"    color: black;\n"
"}\n"
"\n"
"QProgressBar::chunk {\n"
"    background: #00BCD4;\n"
"    width: 10px;\n"
"}\n"
""));
        progressBar->setValue(0);
        progressBar->setTextVisible(true);
        shapeCountLabel = new QLabel(centralwidget);
        shapeCountLabel->setObjectName("shapeCountLabel");
        shapeCountLabel->setGeometry(QRect(1120, 0, 171, 21));
        spaceSpinBox = new QSpinBox(centralwidget);
        spaceSpinBox->setObjectName("spaceSpinBox");
        spaceSpinBox->setGeometry(QRect(1000, 470, 111, 26));
        spaceSpinBox->setStyleSheet(QString::fromUtf8("QSpinBox{\n"
"	background-color: rgb(255, 255, 255);\n"
"}"));
        spaceSpinBox->setMaximum(1000000);
        spaceSpinBox->setValue(0);
        Taille_txt_3 = new QLabel(centralwidget);
        Taille_txt_3->setObjectName("Taille_txt_3");
        Taille_txt_3->setGeometry(QRect(920, 470, 71, 31));
        verticalLayoutWidget_2 = new QWidget(centralwidget);
        verticalLayoutWidget_2->setObjectName("verticalLayoutWidget_2");
        verticalLayoutWidget_2->setGeometry(QRect(1120, 0, 69, 401));
        verticalLayout_2 = new QVBoxLayout(verticalLayoutWidget_2);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        ButtonUp = new QPushButton(verticalLayoutWidget_2);
        ButtonUp->setObjectName("ButtonUp");
        ButtonUp->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/icons/arrow-up.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        ButtonUp->setIcon(icon9);
        ButtonUp->setAutoRepeat(true);

        verticalLayout_2->addWidget(ButtonUp);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        ButtonDown = new QPushButton(verticalLayoutWidget_2);
        ButtonDown->setObjectName("ButtonDown");
        ButtonDown->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/icons/arrow-down.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        ButtonDown->setIcon(icon10);
        ButtonDown->setAutoRepeat(true);

        verticalLayout_2->addWidget(ButtonDown);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_2);

        ButtonRight = new QPushButton(verticalLayoutWidget_2);
        ButtonRight->setObjectName("ButtonRight");
        ButtonRight->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        QIcon icon11;
        icon11.addFile(QString::fromUtf8(":/icons/arrow-right.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        ButtonRight->setIcon(icon11);
        ButtonRight->setAutoRepeat(true);

        verticalLayout_2->addWidget(ButtonRight);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_3);

        ButtonLeft = new QPushButton(verticalLayoutWidget_2);
        ButtonLeft->setObjectName("ButtonLeft");
        ButtonLeft->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        QIcon icon12;
        icon12.addFile(QString::fromUtf8(":/icons/arrow-left.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        ButtonLeft->setIcon(icon12);
        ButtonLeft->setAutoRepeat(true);

        verticalLayout_2->addWidget(ButtonLeft);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_4);

        ButtonRotationLeft = new QPushButton(verticalLayoutWidget_2);
        ButtonRotationLeft->setObjectName("ButtonRotationLeft");
        ButtonRotationLeft->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        QIcon icon13;
        icon13.addFile(QString::fromUtf8(":/icons/rotate-left.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        ButtonRotationLeft->setIcon(icon13);

        verticalLayout_2->addWidget(ButtonRotationLeft);

        verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_5);

        ButtonRotationRight = new QPushButton(verticalLayoutWidget_2);
        ButtonRotationRight->setObjectName("ButtonRotationRight");
        ButtonRotationRight->setStyleSheet(QString::fromUtf8("QPushButton {\n"
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
        QIcon icon14;
        icon14.addFile(QString::fromUtf8(":/icons/rotate-right.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        ButtonRotationRight->setIcon(icon14);

        verticalLayout_2->addWidget(ButtonRotationRight);

        layoutWidget = new QWidget(centralwidget);
        layoutWidget->setObjectName("layoutWidget");
        layoutWidget->setGeometry(QRect(0, 570, 220, 81));
        horizontalLayout_5 = new QHBoxLayout(layoutWidget);
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        horizontalLayout_5->setContentsMargins(0, 0, 0, 0);
        Stop = new QPushButton(layoutWidget);
        Stop->setObjectName("Stop");
        Stop->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: rgb(255, 10, 10);  /* Cyan primaire */\n"
"    color: black; \n"
"    font-size: 14px;\n"
"    border-radius: 19px;\n"
"    padding: 6px 12px;\n"
"}\n"
"\n"
""));
        QIcon icon15;
        icon15.addFile(QString::fromUtf8(":/icons/stop.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        Stop->setIcon(icon15);
        Stop->setIconSize(QSize(40, 40));

        horizontalLayout_5->addWidget(Stop);

        Pause = new QPushButton(layoutWidget);
        Pause->setObjectName("Pause");
        Pause->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: yellow;  /* Cyan primaire */\n"
"    color: black; \n"
"    font-size: 14px;\n"
"    border-radius: 19px;\n"
"    padding: 6px 12px;\n"
"}\n"
""));
        QIcon icon16;
        icon16.addFile(QString::fromUtf8(":/icons/pause.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        Pause->setIcon(icon16);
        Pause->setIconSize(QSize(40, 40));

        horizontalLayout_5->addWidget(Pause);

        Play = new QPushButton(layoutWidget);
        Play->setObjectName("Play");
        Play->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: rgb(0, 170, 0);  /* Cyan primaire */\n"
"    color: black; \n"
"    font-size: 14px;\n"
"    border-radius: 19px;\n"
"    padding: 6px 12px;\n"
"}\n"
"\n"
"QPushButton :pressed {\n"
"	\n"
"	background-color: rgb(0, 140, 0);\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"	background-color: rgb(0, 100, 0);\n"
"}\n"
""));
        QIcon icon17;
        icon17.addFile(QString::fromUtf8(":/icons/play.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        Play->setIcon(icon17);
        Play->setIconSize(QSize(40, 40));

        horizontalLayout_5->addWidget(Play);

        formeVisualizationWidget = new FormeVisualization(centralwidget);
        formeVisualizationWidget->setObjectName("formeVisualizationWidget");
        formeVisualizationWidget->setGeometry(QRect(520, 0, 600, 400));
        formeVisualizationWidget->setMinimumSize(QSize(600, 400));
        formeVisualizationWidget->setMaximumSize(QSize(650, 450));
        formeVisualizationWidget->setCursor(QCursor(Qt::CursorShape::CrossCursor));
        formeVisualizationWidget->setAutoFillBackground(false);
        formeVisualizationWidget->setStyleSheet(QString::fromUtf8("QWidget {\n"
"	background-color: rgb(255, 255, 255);\n"
"}\n"
""));
        pushButton_2 = new QPushButton(centralwidget);
        pushButton_2->setObjectName("pushButton_2");
        pushButton_2->setGeometry(QRect(1010, 510, 231, 111));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1920, 26));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);
        QObject::connect(pushButton, &QPushButton::clicked, MainWindow, qOverload<>(&QMainWindow::close));

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        Cercle->setText(QCoreApplication::translate("MainWindow", "Cercle                             ", nullptr));
        Rectangle->setText(QCoreApplication::translate("MainWindow", "Rectangle                       ", nullptr));
        Triangle->setText(QCoreApplication::translate("MainWindow", "Triangle                          ", nullptr));
        Coeur->setText(QCoreApplication::translate("MainWindow", "Coeur                            ", nullptr));
        Etoile->setText(QCoreApplication::translate("MainWindow", "Etoile                             ", nullptr));
        optimizePlacementButton->setText(QCoreApplication::translate("MainWindow", "Optimiser placement", nullptr));
        optimizePlacementButton2->setText(QCoreApplication::translate("MainWindow", "Optimiser placement 2", nullptr));
        buttonInventaire->setText(QCoreApplication::translate("MainWindow", "Inventaire                      ", nullptr));
        buttonCustom->setText(QCoreApplication::translate("MainWindow", "Custom                         ", nullptr));
        buttonFileReceiver->setText(QCoreApplication::translate("MainWindow", "R\303\251ception fichier            ", nullptr));
        Vitesse_txt->setText(QCoreApplication::translate("MainWindow", "Vitesse : ", nullptr));
        Pression_txt->setText(QCoreApplication::translate("MainWindow", "Pression : ", nullptr));
        Reglages_txt->setText(QCoreApplication::translate("MainWindow", "R\303\251glages : ", nullptr));
        Taille_txt->setText(QCoreApplication::translate("MainWindow", "Dimensions : ", nullptr));
        x_txt->setText(QCoreApplication::translate("MainWindow", "X", nullptr));
        Hauteur_txt->setText(QCoreApplication::translate("MainWindow", "Hauteur : ", nullptr));
        mm_txt->setText(QCoreApplication::translate("MainWindow", "mm", nullptr));
        x_txt_2->setText(QCoreApplication::translate("MainWindow", "mm", nullptr));
        Titre->setText(QCoreApplication::translate("MainWindow", "Eau-Couper by AE", nullptr));
        pushButton->setText(QString());
        Taille_txt_2->setText(QCoreApplication::translate("MainWindow", "Quantit\303\251e : ", nullptr));
        shapeCountLabel->setText(QString());
        Taille_txt_3->setText(QCoreApplication::translate("MainWindow", "Espacement : ", nullptr));
        ButtonUp->setText(QString());
        ButtonDown->setText(QString());
        ButtonRight->setText(QString());
        ButtonLeft->setText(QString());
        ButtonRotationLeft->setText(QString());
        ButtonRotationRight->setText(QString());
        Stop->setText(QString());
        Pause->setText(QString());
        Play->setText(QString());
        pushButton_2->setText(QCoreApplication::translate("MainWindow", "PushButton", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
