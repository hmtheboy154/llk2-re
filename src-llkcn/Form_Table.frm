VERSION 5.00
Begin VB.Form Form_Table 
      ClientLeft      =   45 
      ClientTop       =   315 
      ClientWidth     =   3240 
      ClientHeight    =   4275 
   Caption         =   "连连看2" 
   BackColor       =   &H000080FF& 
   ScaleMode       =   1 
   AutoRedraw      =   0 'False 
   FontTransparent =   -1 'True 
   BorderStyle     =   4 
   LinkTopic       =   "Form1" 
   MaxButton       =   0 'False 
   MinButton       =   0 'False 
   ShowInTaskbar   =   0 'False 
   StartUpPosition =   2 
   LockControls    =   -1 'True 
   Begin VB.CommandButton Cmd_OK 
      Caption         =   "确定" 
      Left            =   2040 
      Top             =   3720 
      Width           =   975 
      Height          =   375 
      TabIndex        =   36 
   End 
   Begin VB.TextBox Text_Name 
      BackColor       =   &H00FFFFFF& 
      ForeColor       =   &H00000000& 
      Left            =   360 
      Top             =   3780 
      Width           =   1200 
      Height          =   300 
      TabIndex        =   34 
   End 
   Begin VB.Label Label2 
      Caption         =   "玩家姓名" 
      ForeColor       =   &H00FFFFFF& 
      BackStyle       =   0 
      Alignment       =   2 
      Left            =   360 
      Top             =   3525 
      Width           =   1215 
      Height          =   255 
      TabIndex        =   35 
   End 
   Begin VB.Frame Frame1 
      BackColor       =   &H000080FF& 
      ForeColor       =   &H0000FFFF& 
      Left            =   120 
      Top             =   0 
      Width           =   3000 
      Height          =   3400 
      TabIndex        =   0 
      Begin VB.Label Lab_Time 
         Index           =   9 
         BackColor       =   &H00FFFFFF& 
         Left            =   2080 
         Top             =   3000 
         Width           =   800 
         Height          =   250 
         TabIndex        =   33 
         Alignment       =   1 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Time 
         Index           =   8 
         BackColor       =   &H00FFFFFF& 
         Left            =   2080 
         Top             =   2725 
         Width           =   800 
         Height          =   250 
         TabIndex        =   32 
         Alignment       =   1 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Time 
         Index           =   7 
         BackColor       =   &H00FFFFFF& 
         Left            =   2080 
         Top             =   2450 
         Width           =   800 
         Height          =   250 
         TabIndex        =   31 
         Alignment       =   1 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Time 
         Index           =   6 
         BackColor       =   &H00FFFFFF& 
         Left            =   2080 
         Top             =   2175 
         Width           =   800 
         Height          =   250 
         TabIndex        =   30 
         Alignment       =   1 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Time 
         Index           =   5 
         BackColor       =   &H00FFFFFF& 
         Left            =   2080 
         Top             =   1900 
         Width           =   800 
         Height          =   250 
         TabIndex        =   29 
         Alignment       =   1 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Time 
         Index           =   4 
         BackColor       =   &H00FFFFFF& 
         Left            =   2080 
         Top             =   1625 
         Width           =   800 
         Height          =   250 
         TabIndex        =   28 
         Alignment       =   1 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Time 
         Index           =   3 
         BackColor       =   &H00FFFFFF& 
         Left            =   2080 
         Top             =   1350 
         Width           =   800 
         Height          =   250 
         TabIndex        =   27 
         Alignment       =   1 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Time 
         Index           =   2 
         BackColor       =   &H00FFFFFF& 
         Left            =   2080 
         Top             =   1075 
         Width           =   800 
         Height          =   250 
         TabIndex        =   26 
         Alignment       =   1 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Time 
         Index           =   1 
         BackColor       =   &H00FFFFFF& 
         Left            =   2080 
         Top             =   800 
         Width           =   800 
         Height          =   250 
         TabIndex        =   25 
         Alignment       =   1 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Name 
         Index           =   9 
         BackColor       =   &H00FFFFFF& 
         Left            =   550 
         Top             =   3000 
         Width           =   1500 
         Height          =   250 
         TabIndex        =   24 
         Alignment       =   2 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Name 
         Index           =   8 
         BackColor       =   &H00FFFFFF& 
         Left            =   550 
         Top             =   2725 
         Width           =   1500 
         Height          =   250 
         TabIndex        =   23 
         Alignment       =   2 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Name 
         Index           =   7 
         BackColor       =   &H00FFFFFF& 
         Left            =   550 
         Top             =   2450 
         Width           =   1500 
         Height          =   250 
         TabIndex        =   22 
         Alignment       =   2 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Name 
         Index           =   6 
         BackColor       =   &H00FFFFFF& 
         Left            =   550 
         Top             =   2175 
         Width           =   1500 
         Height          =   250 
         TabIndex        =   21 
         Alignment       =   2 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Name 
         Index           =   5 
         BackColor       =   &H00FFFFFF& 
         Left            =   550 
         Top             =   1900 
         Width           =   1500 
         Height          =   250 
         TabIndex        =   20 
         Alignment       =   2 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Name 
         Index           =   4 
         BackColor       =   &H00FFFFFF& 
         Left            =   550 
         Top             =   1625 
         Width           =   1500 
         Height          =   250 
         TabIndex        =   19 
         Alignment       =   2 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Name 
         Index           =   3 
         BackColor       =   &H00FFFFFF& 
         Left            =   550 
         Top             =   1350 
         Width           =   1500 
         Height          =   250 
         TabIndex        =   18 
         Alignment       =   2 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Name 
         Index           =   2 
         BackColor       =   &H00FFFFFF& 
         Left            =   550 
         Top             =   1075 
         Width           =   1500 
         Height          =   250 
         TabIndex        =   17 
         Alignment       =   2 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Name 
         Index           =   1 
         BackColor       =   &H00FFFFFF& 
         Left            =   550 
         Top             =   800 
         Width           =   1500 
         Height          =   250 
         TabIndex        =   16 
         Alignment       =   2 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Time 
         Index           =   0 
         BackColor       =   &H00FFFFFF& 
         Left            =   2080 
         Top             =   525 
         Width           =   800 
         Height          =   250 
         TabIndex        =   15 
         Alignment       =   1 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Lab_Name 
         Index           =   0 
         BackColor       =   &H00FFFFFF& 
         Left            =   550 
         Top             =   525 
         Width           =   1500 
         Height          =   250 
         TabIndex        =   14 
         Alignment       =   2 
         FontName        =   "Arial" 
         FontSize        =   9 
      End 
      Begin VB.Label Label1 
         Index           =   12 
         Caption         =   "10" 
         BackColor       =   &H00FFFFFF& 
         Left            =   120 
         Top             =   3000 
         Width           =   400 
         Height          =   250 
         TabIndex        =   13 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   11 
         Caption         =   "9" 
         BackColor       =   &H00FFFFFF& 
         Left            =   120 
         Top             =   2725 
         Width           =   400 
         Height          =   250 
         TabIndex        =   12 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   10 
         Caption         =   "8" 
         BackColor       =   &H00FFFFFF& 
         Left            =   120 
         Top             =   2450 
         Width           =   400 
         Height          =   250 
         TabIndex        =   11 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   9 
         Caption         =   "7" 
         BackColor       =   &H00FFFFFF& 
         Left            =   120 
         Top             =   2175 
         Width           =   400 
         Height          =   250 
         TabIndex        =   10 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   8 
         Caption         =   "6" 
         BackColor       =   &H00FFFFFF& 
         Left            =   120 
         Top             =   1900 
         Width           =   400 
         Height          =   250 
         TabIndex        =   9 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   7 
         Caption         =   "5" 
         BackColor       =   &H00FFFFFF& 
         Left            =   120 
         Top             =   1625 
         Width           =   400 
         Height          =   250 
         TabIndex        =   8 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   6 
         Caption         =   "4" 
         BackColor       =   &H00FFFFFF& 
         Left            =   120 
         Top             =   1350 
         Width           =   400 
         Height          =   250 
         TabIndex        =   7 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   5 
         Caption         =   "3" 
         BackColor       =   &H00FFFFFF& 
         Left            =   120 
         Top             =   1075 
         Width           =   400 
         Height          =   250 
         TabIndex        =   6 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   4 
         Caption         =   "2" 
         BackColor       =   &H00FFFFFF& 
         Left            =   120 
         Top             =   800 
         Width           =   400 
         Height          =   250 
         TabIndex        =   5 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   3 
         Caption         =   "1" 
         BackColor       =   &H00FFFFFF& 
         Left            =   120 
         Top             =   525 
         Width           =   400 
         Height          =   250 
         TabIndex        =   4 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   2 
         Caption         =   "积分" 
         BackColor       =   &H00FF0000& 
         ForeColor       =   &H00FFFFFF& 
         Left            =   2080 
         Top             =   250 
         Width           =   800 
         Height          =   250 
         TabIndex        =   3 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   1 
         Caption         =   "姓名" 
         BackColor       =   &H00FF0000& 
         ForeColor       =   &H00FFFFFF& 
         Left            =   550 
         Top             =   250 
         Width           =   1500 
         Height          =   250 
         TabIndex        =   2 
         Alignment       =   2 
      End 
      Begin VB.Label Label1 
         Index           =   0 
         Caption         =   "名次" 
         BackColor       =   &H00FF0000& 
         ForeColor       =   &H00FFFFFF& 
         Left            =   120 
         Top             =   250 
         Width           =   400 
         Height          =   250 
         TabIndex        =   1 
         Alignment       =   2 
      End 
   End 
End 
Attribute VB_Name = "Form_Table"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

' Form_Table code - reconstructed from procs57 43..51 (0x4B3830..0x4B5080)
' Procedure order below matches the original form vtable slot order
' (slot00 0x4B3830, slot01 0x4B3D30, slot02 0x4B4570, Cmd_OK_Click, Form_Load,
'  Form_QueryUnload, Text_Name_Change, Text_Name_GotFocus, Text_Name_KeyPress).
'
' Shared globals used (declared Public in Mainbas):
'   HisFile      g_4B607C  = <WindowsDir> & "\d4s.hst"   (built by Form_Main.Form_Load)
'   NewScoreIdx  g_4B609C  -1 = a new score qualified, Form_Table runs in insert mode
'                          (set by Form_Main procs 21/22/23 via FormSlot25; reset to 0 here)
'   Score        g_4B6088  final game score to insert
'   PlayerName   g_4B6080  player name (default "Player", live-edited via Text_Name)
'
' d4s.hst layout: 10 records x 12 bytes on disk, For Binary (record = RecordType:
' Name String*8 = 8 ANSI/DBCS bytes on disk + Score Single = 4 bytes on disk;
' in RAM VB5 stores String*8 as 16 UTF-16 bytes + 4-byte Single = 20 bytes).
' Names are stored Scramble()d on disk and Unscramble()d for display only.

Dim NewRank As Integer      ' form member +0x34: row where the new score landed
Dim OldText As String       ' form member +0x38: Text_Name content before last keypress

' === slot00 0x4B3830: read d4s.hst and fill the table (view mode) ===
Private Sub ShowTable()
    Dim Table(9) As RecordType
    Dim i As Integer
    Dim j As Integer
    Dim s As String
    Dim fd As Integer
    If Dir$(HisFile) <> "" Then
        fd = FreeFile
        Open HisFile For Binary As #fd
        For i = 0 To 9
            Get #fd, , Table(i)
            s = Table(i).Name
            Table(i).Name = Unscramble(s)
        Next i
        Close #fd
    End If
    For j = 0 To 9
        Lab_Name(j).BackColor = &HFFFFFF&
        Lab_Name(j).Caption = Table(j).Name
        If Table(j).Score > 0 Then
            Lab_Time(j).Caption = Table(j).Score
        Else
            Lab_Time(j).Caption = ""
        End If
    Next j
End Sub

' === slot01 0x4B3D30: insert Score into the table, bubble-sort, highlight, rewrite file ===
Private Sub InsertScore()
    Dim Table(9) As RecordType
    Dim i As Integer
    Dim j As Integer
    Dim s As String
    Dim sScore As Single
    Dim fd As Integer
    On Error GoTo ErrHandler
    If Dir$(HisFile) <> "" Then
        fd = FreeFile
        Open HisFile For Binary As #fd
        For i = 0 To 9
            Get #fd, , Table(i)
            s = Table(i).Name
            Table(i).Name = Unscramble(s)
        Next i
        Close #fd
        If Score > Table(9).Score Then
            Table(9).Name = PlayerName
            Table(9).Score = Score
            NewRank = 9
            For i = 9 To 1 Step -1
                If Table(i).Score > Table(i - 1).Score Then
                    s = Table(i - 1).Name
                    sScore = Table(i - 1).Score
                    Table(i - 1).Name = Table(i).Name
                    Table(i - 1).Score = Table(i).Score
                    Table(i).Name = s
                    Table(i).Score = sScore
                    NewRank = i - 1
                End If
            Next i
            For j = 0 To 9
                If j = NewRank Then
                    Lab_Name(j).BackColor = &HFFFF&
                Else
                    Lab_Name(j).BackColor = &HFFFFFF&
                End If
                Lab_Name(j).Caption = Table(j).Name
                If Table(j).Score > 0 Then
                    Lab_Time(j).Caption = Table(j).Score
                Else
                    Lab_Time(j).Caption = ""
                End If
            Next j
            fd = FreeFile
            Open HisFile For Binary As #fd
            For i = 0 To 9
                s = Table(i).Name
                Table(i).Name = Scramble(s)
                Put #fd, , Table(i)
            Next i
            Close #fd
        End If
    End If
ErrHandler:
End Sub

' === slot02 0x4B4570: LenB(StrConv(s, vbFromUnicode)) - caps DBCS names at 8 bytes ===
Private Function ByteLen(ByVal s As String) As Integer
    ByteLen = LenB(StrConv(s, vbFromUnicode))
End Function

' === slot03 0x4B4640 ===
Private Sub Cmd_OK_Click()
    Unload Me
End Sub

' === slot04 0x4B4720 ===
Private Sub Form_Load()
    If App.PrevInstance Then End
    Text_Name.Text = PlayerName
    If NewScoreIdx = -1 Then
        InsertScore
    ElseIf NewScoreIdx = 0 Then
        ShowTable
    End If
End Sub

' === slot05 0x4B4880: persist the final player name into record NewRank ===
Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
    Dim Table(9) As RecordType
    Dim i As Integer
    Dim s As String
    Dim fd As Integer
    If NewScoreIdx = -1 Then
        If Dir$(HisFile) <> "" Then
            fd = FreeFile
            Open HisFile For Binary As #fd
            For i = 0 To 9
                Get #fd, , Table(i)
                s = Table(i).Name
                Table(i).Name = Unscramble(s)
            Next i
            Close #fd
            Table(NewRank).Name = PlayerName
            Open HisFile For Binary As #fd
            For i = 0 To 9
                s = Table(i).Name
                Table(i).Name = Scramble(s)
                Put #fd, , Table(i)
            Next i
            Close #fd
        End If
    End If
    NewScoreIdx = 0
End Sub

' === slot06 0x4B4C50: cap name at 8 DBCS bytes, mirror into PlayerName and the highlighted row ===
Private Sub Text_Name_Change()
    Dim SelPos As Integer
    Dim n As Integer
    SelPos = Text_Name.SelStart
    n = ByteLen(Text_Name.Text)
    If n > 8 Then
        Text_Name.Text = OldText
        Text_Name.SelStart = SelPos
    End If
    PlayerName = Text_Name.Text
    If NewScoreIdx = -1 Then
        Lab_Name(NewRank).Caption = Text_Name.Text
    End If
End Sub

' === slot07 0x4B4F20: select the whole text on focus ===
Private Sub Text_Name_GotFocus()
    Text_Name.SelStart = 0
    Text_Name.SelLength = Len(Text_Name.Text)
End Sub

' === slot08 0x4B5080: Enter acts as OK ===
Private Sub Text_Name_KeyPress(KeyAscii As Integer)
    OldText = Text_Name.Text
    If KeyAscii = 13 Then
        KeyAscii = 0
        Unload Me
    End If
End Sub
