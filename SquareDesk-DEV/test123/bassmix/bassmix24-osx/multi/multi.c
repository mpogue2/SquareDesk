/*
	BASSmix multiple output example
	Copyright (c) 2009 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include <stdio.h>
#include "bass.h"
#include "bassmix.h"

WindowPtr win;

DWORD outdev[2];	// output devices
HSTREAM chan;		// the source stream
HSTREAM ochan[2];	// the output/splitter streams

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
				BASS_StreamFree(chan); // free old stream (splitters automatically freed too)
				if (!(chan=BASS_StreamCreateFile(FALSE,file,0,0,BASS_STREAM_DECODE|BASS_SAMPLE_LOOP))) {
					SetControlTitleWithCFString(inUserData,CFSTR("click here to open a file..."));
					Error("Can't play the file");
				} else {
					// set the device to create 1st splitter stream on, and then create it
					BASS_SetDevice(outdev[0]);
					if (!(ochan[0]=BASS_Split_StreamCreate(chan,0,NULL))) {
						BASS_StreamFree(chan);
						SetControlTitleWithCFString(inUserData,CFSTR("click here to open a file..."));
						Error("Can't create splitter");
					}
					// set the device to create 2nd splitter stream on, and then create it
					BASS_SetDevice(outdev[1]); 
					if (!(ochan[1]=BASS_Split_StreamCreate(chan,0,NULL))) {
						BASS_StreamFree(chan);
						SetControlTitleWithCFString(inUserData,CFSTR("click here to open a file..."));
						Error("Can't create splitter");
					}
					CFStringRef cs=CFStringCreateWithCString(0,file,kCFStringEncodingUTF8);
					SetControlTitleWithCFString(inUserData,cs);
					CFRelease(cs);
					{ // update scroller range
						QWORD bytes=BASS_ChannelGetLength(chan,BASS_POS_BYTE);
						DWORD secs=BASS_ChannelBytes2Seconds(chan,bytes);
						SetControl32BitMaximum(GetControl(12),secs);
					}
					BASS_ChannelSetLink(ochan[0],ochan[1]); // link the splitters so that they stop/start together
					BASS_ChannelPlay(ochan[0],FALSE); // start playback
				}
			}
			NavDisposeReply(&r);
		}
	}
	NavDialogDispose(fileDialog);
	return noErr;
}

pascal void PosEventHandler(ControlHandle control, SInt16 part)
{
	DWORD pos=GetControl32BitValue(control);
	BASS_ChannelPause(ochan[0]); // pause splitter streams (so that resumption following seek can be synchronized)
	BASS_ChannelSetPosition(chan,BASS_ChannelSeconds2Bytes(chan,pos),BASS_POS_BYTE); // set source position
	BASS_Split_StreamReset(chan); // reset buffers of all (both) the source's splitters
	BASS_ChannelPlay(ochan[0],FALSE); // resume playback
}

pascal void TimerProc(EventLoopTimerRef inTimer, void *inUserData)
{
	SetControl32BitValue(GetControl(12),(DWORD)BASS_ChannelBytes2Seconds(chan,BASS_ChannelGetPosition(chan,BASS_POS_BYTE))); // update position (using 1st splitter)
}

// Simple device selector dialog stuff begins here
pascal OSStatus SelectorEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	HICommand command;
	GetEventParameter(inEvent,kEventParamDirectObject,typeHICommand,NULL,sizeof (HICommand),NULL,&command);
	if (command.commandID=='ok  ')
		QuitAppModalLoopForWindow(inUserData);
	return noErr;
}

OSStatus SelectorItemDataCallback(ControlRef browser, DataBrowserItemID item, DataBrowserPropertyID property, DataBrowserItemDataRef itemData, Boolean setValue)
{
	if (!setValue && property=='blah') {
		BASS_DEVICEINFO i;
		BASS_GetDeviceInfo(item,&i);
		CFStringRef cs=CFStringCreateWithCString(0,i.name,0);
		SetDataBrowserItemDataText(itemData,cs);
		CFRelease(cs);
	}
	return noErr;
}

int SelectDevice(IBNibRef nibRef, CFStringRef title)
{
	WindowRef win;
    
	CreateWindowFromNib(nibRef, CFSTR("Selector"), &win);

	SetWindowTitleWithCFString(win,title);

	ControlRef db;
	ControlID cid={0,10};
	GetControlByID(win,&cid,&db);

	DataBrowserCallbacks dbc;
	dbc.version=kDataBrowserLatestCallbacks;
	InitDataBrowserCallbacks(&dbc);
	dbc.u.v1.itemDataCallback=SelectorItemDataCallback;
	SetDataBrowserCallbacks(db,&dbc);

	{
		BASS_DEVICEINFO i;
		int c;
		for (c=1;BASS_GetDeviceInfo(c,&i);c++) { // device 1 = 1st real device
			if (i.flags&BASS_DEVICE_ENABLED) // enabled, so add it...
				AddDataBrowserItems(db,kDataBrowserNoItem,1,&c,kDataBrowserItemNoProperty);
		}
	}

	EventTypeSpec etype={kEventClassCommand, kEventCommandProcess};
	InstallWindowEventHandler(win, NewEventHandlerUPP(SelectorEventHandler), 1, &etype, win, NULL);

	ShowWindow(win);
	RunAppModalLoopForWindow(win);

	DWORD sel;
	GetDataBrowserSelectionAnchor(db,(DataBrowserItemID*)&sel,(DataBrowserItemID*)&sel);

	DisposeWindow(win);
	
	return sel;
}
// Simple device selector dialog stuff ends here

int main(int argc, char* argv[])
{
	IBNibRef nibRef;
	OSStatus err;
    
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	err = CreateNibReference(CFSTR("multi"), &nibRef);
	if (err) return err;

	// Let the user choose the output devices
	outdev[0]=SelectDevice(nibRef,CFSTR("Select output device #1"));
	outdev[1]=SelectDevice(nibRef,CFSTR("Select output device #2"));

	// setup output devices
	if (!BASS_Init(outdev[0],44100,0,NULL,NULL)) {
		Error("Can't initialize device 1");
		return 0;
	}
	if (!BASS_Init(outdev[1],44100,0,NULL,NULL)) {
		Error("Can't initialize device 2");
		return 0;
	}

	err = CreateWindowFromNib(nibRef, CFSTR("Window"), &win);
	if (err) return err;
	DisposeNibReference(nibRef);

	SetupControlHandler(10,kEventControlHit,OpenEventHandler);
	SetControlAction(GetControl(12),NewControlActionUPP(PosEventHandler));

	EventLoopTimerRef timer;
	InstallEventLoopTimer(GetCurrentEventLoop(),kEventDurationNoWait,kEventDurationSecond/2,NewEventLoopTimerUPP(TimerProc),0,&timer);

	ShowWindow(win);
	RunApplicationEventLoop();

	// release both devices
	BASS_SetDevice(outdev[0]);
	BASS_Free();
	BASS_SetDevice(outdev[1]);
	BASS_Free();

	return 0; 
}
