Attribute VB_Name = "Mainbas"
Option Explicit

' === API declares recovered from the original declare tables (0x48E41C..0x48E7E0) ===
Public Declare Function GetTickCount Lib "kernel32" () As Long
Public Declare Function GetSystemMenu Lib "User32" (ByVal hwnd As Long, ByVal bRevert As Long) As Long
Public Declare Function RemoveMenu Lib "User32" (ByVal hMenu As Long, ByVal nPosition As Long, ByVal wFlags As Long) As Long
Public Declare Function GetWindowsDirectoryA Lib "kernel32" (ByVal lpBuffer As String, ByVal nSize As Long) As Long
Public Declare Function GetTempPathA Lib "kernel32.dll" (ByVal nBufferLength As Long, ByVal lpBuffer As String) As Long
Public Declare Function ShellExecuteA Lib "shell32.dll" (ByVal hwnd As Long, ByVal lpOperation As String, ByVal lpFile As String, ByVal lpParameters As String, ByVal lpDirectory As String, ByVal nShowCmd As Long) As Long
Public Declare Function StretchBlt Lib "gdi32" (ByVal hDestDC As Long, ByVal X As Long, ByVal Y As Long, ByVal nWidth As Long, ByVal nHeight As Long, ByVal hSrcDC As Long, ByVal XSrc As Long, ByVal YSrc As Long, ByVal nSrcWidth As Long, ByVal nSrcHeight As Long, ByVal dwRop As Long) As Long
Public Declare Function BitBlt Lib "gdi32" (ByVal hDestDC As Long, ByVal X As Long, ByVal Y As Long, ByVal nWidth As Long, ByVal nHeight As Long, ByVal hSrcDC As Long, ByVal XSrc As Long, ByVal YSrc As Long, ByVal dwRop As Long) As Long

' === Board cell (6 bytes/cell in the original; g_4B6044, 18x11) ===
Public Type BoardCell
    Occupied As Integer      ' -1 = tile present, 0 = empty (border ring)
    TileId As Integer        ' visual face id (0..41; flowers 34..41)
    MatchId As Integer       ' match class (0..35; classes 34/35 = flower groups)
End Type

' === High-score record (d4s.hst, 10 records x 12 bytes on disk, For Binary) ===
Public Type RecordType
    Name As String * 8
    Score As Single
End Type

' === Public state (exact original global block 0x4B6024..0x4B60F8, size 0xD4 = 212 bytes) ===
Public BitBltRet As Long         ' g_4B6024 (+0x00) return value of BitBlt calls
Public TwipsX As Long            ' g_4B6028 (+0x04) Screen.TwipsPerPixelX
Public TwipsY As Long            ' g_4B602C (+0x08) Screen.TwipsPerPixelY
Public WinDirPath As String      ' g_4B6030 (+0x0C) Windows directory (GetWindowsDirectoryA)
Public TempDir As String         ' g_4B6034 (+0x10) temp directory (GetTempPathA)
Public Board(0 To 17, 0 To 10) As BoardCell ' g_4B6038..0x4B6057 (+0x14, pvData=0x4B6044)
Public Sel1C As Single           ' g_4B6058 (+0x34) first selected col (-1 = none)
Public Sel1R As Single           ' g_4B605C (+0x38) first selected row
Public Sel2C As Single           ' g_4B6060 (+0x3C) second selected col
Public Sel2R As Single           ' g_4B6064 (+0x40) second selected row
Public HoverC As Single          ' g_4B6068 (+0x44) hovered cell col (-1 = none)
Public HoverR As Single          ' g_4B606C (+0x48) hovered cell row (-1 = none)
Public PaletteOff As Integer     ' g_4B6070 (+0x4C) random face-palette offset (0..5)
Public Unused_4B6072 As Integer  ' g_4B6072 (+0x4E) unused scalar slot
Public Unused_4B6074 As Long     ' g_4B6074 (+0x50) unused scalar slot
Public TilesLeft As Integer      ' g_4B6078 (+0x54) remaining tiles (starts 144)
Public HisFile As String         ' g_4B607C (+0x58) high-score file path = WinDirPath & "\d4s.hst"
Public PlayerName As String      ' g_4B6080 (+0x5C) player name (default "Player")
Public LastTick As Single        ' g_4B6084 (+0x60) elapsed ms in current level
Public Score As Single           ' g_4B6088 (+0x64) current score
Public m_HasSound As Integer     ' g_4B608C (+0x68) sound card present (-1/0)
Public OffX As Single            ' g_4B6090 (+0x6C) board X origin inside Pic_Show
Public OffY As Single            ' g_4B6094 (+0x70) board Y origin inside Pic_Show
Public m_BoardActive As Integer  ' g_4B6098 (+0x74) -1 = board active in play
Public m_QuitFlag As Integer     ' g_4B609A (+0x76) -1 = leave GameLoop
Public NewScoreIdx As Integer    ' g_4B609C (+0x78) -1 = new score qualified for Form_Table
Public GameState As Integer      ' g_4B609E (+0x7A) 0=playing 1=level-clear 2=deadlock 3=timeout
Public Unused_4B60A0 As Long     ' g_4B60A0 (+0x7C) unused scalar slot
Public Unused_4B60A4 As Long     ' g_4B60A4 (+0x80) unused scalar slot
Public Level As Integer          ' g_4B60A8 (+0x84) current stage 0..6
Public Lives As Integer          ' g_4B60AA (+0x86) remaining shuffle lives
Public Unused_4B60AC As Long     ' g_4B60AC (+0x88) unused scalar slot
Public LinkN As Integer          ' g_4B60B0 (+0x8C) number of link segments (1..3)
Public LinkX1 As Single          ' g_4B60B4 (+0x90) link corner 1 col
Public LinkY1 As Single          ' g_4B60B8 (+0x94) link corner 1 row
Public LinkX2 As Single          ' g_4B60BC (+0x98) link corner 2 col
Public LinkY2 As Single          ' g_4B60C0 (+0x9C) link corner 2 row
Public LinkX3 As Single          ' g_4B60C4 (+0xA0) link corner 3 col
Public LinkY3 As Single          ' g_4B60C8 (+0xA4) link corner 3 row
Public LinkX4 As Single          ' g_4B60CC (+0xA8) link corner 4 col
Public LinkY4 As Single          ' g_4B60D0 (+0xAC) link corner 4 row
Public HintX1 As Single          ' g_4B60D4 (+0xB0) cached valid-move col 1
Public HintY1 As Single          ' g_4B60D8 (+0xB4) cached valid-move row 1
Public HintX2 As Single          ' g_4B60DC (+0xB8) cached valid-move col 2
Public HintY2 As Single          ' g_4B60E0 (+0xBC) cached valid-move row 2
Public TimeLeft As Single        ' g_4B60E4 (+0xC0) 600s stage budget
Public MidiPath As String        ' g_4B60E8 (+0xC4) extracted MIDI file path
Public m_MidiTrack As Integer    ' g_4B60EC (+0xC8) current MIDI resource 101..103
Public MusicOn As Integer        ' g_4B60EE (+0xCA) music enabled (-1/0)
Public SoundOn As Integer        ' g_4B60F0 (+0xCC) sound enabled (-1/0)
Public Unused_4B60F2 As Integer  ' g_4B60F2 (+0xCE) unused scalar slot (total = 0xD4 bytes)

' === 40: Scramble (0x4B3410 / 0x4B3570) ===
Public Function Scramble(ByVal s As String) As String
    Dim i As Integer
    Dim ch As String
    s = RTrim(s)
    For i = 1 To Len(s)
        ch = Mid$(s, i, 1)
        Mid(s, i, 1) = Chr(Asc(ch) + i)
    Next i
    Scramble = s
End Function

' === 41: Unscramble (0x4B35B0 / 0x4B3710) ===
Public Function Unscramble(ByVal s As String) As String
    Dim i As Integer
    Dim ch As String
    s = RTrim(s)
    For i = 1 To Len(s)
        ch = Mid$(s, i, 1)
        Mid(s, i, 1) = Chr(Asc(ch) - i)
    Next i
    Unscramble = s
End Function

' === 42: DelayMs (0x4B3750 / 0x4B38B0) ===
Public Sub DelayMs(ms As Integer)
    Dim t0 As Long
    t0 = GetTickCount()
    Do While Val(CStr(ms)) > GetTickCount() - t0
        DoEvents
    Loop
End Sub
