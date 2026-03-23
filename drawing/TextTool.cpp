#include "TextTool.h"

#include <QFont>
#include <QString>

TextTool::TextTool(QObject *parent)
    : QObject(parent)
    , m_textFont(QStringLiteral("Arial"), 16)
    , m_currentText(QStringLiteral("Votre texte ici"))
{
}

void TextTool::setTextFont(const QFont &font)
{
    m_textFont = font;
}

QFont TextTool::getTextFont() const
{
    return m_textFont;
}

void TextTool::setCurrentText(const QString &text)
{
    m_currentText = text;
}

QString TextTool::getCurrentText() const
{
    return m_currentText;
}
