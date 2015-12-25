//
//  ofxBPMDetector.cpp
//  ofxBPMDetectorExample
//
//  Created by Nao Tokui on 11/20/15.
//  www.naotokui.net
//
//  Including source code of the following library:
//  The SoundTouch Library Copyright © Olli Parviainen 2001-2015
//  http://www.surina.net/soundtouch/
//


#include "ofxBPMDetector.h"


//ofxBPMDetector::ofxBPMDetector(){
//    detector = new BpmDetect(nChannels, sampleRate, MIN_BPM, MAX_BPM);
//}

ofxBPMDetector::ofxBPMDetector(int nChannels, int sampleRate, int minBPM, int maxBPM){
    detector = new BpmDetect(nChannels, sampleRate, minBPM, maxBPM);
    this->nChannels = nChannels;
    this->sampleRate = sampleRate;
    
}


ofxBPMDetector::~ofxBPMDetector(){
    delete detector;
}

void ofxBPMDetector::processFrame(float *input, int bufferSize, int nChannels){
    assert(nChannels == this->nChannels);
    detector->inputSamples(input, bufferSize);
}

void ofxBPMDetector::processFrame(std::vector<float> inputs, int nChannels){
    assert(nChannels == this->nChannels);
    detector->inputSamples(inputs.data(), inputs.size());
}

float ofxBPMDetector::getBPM(){
    return detector->getBpm();
}
