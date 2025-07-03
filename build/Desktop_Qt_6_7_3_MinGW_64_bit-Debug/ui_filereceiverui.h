/********************************************************************************
** Form generated from reading UI file 'filereceiverui.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FILERECEIVERUI_H
#define UI_FILERECEIVERUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FileReceiverUI
{
public:
    QListWidget *fileListWidget;

    void setupUi(QWidget *FileReceiverUI)
    {
        if (FileReceiverUI->objectName().isEmpty())
            FileReceiverUI->setObjectName("FileReceiverUI");
        FileReceiverUI->resize(1211, 707);
        fileListWidget = new QListWidget(FileReceiverUI);
        fileListWidget->setObjectName("fileListWidget");
        fileListWidget->setGeometry(QRect(20, 10, 441, 661));

        retranslateUi(FileReceiverUI);

        QMetaObject::connectSlotsByName(FileReceiverUI);
    } // setupUi

    void retranslateUi(QWidget *FileReceiverUI)
    {
        FileReceiverUI->setWindowTitle(QCoreApplication::translate("FileReceiverUI", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FileReceiverUI: public Ui_FileReceiverUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FILERECEIVERUI_H
