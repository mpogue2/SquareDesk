/*
	BASSmix multiple output example
	Copyright (c) 2009-2015 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include "bass.h"
#include "bassmix.h"

HWND win=NULL;

DWORD outdev[2];		// output devices
HSTREAM chan;			// the source stream
HSTREAM ochan[2]={0};	// the output/splitter streams

// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	MessageBox(win,mes,0,0);
}

#define MESS(id,m,w,l) SendDlgItemMessage(win,id,m,(WPARAM)(w),(LPARAM)(l))

BOOL CreateClone()
{
	// set the device to create 2nd splitter stream on, and then create it
	BASS_SetDevice(outdev[1]); 
	if (!(ochan[1]=BASS_Split_StreamCreate(chan,0,NULL))) {
		Error("Can't create splitter");
		return FALSE;
	}
	BASS_ChannelSetLink(ochan[0],ochan[1]); // link the splitters to start together
	return TRUE;
}

INT_PTR CALLBACK dialogproc(HWND h,UINT m,WPARAM w,LPARAM l)
{
	static OPENFILENAME ofn;

	switch (m) {
		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDCANCEL:
					DestroyWindow(h);
					break;
				case 10: // open a file to play
					{
						char file[MAX_PATH]="";
						ofn.lpstrFilter="streamable files\0*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif\0All files\0*.*\0\0";
						ofn.lpstrFile=file;
						if (GetOpenFileName(&ofn)) {
							BASS_StreamFree(chan); // free old stream (splitters automatically freed too)
							if (!(chan=BASS_StreamCreateFile(FALSE,file,0,0,BASS_STREAM_DECODE|BASS_SAMPLE_LOOP))) {
								MESS(10,WM_SETTEXT,0,"click here to open a file...");
								Error("Can't play the file");
								break;
							}
							// set the device to create 1st splitter stream on, and then create it
							BASS_SetDevice(outdev[0]);
							if (!(ochan[0]=BASS_Split_StreamCreate(chan,0,NULL))) {
								BASS_StreamFree(chan);
								MESS(10,WM_SETTEXT,0,"click here to open a file...");
								Error("Can't create splitter");
							}
							if (MESS(11,BM_GETCHECK,0,0))
								CreateClone(); // create a clone
							else
								ochan[1]=0; // no clone
							MESS(10,WM_SETTEXT,0,file);
							{ // update scroller range
								QWORD bytes=BASS_ChannelGetLength(chan,BASS_POS_BYTE);
								DWORD secs=BASS_ChannelBytes2Seconds(chan,bytes);
								MESS(12,TBM_SETRANGE,1,MAKELONG(0,secs));
							}
							BASS_ChannelPlay(ochan[0],FALSE); // start playback
						}
					}
					break;
				case 11:
					if (!ochan[0]) break;
					if (MESS(11,BM_GETCHECK,0,0)) { // create clone on device #2
						if (!ochan[1] && CreateClone()) {
							int offset;
							BASS_INFO info;
							BASS_GetInfo(&info);
							offset=BASS_Split_StreamGetAvailable(ochan[0]) // get the amount of data the 1st splitter has buffered
								+BASS_ChannelGetData(ochan[0],NULL,BASS_DATA_AVAILABLE) // add the amount in its playback buffer
								-BASS_ChannelSeconds2Bytes(ochan[0],info.latency/1000.0); // subtract the device's playback delay
							if (offset<0) offset=0; // just in case
							BASS_Split_StreamResetEx(ochan[1],offset); // set the new splitter that far back in the source buffer
							BASS_ChannelPlay(ochan[1],FALSE); // start the clone
						}
					} else { // remove clone on device #2
						BASS_StreamFree(ochan[1]);
						ochan[1]=0;
					}
					break;
			}
			break;

		case WM_HSCROLL:
			if (l && LOWORD(w)!=SB_THUMBPOSITION && LOWORD(w)!=SB_ENDSCROLL) { // set the position
				int pos=SendMessage((HWND)l,TBM_GETPOS,0,0);
				BASS_ChannelPause(ochan[0]); // pause splitter streams (so that resumption following seek can be synchronized)
				BASS_ChannelSetPosition(chan,BASS_ChannelSeconds2Bytes(chan,pos),BASS_POS_BYTE); // set source position
				BASS_Split_StreamReset(chan); // reset buffers of all (both) the source's splitters
				BASS_ChannelPlay(ochan[0],FALSE); // resume playback
			}
			break;

		case WM_TIMER:
			MESS(12,TBM_SETPOS,1,(DWORD)BASS_ChannelBytes2Seconds(ochan[0],BASS_ChannelGetPosition(ochan[0],BASS_POS_BYTE))); // update position (using 1st splitter)
			break;

		case WM_INITDIALOG:
			win=h;
			memset(&ofn,0,sizeof(ofn));
			ofn.lStructSize=sizeof(ofn);
			ofn.hwndOwner=h;
			ofn.nMaxFile=MAX_PATH;
			ofn.Flags=OFN_HIDEREADONLY|OFN_EXPLORER;
			// initialize output devices (and measure latencies)
			if (!BASS_Init(outdev[0],44100,BASS_DEVICE_LATENCY,win,NULL)) {
				Error("Can't initialize device 1");
				DestroyWindow(win);
			}
			if (!BASS_Init(outdev[1],44100,BASS_DEVICE_LATENCY,win,NULL)) {
				Error("Can't initialize device 2");
				DestroyWindow(win);
			}
			SetTimer(h,0,500,0); // timer to update the position
			return 1;

		case WM_DESTROY:
			// release both devices
			BASS_SetDevice(outdev[0]);
			BASS_Free();
			BASS_SetDevice(outdev[1]);
			BASS_Free();
			break;
	}
	return 0;
}


// Simple device selector dialog stuff begins here
INT_PTR CALLBACK devicedialogproc(HWND h,UINT m,WPARAM w,LPARAM l)
{
	switch (m) {
		case WM_COMMAND:
			switch (LOWORD(w)) {
				case 10:
					if (HIWORD(w)!=LBN_DBLCLK) break;
				case IDOK:
					{
						int device=SendDlgItemMessage(h,10,LB_GETCURSEL,0,0);
						device=SendDlgItemMessage(h,10,LB_GETITEMDATA,device,0); // get device #
						EndDialog(h,device);
					}
					break;
			}
			break;

		case WM_INITDIALOG:
			{
				char text[30];
				BASS_DEVICEINFO i;
				int c;
				sprintf(text,"Select output device #%d",l);
				SetWindowText(h,text);
				for (c=1;BASS_GetDeviceInfo(c,&i);c++) { // device 1 = 1st real device
					if (i.flags&BASS_DEVICE_ENABLED) { // enabled, so add it...
						int idx=SendDlgItemMessage(h,10,LB_ADDSTRING,0,(LPARAM)i.name);
						SendDlgItemMessage(h,10,LB_SETITEMDATA,idx,c); // store device #
					}
				}
				SendDlgItemMessage(h,10,LB_SETCURSEL,0,0);
			}
			return 1;
	}
	return 0;
}
// Device selector stuff ends here

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int nCmdShow)
{
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		MessageBox(0,"An incorrect version of BASS.DLL was loaded",0,MB_ICONERROR);
		return 0;
	}

	// Let the user choose the output devices
	outdev[0]=DialogBoxParam(hInstance,(char*)2000,0,&devicedialogproc,1);
	outdev[1]=DialogBoxParam(hInstance,(char*)2000,0,&devicedialogproc,2);

	DialogBox(hInstance,(char*)1000,0,&dialogproc);

	return 0;
}

