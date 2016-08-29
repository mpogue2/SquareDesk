/*
	BASSmix multi-speaker example
	Copyright (c) 2009 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include <stdio.h>
#include "bass.h"
#include "bassmix.h"

#define min(a,b) ((a)<(b)?(a):(b))

WindowPtr win;

HSTREAM mixer,source; // mixer and source channels

// display error messages
void Error(const char *es)
{
	short i;
	char mes[200];
	sprintf(mes,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	CFStringRef ces=CFStringCreateWithCString(0,mes,0);
	DialogRef alert;
	CreateStandardAlert(0,CFSTR("Error"),ces,NULL,&alert);
	RunStandardAlert(alert,NULL,&i);
	CFRelease(ces);
}

ControlRef GetControl(int id)
{
	ControlRef cref;
	ControlID cid={0,id};
	GetControlByID(win,&cid,&cref);
	return cref;
}

void SetupControlHandler(int id, DWORD event, EventHandlerProcPtr proc)
{
	EventTypeSpec etype={kEventClassControl,event};
	ControlRef cref=GetControl(id);
	InstallControlEventHandler(cref,NewEventHandlerUPP(proc),1,&etype,cref,NULL);
}

void SetMatrix()
{
	float *matrix;
	BASS_CHANNELINFO mi,si;
	BASS_ChannelGetInfo(mixer,&mi); // get mixer info for channel count
	BASS_ChannelGetInfo(source,&si); // get source info for channel count
	matrix=(float*)malloc(mi.chans*si.chans*sizeof(float)); // allocate matrix (mixer channel count * source channel count)
	memset(matrix,0,mi.chans*si.chans*sizeof(float)); // initialize it to empty/silence
/*
	set the mixing matrix depending on the speaker switches
	mono & stereo sources are duplicated on each enabled pair of speakers
*/
	if (GetControl32BitValue(GetControl(11))) { // 1st pair of speakers enabled
		matrix[0*si.chans+0]=1;
		if (si.chans==1) // mono source
			matrix[1*si.chans+0]=1;
		else
			matrix[1*si.chans+1]=1;
	}
	if (mi.chans>=4 && GetControl32BitValue(GetControl(12))) { // 2nd pair of speakers enabled
		if (si.chans>2) { // multi-channel source
			matrix[2*si.chans+2]=1;
			if (si.chans>3) matrix[3*si.chans+3]=1;
		} else {
			matrix[2*si.chans+0]=1;
			if (si.chans==1) // mono source
				matrix[3*si.chans+0]=1;
			else // stereo source
				matrix[3*si.chans+1]=1;
		}
	}
	if (mi.chans>=6 && GetControl32BitValue(GetControl(13))) { // 3rd pair of speakers enabled
		if (si.chans>2) { // multi-channel source
			if (si.chans>4) matrix[4*si.chans+4]=1;
			if (si.chans>5) matrix[5*si.chans+5]=1;
		} else {
			matrix[4*si.chans+0]=1;
			if (si.chans==1) // mono source
				matrix[5*si.chans+0]=1;
			else // stereo source
				matrix[5*si.chans+1]=1;
		}
	}
	if (mi.chans>=8 && GetControl32BitValue(GetControl(14))) { // 4th pair of speakers enabled
		if (si.chans>2) { // multi-channel source
			if (si.chans>6) matrix[6*si.chans+6]=1;
			if (si.chans>7) matrix[7*si.chans+7]=1;
		} else {
			matrix[6*si.chans+0]=1;
			if (si.chans==1) // mono source
				matrix[7*si.chans+0]=1;
			else // stereo source
				matrix[7*si.chans+1]=1;
		}
	}
	BASS_Mixer_ChannelSetMatrix(source,matrix); // apply the matrix
	free(matrix);
}

pascal OSStatus OpenEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	NavDialogRef fileDialog;
	NavDialogCreationOptions fo;
	NavGetDefaultDialogCreationOptions(&fo);
	fo.optionFlags=0;
	fo.parentWindow=win;
	NavCreateChooseFileDialog(&fo,NULL,NULL,NULL,NULL,NULL,&fileDialog);
	if (!NavDialogRun(fileDialog)) {
		NavReplyRecord r;
		if (!NavDialogGetReply(fileDialog,&r)) {
			AEKeyword k;
			FSRef fr;
			if (!AEGetNthPtr(&r.selection,1,typeFSRef,&k,NULL,&fr,sizeof(fr),NULL)) {
				char file[256];
				FSRefMakePath(&fr,(BYTE*)file,sizeof(file));
				BASS_StreamFree(mixer); // free old mixer (and source due to AUTOFREE) before opening new
				source=BASS_StreamCreateFile(FALSE,file,0,0,BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT|BASS_SAMPLE_LOOP); // create source
				if (!source) { // failed
					SetControlTitleWithCFString(inUserData,CFSTR("click here to open a file..."));
					Error("Can't play the file");
				} else {
					BASS_CHANNELINFO ci;
					BASS_INFO di;
					BASS_ChannelGetInfo(source,&ci); // get source info for sample rate
					BASS_GetInfo(&di); // get device info for speaker count
					mixer=BASS_Mixer_StreamCreate(ci.freq,min(di.speakers,8),0); // create mixer with source sample rate and device speaker count
					if (!mixer) { // failed
						BASS_StreamFree(source);
						SetControlTitleWithCFString(inUserData,CFSTR("click here to open a file..."));
						Error("Can't create mixer");
					} else {
						BASS_Mixer_StreamAddChannel(mixer,source,BASS_MIXER_MATRIX|BASS_STREAM_AUTOFREE); // add the source to the mix with matrix-mixing enabled
						SetMatrix(); // set the matrix
						BASS_ChannelPlay(mixer,FALSE); // start playing
						CFStringRef cs=CFStringCreateWithCString(0,file,kCFStringEncodingUTF8);
						SetControlTitleWithCFString(inUserData,cs);
						CFRelease(cs);
						// enable the speaker switches according to the speaker count
						if (di.speakers>=4)
							ActivateControl(GetControl(12));
						else
							DeactivateControl(GetControl(12));
						if (di.speakers>=6)
							ActivateControl(GetControl(13));
						else
							DeactivateControl(GetControl(13));
						if (di.speakers>=8)
							ActivateControl(GetControl(14));
						else
							DeactivateControl(GetControl(14));
					}
				}
			}
			NavDisposeReply(&r);
		}
	}
	NavDialogDispose(fileDialog);
    return noErr;
}

pascal OSStatus SpeakerEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	SetMatrix();
	return noErr;
}

int main(int argc, char* argv[])
{
	IBNibRef 		nibRef;
	OSStatus		err;
    
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	// initialize default device
	if (!BASS_Init(-1,44100,0,NULL,NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	// Create Window and stuff
	err = CreateNibReference(CFSTR("speakers"), &nibRef);
	if (err) return err;
	err = CreateWindowFromNib(nibRef, CFSTR("Window"), &win);
	if (err) return err;
	DisposeNibReference(nibRef);

	SetupControlHandler(10,kEventControlHit,OpenEventHandler);
	SetupControlHandler(11,kEventControlHit,SpeakerEventHandler);
	SetupControlHandler(12,kEventControlHit,SpeakerEventHandler);
	SetupControlHandler(13,kEventControlHit,SpeakerEventHandler);
	SetupControlHandler(14,kEventControlHit,SpeakerEventHandler);

	ShowWindow(win);
	RunApplicationEventLoop();

	BASS_Free();

    return 0; 
}
