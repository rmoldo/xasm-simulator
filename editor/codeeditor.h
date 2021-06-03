#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QObject>
#include <QPlainTextEdit>

#include <editor/xasmhighlighter.h>

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);

    // lineNumberAreaPaintEvent is called from LineNumberArea whenever it
    // receives a paint event
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    // lineNumberAreaWidth calculates the width of the LineNumberArea widget
    int  lineNumberAreaWidth();

    //
    void loadFile(const QString &fileName);

    QString getFileName();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);

public slots:
    void open();

signals:
    void loadFinished();

private:
    QWidget *lineNumberArea;
    XASMHighlighter *highlighter;
    QString fileName;
};

#endif // CODEEDITOR_H
