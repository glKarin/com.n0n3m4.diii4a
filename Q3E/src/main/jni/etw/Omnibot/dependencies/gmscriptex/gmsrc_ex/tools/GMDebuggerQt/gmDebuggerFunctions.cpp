#include "gmdebuggerqt.h"

void GMDebuggerQt::gmDebuggerBreak(int a_threadId, int a_sourceId, int a_lineNumber)
{
	gmMachineGetContext(a_threadId, 0);
}

void GMDebuggerQt::gmDebuggerRun(int a_threadId, int a_lineNum, const char *a_func, const char *a_file)
{
	if(!a_file || !a_file[0])
		a_file = "<unknownfile>";
	if(!a_func || !a_func[0])
		a_func = "<unknownfunc>";

	AddUniqueThread( a_threadId, "Running", a_lineNum, a_func, a_file );
}

void GMDebuggerQt::gmDebuggerStop(int a_threadId)
{
	RemoveThread(a_threadId);
}

void GMDebuggerQt::gmDebuggerSource(int a_sourceId, const char * a_sourceName, const char * a_source)
{
	SourceInfo info;
	info.sourceFile = a_sourceName;
	info.sourceData = a_source;
	sourceMap.insert(a_sourceId,info);

	if ( currentSourceId == a_sourceId ) {
		ui.scriptEdit->SetSource( a_sourceName, a_source );
		if ( currentLineOnSrcRecieved != -1 ) {
			ui.scriptEdit->SetLineSelected( currentLineOnSrcRecieved );
			ui.scriptEdit->SetInstructionLine( currentLineOnSrcRecieved );
		}
	}
}

void GMDebuggerQt::gmDebuggerException(int a_threadId)
{
	gmMachineGetContext(a_threadId, 0);
}

void GMDebuggerQt::gmDebuggerBeginContext(int a_threadId, int a_callFrame)
{
	while(ui.callStack->rowCount())
		ui.callStack->removeRow(0);
	while(ui.context->rowCount() > 1) {
		ui.context->removeRow(ui.context->rowCount()-1);
	}

	currentThreadId = a_threadId;
	currentCallFrame = a_callFrame;
	ui.scriptEdit->ClearBreakPoints();
}

void GMDebuggerQt::gmDebuggerContextCallFrame(int a_callFrame, const char * a_functionName, int a_sourceId, int a_lineNumber, const char * a_thisSymbol, const char * a_thisValue, const char * a_thisType, int a_thisId)
{
	// add the var to the callstack
	{
		QString str;
		str.sprintf( "%s (%d)", a_functionName, a_lineNumber );
		const int newRow = ui.callStack->rowCount();
		ui.callStack->insertRow( newRow );
		ui.callStack->setItem( newRow, Callstack_FuncName, new QTableWidgetItem(str) );
	}

	currentSourceId = a_sourceId;

	// Update the call stack.
	if(currentCallFrame == a_callFrame)
	{
		{
			const int newRow = ui.context->rowCount();
			ui.context->insertRow( newRow );

			QTableWidgetItem * itemName = new QTableWidgetItem( a_thisSymbol );
			QTableWidgetItem * itemType = new QTableWidgetItem( a_thisType );
			QTableWidgetItem * itemValue = new QTableWidgetItem( a_thisValue );

			ui.context->setItem( newRow, Context_Name, itemName );
			ui.context->setItem( newRow, Context_Type, itemType );
			ui.context->setItem( newRow, Context_Value, itemValue );
		}

		// do we have the source code?
		currentLineOnSrcRecieved = -1;
		QMap<int,SourceInfo>::iterator it = sourceMap.find( a_sourceId );
		if ( it == sourceMap.end() ) {
			gmMachineGetSource( a_sourceId );
			currentLineOnSrcRecieved = a_lineNumber;
		} else {
			ui.scriptEdit->SetSource( it.value().sourceFile, it.value().sourceData );
			ui.scriptEdit->SetLineSelected( a_lineNumber );
			ui.scriptEdit->SetInstructionLine( a_lineNumber );
		}
	}
}

void GMDebuggerQt::gmDebuggerContextVariable(const char * a_varSymbol, const char * a_varValue, const char * a_varType, int a_varId)
{
	const int newRow = ui.context->rowCount();
	ui.context->insertRow( newRow );
	ui.context->setItem( newRow, Context_Name, new QTableWidgetItem( a_varSymbol ) );
	ui.context->setItem( newRow, Context_Type, new QTableWidgetItem( a_varType ) );
	ui.context->setItem( newRow, Context_Value, new QTableWidgetItem( a_varValue ) );
}

void GMDebuggerQt::gmDebuggerContextBreakpoint(int a_lineNum)
{
	ui.scriptEdit->AddBreakPoint( a_lineNum );
}

void GMDebuggerQt::gmDebuggerEndContext()
{
}

void GMDebuggerQt::gmDebuggerBeginThreadInfo()
{
	/*while(ui.threadTable->rowCount())
		ui.threadTable->removeRow(0);
	threadRowMap.clear();*/
}

void GMDebuggerQt::gmDebuggerThreadInfo(int a_threadId, const char * a_threadState, int a_lineNum, const char * a_func, const char * a_file)
{
	AddUniqueThread( a_threadId, a_threadState, a_lineNum, a_func, a_file );
}

void GMDebuggerQt::gmDebuggerEndThreadInfo()
{
	//ui.threadTable->sortItems(0);
	for( int i = 0; i < ThreadColumn_Num; ++i ) {
		ui.threadTable->horizontalHeader()->resizeSection( i, threadColumnWidths[i] );
	}
}

void GMDebuggerQt::gmDebuggerError(const char * a_error)
{
	ui.outputWindow->append( a_error );
}

void GMDebuggerQt::gmDebuggerMessage(const char * a_message)
{
	ui.outputWindow->append( a_message );
}

void GMDebuggerQt::gmDebuggerBreakPointSet(int a_sourceId, int a_lineNum, int a_enabled) {
	if ( a_enabled ) {
		ui.scriptEdit->AddBreakPoint( a_lineNum );
	} else {
		ui.scriptEdit->RemoveBreakPoint( a_lineNum );
	}
}

void GMDebuggerQt::gmDebuggerBreakClear() {
	ui.scriptEdit->ClearBreakPoints();
}

void GMDebuggerQt::gmDebuggerQuit()
{
	statusBar()->showMessage("Debug Session Closed...", 5000);
	//GMDebuggerNet::Disconnect();
}

//////////////////////////////////////////////////////////////////////////


void GMDebuggerQt::gmDebuggerBeginGlobals(int a_varId)
{
	if ( a_varId ) {
		VarItemMap::iterator it = varIdTreeMap.find( a_varId );
		if( it != varIdTreeMap.end() ) {
			parentCurrent = *it;
		}
	} else {
		parentCurrent = parentGlobals;
	}
}

void GMDebuggerQt::gmDebuggerGlobal(const char * a_varSymbol, 
									const char * a_varValue,
									const char * a_varType,
									int a_varId)
{
	if ( !parentCurrent ) {
		return;
	}

	// find parent to put under
	QTreeWidgetItem *newItem = new QTreeWidgetItem( parentCurrent );
	newItem->setText(TreeColumn_Name, a_varSymbol);
	newItem->setText(TreeColumn_Type, a_varType);
	newItem->setText(TreeColumn_Value, a_varValue);
	newItem->setData(0,Qt::UserRole,QVariant(a_varId));

	for( int i = 0; i < TreeColumn_Num; ++i ) {
		globalColumnWidths[i] =
			qMax( globalColumnWidths[i], 
					fontMetrics().width(QLatin1Char('9')) * (newItem->text(i).size() + 4) );
	}	
	
	varIdTreeMap.insert( a_varId, newItem );

	if(a_varId != 0)
		gmMachineGetGlobalsInfo(a_varId);
}

void GMDebuggerQt::gmDebuggerEndGlobals()
{
	for( int i = 0; i < TreeColumn_Num; ++i ) {
		ui.globalsTable->header()->resizeSection( i, globalColumnWidths[i] );
	}
	//ui.globalsTable->update();
	//ui.globalsTable->header()->resizeSections( QHeaderView::ResizeToContents );
}

void GMDebuggerQt::gmDebuggerReturnValue(const char * a_retVal, const char * a_retType, int a_retVarId)
{
	ui.context->item( 0, Context_Name )->setText( "returned" );
	ui.context->item( 0, Context_Type )->setText( a_retType );
	ui.context->item( 0, Context_Value )->setText( a_retVal );
}

int GMDebuggerQt::AddUniqueThread( int a_threadId, const char * a_status, int a_line, const char * a_func, const char * a_file )
{
	int threadRow = -1;

	QList<QTableWidgetItem *> items = 
		ui.threadTable->findItems( QString::number(a_threadId),Qt::MatchExactly );

	if( !items.isEmpty() ) {
		threadRow = items[0]->row();
	} else {
		threadRow = ui.threadTable->rowCount();
		ui.threadTable->insertRow( threadRow );	
	}

	QString func;
	func.sprintf( "%s(%d)",a_func,a_line );	

	if(ui.threadTable->item(threadRow,Thread_Id)) {
		ui.threadTable->item(threadRow,Thread_Id)->setText(QString::number(a_threadId));
	} else {
		ui.threadTable->setItem(threadRow,Thread_Id,new QTableWidgetItem(QString::number(a_threadId)));
	}
	if(ui.threadTable->item(threadRow,Thread_Status)) {
		ui.threadTable->item(threadRow,Thread_Status)->setText(a_status);
	} else {
		ui.threadTable->setItem(threadRow,Thread_Status,new QTableWidgetItem(a_status));
	}
	if(ui.threadTable->item(threadRow,Thread_Function)) {
		ui.threadTable->item(threadRow,Thread_Function)->setText(func);
	} else {
		ui.threadTable->setItem(threadRow,Thread_Function,new QTableWidgetItem(func));
	}

	if(ui.threadTable->item(threadRow,Thread_Script)) {
		ui.threadTable->item(threadRow,Thread_Script)->setText(a_file);
	} else {
		ui.threadTable->setItem(threadRow,Thread_Script,new QTableWidgetItem(a_file));
	}
	
	const int fontWidth = fontMetrics().width(QLatin1Char('9'));
	for(int i = 0; i < ThreadColumn_Num; ++i) {
		threadColumnWidths[i] = 
			qMax( threadColumnWidths[i], fontWidth * (ui.threadTable->item(threadRow,i)->text().size() + 4) );
	}
	return threadRow;
}

void GMDebuggerQt::RemoveThread( int a_threadId )
{
	QList<QTableWidgetItem *> items = ui.threadTable->findItems(QString::number(a_threadId),Qt::MatchExactly);
	if ( !items.empty() ) {
		ui.threadTable->removeRow( items[0]->row() );
	}
}

void GMDebuggerQt::RemoveExpiredThreads()
{
}

void GMDebuggerQt::ThreadSelectionChanged()
{
	QList<QTableWidgetItem *> selected = ui.threadTable->QTableWidget::selectedItems();
	for ( int i = 0; i < selected.size(); ++i ) {
		QTableWidgetItem *threadId = ui.threadTable->item(selected[i]->row(),Thread_Id);
		if ( threadId ) {
			currentThreadId = threadId->text().toInt();
			gmMachineGetContext(currentThreadId, 0);
		}
		break;
	}
}

void GMDebuggerQt::OnBreakPointChanged( int lineNum, bool enabled )
{
	gmMachineSetBreakPoint( currentSourceId, lineNum, currentThreadId, enabled );
}
