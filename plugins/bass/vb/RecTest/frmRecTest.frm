VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "mscomctl.ocx"
Begin VB.Form frmRecTest 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "BASSWASAPI recording test"
   ClientHeight    =   1620
   ClientLeft      =   600
   ClientTop       =   990
   ClientWidth     =   4755
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   108
   ScaleMode       =   3  'Pixel
   ScaleWidth      =   317
   StartUpPosition =   2  'CenterScreen
   Begin VB.PictureBox picLevelBar 
      Appearance      =   0  'Flat
      AutoRedraw      =   -1  'True
      BackColor       =   &H00C0C0C0&
      ForeColor       =   &H80000008&
      Height          =   1380
      Left            =   4560
      ScaleHeight     =   90
      ScaleMode       =   3  'Pixel
      ScaleWidth      =   6
      TabIndex        =   10
      Top             =   120
      Width           =   120
   End
   Begin VB.ComboBox cmbRate 
      Height          =   315
      Left            =   480
      Style           =   2  'Dropdown List
      TabIndex        =   9
      Top             =   840
      Width           =   1335
   End
   Begin MSComctlLib.Slider sldInputLevel 
      Height          =   255
      Left            =   2640
      TabIndex        =   5
      Top             =   480
      Width           =   1815
      _ExtentX        =   3201
      _ExtentY        =   450
      _Version        =   393216
      Max             =   100
      SelectRange     =   -1  'True
      TickStyle       =   3
   End
   Begin VB.ComboBox cmbInput 
      Height          =   315
      Left            =   120
      Style           =   2  'Dropdown List
      TabIndex        =   3
      Top             =   120
      Width           =   4335
   End
   Begin MSComDlg.CommonDialog cmd 
      Left            =   480
      Top             =   1200
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.Timer tmrRecTest 
      Enabled         =   0   'False
      Interval        =   100
      Left            =   0
      Top             =   1200
   End
   Begin VB.CommandButton btnSave 
      Caption         =   "Save"
      Enabled         =   0   'False
      Height          =   300
      Left            =   3480
      TabIndex        =   2
      Top             =   1200
      Width           =   975
   End
   Begin VB.CommandButton btnPlay 
      Caption         =   "Play"
      Enabled         =   0   'False
      Height          =   300
      Left            =   3480
      TabIndex        =   1
      Top             =   840
      Width           =   975
   End
   Begin VB.CommandButton btnRecord 
      Caption         =   "Record"
      Height          =   300
      Left            =   1800
      TabIndex        =   0
      Top             =   840
      Width           =   1575
   End
   Begin VB.Label Label1 
      AutoSize        =   -1  'True
      Caption         =   "rate:"
      Height          =   195
      Left            =   120
      TabIndex        =   8
      Top             =   900
      Width           =   315
   End
   Begin VB.Label lblVolume 
      AutoSize        =   -1  'True
      Caption         =   "volume:"
      Height          =   195
      Left            =   2040
      TabIndex        =   7
      Top             =   480
      Width           =   555
   End
   Begin VB.Label lblPos 
      Alignment       =   2  'Center
      BorderStyle     =   1  'Fixed Single
      Height          =   285
      Left            =   120
      TabIndex        =   6
      Top             =   1200
      Width           =   3255
   End
   Begin VB.Label lblInputType 
      Caption         =   "type: "
      Height          =   195
      Left            =   120
      TabIndex        =   4
      Top             =   480
      Width           =   1920
   End
End
Attribute VB_Name = "frmRecTest"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'///////////////////////////////////////////////////////////////////////////
' frmRecTest.frm - Copyright (c) 2013 (: JOBnik! :) [Arthur Aminov, ISRAEL]
'                                                   [http://www.jobnik.org]
'                                                   [  jobnik@jobnik.org  ]
'
' Other source: modRecTest.bas
'
' BASSWASAPI Recording example
' Originally translated from - rectest.c - Example of Ian Luck
'///////////////////////////////////////////////////////////////////////////

Option Explicit

Dim init As Boolean ' to prevent initializing a device twice

Private Sub Form_Load()
    ' change and set the current path, to prevent from VB not finding BASS.DLL
    ChDrive App.Path
    ChDir App.Path

    ' check the correct BASS was loaded
    If (HiWord(BASS_GetVersion) <> BASSVERSION) Then
        Call MsgBox("An incorrect version of BASS.DLL was loaded", vbCritical)
        End
    End If

    indev = -1
    outdev = -1

    ' not playing anything via BASS, so don't need an update thread
    Call BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0)

    ' setup BASS - "no sound" device
    Call BASS_Init(0, 48000, 0, 0, 0)

    ' get list of WASAPI input devices
    Dim c As Integer, i As Integer, di As BASS_WASAPI_DEVICEINFO
    c = 0
    i = 0
    While BASS_WASAPI_GetDeviceInfo(c, di)
        If ((di.flags And BASS_DEVICE_INPUT) = BASS_DEVICE_INPUT And (di.flags And BASS_DEVICE_ENABLED) = BASS_DEVICE_ENABLED) Then ' it's an enabled input device
            With cmbInput
                .AddItem VBStrFromAnsiPtr(di.name)
                i = .ListCount - 1
                .ItemData(i) = c    ' retain device # for later
            End With
            If ((di.flags And BASS_DEVICE_DEFAULT) = BASS_DEVICE_DEFAULT) Then ' it's the default
                indev = c
                ' initialize device
                init = True
                cmbInput.ListIndex = i  ' will call cmbInput_Click() sub
            End If
        End If
        c = c + 1
    Wend

    init = False

    If (indev = -1) Then
        Call Error_("Can't find any WASAPI input devices")
        Unload Me
    End If

    ' initialize default WASAPI output device for playback
    If (BASS_WASAPI_Init(-1, 0, 0, 0, 0.4, 0.05, AddressOf OutWasapiProc, 0)) Then
        Dim wi As BASS_WASAPI_INFO
        outdev = BASS_WASAPI_GetDevice()
        ' create a mixer to feed the output device
        Call BASS_WASAPI_GetInfo(wi)
        outmixer = BASS_Mixer_StreamCreate(wi.freq, wi.chans, BASS_SAMPLE_FLOAT Or BASS_STREAM_DECODE Or BASS_MIXER_END Or BASS_MIXER_POSEX)
    End If

    ' initialize sample rate list
    With cmbRate
        .AddItem "96000"
        .AddItem "48000"
        .AddItem "44100"
        .AddItem "32000"
        .AddItem "22050"
        .ListIndex = 1
    End With

    tmrRecTest.Enabled = True   ' timer to update the position & level display (100ms)

    recPtr = 0
    reclen = 0
    BUFSTEP = 200000    ' memory allocation unit
End Sub

Private Sub Form_Unload(Cancel As Integer)
    ' release all BASS/WASAPI stuff
    While (BASS_WASAPI_Free()):: Wend
    Call BASS_Free
    If (recPtr) Then Call GlobalFree(ByVal recPtr)
End Sub

' input selection changed
Private Sub cmbInput_Click()
    If (Not init) Then
        Dim i As Integer
        ' free current device and its mixer feeder
        Call BASS_WASAPI_SetDevice(indev)
        Call BASS_WASAPI_Free
        Call BASS_StreamFree(instream)
        instream = 0
        i = cmbInput.ListIndex  ' get the selected device
        indev = cmbInput.ItemData(i)    ' get the device #
    End If
    Call InitInputDevice   ' initialize device
End Sub

Private Sub btnPlay_Click()
    Call StartPlaying
End Sub

Private Sub btnRecord_Click()
    If (inmixer = 0) Then
        Call StartRecording
    Else
        Call StopRecording
    End If
End Sub

Private Sub btnSave_Click()
    Call WriteToDisk
End Sub

' set device volume level
Private Sub sldInputLevel_Scroll()
    Dim level As Single
    level = sldInputLevel.value / 100
    If (BASS_WASAPI_SetDevice(indev)) Then _
        Call BASS_WASAPI_SetVolume(BASS_WASAPI_CURVE_WINDOWS, level)
End Sub

Private Sub tmrRecTest_Timer()
    ' update the recording/playback counter
    Dim text As String
    If (outstream) Then
        Call BASS_WASAPI_SetDevice(outdev)
        If (BASS_WASAPI_IsStarted()) Then ' playing
            Dim pos As Long, delay As Long
            Call BASS_WASAPI_Lock(BASSTRUE) ' prevent processing mid-calculation
            delay = BASS_WASAPI_GetData(0, BASS_DATA_AVAILABLE) ' get amount of buffered data
            pos = BASS_Mixer_ChannelGetPositionEx(outstream, BASS_POS_BYTE, delay) ' get source position at that point
            Call BASS_WASAPI_Lock(BASSFALSE)
            text = pos & " / " & BASS_ChannelGetLength(outstream, BASS_POS_BYTE)
        Else
            text = BASS_ChannelGetLength(outstream, BASS_POS_BYTE)
        End If
    ElseIf (inmixer) Then ' recording
        text = BASS_ChannelGetPosition(inmixer, BASS_POS_BYTE)
    End If
    lblPos.Caption = text

    ' update the input level meter
    Dim level As Single
    level = BASS_WASAPI_GetDeviceLevel(indev, -1)
    inlevel = IIf(inlevel > 0.1, inlevel - 0.1, 0)
    If (level > inlevel) Then inlevel = level

    ' draw the level bar (form's ScaleMode should be "Pixels")
    picLevelBar.Cls
    picLevelBar.Line (picLevelBar.Width, picLevelBar.Height)-(0, picLevelBar.Height * (1 - inlevel)), vbWhite, BF
End Sub
