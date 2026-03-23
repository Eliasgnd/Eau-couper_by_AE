#ifndef TEXTTOOL_H
#define TEXTTOOL_H

#include <QObject>
#include <QFont>

class QString;

class TextTool : public QObject
{
    Q_OBJECT
public:
    explicit TextTool(QObject *parent = nullptr);

    void setTextFont(const QFont &font);
    QFont getTextFont() const;

    void setCurrentText(const QString &text);
    QString getCurrentText() const;

private:
    QFont m_textFont;
    QString m_currentText;
};

#endif // TEXTTOOL_H
