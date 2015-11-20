#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    soundStream.setup( this, 0, 2, 44100, 512, 4 );
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(100);
    ofDrawBitmapString("Analyzing mic input...", 50, 50);

    ofSetColor(0);
    float bpm = bpmDetector.getBPM();
    ofDrawBitmapString("BPM: " + ofToString(bpm, 3), 50, 100);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

//--------------------------------------------------------------

void ofApp::audioReceived(float *input, int bufferSize, int nChannels){
    bpmDetector.processFrame(input, bufferSize, nChannels);
}
