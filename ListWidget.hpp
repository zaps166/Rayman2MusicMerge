#ifndef LISTWIDGET_HPP
#define LISTWIDGET_HPP

#include <QListWidget>

class ListWidget : public QListWidget
{
	Q_OBJECT
public:
	ListWidget( QWidget *parent = NULL );

	inline bool isModified() const
	{
		return modified;
	}

	inline void setNotModified()
	{
		modified = false;
	}
signals:
	void itemsDropped();
private:
	void dragEnterEvent( QDragEnterEvent *event );
	void dragLeaveEvent( QDragLeaveEvent *event );
	void dropEvent( QDropEvent *event );

	bool modified;
};

#endif // LISTWIDGET_HPP
