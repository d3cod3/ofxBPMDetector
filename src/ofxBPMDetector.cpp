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


ofxBPMDetector::ofxBPMDetector(int nChannels, int sampleRate, int spectrumSize){
    detector = new soundtouch::BPMDetect(nChannels, sampleRate);
    this->nChannels = nChannels;
    this->sampleRate = sampleRate;
    this->spectrumSize = spectrumSize;

    historyPos = 0;

    for(int l = 0; l < ENERGY_HISTORY; l++){
        energyHistory[l] = 0;
    }
    averageEnergy = 0;
    variance = 0;
    beatValue = 0;
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
    detector->inputSamples(inputs.data(), static_cast<int>(inputs.size()));
}

float ofxBPMDetector::getBPM(){
    return detector->getBpm();
}

bool ofxBPMDetector::getPeak(float &power){

    variance += pow(power, 2);

    beatValue = (-0.00025714f*variance)+1.35f;

    averageEnergy = 0;
    for(int h = 0; h < ENERGY_HISTORY; h++) {
        averageEnergy += energyHistory[h];
    }
    averageEnergy /= ENERGY_HISTORY;

    energyHistory[historyPos] = power;

    historyPos = (historyPos+1) % ENERGY_HISTORY;


    return power > averageEnergy*beatValue;
}
