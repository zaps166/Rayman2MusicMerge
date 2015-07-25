#include <QApplication>
#if QT_VERSION < 0x050000
	#include <QTextCodec>
#endif

#include "MainWindow.hpp"

int main( int argc, char *argv[] )
{
#if QT_VERSION < 0x050000
	QTextCodec::setCodecForCStrings( QTextCodec::codecForName( "UTF-8" ) );
#endif
	QApplication app( argc, argv );
	MainWindow w;
	return app.exec();
}
