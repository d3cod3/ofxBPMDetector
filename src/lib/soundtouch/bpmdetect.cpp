/***************************************************************************
   bpmdetect.cpp  -  adaption of the soundtouch bpm detection code
   -------------------
   begin                : Sat, Aug 4., 2007
   copyright            : (C) 2007 by Micah Lee
   email                : snipexv@gmail.com
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

////////////////////////////////////////////////////////////////////////////////
///
/// Beats-per-minute (BPM) detection routine.
///
/// The beat detection algorithm works as follows:
/// - Use function 'inputSamples' to input a chunks of samples to the class for
///   analysis. It's a good idea to enter a large sound file or stream in smallish
///   chunks of around few kilosamples in order not to extinguish too much RAM memory.
/// - Inputted sound data is decimated to approx 500 Hz to reduce calculation burden,
///   which is basically ok as low (bass) frequencies mostly determine the beat rate.
///   Simple averaging is used for anti-alias filtering because the resulting signal
///   quality isn't of that high importance.
/// - Decimated sound data is enveloped, i.e. the amplitude shape is detected by
///   taking absolute value that's smoothed by sliding average. Signal levels that
///   are below a couple of times the general RMS amplitude level are cut away to
///   leave only notable peaks there.
/// - Repeating sound patterns (e.g. beats) are detected by calculating short-term
///   autocorrelation function of the enveloped signal.
/// - After whole sound data file has been analyzed as above, the bpm level is
///   detected by function 'getBpm' that finds the highest peak of the autocorrelation
///   function, calculates it's precise location and converts this reading to bpm's.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2006/02/05 16:44:06 $
// File revision : $Revision: 1.7 $
//
// $Id: BPMDetect.cpp,v 1.7 2006/02/05 16:44:06 Olli Exp $
//
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <assert.h>
#include <string.h>
#include "FIFOSampleBuffer.h"
#include "peakfinder.h"
#include "bpmdetect.h"
#ifdef __C_METRICS__
#include "cmetrics.h"
#endif

//#include <QtDebug>

using namespace soundtouch;

#define INPUT_BLOCK_SAMPLES       1024
#define DECIMATED_BLOCK_SAMPLES   256
#define DECIMATE_FREQ             1000

typedef unsigned short ushort;

/// decay constant for calculating RMS volume sliding average approximation
/// (time constant is about 10 sec)
const float avgdecay = 0.99986f;

/// Normalization coefficient for calculating RMS sliding average approximation.
const float avgnorm = (1 - avgdecay);

float BpmDetect::correctBPM( float BPM, int min, int max) {
    if ( BPM == 0 ) return BPM;

    if( BPM*2 < max ) BPM *= 2;
    while ( BPM > max ) BPM /= 2;
    while ( BPM < min ) BPM *= 2;

    return BPM;
}

BpmDetect::BpmDetect(int numChannels, int sampleRate, int _minBpm, int _maxBpm)
{
    xcorr = NULL;

    buffer = new FIFOSampleBuffer();

    decimateSum = 0;
    decimateCount = 0;
    decimateBy = 0;

    this->sampleRate = sampleRate;
    this->channels = numChannels;

    envelopeAccu = 0;

    maxBpm = _maxBpm;
    minBpm = _minBpm;

    // Initialize RMS volume accumulator to RMS level of 3000 (out of 32768) that's
    // a typical RMS signal level value for song data. This value is then adapted
    // to the actual level during processing.
#ifdef INTEGER_SAMPLES
    // integer samples
    RMSVolumeAccu = (3000 * 3000) / avgnorm;
#else
    // float samples, scaled to range [-1..+1[
    RMSVolumeAccu = (0.092f * 0.092f) / avgnorm;
#endif

    init(numChannels, sampleRate);
}

void BpmDetect::init(int numChannels, unsigned int sampleRate)
{
    this->sampleRate = sampleRate;
    
    // choose decimation factor so that result is approx. 500 Hz
    decimateBy = sampleRate / DECIMATE_FREQ;
    assert(decimateBy > 0);
    assert(INPUT_BLOCK_SAMPLES < decimateBy * DECIMATED_BLOCK_SAMPLES);
    
    assert(minBpm > 0);
    assert(maxBpm > 0);
    // Calculate window length & starting item according to desired min & max bpms
    windowLen = (60 * sampleRate) / (decimateBy * minBpm);
    windowStart = (60 * sampleRate) / (decimateBy * maxBpm);
    
    assert(windowLen > windowStart);
    
    // allocate new working objects
    xcorr = new float[windowLen];
    memset(xcorr, 0, windowLen * sizeof(float));
    
    // we do processing in mono mode
    buffer->setChannels(1);
    buffer->clear();
}

BpmDetect::~BpmDetect()
{
    delete[] xcorr;
    delete buffer;
}


/// low-pass filter & decimate to about 500 Hz. return number of outputted samples.
///
/// Decimation is used to remove the unnecessary frequencies and thus to reduce
/// the amount of data needed to be processed as calculating autocorrelation
/// function is a very-very heavy operation.
///
/// Anti-alias filtering is done simply by averaging the samples. This is really a
/// poor-man's anti-alias filtering, but it's not so critical in this kind of application
/// (it'd also be difficult to design a high-quality filter with steep cut-off at very
/// narrow band)
int BpmDetect::decimate(SAMPLETYPE * dest, const SAMPLETYPE * src, int numsamples)
{
    int count, outcount;
    LONG_SAMPLETYPE out;

    assert(decimateBy != 0);
    outcount = 0;
    for (count = 0; count < numsamples; count++)
    {
//        qDebug() << "======"<< "count:"<< count << "numsamples:"<<numsamples << "outcount:"<< outcount << "decimateSum:"<< decimateSum << "decimateCount:"<< decimateCount;
        decimateSum += src[count];

        decimateCount++;
        if (decimateCount >= decimateBy)
        {
            // Store every Nth sample only
            out = (LONG_SAMPLETYPE)(decimateSum / decimateBy);
            decimateSum = 0;
            decimateCount = 0;
#ifdef INTEGER_SAMPLES
            // check ranges for sure (shouldn't actually be necessary)
            if (out > 32767)
            {
                out = 32767;
            }
            else if (out < -32768)
            {
                out = -32768;
            }
#endif // INTEGER_SAMPLES
            dest[outcount] = (SAMPLETYPE)out;
            outcount++;
        }
    }
    return outcount;
}



// Calculates autocorrelation function of the sample history buffer
void BpmDetect::updateXCorr(int process_samples)
{
    int offs;
    SAMPLETYPE * pBuffer;

    assert(buffer->numSamples() >= (uint)(process_samples + windowLen));

    pBuffer = buffer->ptrBegin();
    for (offs = windowStart; offs < windowLen; offs++)
    {
        LONG_SAMPLETYPE sum;
        int i;

        sum = 0;
        for (i = 0; i < process_samples; i++)
        {
            sum += pBuffer[i] * pBuffer[i + offs];    // scaling the sub-result shouldn't be necessary
        }
        xcorr[offs] *= xcorr_decay;   // decay 'xcorr' here with suitable coefficients
        // if it's desired that the system adapts automatically to
        // various bpms, e.g. in processing continouos music stream.
        // The 'xcorr_decay' should be a value that's smaller than but
        // close to one, and should also depend on 'process_samples' value.

        xcorr[offs] += (float)sum;
    }
}



// Calculates envelope of the sample data
void BpmDetect::calcEnvelope(SAMPLETYPE * samples, int numsamples)
{
    const float decay = 0.7f;               // decay constant for smoothing the envelope
    const float norm = (1 - decay);

    int i;
    LONG_SAMPLETYPE out;
    float val;

    for (i = 0; i < numsamples; i++)
    {
        // calc average RMS volume
        RMSVolumeAccu *= avgdecay;
        val = (float)fabs((float)samples[i]);
        RMSVolumeAccu += val * val;

        // cut amplitudes that are below 2 times average RMS volume
        // (we're interested in peak values, not the silent moments)
        val -= 2 * (float)sqrt(RMSVolumeAccu * avgnorm);
        val = (val > 0) ? val : 0;

        // smooth amplitude envelope
        envelopeAccu *= decay;
        envelopeAccu += val;
        out = (LONG_SAMPLETYPE)(envelopeAccu * norm);

#ifdef INTEGER_SAMPLES
        // cut peaks (shouldn't be necessary though)
        if (out > 32767) out = 32767;
#endif // INTEGER_SAMPLES
        samples[i] = (SAMPLETYPE)out;
    }
}



void BpmDetect::inputSamples(SAMPLETYPE * samples, int numSamples)
{
    SAMPLETYPE decimated[DECIMATED_BLOCK_SAMPLES];

    // convert from stereo to mono if necessary
    if (channels == 2)
    {
        int i;

        for (i = 0; i < numSamples; i++)
        {
            samples[i] = (samples[i * 2] + samples[i * 2 + 1]) * 0.5;
        }
    }

    // decimate
    numSamples = decimate(decimated, samples, numSamples);

    // envelope new samples and add them to buffer
    calcEnvelope(decimated, numSamples);
    buffer->putSamples(decimated, numSamples);

    // when the buffer has enought samples for processing...
    if ((int)buffer->numSamples() > windowLen)
    {
        int processLength;

        // how many samples are processed
        processLength = buffer->numSamples() - windowLen;

        // ... calculate autocorrelations for oldest samples...
        updateXCorr(processLength);
        // ... and remove them from the buffer
        buffer->receiveSamples(processLength);
    }
}

float BpmDetect::getBpm()
{
    float peakPos;
    PeakFinder peakFinder;

    // find peak position
    peakPos = peakFinder.detectPeak(xcorr, windowStart, windowLen);

    assert(decimateBy != 0);
    if (peakPos < 1e-6) return 0.0; // detection failed.

    // calculate BPM
    return 60.0f * (((float)sampleRate / (float)decimateBy) / peakPos);
}

void BpmDetect::reset(){
    buffer->setChannels(1);
    buffer->clear();
}
