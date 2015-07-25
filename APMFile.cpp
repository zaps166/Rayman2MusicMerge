#include "APMFile.hpp"

#define APM_HEADER_SIZE 0x64U

static const unsigned short ima_step_table[ 89 ] =
{
	    7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
	   19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
	   50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
	  130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
	  337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
	  876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
	 2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
	 5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};
static const char ima_index_table[ 8 ] =
{
	-1, -1, -1, -1, 2, 4, 6, 8
};

template < typename T, typename T1, typename T2 > static inline void clip( T &val, T1 min, T2 max )
{
	if ( val > max )
		val = max;
	else if ( val < min )
		val = min;
}

/**/

APMFile::APMFile( const QString &apmPath ) :
		sampleRate( 0 ),
		chn( 0 ),
		duration( 0.0 ),
		mustRead( true )
{
	memset( sample, 0, sizeof sample );
	f.setFileName( apmPath );
	if ( !apmPath.isEmpty() && f.open( QFile::ReadOnly ) )
	{
		f.seek( 2 );
		f.read( ( char * )&chn, sizeof chn );
		f.read( ( char * )&sampleRate, sizeof sampleRate );
		f.seek( 0x14 );
		bool ok = f.read( 4 ) == "vs12";
		f.seek( 0x60 );
		ok &= f.read( 4 ) == "DATA";
		if ( !ok || chn > MAX_APM_CHN || !sampleRate )
			f.close();
		else
		{
			f.seek( 0x1C );
			unsigned l;
			f.read( ( char * )&l, sizeof l );
			duration = l / ( double )sampleRate;

			f.seek( 0x2C );
			if ( chn == 2 )
			{
				f.read( ( char * )&predictor[ 1 ], 4 );
				f.read( ( char * )&step_index[ 1 ], 2 );
				f.seek( f.pos() + 6 );
			}
			f.read( ( char * )&predictor[ 0 ], 4 );
			f.read( ( char * )&step_index[ 0 ], 2 );
			f.seek( APM_HEADER_SIZE );
		}
	}
}

bool APMFile::getSample( Sample &smp )
{
	if ( mustRead )
	{
		unsigned char sampleCode[ MAX_APM_CHN ];
		if ( f.read( ( char * )sampleCode, chn ) == chn )
		{
			for ( unsigned short c = 0 ; c < chn ; ++c )
			{
				sample[ c ][ 0 ].encoded = sampleCode[ c ] >> 4;
				sample[ c ][ 0 ].decoded = decode( sample[ c ][ 0 ].encoded, step_index[ c ], predictor[ c ] );
				sample[ c ][ 0 ].stepIndex = step_index[ c ];
				sample[ c ][ 1 ].encoded = sampleCode[ c ] & 0x0F;
				sample[ c ][ 1 ].decoded = decode( sample[ c ][ 1 ].encoded, step_index[ c ], predictor[ c ] );
				sample[ c ][ 1 ].stepIndex = step_index[ c ];
			}
			currChn = currNibble = 0;
			mustRead = false;
		}
	}
	if ( !mustRead )
	{
		smp = sample[ currChn ][ currNibble ];
		if ( ++currChn >= chn )
		{
			if ( ++currNibble == 2 )
				mustRead = true;
			else
				currChn = 0;
		}
		return true;
	}
	return false;
}
QByteArray APMFile::getSamples()
{
	QByteArray samples;
	samples.reserve( sizeof( short ) * ( f.size() - APM_HEADER_SIZE ) * 2 );
	Sample sample;
	for ( ;; )
	{
		if ( getSample( sample ) )
			samples.append( ( char * )&sample.decoded, sizeof sample.decoded );
		else
			break;
	}
	return samples;
}

unsigned char APMFile::encode( int &samplePrev, short sample, short &stepindex )
{
	int diff = sample - samplePrev;
	int sign = ( diff < 0 ) ? 8 : 0;
	if ( sign )
		diff = -diff;

	int step = ima_step_table[ stepindex ];

	int delta = 0;
	int prevDiff = step >> 3;

	if ( diff >= step )
	{
		delta |= 4;
		diff -= step;
		prevDiff += step;
	}
	step >>= 1;
	if ( diff >= step  )
	{
		delta |= 2;
		diff -= step;
		prevDiff += step;
	}
	step >>= 1;
	if ( diff >= step )
	{
		delta |= 1;
		prevDiff += step;
	}

	if ( sign )
	  samplePrev -= prevDiff;
	else
	  samplePrev += prevDiff;

	clip( samplePrev, -32768, 32767 );

	stepindex += ima_index_table[ delta ];
	clip( stepindex, 0, 88 );

	return ( delta | sign ) & 0x0F;
}
short APMFile::decode( unsigned char nibble, short &stepindex, int &predictor )
{
	int step = ima_step_table[ stepindex ];
	int diff = step >> 3;

	if ( nibble & 1 )
		diff += step >> 2;
	if ( nibble & 2 )
		diff += step >> 1;
	if ( nibble & 4 )
		diff += step;
	if ( nibble & 8 )
		diff = -diff;

	predictor += diff;
	clip( predictor, -32768, 32767 );

	stepindex += ima_index_table[ nibble & 0x07 ];
	clip( stepindex, 0, 88 );

	return predictor;
}
