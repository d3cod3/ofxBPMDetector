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

#include "bpmdetect.h"


class ofxBPMDetector
{
    public:
    
    /// @note _maxBpm should be at least 2 * _minBpm, otherwise BPM won't always be detected
    ofxBPMDetector(int numChannels = 2, int sampleRate = 44100, int minBPM= MIN_BPM, int maxBPM = MAX_BPM);
    ~ofxBPMDetector();

    void processFrame(std::vector<float> inputs, int nChannels);
    void processFrame(float *input, int bufferSize, int nChannels);
    float getBPM();
    
    BpmDetect *detector;

    private:
    int nChannels = 2;      // mono or stereo
    int sampleRate = 44100; // sampling rate
};



#endif /* ofxBPMDetector_h */
