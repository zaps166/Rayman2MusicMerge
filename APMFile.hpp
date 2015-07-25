#ifndef APMFILE_HPP
#define APMFILE_HPP

struct Sample
{
	unsigned char encoded; //4bit
	unsigned char stepIndex;
	short decoded;
};

#define MAX_APM_CHN 2U

#include <QFile>

class APMFile
{
public:
	APMFile( const QString &apmPath );

	inline bool isOpen() const
	{
		return f.isOpen();
	}

	inline unsigned short Channels() const
	{
		return chn;
	}
	inline unsigned SampleRate() const
	{
		return sampleRate;
	}

	inline double Duration() const
	{
		return duration;
	}

	bool getSample( Sample &smp );
	QByteArray getSamples();

	static unsigned char encode( int &samplePrev, short sample, short &stepindex );
	static short decode( unsigned char nibble, short &stepindex, int &predictor );
private:
	QFile f;

	unsigned sampleRate;
	unsigned short chn;

	double duration;

	int   predictor [ MAX_APM_CHN ];
	short step_index[ MAX_APM_CHN ];

	Sample sample[ MAX_APM_CHN ][ 2 ];
	unsigned currChn, currNibble;
	bool mustRead;
};

#endif // APMFILE_HPP
