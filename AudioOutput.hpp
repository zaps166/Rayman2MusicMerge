#ifndef AUDIOOUTPUT_HPP
#define AUDIOOUTPUT_HPP

#include <QByteArray>

struct PaStreamParameters;
typedef void PaStream;

class AudioOutput
{
public:
	AudioOutput();
	~AudioOutput();

	inline bool isOpen()
	{
		return stream;
	}

	void setParams( int chn, int sampleRate );

	bool open();
	void close( const bool &stop );

	void play( const QByteArray &data, const bool &stop );
private:
	PaStreamParameters *outputParameters;
	PaStream *stream;
	int sampleRate;
};

#endif // AUDIOOUTPUT_HPP
