#include "AudioOutput.hpp"

#include <QCoreApplication>
#include <portaudio.h>

AudioOutput::AudioOutput() :
	outputParameters( new PaStreamParameters() ),
	stream( NULL ),
	sampleRate( 0 )
{
	static bool once;
	if ( !once )
	{
		Pa_Initialize();
		once = true;
	}
	outputParameters->device = Pa_GetDefaultOutputDevice();
#ifndef Q_OS_WIN
	outputParameters->suggestedLatency = 0.1;
#else
	outputParameters->suggestedLatency = 0.3;
#endif
	outputParameters->sampleFormat = paInt16;
}
AudioOutput::~AudioOutput()
{
	delete outputParameters;
}

void AudioOutput::setParams( int chn, int sampleRate )
{
	outputParameters->channelCount = chn;
	this->sampleRate = sampleRate;
}

bool AudioOutput::open()
{
	Pa_OpenStream( &stream, NULL, outputParameters, sampleRate, 0, 0, NULL, NULL );
	return isOpen();
}

void AudioOutput::close( const bool &stop )
{
	if ( stream )
	{
		if ( !stop && Pa_IsStreamActive( stream ) )
		{
			const int samplesToWrite = Pa_GetStreamInfo( stream )->outputLatency * sampleRate;
			short *silence = new short[ samplesToWrite * outputParameters->channelCount ]();
			Pa_WriteStream( stream, silence, samplesToWrite );
			delete[] silence;
		}
		Pa_CloseStream( stream );
		stream = NULL;
	}
}

void AudioOutput::play( const QByteArray &data, const bool &stop )
{
	int chunk = ( int )( sampleRate * 0.02 ) * outputParameters->channelCount * sizeof( short );
	int offset = 0;
	if ( !Pa_IsStreamActive( stream ) )
		Pa_StartStream( stream );
	while ( !stop && data.size() > offset )
	{
		int chunkSize = qMin( chunk, data.size() - offset );
		if ( Pa_WriteStream( stream, data.data() + offset, chunkSize / outputParameters->channelCount / sizeof( short ) ) == paUnanticipatedHostError )
			break;
		offset += chunkSize;
		qApp->processEvents();
	}
}
