#include "ListWidget.hpp"

#include <QDropEvent>

ListWidget::ListWidget( QWidget *parent ) :
	QListWidget( parent ),
	modified( false )
{
	setDragDropMode( QAbstractItemView::InternalMove );
}

void ListWidget::dragEnterEvent( QDragEnterEvent *event )
{
	if ( event->source() && event->source() != this )
		setDragDropMode( QAbstractItemView::DragDrop );
	QListWidget::dragEnterEvent( event );
}
void ListWidget::dragLeaveEvent( QDragLeaveEvent *event )
{
	QListWidget::dragLeaveEvent( event );
	setDragDropMode( QAbstractItemView::InternalMove );
}
void ListWidget::dropEvent( QDropEvent *event )
{
	QListWidget::dropEvent( event );
	if ( event->source() != this )
		emit itemsDropped();
	setDragDropMode( QAbstractItemView::InternalMove );
	modified = true;
}
