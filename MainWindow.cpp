#include "MainWindow.hpp"
#include "APMFile.hpp"
#include "Merge.hpp"

#include <QCloseEvent>

static inline QString getWavName( QString fPth )
{
	fPth = fPth.right( fPth.length() - fPth.lastIndexOf( '/' ) - 1 );
	int digitPos = -1;
	for ( int i = 0 ; i < fPth.length() ; ++i )
	{
		if ( fPth[ i ] == '.' )
			break;
		if ( fPth[ i ].isDigit() )
		{
			digitPos = i;
			break;
		}
	}
	if ( digitPos > -1 )
		fPth.remove( digitPos, fPth.length() - digitPos );
	else
		fPth.chop( 4 );
	return fPth;
}
static inline QString getPath( const QString &fPth )
{
	return fPth.left( fPth.lastIndexOf( '/' ) + 1 );
}
static inline QString getFileName( const QString &fName )
{
	int idx = fName.lastIndexOf( '.' );
	if ( idx > -1 )
		return fName.mid( 0, idx );
	return fName;
}

/**/

#define RoleFileName Qt::UserRole + 0
#define RoleChnCount Qt::UserRole + 1
#define RoleSmplRate Qt::UserRole + 2
#define RoleDuration Qt::UserRole + 3

static const char ProjectFile[] = "Rayman2Music Project (*.ray2mus_project)";
static const QString RAY2MUSPRO( "RAY2MUSPRO" );

#include <QMessageBox>
#include <QFileDialog>

#ifdef Q_OS_WIN
	#include <windows.h>
#endif

MainWindow::MainWindow() :
	exportPath( QDir::homePath() + "/" )
{
	ui.setupUi( this );
	calcTime();
	show();

#ifdef Q_OS_WIN
	QString ubisoft_settings = getenv( "WINDIR" );
	if ( !ubisoft_settings.isNull() )
	{
		char Rayman2Path[ MAX_PATH ] = { 0 };
		ubisoft_settings += "\\UbiSoft\\ubi.ini";
		GetPrivateProfileStringA( "rayman2", "Directory", NULL, Rayman2Path, sizeof Rayman2Path, ubisoft_settings.toLocal8Bit().data() );
		if ( QFileInfo( Rayman2Path ).exists() )
		{
			apmPath = Rayman2Path;
			apmPath.replace( '\\', '/' );
			if ( apmPath.right( 1 ) != "/" )
				apmPath += "/";
			apmPath += "Data/World/Sound/";
		}
	}
#endif
	if ( !QFileInfo( apmPath ).exists() )
	{
		apmPath = QDir::homePath() + "/";
		ui.action_Choose_a_music_directory->trigger();
	}
	else
		loadAPMFiles();

	filterList();
}

void MainWindow::on_action_Load_project_triggered()
{
	if ( !ui.apmListW->count() )
	{
		QMessageBox::information( this, "Loading project", "Choose a music directory" );
		return;
	}
	QFile f( QFileDialog::getOpenFileName( this, "Loading project", exportPath, ProjectFile ) );
	if ( !f.fileName().isEmpty() && f.open( QFile::ReadOnly ) )
	{
		QDataStream stream( &f );
		QString header;
		stream >> header;
		if ( header == "RAY2MUSPRO" )
		{
			exportPath = getPath( f.fileName() );
			ui.playListW->clear();
			while ( !stream.atEnd() )
			{
				QListWidgetItem *lW = new QListWidgetItem( ui.playListW );
				lW->read( stream );
			}
			ui.playListW->setCurrentRow( 0 );
			ui.playListW->setFocus();
		}
		else
			QMessageBox::information( this, "Loading project", "Invalid project file" );
	}
}
void MainWindow::on_action_Choose_a_music_directory_triggered()
{
	QString dirStr = QFileDialog::getExistingDirectory( this, "Choose a directory with Rayman 2 music files (file extension *.apm)", apmPath );
	if ( !dirStr.isEmpty() )
	{
		QDir dir( dirStr );
		apmPath = QDir::cleanPath( dir.absolutePath() );
		if ( apmPath != "/" )
			apmPath += "/";
		loadAPMFiles();
	}
}
void MainWindow::on_action_Save_project_triggered()
{
	if ( !ui.playListW->count() )
	{
		QMessageBox::information( this, "Saving project", "Playlist is empty, there is nothing to save" );
		return;
	}
	QFile f( QFileDialog::getSaveFileName( this, "Saving project", exportPath + getWavName( ui.playListW->item( 0 )->data( RoleFileName ).toString() ), ProjectFile ) );
	if ( !f.fileName().isEmpty() && f.open( QFile::WriteOnly ) )
	{
		QDataStream stream( &f );
		stream << RAY2MUSPRO;
		foreach ( QListWidgetItem *lW, ui.playListW->findItems( QString(), Qt::MatchContains ) )
			lW->write( stream );
		exportPath = getPath( f.fileName() );
		ui.playListW->setNotModified();
	}
}
void MainWindow::on_action_Export_to_WAV_triggered()
{
	if ( ui.playListW->count() == 0 )
	{
		QMessageBox::information( this, "Export to WAV", "Playlist is empty, there is nothing to export" );
		return;
	}
	QStringList apmFiles;
	foreach ( QListWidgetItem *lWI, ui.playListW->findItems( QString(), Qt::MatchContains ) )
		apmFiles += apmPath + lWI->data( RoleFileName ).toString();
	QString wavFile = QFileDialog::getSaveFileName( this, "Export to WAV", exportPath + getWavName( apmFiles[ 0 ] ), "IMA ADPCM WAV (*.wav)" );
	if ( !wavFile.isEmpty() )
	{
		exportPath = getPath( wavFile );
		Merge merge( this );
		if ( merge.process( wavFile, apmFiles, ui.chnB->value(), ui.srateB->value() ) )
			ui.playListW->setNotModified();
	}
}
void MainWindow::on_action_Remove_selected_entries_triggered()
{
	if ( ui.playListW->hasFocus() )
	{
		QList< QListWidgetItem * > selected = ui.playListW->selectedItems();
		int currentRow = ui.playListW->currentRow();
		foreach ( QListWidgetItem *lWI, selected )
			delete lWI;
		if ( !selected.isEmpty() )
		{
			ui.playListW->setCurrentRow( currentRow );
			if ( ui.playListW->currentRow() == -1 && ui.playListW->count() )
				ui.playListW->setCurrentRow( ui.playListW->count() - 1 );
			calcTime();
		}
	}
}
void MainWindow::on_action_Clear_playlist_triggered()
{
	if ( !ui.playListW->count() )
		QMessageBox::information( this, "Cleaning playlist", "Playlist is empty" );
	else if ( QMessageBox::question( this, "Cleaning playlist", "Are you sure you want to clear the playlist?", QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes )
	{
		ui.playListW->clear();
		calcTime();
	}
}
void MainWindow::on_action_Play_selected_file_triggered()
{
	if ( ui.apmListW->hasFocus() )
		play( ui.apmListW->currentItem() );
	else if ( ui.playListW->hasFocus() )
		play( ui.playListW->currentItem() );
}
void MainWindow::on_action_Stop_playing_triggered()
{
	stop = true;
}
void MainWindow::on_action_About_Programmer_triggered()
{
	QMessageBox::information( this, "About", "<B>Programmer:</B> Błażej Szczygieł<BR/><B>Web page:</B> <A href='http://zaps166.sourceforge.net/'>http://zaps166.sourceforge.net/</A>" );
}
void MainWindow::on_action_About_Qt_triggered()
{
	qApp->aboutQt();
}

void MainWindow::playTheList()
{
	stop = false;
	if ( ui.playListW->count() == 0 )
	{
		QMessageBox::information( this, "Playing the playlist", "Playlist is empty, there is nothing to play" );
		return;
	}
	if ( audio.open() )
	{
		bool focus = ui.playListW->hasFocus();
		int i = ui.playListW->currentRow();
		if ( i < 0 || sender() == ui.action_Play_from_the_beginning )
			i = 0;
		for ( ; !stop && i < ui.playListW->count() ; ++i )
		{
			ui.playListW->setCurrentRow( i );
			play( ui.playListW->currentItem() );
		}
		if ( focus )
			ui.playListW->setFocus();
		audio.close( stop );
	}
	else
		QMessageBox::warning( this, "Playing the playlist", "Błąd odtwarzania dźwięku" );
}
void MainWindow::play( QListWidgetItem *item )
{
	stop = false;
	if ( !item )
		QMessageBox::information( this, "Playing the file", "No file is selected" );
	else
	{
		APMFile apm( apmPath + item->data( RoleFileName ).toString() );
		if ( apm.isOpen() )
		{
			QWidget *focusW = focusWidget();
			setItemsEnabled( false );
			bool openAudio = !audio.isOpen();
			if ( openAudio )
				audio.open();
			if ( audio.isOpen() )
			{
				audio.play( apm.getSamples(), stop );
				if ( openAudio )
					audio.close( stop );
			}
			else
				QMessageBox::warning( this, "Playing the file", "Sound playback error" );
			setItemsEnabled( true );
			if ( focusW )
				focusW->setFocus();
		}
	}
}
void MainWindow::filterList()
{
	int chn = ui.chnB->value();
	int srate = ui.srateB->value();
	audio.setParams( chn, srate );
	foreach ( QListWidgetItem *lWI, ui.apmListW->findItems( QString(), Qt::MatchContains ) )
		lWI->setHidden( lWI->data( RoleChnCount ).toInt() != chn || lWI->data( RoleSmplRate ).toInt() != srate );
}
void MainWindow::calcTime()
{
	double total = 0.0;
	foreach ( QListWidgetItem *lWI, ui.playListW->findItems( QString(), Qt::MatchContains ) )
		total += lWI->data( RoleDuration ).toDouble();
	ui.playListTimeL->setText( "Duration: " + QString::number( total, 'f', 1 ) + " sec" );
	if ( total == 0.0 )
		ui.playListW->setNotModified();
}

void MainWindow::closeEvent( QCloseEvent *e )
{
	stop = true;
	if ( ui.playListW->isModified() )
	{
		switch ( QMessageBox::question( this, "Closing the program", "Do you want to save the project?", QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel ) )
		{
			case QMessageBox::Yes:
				ui.action_Save_project->trigger();
				break;
			case QMessageBox::Cancel:
				e->ignore();
				return;
		}
	}
	QMainWindow::closeEvent( e );
}

void MainWindow::loadAPMFiles()
{
	ui.apmListW->clear();
	qApp->setOverrideCursor( Qt::WaitCursor );
	repaint();
	foreach ( QString fileName, QDir( apmPath ).entryList( QStringList(), QDir::Files, QDir::Name | QDir::IgnoreCase ) )
	{
		APMFile apm( apmPath + fileName );
		if ( apm.isOpen() )
		{
			QListWidgetItem *lWI = new QListWidgetItem( ui.apmListW );
			lWI->setToolTip( "Duration: " + QString::number( apm.Duration(), 'f', 1 ) + " sec" );
			lWI->setText( getFileName( fileName ) );
			lWI->setData( RoleSmplRate, apm.SampleRate() );
			lWI->setData( RoleChnCount, apm.Channels() );
			lWI->setData( RoleDuration, apm.Duration() );
			lWI->setData( RoleFileName, fileName );
		}
	}
	ui.apmListW->setCurrentRow( 0 );
	qApp->restoreOverrideCursor();
}
void MainWindow::setItemsEnabled( bool e )
{
	ui.action_Export_to_WAV->setEnabled( e );
	ui.action_Clear_playlist->setEnabled( e );
	ui.action_Save_project->setEnabled( e );
	ui.action_Play_from_currect_position->setEnabled( e );
	ui.action_Load_project->setEnabled( e );
	ui.action_Choose_a_music_directory->setEnabled( e );
	ui.action_Play_selected_file->setEnabled( e );
	ui.action_Remove_selected_entries->setEnabled( e );
	ui.action_Play_from_the_beginning->setEnabled( e );
	ui.playListW->setEnabled( e );
	ui.apmListW->setEnabled( e );
	ui.srateB->setEnabled( e );
	ui.chnB->setEnabled( e );
}
