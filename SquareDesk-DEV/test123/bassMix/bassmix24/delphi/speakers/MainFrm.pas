unit MainFrm;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, Bass, bassmix, Math;

type
  TForm1 = class(TForm)
    btnOpen: TButton;
    lblSpeakers: TLabel;
    cbPair1: TCheckBox;
    cbPair2: TCheckBox;
    cbPair3: TCheckBox;
    cbPair4: TCheckBox;
    OpenDlg: TOpenDialog;
    procedure FormCreate(Sender: TObject);
    procedure btnOpenClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure cbPair1Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    function SetMixer: integer;
  end;

var
  Form1: TForm1;
  stream, mixer : HSTREAM;     //Set global variables for Stream and Mixer channels
  BassInfo : BASS_INFO;
  ChannelInfo : BASS_CHANNELINFO;

implementation

{$R *.dfm}

function Error(ErrMsg : string): integer;
begin
  MessageDlg(Format('Unhandled Error!' + #10#13 + 'Error Code: %d' + #10#13 + 'Error Message: %s',[BASS_ErrorGetCode, Errmsg]), mtError, [mbOK], 0);
end;

procedure TForm1.btnOpenClick(Sender: TObject);
begin
  if OpenDlg.Execute then
    begin
      if BASS_ChannelIsActive(stream) > 0  then      // If Channel Is Active then Stop Playing
        BASS_ChannelStop(stream);
      if stream <> 0 then                       // If Channel is Initialized then Remove it from Mixer and Free it
        begin
          BASS_Mixer_ChannelRemove(stream);
          BASS_StreamFree(stream);
        end;

      stream := BASS_StreamCreateFile(false, PChar(OpenDlg.FileName), 0, 0, BASS_STREAM_DECODE or BASS_SAMPLE_FLOAT or BASS_SAMPLE_LOOP {$IFDEF UNICODE} or BASS_UNICODE {$ENDIF});             // Create Stream From File
      if stream = 0 then
        begin
          Error('Can''t Load File!');
          exit;
        end;

      BASS_ChannelGetInfo(stream, ChannelInfo);            // Get Channel Info

      BASS_Mixer_StreamAddChannel(mixer, stream, BASS_MIXER_MATRIX or BASS_STREAM_AUTOFREE);   // Add Stream to mixer

      SetMixer;                 // Set Mixing Matrix

      BASS_ChannelPlay(mixer, false);         // Play Mixer
    end;
end;

procedure TForm1.cbPair1Click(Sender: TObject);
begin
  SetMixer;               // Update Mixing Matrix on Checkbox click
end;

procedure TForm1.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  BASS_Free;
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
  if not BASS_Init(-1, 44100, 0, Application.Handle, nil) then   // Init Bass on Default Output @ 44100 Hz
    begin
      Error('Can''t Initialize Bass.');
      Application.Terminate;
      Exit;
    end;
  if HiWord(BASS_GetVersion) <> BASSVERSION then     //Check for correct Bass Version
    begin
      Error('Wrong Bass Version! Bass 2.4.3.1 Needed!');
      Application.Terminate;
      Exit;
    end;

  BASS_GetInfo(BassInfo);                                   // Get Device Info (to find out how many output channels does the soundcard support)
  mixer := BASS_Mixer_StreamCreate(44100, Min(BassInfo.speakers, 8), 0);     //Create Mixer Stream

 // BASS_ChannelPlay(mixer, false);

  cbPair2.Enabled := Trunc(BASSInfo.Speakers) >=4;
  cbPair3.Enabled := Trunc(BASSInfo.Speakers) >=6;
  cbPair4.Enabled := Trunc(BASSInfo.Speakers) >=8;

  cbPair2.Checked := Trunc(BASSInfo.Speakers) >=4;
  cbPair3.Checked := Trunc(BASSInfo.Speakers) >=6;
  cbPair4.Checked := Trunc(BASSInfo.Speakers) >=8;

  BASS_SetConfig(BASS_CONFIG_BUFFER, 200);
end;

function TForm1.SetMixer: integer;
var
  mi, si : BASS_CHANNELINFO;
  matrix : array of single;
begin
  if stream = 0 then           // if stream is not loaded then prevent the function execution
    exit;

  BASS_ChannelGetInfo(mixer, mi);        // Get Mixer Info for channel count
  BASS_ChannelGetInfo(stream, si);       // Get Source Info for channel count

  SetLength(matrix, Min(BassInfo.speakers, 8) * ChannelInfo.chans);       // allocate matrix (mixer channel count * source channel count)
  FillMemory(matrix, Min(BassInfo.speakers, 8) * ChannelInfo.chans * SizeOf(single), 0);        // Initialize matrix to empty/silence

{
	set the mixing matrix depending on the speaker switches
	mono & stereo sources are duplicated on each enabled pair of speakers
}

  if cbPair1.Checked then begin  // 1st pair of speakers enabled
    matrix[0*si.chans+0] := 1;
    if si.chans = 1 then       // mono source
      matrix[1*si.chans+0] := 1
    else
      matrix[1*si.chans+1] := 1
  end;

  if (mi.chans >= 4) and (cbPair2.Checked) then begin     // 2nd pair of speakers enabled
    if si.chans>2 then begin                // multi-channel source
      matrix[2*si.chans+2] := 1;
      if si.chans>3 then
        matrix[3*si.chans+3] := 1;
    end else begin
      matrix[2*si.chans+0] := 1;
      if si.chans = 1 then            // mono source
        matrix[3*si.chans+0] := 1
      else                            //stereo source
        matrix[3*si.chans+1] := 1;
    end;
  end;

  if (mi.chans >= 6) and (cbPair3.Checked) then begin
    if si.chans > 2 then begin                // multi-channel source
      if si.chans > 4 then matrix[4*si.chans+4] := 1;
      if si.chans > 5 then matrix[5*si.chans+5] := 1;
    end else begin
      matrix[4*si.chans+0] := 1;
      if si.chans = 1 then          // mono source
        matrix[5*si.chans+0] := 1
      else                          // stereo source
        matrix[5*si.chans+1] := 1;
    end;
  end;

  if (mi.chans >= 8) and (cbPair4.Checked) then begin
    if si.chans > 2 then begin           // multi-channel source
      if si.chans > 6 then matrix[6*si.chans+6] := 1;
      if si.chans > 7 then matrix[7*si.chans+7] := 1;
    end else begin
      matrix[6*si.chans+0] := 1;
      if si.chans = 1 then             // mono source
        matrix[7*si.chans+0] := 1
      else                              // stereo source
        matrix[7*si.chans+1] := 1;
    end;
  end;

  BASS_Mixer_ChannelSetMatrix(stream, matrix);     // apply the matrix
end;

end.
