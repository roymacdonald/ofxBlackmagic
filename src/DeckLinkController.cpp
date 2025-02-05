
#include "DeckLinkController.h"

DeckLinkController::DeckLinkController()
: selectedDevice(NULL)
, deckLinkInput(NULL)
, supportFormatDetection(false)
, currentlyCapturing(false)
, rgbaFrame(NULL)  {
}

DeckLinkController::~DeckLinkController()  {
	vector<IDeckLink*>::iterator it;
	
	// Release the IDeckLink list
	for(it = deviceList.begin(); it != deviceList.end(); it++) {
		(*it)->Release();
	}
    
    // Release the IDeckLinkVideoConversion
    if (this->videoConverter) {
        this->videoConverter->Release();
    }
    
    delete rgbaFrame;
}

bool DeckLinkController::init()  {
	IDeckLinkIterator* deckLinkIterator = NULL;
	IDeckLink* deckLink = NULL;
	bool result = false;
	
	// Create an iterator
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (deckLinkIterator == NULL) {
		ofLogError("DeckLinkController") << "Please install the Blackmagic Desktop Video drivers to use the features of this application.";
		goto bail;
	}
	
	// List all DeckLink devices
	while (deckLinkIterator->Next(&deckLink) == S_OK) {
		// Add device to the device list
		deviceList.push_back(deckLink);
	}
	
	if (deviceList.size() == 0) {
		ofLogError("DeckLinkController") << "You will not be able to use the features of this application until a Blackmagic device is installed.";
		goto bail;
	}
    
    // Create a video converter
    videoConverter = CreateVideoConversionInstance();
    videoConverter->AddRef();
	
	result = true;
	
bail:
	if (deckLinkIterator != NULL) {
		deckLinkIterator->Release();
		deckLinkIterator = NULL;
	}
	
	return result;
}


int DeckLinkController::getDeviceCount()  {
	return deviceList.size();
}

vector<string> DeckLinkController::getDeviceNameList()  {
	vector<string> nameList;
	int deviceIndex = 0;
	
	while (deviceIndex < deviceList.size()) {
		CFStringRef cfStrName;
		
		// Get the name of this device
		if (deviceList[deviceIndex]->GetDisplayName(&cfStrName) == S_OK) {
			nameList.push_back(string(CFStringGetCStringPtr(cfStrName, kCFStringEncodingMacRoman)));
			CFRelease(cfStrName);
		}
		else {
			nameList.push_back("DeckLink");
		}
		
		deviceIndex++;
	}
	
	return nameList;
}


bool DeckLinkController::selectDevice(int index)  {
	IDeckLinkAttributes* deckLinkAttributes = NULL;
	IDeckLinkDisplayModeIterator* displayModeIterator = NULL;
	IDeckLinkDisplayMode* displayMode = NULL;
	bool result = false;
	
	// Check index
	if (index >= deviceList.size()) {
		ofLogError("DeckLinkController") << "This application was unable to select the device.";
		goto bail;
	}
	
	// A new device has been selected.
	// Release the previous selected device and mode list
	if (deckLinkInput != NULL)
		deckLinkInput->Release();
	
	while(modeList.size() > 0) {
		modeList.back()->Release();
		modeList.pop_back();
	}
	
	
	// Get the IDeckLinkInput for the selected device
	if ((deviceList[index]->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput) != S_OK)) {
		ofLogError("DeckLinkController") << "This application was unable to obtain IDeckLinkInput for the selected device.";
		deckLinkInput = NULL;
		goto bail;
	}
	
	//
	// Retrieve and cache mode list
	if (deckLinkInput->GetDisplayModeIterator(&displayModeIterator) == S_OK) {
		while (displayModeIterator->Next(&displayMode) == S_OK)
			modeList.push_back(displayMode);
		
		displayModeIterator->Release();
	}
	
	//
	// Check if input mode detection format is supported.
	
	supportFormatDetection = false; // assume unsupported until told otherwise
	if (deviceList[index]->QueryInterface(IID_IDeckLinkAttributes, (void**) &deckLinkAttributes) == S_OK) {
		if (deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &supportFormatDetection) != S_OK)
			supportFormatDetection = false;
		
		deckLinkAttributes->Release();
	}
	
	result = true;
	
bail:
	return result;
}

void DeckLinkController::setColorConversionTimeout(int ms)  {
    colorConversionTimeout = ms;
}


vector<ofVideoFormat> DeckLinkController::getFormats(){
	vector<ofVideoFormat> formats;
	BMDTimeValue frameDuration;
	BMDTimeScale timeScale;
//	GetFrameRate (/* out */ BMDTimeValue *frameDuration, /* out */ BMDTimeScale *timeScale)
//	GetWidth (void) = 0;
//	virtual long GetHeight (void) =
	for (int modeIndex = 0; modeIndex < modeList.size(); modeIndex++) {
		if (modeList[modeIndex]->GetFrameRate (&frameDuration, &timeScale) == S_OK) {
			ofVideoFormat f;
			f.height = modeList[modeIndex]->GetHeight();
			f.width = modeList[modeIndex]->GetWidth();
			f.framerates = { round(100*((float)timeScale/(float)frameDuration))/100 };
			
			
			bool bFound = false;
//			size_t foundIndex = 0;
			for(size_t i = 0; i < formats.size(); i++){
				if(formats[i].width == f.width && formats[i].height == f.height && formats[i].framerates[0] == f.framerates[0]){
				
				
//				if(formats[i].width == f.width && formats[i].height == f.height){
//					foundIndex = i;
					bFound = true;
					break;
				}
			}
//			if(bFound){
//				formats[foundIndex].framerates.push_back(f.framerates[0]);
//			}else{
			
			if(!bFound){
				formats.push_back(f);
			}
		}
		else {
			
		}
	}
	
	return formats;
	
}

vector<string> DeckLinkController::getDisplayModeNames()  {
	vector<string> modeNames;
	int modeIndex;
	CFStringRef modeName;
	
	for (modeIndex = 0; modeIndex < modeList.size(); modeIndex++) {
		if (modeList[modeIndex]->GetName(&modeName) == S_OK) {
			modeNames.push_back(string(CFStringGetCStringPtr(modeName, kCFStringEncodingMacRoman)));
			CFRelease(modeName);
		}
		else {
			modeNames.push_back("Unknown mode");
		}
	}
	
	return modeNames;
}

bool DeckLinkController::isFormatDetectionEnabled()  {
	return supportFormatDetection;
}

bool DeckLinkController::isCapturing()  {
	return currentlyCapturing;
}

unsigned long DeckLinkController::getDisplayModeBufferSize(BMDDisplayMode mode) {

	if(mode == bmdModeNTSC2398
			|| mode == bmdModeNTSC
			|| mode == bmdModeNTSCp) {
		return 720 * 486 * 2;
	} else if( mode == bmdModePAL
			|| mode == bmdModePALp) {
		return 720 * 576 * 2;
	} else if( mode == bmdModeHD720p50
			|| mode == bmdModeHD720p5994
			|| mode == bmdModeHD720p60) {
		return 1280 * 720 * 2;
	} else if( mode == bmdModeHD1080p2398
			|| mode == bmdModeHD1080p24
			|| mode == bmdModeHD1080p25
			|| mode == bmdModeHD1080p2997
			|| mode == bmdModeHD1080p30
			|| mode == bmdModeHD1080i50
			|| mode == bmdModeHD1080i5994
			|| mode == bmdModeHD1080i6000
			|| mode == bmdModeHD1080p50
			|| mode == bmdModeHD1080p5994
			|| mode == bmdModeHD1080p6000) {
		return 1920 * 1080 * 2;
	} else if( mode == bmdMode2k2398
			|| mode == bmdMode2k24
			|| mode == bmdMode2k25) {
		return 2048 * 1556 * 2;
	} else if( mode == bmdMode2kDCI2398
			|| mode == bmdMode2kDCI24
			|| mode == bmdMode2kDCI25) {
		return 2048 * 1080 * 2;
	} else if( mode == bmdMode4K2160p2398
			|| mode == bmdMode4K2160p24
			|| mode == bmdMode4K2160p25
			|| mode == bmdMode4K2160p2997
			|| mode == bmdMode4K2160p30) {
		return 3840 * 2160 * 2;
	} else if( mode == bmdMode4kDCI2398
			|| mode == bmdMode4kDCI24
			|| mode == bmdMode4kDCI25) {
		return 4096 * 2160 * 2;
	}

	return 0;
}

bool DeckLinkController::startCaptureWithIndex(int videoModeIndex)  {
	// Get the IDeckLinkDisplayMode from the given index
	if ((videoModeIndex < 0) || (videoModeIndex >= modeList.size())) {
		ofLogError("DeckLinkController") << "An invalid display mode was selected.";
		return false;
	}
	
	return startCaptureWithMode(modeList[videoModeIndex]->GetDisplayMode());
}

bool DeckLinkController::startCaptureWithMode(BMDDisplayMode videoMode) {
    int bufferSize = getDisplayModeBufferSize(videoMode);
    
	if(bufferSize != 0) {
		vector<unsigned char> prototype(bufferSize);
		buffer.setup(prototype);
	} else{
		ofLogError("DeckLinkController") << "DeckLinkController needs to be updated to support that mode.";
		return false;
	}
	
	BMDVideoInputFlags videoInputFlags;
	
	// Enable input video mode detection if the device supports it
	videoInputFlags = supportFormatDetection ? bmdVideoInputEnableFormatDetection : bmdVideoInputFlagDefault;
	
	// Set capture callback
	deckLinkInput->SetCallback(this);
	
	// Set the video input mode
	if (deckLinkInput->EnableVideoInput(videoMode, bmdFormat8BitYUV, videoInputFlags) != S_OK) {
		ofLogError("DeckLinkController") << "This application was unable to select the chosen video mode. Perhaps, the selected device is currently in-use.";
		return false;
	}
	
	// Start the capture
	if (deckLinkInput->StartStreams() != S_OK) {
		ofLogError("DeckLinkController") << "This application was unable to start the capture. Perhaps, the selected device is currently in-use.";
		return false;
	}
	
	currentlyCapturing = true;
	
	return true;
}

void DeckLinkController::stopCapture()  {
	// Stop the capture
	deckLinkInput->StopStreams();
	
	// Delete capture callback
	deckLinkInput->SetCallback(NULL);
	
	currentlyCapturing = false;
}


HRESULT DeckLinkController::VideoInputFormatChanged (/* in */ BMDVideoInputFormatChangedEvents notificationEvents, /* in */ IDeckLinkDisplayMode *newMode, /* in */ BMDDetectedVideoInputFormatFlags detectedSignalFlags)  {
	bool shouldRestartCaptureWithNewVideoMode = true;
	
	// Restart capture with the new video mode if told to
	if (shouldRestartCaptureWithNewVideoMode) {
		// Stop the capture
		deckLinkInput->StopStreams();
		
		// Set the video input mode
		if (deckLinkInput->EnableVideoInput(newMode->GetDisplayMode(), bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection) != S_OK) {
			ofLogError("DeckLinkController") << "This application was unable to select the new video mode.";
			goto bail;
		}
		
		// Start the capture
		if (deckLinkInput->StartStreams() != S_OK) {
			ofLogError("DeckLinkController") << "This application was unable to start the capture on the selected device.";
			goto bail;
		}
	}
	
bail:
	return S_OK;
}

typedef struct {
	// VITC timecodes and user bits for field 1 & 2
	string vitcF1Timecode;
	string vitcF1UserBits;
	string vitcF2Timecode;
	string vitcF2UserBits;
	
	// RP188 timecodes and user bits (VITC1, VITC2 and LTC)
	string rp188vitc1Timecode;
	string rp188vitc1UserBits;
	string rp188vitc2Timecode;
	string rp188vitc2UserBits;
	string rp188ltcTimecode;
	string rp188ltcUserBits;
} AncillaryDataStruct;

HRESULT DeckLinkController::VideoInputFrameArrived (/* in */ IDeckLinkVideoInputFrame* videoFrame, /* in */ IDeckLinkAudioInputPacket* audioPacket)  {
//	bool hasValidInputSource = (videoFrame->GetFlags() & bmdFrameHasNoInputSource) != 0;
	
//	AncillaryDataStruct ancillaryData;
	
	// Get the various timecodes and userbits for this frame
//	getAncillaryDataFromFrame(videoFrame, bmdTimecodeVITC, ancillaryData.vitcF1Timecode, ancillaryData.vitcF1UserBits);
//	getAncillaryDataFromFrame(videoFrame, bmdTimecodeVITCField2, ancillaryData.vitcF2Timecode, ancillaryData.vitcF2UserBits);
//	getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188VITC1, ancillaryData.rp188vitc1Timecode, ancillaryData.rp188vitc1UserBits);
//	getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188LTC, ancillaryData.rp188ltcTimecode, ancillaryData.rp188ltcUserBits);
//	getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188VITC2, ancillaryData.rp188vitc2Timecode, ancillaryData.rp188vitc2UserBits);
    
    // Using DeckLink SDK for colour conversion
    if (!rgbaFrame) {
        rgbaFrame = new VideoFrame(videoFrame->GetWidth(), videoFrame->GetHeight());
    }
    
    if (rgbaFrame->lock.try_lock_for(std::chrono::milliseconds(colorConversionTimeout))) {
        videoConverter->ConvertFrame(videoFrame, rgbaFrame);
        rgbaFrame->lock.unlock();
    }
    else {
        cout << "Cannot copy frame data as videoFrame is locked" << endl;
    }

    // Raw data
	void* bytes;
	videoFrame->GetBytes(&bytes);
	unsigned char* raw = (unsigned char*) bytes;
	vector<unsigned char>& back = buffer.getBack();
	back.assign(raw, raw + back.size());
	buffer.swapBack();
	
	return S_OK;
}

void DeckLinkController::getAncillaryDataFromFrame(IDeckLinkVideoInputFrame* videoFrame, BMDTimecodeFormat timecodeFormat, string& timecodeString, string& userBitsString)  {
	IDeckLinkTimecode* timecode = NULL;
	CFStringRef timecodeCFString;
	BMDTimecodeUserBits userBits = 0;
	
	if ((videoFrame != NULL)
		&& (videoFrame->GetTimecode(timecodeFormat, &timecode) == S_OK)) {
		if (timecode->GetString(&timecodeCFString) == S_OK) {
			timecodeString = string(CFStringGetCStringPtr(timecodeCFString, kCFStringEncodingMacRoman));
			CFRelease(timecodeCFString);
		}
		else {
			timecodeString = "";
		}
		
		timecode->GetTimecodeUserBits(&userBits);
		userBitsString = "0x" + ofToHex(userBits);
		
		timecode->Release();
	}
	else {
		timecodeString = "";
		userBitsString = "";
	}
	
	
}

// picks the mode with matching resolution, with highest available framerate
// and a preference for progressive over interlaced
BMDDisplayMode DeckLinkController::getDisplayMode(int w, int h) {

	if (w == 720 && h == 486) {				// NTSC
		return bmdModeNTSCp;
	} else if (w == 720 && h == 576) {		// PAL
		return bmdModePALp;
	} else if (w == 1280 && h == 720) {		// HD 720
		return bmdModeHD720p60;
	} else if (w == 1920 && h == 1080) {	// HD 1080
		return bmdModeHD1080p6000;
	} else if (w == 2048 && h == 1556) {	// 2k
		return bmdMode2k25;
	} else if (w == 2048 && h == 1080) {	// 2k DCI
		return bmdMode2kDCI25;
	} else if (w == 3840 && h == 2160) {	// 4K
		return bmdMode4K2160p30;
	} else if (w == 4096 && h == 2160) {	// 4k DCI
		return bmdMode4kDCI25;
	}

	return bmdModeUnknown;
}

BMDDisplayMode DeckLinkController::getDisplayMode(int w, int h, float framerate) {
	string err = "invalid framerate, for this resolution you can use:";

	if (w == 720 && h == 486) {									// NTSC
		if (framerate == 29.97f) {
		    return bmdModeNTSC;
		} else if (framerate == 23.98f) {
            return bmdModeNTSC2398;
		} else if (framerate == 59.94f) {
		    return bmdModeNTSCp;
		} else {
			ofLogError("DeckLinkController") << err << endl
				<< "23.98, 29.97, 59.94";
			return bmdModeUnknown;
		}
	} else if (w == 720 && h == 576) {							// PAL
		if (framerate == 25.f) {
		    return bmdModePAL;
		} else if (framerate == 50.f) {
		    return bmdModePALp;
		} else {
			ofLogError("DeckLinkController") << err << endl
				<< "25, 50";
			return bmdModeUnknown;
		}
	} else if (w == 1280 && h == 720) {							// HD 720
        if (framerate == 50.f) {
            return bmdModeHD720p50;
		} else if (framerate == 59.94f) {
		    return bmdModeHD720p5994;
		} else if (framerate == 60.f) {
			return bmdModeHD720p60;
		} else {
			ofLogError("DeckLinkController") << err << endl
				<< "50, 59.94, 60";
			return bmdModeUnknown;
		}
	} else if (w == 1920 && h == 1080) {						// HD 1080
		if (framerate == 23.98f) {
			return bmdModeHD1080p2398;
		} else if (framerate == 24.f) {
			return bmdModeHD1080p24;
		} else if (framerate == 25.f) {
			return bmdModeHD1080p25;
		} else if (framerate == 29.97f) {
			return bmdModeHD1080p2997;
		} else if (framerate == 30.f) {
			return bmdModeHD1080p30;
		} else if (framerate == 50.f) {
			return bmdModeHD1080i50;
		} else if (framerate == 59.94f) {
			return bmdModeHD1080i5994;
		} else if (framerate == 60.f) {
			return bmdModeHD1080i6000;
		} else if (framerate == 50.f) {
			return bmdModeHD1080p50;
		} else if (framerate == 59.94f) {
			return bmdModeHD1080p5994;
		} else if (framerate == 60.f) {
			return bmdModeHD1080p6000;
		} else {
			ofLogError("DeckLinkController") << err << endl
				<< "23.94, 24, 25, 29.97, 30" << endl
				<< "50, 59.94, 60, 50, 59.94, 60";
			return bmdModeUnknown;
		}
	} else if (w == 2048 && h == 1556) {						// 2k
		if (framerate == 23.98f) {
			return bmdMode2k2398;
		} else if (framerate == 24.f) {
			return bmdMode2k24;
		} else if (framerate == 25.f) {
			return bmdMode2k25;
		} else {
			ofLogError("DeckLinkController") << err << endl
				<< "23.94, 24, 25";
			return bmdModeUnknown;
		}
	} else if (w == 2048 && h == 1080) {						// 2k DCI
		if (framerate == 23.98f) {
			return bmdMode2kDCI2398;
		} else if (framerate == 24.f) {
			return bmdMode2kDCI24;
		} else if (framerate == 25.f) {
			return bmdMode2kDCI25;
		} else {
			ofLogError("DeckLinkController") << err << endl
				<< "23.98 24, 25";
			return bmdModeUnknown;
		}
	} else if (w == 3840 && h == 2160) {						// 4K
		if (framerate == 23.98f) {
			return bmdMode4K2160p2398;
		} else if (framerate == 24.f) {
			return bmdMode4K2160p24;
		} else if (framerate == 25.f) {
			return bmdMode4K2160p25;
		} else if (framerate == 29.97f) {
			return bmdMode4K2160p2997;
		} else if (framerate == 30.f) {
			return bmdMode4K2160p30;
		} else {
			ofLogError("DeckLinkController") << err << endl
				<< "23.98, 24, 25, 29.97, 30";
			return bmdModeUnknown;
		}
	} else if (w == 4096 && h == 2160) {						// 4k DCI
		if (framerate == 23.98f) {
		    return bmdMode4kDCI2398;
		} else if (framerate == 24.f) {
		    return bmdMode4kDCI24;
		} else if (framerate == 25.f) {
		    return bmdMode4kDCI25;
		} else {
			ofLogError("DeckLinkController") << err << endl
				<< "23.98, 24, 25";
			return bmdModeUnknown;
		}
	}

	return bmdModeUnknown;
}
