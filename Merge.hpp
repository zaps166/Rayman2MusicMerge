#ifndef MERGE_HPP
#define MERGE_HPP

#include "ui_Merge.h"

#include "APMFile.hpp"

class Merge : public QDialog
{
	Q_OBJECT
public:
	Merge( QWidget *parent );

	bool process( const QString &wavPath, const QStringList &apmPaths, const unsigned short chn, const unsigned sampleRate );
private:
	bool appendAndExec( const QString &txt, const bool ret );
	void reject();

	Ui::Merge ui;
	bool stop;
};

#endif // MERGE_HPP
