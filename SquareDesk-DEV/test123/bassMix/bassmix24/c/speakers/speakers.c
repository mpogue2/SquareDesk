/*
	BASSmix multi-speaker example
	Copyright (c) 2009 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <stdio.h>
#include "bass.h"
#include "bassmix.h"

HWND win=NULL;

HSTREAM mixer,source; // mixer and source channels

// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	MessageBox(win,mes,0,0);
}

#define MESS(id,m,w,l) SendDlgItemMessage(win,id,m,(WPARAM)(w),(LPARAM)(l))

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
	if (MESS(11,BM_GETCHECK,0,0)) { // 1st pair of speakers enabled
		matrix[0*si.chans+0]=1;
		if (si.chans==1) // mono source
			matrix[1*si.chans+0]=1;
		else
			matrix[1*si.chans+1]=1;
	}
	if (mi.chans>=4 && MESS(12,BM_GETCHECK,0,0)) { // 2nd pair of speakers enabled
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
	if (mi.chans>=6 && MESS(13,BM_GETCHECK,0,0)) { // 3rd pair of speakers enabled
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
	if (mi.chans>=8 && MESS(14,BM_GETCHECK,0,0)) { // 4th pair of speakers enabled
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

INT_PTR CALLBACK dialogproc(HWND h,UINT m,WPARAM w,LPARAM l)
{
	static OPENFILENAME ofn;

	switch (m) {
		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDCANCEL:
					DestroyWindow(h);
					return 1;
				case 10: // open a file to play
					{
						char file[MAX_PATH]="";
						ofn.lpstrFile=file;
						if (GetOpenFileName(&ofn)) {
							BASS_CHANNELINFO ci;
							BASS_INFO di;
							BASS_StreamFree(mixer); // free old mixer (and source due to AUTOFREE) before opening new
							source=BASS_StreamCreateFile(FALSE,file,0,0,BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT|BASS_SAMPLE_LOOP); // create source
							if (!source) { // failed
								MESS(10,WM_SETTEXT,0,"click here to open a file...");
								Error("Can't play the file");
								return 1;
							}
							BASS_ChannelGetInfo(source,&ci); // get source info for sample rate
							BASS_GetInfo(&di); // get device info for speaker count
							mixer=BASS_Mixer_StreamCreate(ci.freq,min(di.speakers,8),0); // create mixer with source sample rate and device speaker count
							if (!mixer) { // failed
								BASS_StreamFree(source);
								MESS(10,WM_SETTEXT,0,"click here to open a file...");
								Error("Can't create mixer");
								return 1;
							}
							BASS_Mixer_StreamAddChannel(mixer,source,BASS_MIXER_MATRIX|BASS_STREAM_AUTOFREE); // add the source to the mix with matrix-mixing enabled
							SetMatrix(); // set the matrix
							BASS_ChannelPlay(mixer,FALSE); // start playing
							MESS(10,WM_SETTEXT,0,file);
							// enable the speaker switches according to the speaker count
							EnableWindow(GetDlgItem(h,12),di.speakers>=4);
							EnableWindow(GetDlgItem(h,13),di.speakers>=6);
							EnableWindow(GetDlgItem(h,14),di.speakers>=8);
						}
					}

				case 11: // 1st speakers switch
				case 12: // 2nd speakers switch
				case 13: // 3rd speakers switch
				case 14: // 4th speakers switch
					SetMatrix(); // update the matrix
					return 1;
			}
			break;

		case WM_INITDIALOG:
			win=h;
			memset(&ofn,0,sizeof(ofn));
			ofn.lStructSize=sizeof(ofn);
			ofn.hwndOwner=h;
			ofn.nMaxFile=MAX_PATH;
			ofn.Flags=OFN_HIDEREADONLY|OFN_EXPLORER;
			ofn.lpstrFilter="Streamable files\0*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif\0All files\0*.*\0\0";
			// initialize default device
			if (!BASS_Init(-1,44100,0,win,NULL)) {
				Error("Can't initialize device");
				DestroyWindow(win);
				break;
			}
			MESS(11,BM_SETCHECK,BST_CHECKED,0);
			MESS(12,BM_SETCHECK,BST_CHECKED,0);
			MESS(13,BM_SETCHECK,BST_CHECKED,0);
			MESS(14,BM_SETCHECK,BST_CHECKED,0);
			return 1;

		case WM_DESTROY:
			BASS_Free();
			break;
	}
	return 0;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int nCmdShow)
{
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		MessageBox(0,"An incorrect version of BASS.DLL was loaded",0,MB_ICONERROR);
		return 0;
	}

	DialogBox(hInstance,(char*)1000,0,&dialogproc);

	return 0;
}
