/*
 DeckLinkController is a C++ only port of sample code from the DeckLink SDK
 that demonstrates how to get device info, start and stop a capture stream, and
 get video frame data from an input device. The only addition is a triple
 buffered video data member.
 */

#pragma once

#include "ofMain.h"

#include "DeckLinkAPI.h"
#include "TripleBuffer.h"
#include "VideoFrame.h"

class VideoFrame;

class DeckLinkController : public IDeckLinkInputCallback {
private:
	vector<IDeckLink*> deviceList;
	IDeckLink* selectedDevice;
	IDeckLinkInput* deckLinkInput;
	vector<IDeckLinkDisplayMode*> modeList;
	
	bool supportFormatDetection;
	bool currentlyCapturing;
    
    IDeckLinkVideoConversion *videoConverter;
    int colorConversionTimeout;
	
	void getAncillaryDataFromFrame(IDeckLinkVideoInputFrame* frame, BMDTimecodeFormat format, string& timecodeString, string& userBitsString);
	
public:
	TripleBuffer< vector<unsigned char> > buffer;
	
	DeckLinkController();
	virtual ~DeckLinkController();
	
	
	vector<ofVideoFormat> getFormats();
	
	
	bool init();
	
	int getDeviceCount();
	vector<string> getDeviceNameList();
	
	string getDeviceName();
	
	bool selectDevice(int index);
    void setColorConversionTimeout(int ms);
	
	vector<string> getDisplayModeNames();
	bool isFormatDetectionEnabled();
	bool isCapturing();

	unsigned long getDisplayModeBufferSize(BMDDisplayMode mode);

	bool startCaptureWithMode(BMDDisplayMode videoMode);
	bool startCaptureWithIndex(int videoModeIndex);
	void stopCapture();
    
	virtual HRESULT QueryInterface (REFIID iid, LPVOID *ppv) {return E_NOINTERFACE;}
	virtual ULONG AddRef () {return 1;}
	virtual ULONG Release () {return 1;}
    
	virtual HRESULT VideoInputFormatChanged (/* in */ BMDVideoInputFormatChangedEvents notificationEvents, /* in */ IDeckLinkDisplayMode *newDisplayMode, /* in */ BMDDetectedVideoInputFormatFlags detectedSignalFlags);
	virtual HRESULT VideoInputFrameArrived (/* in */ IDeckLinkVideoInputFrame* videoFrame, /* in */ IDeckLinkAudioInputPacket* audioPacket);
    
	BMDDisplayMode getDisplayMode(int w, int h);
	BMDDisplayMode getDisplayMode(int w, int h, float framerate);
    
    VideoFrame *rgbaFrame;
};
