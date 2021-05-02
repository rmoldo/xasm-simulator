#ifndef XASMHIGHLIGHTER_H
#define XASMHIGHLIGHTER_H

#include <QObject>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextDocument>

class XASMHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    XASMHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat registerFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat functionFormat;
};

#endif // XASMHIGHLIGHTER_H
