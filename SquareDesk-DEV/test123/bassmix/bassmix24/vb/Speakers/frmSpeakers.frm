VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "comdlg32.ocx"
Begin VB.Form frmSpeakers 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "BASSmix multi-speaker example"
   ClientHeight    =   1260
   ClientLeft      =   45
   ClientTop       =   435
   ClientWidth     =   4710
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   1260
   ScaleWidth      =   4710
   StartUpPosition =   2  'CenterScreen
   Begin VB.CheckBox chkPairs 
      Caption         =   "4"
      Height          =   315
      Index           =   3
      Left            =   3720
      TabIndex        =   4
      Top             =   720
      Value           =   1  'Checked
      Width           =   495
   End
   Begin VB.CheckBox chkPairs 
      Caption         =   "3"
      Height          =   315
      Index           =   2
      Left            =   3000
      TabIndex        =   3
      Top             =   720
      Value           =   1  'Checked
      Width           =   495
   End
   Begin VB.CheckBox chkPairs 
      Caption         =   "2"
      Height          =   315
      Index           =   1
      Left            =   2280
      TabIndex        =   2
      Top             =   720
      Value           =   1  'Checked
      Width           =   495
   End
   Begin VB.CheckBox chkPairs 
      Caption         =   "1"
      Height          =   315
      Index           =   0
      Left            =   1680
      TabIndex        =   1
      Top             =   720
      Value           =   1  'Checked
      Width           =   495
   End
   Begin MSComDlg.CommonDialog cmdOpenFile 
      Left            =   4080
      Top             =   120
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton btnOpen 
      Caption         =   "click here to open a file..."
      Height          =   375
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   4455
   End
   Begin VB.Label Label1 
      AutoSize        =   -1  'True
      Caption         =   "Speaker pairs:"
      Height          =   195
      Left            =   480
      TabIndex        =   5
      Top             =   780
      Width           =   1020
   End
End
Attribute VB_Name = "frmSpeakers"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'////////////////////////////////////////////////////////////////////////////
' frmSpeakers.frm - Copyright (c) 2009 (: JOBnik! :) [Arthur Aminov, ISRAEL]
'                                                    [http://www.jobnik.org]
'                                                    [  jobnik@jobnik.org  ]
'
' BASSmix multi-speaker example
' Originally translated from - speakers.c - Example of Ian Luck
'////////////////////////////////////////////////////////////////////////////

Option Explicit

Dim mixer As Long, source As Long   ' mixer and source channels

' display error messages
Sub Error_(ByVal es As String)
    Call MsgBox(es & vbCrLf & vbCrLf & "error code: " & BASS_ErrorGetCode, vbExclamation, "Error")
End Sub

Private Sub Form_Load()
    ' change and set the current path, to prevent from VB not finding BASS.DLL
    ChDrive App.Path
    ChDir App.Path
    
    ' check the correct BASS was loaded
    If (HiWord(BASS_GetVersion) <> BASSVERSION) Then
        Call MsgBox("An incorrect version of BASS.DLL was loaded", vbCritical)
        End
    End If
    
    ' initialize BASS - default device
    If (BASS_Init(-1, 44100, 0, Me.hWnd, 0) = 0) Then
        Call Error_("Can't initialize device")
        End
    End If
End Sub

Private Sub Form_Unload(Cancel As Integer)
    Call BASS_Free
    End
End Sub

Private Sub btnOpen_Click()
    On Local Error Resume Next    ' if Cancel pressed...

    cmdOpenFile.CancelError = True
    cmdOpenFile.flags = cdlOFNExplorer Or cdlOFNFileMustExist Or cdlOFNHideReadOnly
    cmdOpenFile.DialogTitle = "Open"
    cmdOpenFile.Filter = "Streamable files|*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif|All files|*.*"
    cmdOpenFile.ShowOpen

    ' if cancel was pressed, exit the procedure
    If Err.Number = 32755 Then Exit Sub

    ' change path to default to avoid error, that BASSMIX.DLL isn't found in IDE mode.
    ChDrive App.Path
    ChDir App.Path

    Dim ci As BASS_CHANNELINFO
    Dim di As BASS_INFO

    Call BASS_StreamFree(mixer) ' free old mixer (and source due to AUTOFREE) before opening new

    source = BASS_StreamCreateFile(BASSFALSE, StrPtr(cmdOpenFile.filename), 0, 0, BASS_STREAM_DECODE Or BASS_SAMPLE_FLOAT Or BASS_SAMPLE_LOOP) ' create source

    If (source = 0) Then
        btnOpen.Caption = "click here to open a file..."
        Call Error_("Can't play the file")
        Exit Sub
    End If

    Call BASS_ChannelGetInfo(source, ci) ' get source info for sample rate
    Call BASS_GetInfo(di) ' get device info for speaker count

    mixer = BASS_Mixer_StreamCreate(ci.freq, min(di.speakers, 8), 0) ' create mixer with source sample rate and device speaker count

    If (mixer = 0) Then
        Call BASS_StreamFree(source)
        btnOpen.Caption = "click here to open a file..."
        Call Error_("Can't create mixer")
        Exit Sub
    End If

    Call BASS_Mixer_StreamAddChannel(mixer, source, BASS_MIXER_MATRIX Or BASS_STREAM_AUTOFREE) ' add the source to the mix with matrix-mixing enabled

    Call SetMatrix   ' set the matrix

    Call BASS_ChannelPlay(mixer, BASSFALSE) ' start playing
    btnOpen.Caption = cmdOpenFile.filename

    ' enable the speaker switches according to the speaker count
    chkPairs(1).Enabled = IIf(di.speakers >= 4, True, False)
    chkPairs(2).Enabled = IIf(di.speakers >= 6, True, False)
    chkPairs(3).Enabled = IIf(di.speakers >= 8, True, False)
End Sub

Private Sub chkPairs_Click(Index As Integer)
    Call SetMatrix   ' update the matrix
End Sub

Public Sub SetMatrix()
    Dim matrix() As Single
    Dim mi As BASS_CHANNELINFO, si As BASS_CHANNELINFO

    Call BASS_ChannelGetInfo(mixer, mi) ' get mixer info for channel count
    Call BASS_ChannelGetInfo(source, si) ' get source info for channel count

    ' allocate matrix (mixer channel count * source channel count)
    ' and initialize it to empty/silence
    ReDim matrix(mi.chans * si.chans * LenB(matrix(0))) As Single

    ' set the mixing matrix depending on the speaker switches
    ' mono & stereo sources are duplicated on each enabled pair of speakers

    If (chkPairs(0).value = vbChecked) Then ' 1st pair of speakers enabled
        matrix(0 * si.chans + 0) = 1
        If (si.chans = 1) Then ' mono source
            matrix(1 * si.chans + 0) = 1
        Else
            matrix(1 * si.chans + 1) = 1
        End If
    End If
    If (mi.chans >= 4 And chkPairs(1).value = vbChecked) Then ' 2nd pair of speakers enabled
        If (si.chans > 2) Then ' multi-channel source
            matrix(2 * si.chans + 2) = 1
            If (si.chans > 3) Then matrix(3 * si.chans + 3) = 1
        Else
            matrix(2 * si.chans + 0) = 1
            If (si.chans = 1) Then ' mono source
                matrix(3 * si.chans + 0) = 1
            Else ' stereo source
                matrix(3 * si.chans + 1) = 1
            End If
        End If
    End If
    If (mi.chans >= 6 And chkPairs(2).value = vbChecked) Then ' 3rd pair of speakers enabled
        If (si.chans > 2) Then ' multi-channel source
            If (si.chans > 4) Then matrix(4 * si.chans + 4) = 1
            If (si.chans > 5) Then matrix(5 * si.chans + 5) = 1
        Else
            matrix(4 * si.chans + 0) = 1
            If (si.chans = 1) Then ' mono source
                matrix(5 * si.chans + 0) = 1
            Else ' stereo source
                matrix(5 * si.chans + 1) = 1
            End If
        End If
    End If
    If (mi.chans >= 8 And chkPairs(3).value = vbChecked) Then ' 4th pair of speakers enabled
        If (si.chans > 2) Then ' multi-channel source
            If (si.chans > 6) Then matrix(6 * si.chans + 6) = 1
            If (si.chans > 7) Then matrix(7 * si.chans + 7) = 1
        Else
            matrix(6 * si.chans + 0) = 1
            If (si.chans = 1) Then ' mono source
                matrix(7 * si.chans + 0) = 1
            Else ' stereo source
                matrix(7 * si.chans + 1) = 1
            End If
        End If
    End If
    Call BASS_Mixer_ChannelSetMatrix(source, matrix(0)) ' apply the matrix
    Erase matrix
End Sub

' useful functions
Public Function min(ByVal a As Double, ByVal b As Double) As Double
    min = IIf(a < b, a, b)
End Function
