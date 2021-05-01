#include "xasmhighlighter.h"

#include <QStringLiteral>
#include <QSyntaxHighlighter>

XASMHighlighter::XASMHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);

    // Patterns for lowercase and uppercase instructions
    const QString keywordPatterns[] = {
        QStringLiteral("\\bmov\\b"), QStringLiteral("\\badd\\b"), QStringLiteral("\\bsub\\b"), QStringLiteral("\\bcmp\\b"),
        QStringLiteral("\\badd\\b"), QStringLiteral("\\bor\\b"), QStringLiteral("\\bxor\\b"), QStringLiteral("\\bclr\\b"),
        QStringLiteral("\\bneg\\b"), QStringLiteral("\\binc\\b"), QStringLiteral("\\bdec\\b"), QStringLiteral("\\basl\\b"),
        QStringLiteral("\\basr\\b"), QStringLiteral("\\blsr\\b"), QStringLiteral("\\brol\\b"), QStringLiteral("\\bror\\b"),
        QStringLiteral("\\brlc\\b"), QStringLiteral("\\brrc\\b"), QStringLiteral("\\bjmp\\b"), QStringLiteral("\\bcall\\b"),
        QStringLiteral("\\bpush\\b"), QStringLiteral("\\bpop\\b"), QStringLiteral("\\bbr\\b"), QStringLiteral("\\bbne\\b"),
        QStringLiteral("\\bbeq\\b"), QStringLiteral("\\bbpl\\b"), QStringLiteral("\\bbcs\\b"), QStringLiteral("\\bbcc\\b"),
        QStringLiteral("\\bbvs\\b"), QStringLiteral("\\bbvc\\b"), QStringLiteral("\\bclc\\b"), QStringLiteral("\\bclv\\b"),
        QStringLiteral("\\bclz\\b"), QStringLiteral("\\bcls\\b"), QStringLiteral("\\bccc\\b"), QStringLiteral("\\bsec\\b"),
        QStringLiteral("\\bsev\\b"), QStringLiteral("\\bsez\\b"), QStringLiteral("\\bses\\b"), QStringLiteral("\\bscc\\b"),
        QStringLiteral("\\bnop\\b"), QStringLiteral("\\bret\\b"), QStringLiteral("\\breti\\b"), QStringLiteral("\\bhalt\\b"),
        QStringLiteral("\\bwait\\b"), QStringLiteral("\\bpushpc\\b"), QStringLiteral("\\bpoppc\\b"), QStringLiteral("\\bpushflag\\b"),
        QStringLiteral("\\bpopflag\\b"),
        QStringLiteral("\\bMOV\\b"), QStringLiteral("\\bADD\\b"), QStringLiteral("\\bSUB\\b"), QStringLiteral("\\bCMP\\b"),
        QStringLiteral("\\bADD\\b"), QStringLiteral("\\bOR\\b"), QStringLiteral("\\bXOR\\b"), QStringLiteral("\\bCLR\\b"),
        QStringLiteral("\\bNEG\\b"), QStringLiteral("\\bINC\\b"), QStringLiteral("\\bDEC\\b"), QStringLiteral("\\bASL\\b"),
        QStringLiteral("\\bASR\\b"), QStringLiteral("\\bLSR\\b"), QStringLiteral("\\bROL\\b"), QStringLiteral("\\bROR\\b"),
        QStringLiteral("\\bRLC\\b"), QStringLiteral("\\bRRC\\b"), QStringLiteral("\\bJMP\\b"), QStringLiteral("\\bCALL\\b"),
        QStringLiteral("\\bPUSH\\b"), QStringLiteral("\\bPOP\\b"), QStringLiteral("\\bBR\\b"), QStringLiteral("\\bBNE\\b"),
        QStringLiteral("\\bBEQ\\b"), QStringLiteral("\\bBPL\\b"), QStringLiteral("\\bBCS\\b"), QStringLiteral("\\bBCC\\b"),
        QStringLiteral("\\bBVS\\b"), QStringLiteral("\\bBVC\\b"), QStringLiteral("\\bCLC\\b"), QStringLiteral("\\bCLV\\b"),
        QStringLiteral("\\bCLZ\\b"), QStringLiteral("\\bCLS\\b"), QStringLiteral("\\bCCC\\b"), QStringLiteral("\\bSEC\\b"),
        QStringLiteral("\\bSEV\\b"), QStringLiteral("\\bSEZ\\b"), QStringLiteral("\\bSES\\b"), QStringLiteral("\\bSCC\\b"),
        QStringLiteral("\\bNOP\\b"), QStringLiteral("\\bRET\\b"), QStringLiteral("\\bRETI\\b"), QStringLiteral("\\bHALT\\b"),
        QStringLiteral("\\bWAIT\\b"), QStringLiteral("\\bPUSHPC\\b"), QStringLiteral("\\bPOPPC\\b"), QStringLiteral("\\bPUSHFLAG\\b"),
        QStringLiteral("\\bPOPFLAG\\b")
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Registers
    registerFormat.setForeground(Qt::darkRed);
    registerFormat.setFontItalic(true);
    rule.pattern = QRegularExpression(QStringLiteral("\\$(r|R)[0-9]{1,2}"));
    rule.format = registerFormat;
    highlightingRules.append(rule);

    // Comments
    singleLineCommentFormat.setForeground(Qt::darkGray);
    rule.pattern = QRegularExpression(QStringLiteral(";[^\n]*"));
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
}

void XASMHighlighter::highlightBlock(const QString &text) {
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);
}
