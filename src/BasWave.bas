Attribute VB_Name = "BasWave"
Option Explicit

Public Declare Function sndPlaySoundA Lib "winmm" (lpszSoundName As Any, ByVal uFlags As Long) As Long
Public Declare Function mciSendStringA Lib "winmm.dll" (ByVal lpstrCommand As String, ByVal lpstrReturnString As String, ByVal uReturnLength As Long, ByVal hwndCallback As Long) As Long
Public Declare Function waveOutGetNumDevs Lib "winmm.dll" () As Long

Private m_WaveBuf() As Byte      ' 0x4B6100: must stay alive for async playback

Public Const SND_SYNC = &H0
Public Const SND_ASYNC = &H1
Public Const SND_NODEFAULT = &H2
Public Const SND_MEMORY = &H4
Public Const SND_FILENAME = &H20000

' === 52: PlayWave (0x4B51D0 / 0x4B5330) - CUSTOM resource WAVs 201..206 ===
Public Sub PlayWave(vResID As Variant, Optional vFlags As Variant)
    m_WaveBuf = LoadResData(vResID, "CUSTOM")
    If IsMissing(vFlags) Then
        vFlags = SND_NODEFAULT Or SND_MEMORY
    End If
    If (vFlags And SND_MEMORY) = 0 Then
        vFlags = vFlags Or SND_MEMORY
    End If
    sndPlaySoundA m_WaveBuf(0), vFlags
End Sub

' === 53: HasSoundCard (0x4B5410 / 0x4B5570) ===
Public Function HasSoundCard() As Boolean
    If CInt(waveOutGetNumDevs()) > 0 Then
        HasSoundCard = True
    Else
        HasSoundCard = False
    End If
End Function

' === 54: ExtractMidi (0x4B5440 / 0x4B55A0) - MIDI resource 101..103 -> file ===
Public Function ExtractMidi(ByVal resId As Integer, resType As String, outPath As String) As Boolean
    Dim buf() As Byte
    Dim fno As Integer
    On Error GoTo ErrHandler
    buf = LoadResData(resId, resType)
    fno = FreeFile
    Open outPath For Binary As #fno
    Put #fno, , buf
    Close #fno
    ExtractMidi = True
    Exit Function
ErrHandler:
    ExtractMidi = False
End Function
