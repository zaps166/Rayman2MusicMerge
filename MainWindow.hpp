#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "ui_MainWindow.h"

#include "AudioOutput.hpp"

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow();
private slots:
	void on_action_Load_project_triggered();
	void on_action_Choose_a_music_directory_triggered();
	void on_action_Save_project_triggered();
	void on_action_Export_to_WAV_triggered();
	void on_action_Remove_selected_entries_triggered();
	void on_action_Clear_playlist_triggered();
	void on_action_Play_selected_file_triggered();
	void on_action_Stop_playing_triggered();
	void on_action_About_Programmer_triggered();
	void on_action_About_Qt_triggered();

	void playTheList();
	void play( QListWidgetItem *item );
	void filterList();
	void calcTime();
private:
	void closeEvent( QCloseEvent * );

	void loadAPMFiles();
	void setItemsEnabled( bool );

	Ui::MainWindow ui;

	AudioOutput audio;

	bool stop;
	QString apmPath, exportPath;
};

#endif // MAINWINDOW_HPP
