//
//  ofxBPMDetector.h
//  ofxBPMDetectorExample
//
//  Created by Nao Tokui on 11/20/15.
//  www.naotokui.net
//
//  Including source code of:
//  The SoundTouch Library Copyright © Olli Parviainen 2001-2015
//  http://www.surina.net/soundtouch/
//

#ifndef ofxBPMDetector_h
#define ofxBPMDetector_h

#include <stdio.h>
#include <vector>
#include <math.h>

#include "BPMDetect.h"


#define ENERGY_HISTORY 25

class ofxBPMDetector
{
    public:

    ofxBPMDetector(int numChannels, int sampleRate, int spectrumSize);
    ~ofxBPMDetector();

    void    processFrame(std::vector<float> inputs, int nChannels);
    void    processFrame(float *input, int bufferSize, int nChannels);

    void    setSpectrumSize(int spectrumSize);

    float   getBPM();
    bool    getPeak(float &power);

    soundtouch::BPMDetect *detector;

    float   energyHistory[ENERGY_HISTORY];
    float   averageEnergy;
    float   variance;
    float   beatValue;
    int     historyPos;

    private:
    int nChannels = 1;
    int sampleRate = 44100;
    int spectrumSize = 129;
};



#endif /* ofxBPMDetector_h */
