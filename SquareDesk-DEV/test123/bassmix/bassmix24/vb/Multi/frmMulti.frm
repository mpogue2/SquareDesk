VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "comdlg32.ocx"
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Begin VB.Form frmMulti 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "BASSmix multiple output example"
   ClientHeight    =   1410
   ClientLeft      =   45
   ClientTop       =   435
   ClientWidth     =   4695
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   1410
   ScaleWidth      =   4695
   StartUpPosition =   2  'CenterScreen
   Begin ComctlLib.Slider sldPosition 
      Height          =   555
      Left            =   360
      TabIndex        =   1
      Top             =   720
      Width           =   3855
      _ExtentX        =   6800
      _ExtentY        =   979
      _Version        =   327682
      Max             =   1
      TickStyle       =   2
      TickFrequency   =   0
   End
   Begin VB.Timer tmrMulti 
      Interval        =   500
      Left            =   4200
      Top             =   840
   End
   Begin MSComDlg.CommonDialog cmd 
      Left            =   4080
      Top             =   120
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdOpen 
      Caption         =   "click here to open a file..."
      Height          =   375
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   4455
   End
End
Attribute VB_Name = "frmMulti"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'/////////////////////////////////////////////////////////////////////////
' frmMulti.frm - Copyright (c) 2009 (: JOBnik! :) [Arthur Aminov, ISRAEL]
'                                                 [http://www.jobnik.org]
'                                                 [  jobnik@jobnik.org  ]
' Other sources: frmDevice.frm
'
' BASSmix multiple output example
' Originally translated from - multi.c - Example of Ian Luck
'/////////////////////////////////////////////////////////////////////////
 
Option Explicit

Dim outdev(2) As Long   ' output devices
Dim chan As Long        ' the source stream
Dim ochan(2) As Long    ' the output/splitter streams

' display error messages
Sub Error_(ByVal es As String)
    Call MsgBox(es & vbCrLf & vbCrLf & "Error Code : " & BASS_ErrorGetCode, vbExclamation, "Error")
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

    ' Let the user choose the output devices
    With frmDevice
        .SelectDevice 1
        .Show vbModal, Me
        outdev(0) = .device
        .SelectDevice 2
        .Show vbModal, Me
        outdev(1) = .device
    End With

    ' initialize output devices
    If (BASS_Init(outdev(0), 44100, 0, Me.hWnd, 0) = 0) Then
        Call Error_("Can't initialize device 1")
        Unload Me
    End If

    If (BASS_Init(outdev(1), 44100, 0, Me.hWnd, 0) = 0) Then
        Call Error_("Can't initialize device 2")
        Unload Me
    End If
End Sub

Private Sub Form_Unload(Cancel As Integer)
    ' release both devices
    Call BASS_SetDevice(outdev(0))
    Call BASS_Free
    Call BASS_SetDevice(outdev(1))
    Call BASS_Free
    End
End Sub

Private Sub cmdOpen_Click()
    On Local Error Resume Next    ' if Cancel pressed...

    ' open a file to play on selected device
    cmd.CancelError = True
    cmd.flags = cdlOFNExplorer Or cdlOFNFileMustExist Or cdlOFNHideReadOnly
    cmd.DialogTitle = "Open"
    cmd.Filter = "streamable files|*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif|All files|*.*"
    cmd.ShowOpen

    ' if cancel was pressed, exit the procedure
    If Err.Number = 32755 Then Exit Sub

    ' change path to default to avoid error, that BASSMIX.DLL isn't found in IDE mode.
    ChDrive App.Path
    ChDir App.Path

    Call BASS_StreamFree(chan)   ' free old stream (splitters automatically freed too)
    chan = BASS_StreamCreateFile(BASSFALSE, StrPtr(cmd.filename), 0, 0, BASS_STREAM_DECODE Or BASS_SAMPLE_LOOP)
    If (chan = 0) Then
        cmdOpen.Caption = "click here to open a file..."
        Call Error_("Can't play the file")
        Exit Sub
    End If

    ' set the device to create 1st splitter stream on, and then create it
    Call BASS_SetDevice(outdev(0))
    ochan(0) = BASS_Split_StreamCreate(chan, 0, ByVal 0)
    If (ochan(0) = 0) Then
        cmdOpen.Caption = "click here to open a file..."
        Call Error_("Can't create splitter")
        Call BASS_StreamFree(chan)
        Exit Sub
    End If

    ' set the device to create 2nd splitter stream on, and then create it
    Call BASS_SetDevice(outdev(1))
    ochan(1) = BASS_Split_StreamCreate(chan, 0, ByVal 0)
    If (ochan(1) = 0) Then
        cmdOpen.Caption = "click here to open a file..."
        Call Error_("Can't create splitter")
        Call BASS_StreamFree(chan)
        Exit Sub
    End If

    cmdOpen.Caption = cmd.filename
    
    ' update scroller range
    Dim bytes As Long, secs As Long
    bytes = BASS_ChannelGetLength(chan, BASS_POS_BYTE)
    secs = BASS_ChannelBytes2Seconds(chan, bytes)
    sldPosition.min = 0
    sldPosition.max = secs

    Call BASS_ChannelSetLink(ochan(0), ochan(1)) ' link the splitters so that they stop/start together
    Call BASS_ChannelPlay(ochan(0), BASSFALSE) ' start playback
End Sub

' set the position
Private Sub sldPosition_Scroll()
    Call BASS_ChannelPause(ochan(0)) ' pause splitter streams (so that resumption following seek can be synchronized)
    Call BASS_ChannelSetPosition(chan, BASS_ChannelSeconds2Bytes(chan, sldPosition.value), BASS_POS_BYTE) ' set source position
    Call BASS_Split_StreamReset(chan) ' reset buffers of all (both) the source's splitters
    Call BASS_ChannelPlay(ochan(0), BASSFALSE) ' resume playback
End Sub

Private Sub tmrMulti_Timer()
    sldPosition.value = BASS_ChannelBytes2Seconds(ochan(0), BASS_ChannelGetPosition(ochan(0), BASS_POS_BYTE)) ' update position (using 1st splitter)
End Sub
