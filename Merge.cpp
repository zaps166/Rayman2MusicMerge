#include "Merge.hpp"
#include "APMFile.hpp"

#include <QLinkedList>
#include <QFile>

template <typename T> static inline void writeNumber( T n, QFile &f )
{
	f.write( ( char * )&n, sizeof n );
}

#define WAVWriteAndCheck( data, size )                                                    \
	if ( wav.write( ( char * )data, size ) != ( int )size )                               \
	{                                                                                     \
		wav.remove();                                                                     \
		return appendAndExec( "<B>Error writing file:</B> " + wavPath, false );           \
	}
#define WAVWriteBlock( data, size )                                                                                           \
	for ( int j = 0 ; j < 8 ; j += 2 )                                                                                        \
		for ( unsigned short c = 0 ; c < chn ; ++c )                                                                          \
			data[ c ][ j / 2 ] = samplesBlock[ c ][ j + 0 ].encoded | ( samplesBlock[ c ][ j + 1 ].encoded << 4 );            \
	WAVWriteAndCheck( data, size );                                                                                           \
	sampleIndex = 0;

/**/

Merge::Merge( QWidget *parent ) :
	QDialog( parent )
{
	ui.setupUi( this );
	open();
}

bool Merge::process( const QString &wavPath, const QStringList &apmPaths, const unsigned short chn, const unsigned sampleRate )
{
	qApp->setOverrideCursor( Qt::BusyCursor );
	stop = false;
	QFile wav( wavPath );
	if ( wav.open( QFile::WriteOnly | QFile::Truncate ) )
	{
		struct WAV_ADPCM_Header
		{
			short iSamp0;
			unsigned char stapTableIdx, reserved;
		};
		static const unsigned short BitsPerSample = 4;

		wav.seek( 60 ); //WAV Header

		const unsigned short ChnBlock = chn * 4;
		const unsigned short blockAlign = 256 * ChnBlock;
		const unsigned short samplesPerBlock = ( ( blockAlign - ChnBlock ) * 8 ) / ( BitsPerSample * chn ) + 1;
		const unsigned short BytesPerBlock = blockAlign - ChnBlock;
		unsigned averageBytesPerSecond = chn * sampleRate / 2;
		averageBytesPerSecond += averageBytesPerSecond / ( blockAlign / sizeof( WAV_ADPCM_Header ) / chn );

		unsigned char wavSamplesBlock[ MAX_APM_CHN ][ 4 ];
		Sample samplesBlock[ MAX_APM_CHN ][ 8 ];
		unsigned bytesWritten = BytesPerBlock;
		short stepIndex[ MAX_APM_CHN ];
		int samplePrev[ MAX_APM_CHN ];
		unsigned samplesCount = 0;
		unsigned sampleIndex = 0;
		for ( int i = 0 ; i < apmPaths.count() ; ++i )
		{
			APMFile apm( apmPaths[ i ] );
			if ( apm.isOpen() && apm.Channels() == chn && apm.SampleRate() == sampleRate )
			{
				bool reEncode = i;
				bool ok = true;
				for ( ;; )
				{
					if ( bytesWritten == BytesPerBlock )
					{
						WAV_ADPCM_Header header[ MAX_APM_CHN ];
						for ( int c = 0 ; c < chn ; ++c )
						{
							Sample sample;
							if ( !apm.getSample( sample ) )
								break;
							header[ c ].iSamp0 = sample.decoded;
							header[ c ].stapTableIdx = sample.stepIndex;
							header[ c ].reserved = 0;
						}
						WAVWriteAndCheck( header, sizeof( WAV_ADPCM_Header ) * chn );
						samplesCount += chn;
						bytesWritten = 0;
						reEncode = false;
					}
					for ( ; sampleIndex < 8 ; ++sampleIndex )
					{
						for ( unsigned short c = 0 ; c < chn ; ++c )
						{
							Sample &sample = samplesBlock[ c ][ sampleIndex ];
							if ( ( ok = apm.getSample( sample ) ) )
							{
								if ( reEncode )
									sample.encoded = APMFile::encode( samplePrev[ c ], sample.decoded, stepIndex[ c ] );
								else
								{
									stepIndex[ c ] = sample.stepIndex;
									samplePrev[ c ] = sample.decoded;
								}
								++samplesCount;
							}
							else break;
						}
						if ( !ok )
							break;
					}
					if ( samplesCount > 0x7FFFFFFF )
					{
						wav.remove();
						return appendAndExec( "<B>Too many samples, select fewer files</B>", false );
					}
					if ( ok )
					{
						WAVWriteBlock( wavSamplesBlock, ChnBlock );
						bytesWritten += ChnBlock;
					}
					else break;
				}
				ui.infoE->append( "<B>Attached:</B> " + apmPaths[ i ] );
			}
			else
				ui.infoE->append( "<B>Ignored:</B> " + apmPaths[ i ] );
			qApp->processEvents();
			if ( stop )
			{
				stop = false;
				wav.remove();
				return appendAndExec( "Aborted", false );
			}
		}

		//Saving residues and silence at the end of block
		const unsigned moreSamples = ( samplesPerBlock * chn ) - samplesCount % ( samplesPerBlock * chn );
		for ( unsigned k = 0 ; k < moreSamples / 2 ; k += ChnBlock )
		{
			for ( ; sampleIndex < 8 ; ++sampleIndex )
				for ( unsigned short c = 0 ; c < chn ; ++c )
					samplesBlock[ c ][ sampleIndex ].encoded = APMFile::encode( samplePrev[ c ], 0, stepIndex[ c ] );
			if ( sampleIndex == 8 )
				WAVWriteBlock( wavSamplesBlock, ChnBlock );
		}

		//WAV header
		wav.seek( 0 );
		wav.write( "RIFF" );
		writeNumber< unsigned >( wav.size() - 8, wav ); //Data size + header (8 bytes)
		wav.write( "WAVEfmt " );
		writeNumber( 20, wav ); //"fmt " Chunk size, always 20 bytes for IMA ADPCM
		writeNumber< unsigned short >( 0x11, wav ); //IMA-ADPCM
		writeNumber( chn, wav );
		writeNumber( sampleRate, wav );
		writeNumber( averageBytesPerSecond, wav );
		writeNumber( blockAlign, wav );
		writeNumber( BitsPerSample, wav );
		writeNumber< unsigned short >( 2, wav ); //Extra size
		writeNumber( samplesPerBlock, wav );
		wav.write( "fact" );
		writeNumber( 4, wav );
		writeNumber( samplesCount / chn, wav ); //Time length in samples
		wav.write( "data" );
		writeNumber< unsigned >( wav.size() - 60, wav ); //Will be set at the end
		wav.close();

		return appendAndExec( "<B>Export is successfully completed</B>", true );
	}
	return appendAndExec( "<B>Can't open:</B> " + wavPath, false );
}

bool Merge::appendAndExec( const QString &txt, const bool ret )
{
	qApp->restoreOverrideCursor();
	ui.buttonBox->setStandardButtons( QDialogButtonBox::Close );
	ui.infoE->append( txt );
	exec();
	return ret;
}
void Merge::reject()
{
	if ( ui.buttonBox->standardButtons() == QDialogButtonBox::Close )
		QDialog::reject();
	else
		stop = true;
}
