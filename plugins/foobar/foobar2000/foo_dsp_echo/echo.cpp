#include "echo.h"

#include <foobar2000.h>


Echo::Echo()
{
	history = NULL;
	rate = 44100;
	SetDelay(200);		// default delay is 200ms
	SetAmp(128);		// default amplification is 50%
	pos = 0;
}


Echo::~Echo()
{
	delete [] history;
	history = NULL;
}


void Echo::SetDelay( int ms )
{
	// calculate number of samples needed for history
	int newDelay = ms * rate / 1000;

	// create new history buffer
	float *newHistory = new float[newDelay];
	memset( newHistory, 0, newDelay * sizeof(float) );

	// if there already is a history buffer, fill the new one with
	// old data (might not work right in all situations)
	if ( history )
	{
		int howMuch = delay - pos;
		howMuch = min( howMuch, newDelay );
		memcpy( newHistory, history + pos, howMuch * sizeof(float) );
		if ( howMuch < newDelay )
		{
			int i = howMuch;
			howMuch = newDelay - howMuch;
			howMuch = min( howMuch, delay );
			howMuch = min( howMuch, pos );
			memcpy( newHistory + i, history, howMuch * sizeof(float) );
		}
		delete [] history;
	}

	// remember new values
	history = newHistory;
	pos = 0;
	delay = newDelay;
	this->ms = ms;
}


// amp is in [0, 256] where 0 means no echoes and 256 means infinite echoes
void Echo::SetAmp( int amp )
{
	this->amp = amp;
	f_amp = ( float ) amp / 256.0f;
}


void Echo::SetSampleRate( int rate )
{
	if ( this->rate != rate )
	{
		this->rate = rate;
		SetDelay( ms );
	}
}


int Echo::GetDelay() const
{
	return ms;
}


int Echo::GetAmp() const
{
	return amp;
}


int Echo::GetSampleRate() const
{
	return rate;
}


// do the echo effect
float Echo::Process(float in)
{
	// mix each sample in the input buffer with the appropriately
	// delayed and amplified sample in the history buffer
	float smp = history[pos];   // read sample from history
	smp *= f_amp;				// amplify
	smp += in;                  // add to the matching sample from the input
	history[pos] = smp;         // write sample to the history buffer

	pos = ( pos + 1 ) % delay;

	return smp;
}
