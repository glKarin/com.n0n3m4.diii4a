/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "codeeditor.h"
#include <Windows.h>
//! [0]
Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\bif\\b" << "\\belse\\b" << "\\bfor\\b"
                    << "\\bforeach\\b" << "\\bin\\b" << "\\band\\b"
                    << "\\bor\\b" << "\\bwhile\\b" << "\\bdowhile\\b"
                    << "\\bfunction\\b" << "\\breturn\\b" << "\\bcontinue\\b"
                    << "\\bbreak\\b" << "\\bnull\\b" << "\\bglobal\\b"
                    << "\\blocal\\b" << "\\bmember\\b" << "\\btable\\b"
                    << "\\btrue\\b" << "\\bfalse\\b" << "\\bthis\\b";
    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegExp(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);

    singleLineCommentFormat.setForeground(Qt::red);
    rule.pattern = QRegExp("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::red);

    quotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    functionFormat.setFontItalic(true);
    functionFormat.setForeground(Qt::blue);
    rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegExp("/\\*");
    commentEndExpression = QRegExp("\\*/");
}

void Highlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = commentStartExpression.indexIn(text);

    while (startIndex >= 0) {
        int endIndex = commentEndExpression.indexIn(text, startIndex);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                            + commentEndExpression.matchedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
    }
}

CodeEditor::CodeEditor(QWidget *parent) : QTextEdit(parent)
{
	lineNumberArea = new LineNumberArea(this);
	highlighter = new Highlighter(document());

	imageBreakPoint = new QImage( QString::fromUtf8(":/GMDebuggerQt/Resources/breakpoint.png") );
	imageCurrentLine = new QImage( QString::fromUtf8(":/GMDebuggerQt/Resources/currentline.png") );

	connect(document(), SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
	connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
	connect(lineNumberArea, SIGNAL(lineClicked(int)), this, SLOT(LineMarginClicked(int)));

	updateLineNumberAreaWidth(0);
	highlightCurrentLine();
}

void CodeEditor::LineMarginClicked( int lineNum ) {
	if ( lineBreakPoints.find( lineNum ) != lineBreakPoints.end() ) {
		emit BreakPointChanged( lineNum, false );
	} else {
		emit BreakPointChanged( lineNum, true );
	}
}

int CodeEditor::lineNumberAreaWidth()
{
	int digits = 2;
	int max = qMax(1, document()->blockCount());
	while (max >= 10) {
		max /= 10;
		++digits;
	}

	int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

	space += imageBreakPoint->rect().width();
	space += imageCurrentLine->rect().width() / 2;

	return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{ 
	setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
	if (dy)
		lineNumberArea->scroll(0, dy);
	else
		lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

	if (rect.contains(viewport()->rect()))
		updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
	QTextEdit::resizeEvent(e);

	QRect cr = contentsRect();
	lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
	QList<QTextEdit::ExtraSelection> extraSelections;

	if (!isReadOnly()) {
		QTextEdit::ExtraSelection selection;

		QColor lineColor = QColor(Qt::yellow).lighter(160);

		selection.format.setBackground(lineColor);
		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		selection.cursor = textCursor();
		selection.cursor.clearSelection();
		extraSelections.append(selection);
	}

	setExtraSelections(extraSelections);
}

void CodeEditor::paintEvent(QPaintEvent *event) 
{
	QTextEdit::paintEvent(event);
	//lineNumberAreaPaintEvent(event);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
	QPainter painter(lineNumberArea);
	painter.fillRect(lineNumberArea->rect(), Qt::lightGray);

	QPointF framePos(0.f, verticalScrollBar()->value());
	QRectF fRect = frameRect();
	fRect.moveTo(framePos);

	QSizeF pageSize = document()->size();
	QRect vpRect = viewport()->rect();

	const int instructionLine = currentInstructionLine;

	int minLine = -1, maxLine = -1;

	for (QTextBlock it = document()->begin(); it != document()->end(); it = it.next())
	{
		QRectF blockRect = document()->documentLayout()->blockBoundingRect(it);
		if(blockRect.top() < fRect.bottom() && blockRect.bottom() > fRect.top())
		{
			const int currentLineNum = it.blockNumber() + 1;
			if ( minLine == -1 ) {
				minLine = currentLineNum;
			}
			maxLine = currentLineNum;

			// current line?
			if ( instructionLine == currentLineNum ) {
				painter.drawImage(
					imageCurrentLine->rect().width(),
					(int)(blockRect.top()-framePos.y()),
					*imageCurrentLine);
			}

			// breakpoint on this line?
			if ( lineBreakPoints.find( currentLineNum ) != lineBreakPoints.end()) {
				painter.drawImage(
					0,
					(int)(blockRect.top()-framePos.y()),
					*imageBreakPoint);
			}

			QString number = QString::number(it.blockNumber() + 1);
			painter.setPen(Qt::black);
			painter.drawText(
				0, 
				blockRect.top()-framePos.y(), 
				lineNumberArea->width(), 
				fontMetrics().height(),
				Qt::AlignRight,
				number);
		}
	}

	lineNumberArea->SetLinesDisplayed( minLine, maxLine );

	// why is this necessary?
	update();
}

bool CodeEditor::viewportEvent( QEvent * event )
{
	return QTextEdit::viewportEvent( event );
}

void CodeEditor::SetLineSelected( int line )
{
	QTextCursor cursor( document()->findBlockByLineNumber( line-1 ) );
	setTextCursor( cursor );
	//ensureCursorVisible();
}

void CodeEditor::SetSource( const QString & a_file, const QString & a_source )
{
	setPlainText( a_source );
	setDocumentTitle( a_file );
}

void CodeEditor::AddBreakPoint( int lineNum ) {
	lineBreakPoints.insert( lineNum );
}

void CodeEditor::RemoveBreakPoint( int lineNum ) {
	lineBreakPoints.remove( lineNum );
}

void CodeEditor::ClearBreakPoints() {
	lineBreakPoints.clear();
}

//////////////////////////////////////////////////////////////////////////

LineNumberArea::LineNumberArea(CodeEditor *editor) : QWidget(editor) {
	codeEditor = editor;
	minLine = 0;
	maxLine = 0;
}

void LineNumberArea::SetLinesDisplayed( int min, int max ) {
	minLine = min;
	maxLine = max;
}

QSize LineNumberArea::sizeHint() const {
	return QSize(codeEditor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event) {
	QWidget::paintEvent(event);
	codeEditor->lineNumberAreaPaintEvent(event);
}

void LineNumberArea::mouseReleaseEvent ( QMouseEvent * event ) {
	event->accept();

	const float fontHeight = fontMetrics().height();
	const int lineNum = minLine + event->y() / fontHeight;
	if ( lineNum <= maxLine ) {
		emit lineClicked( lineNum );
	}
}
