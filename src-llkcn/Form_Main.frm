VERSION 5.00
Begin VB.Form Form_Main 
      ClientLeft      =   45 
      ClientTop       =   615 
      ClientWidth     =   7335 
      ClientHeight    =   6195 
   Caption         =   "蟀蟀艘2" 
   ScaleMode       =   3 
   AutoRedraw      =   0 'False 
   FontTransparent =   -1 'True 
   BorderStyle     =   1 
   LinkTopic       =   "Form1" 
   MaxButton       =   0 'False 
   Icon            =   "Form_Main.frx":0000 
   StartUpPosition =   1 
   Begin VB.Timer Timer_PlayMidi 
      Enabled         =   0 'False 
      Interval        =   100 
      Left            =   5040 
   End 
   Begin VB.PictureBox Pic_Title 
      Picture         =   "Form_Main.frx":08CA 
      BackColor       =   &H80000005& 
      ForeColor       =   &H80000008& 
      Left            =   3360 
      Top             =   3600 
      Width           =   7500 
      Height          =   5625 
      Visible         =   0 'False 
      TabIndex        =   11 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      AutoSize        =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.PictureBox Pic_Level 
      Index           =   6 
      Picture         =   "Form_Main.frx":B681 
      BackColor       =   &H80000005& 
      ForeColor       =   &H80000008& 
      Left            =   1200 
      Top             =   2280 
      Width           =   9000 
      Height          =   6750 
      Visible         =   0 'False 
      TabIndex        =   10 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      AutoSize        =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.PictureBox Pic_Level 
      Index           =   5 
      Picture         =   "Form_Main.frx":1AC4C 
      BackColor       =   &H80000005& 
      ForeColor       =   &H80000008& 
      Left            =   1320 
      Top             =   2040 
      Width           =   9000 
      Height          =   6750 
      Visible         =   0 'False 
      TabIndex        =   9 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      AutoSize        =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.PictureBox Pic_Level 
      Index           =   4 
      Picture         =   "Form_Main.frx":2F303 
      BackColor       =   &H80000005& 
      ForeColor       =   &H80000008& 
      Left            =   1080 
      Top             =   2040 
      Width           =   9000 
      Height          =   6750 
      Visible         =   0 'False 
      TabIndex        =   8 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      AutoSize        =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.PictureBox Pic_Level 
      Index           =   3 
      Picture         =   "Form_Main.frx":40805 
      BackColor       =   &H80000005& 
      ForeColor       =   &H80000008& 
      Left            =   840 
      Top             =   1920 
      Width           =   9000 
      Height          =   6750 
      Visible         =   0 'False 
      TabIndex        =   7 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      AutoSize        =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.PictureBox Pic_Level 
      Index           =   2 
      Picture         =   "Form_Main.frx":48FE8 
      BackColor       =   &H80000005& 
      ForeColor       =   &H80000008& 
      Left            =   600 
      Top             =   1560 
      Width           =   9000 
      Height          =   6750 
      Visible         =   0 'False 
      TabIndex        =   6 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      AutoSize        =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.PictureBox Pic_Level 
      Index           =   1 
      Picture         =   "Form_Main.frx":5AF33 
      BackColor       =   &H80000005& 
      ForeColor       =   &H80000008& 
      Left            =   480 
      Top             =   1440 
      Width           =   9000 
      Height          =   6750 
      Visible         =   0 'False 
      TabIndex        =   5 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      AutoSize        =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.PictureBox Pic_Level 
      Index           =   0 
      Picture         =   "Form_Main.frx":66001 
      BackColor       =   &H80000005& 
      ForeColor       =   &H80000008& 
      Left            =   360 
      Top             =   1320 
      Width           =   9000 
      Height          =   6750 
      Visible         =   0 'False 
      TabIndex        =   4 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      AutoSize        =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.PictureBox Pic_Data 
      Picture         =   "Form_Main.frx":6F7C1 
      BackColor       =   &H80000005& 
      ForeColor       =   &H80000008& 
      Left            =   240 
      Top             =   1200 
      Width           =   7650 
      Height          =   3600 
      Visible         =   0 'False 
      TabIndex        =   3 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      AutoSize        =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.PictureBox Pic_MjData 
      Picture         =   "Form_Main.frx":7121E 
      BackColor       =   &H80000005& 
      ForeColor       =   &H80000008& 
      Left            =   120 
      Top             =   1080 
      Width           =   5400 
      Height          =   9180 
      Visible         =   0 'False 
      TabIndex        =   2 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      AutoSize        =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.PictureBox Pic_Buffer 
      BackColor       =   &H00000000& 
      ForeColor       =   &H80000008& 
      Left            =   1200 
      Top             =   120 
      Width           =   1215 
      Height          =   735 
      Visible         =   0 'False 
      TabIndex        =   1 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.PictureBox Pic_Show 
      BackColor       =   &H00000000& 
      ForeColor       =   &H80000008& 
      Left            =   0 
      Top             =   120 
      Width           =   1095 
      Height          =   735 
      TabIndex        =   0 
      ScaleMode       =   3 
      AutoRedraw      =   -1 'True 
      BorderStyle     =   0 
      Appearance      =   0 
   End 
   Begin VB.Menu Mem_Game 
      Caption         =   "蚔牁(&G)" 
      Begin VB.Menu Mem_NewGame 
         Caption         =   "陔擁(&N)" 
      End 
      Begin VB.Menu Mem_QuitGame 
         Caption         =   "溫ィ(&Q)" 
      End 
      Begin VB.Menu Mem_Line1 
         Caption         =   "-" 
      End 
      Begin VB.Menu Mem_Record 
         Caption         =   "郔槽暮翹(&R)" 
      End 
      Begin VB.Menu Mem_Line2 
         Caption         =   "-" 
      End 
      Begin VB.Menu Mem_Exit 
         Caption         =   "賦旰燭羲(&X)" 
      End 
   End 
   Begin VB.Menu Mem_BackGround 
      Caption         =   "掖劓汒秞(&B)" 
      Begin VB.Menu Mem_Music 
         Caption         =   "秞氈(&M)" 
      End 
      Begin VB.Menu Mem_Sound 
         Caption         =   "秞虴(&S)" 
      End 
   End 
   Begin VB.Menu Mem_About 
      Caption         =   "壽衾(&A)" 
      Begin VB.Menu Mem_Explain 
         Caption         =   "蚔牁寞寀(&E)" 
      End 
      Begin VB.Menu Mem_Author 
         Caption         =   "釬氪(&P)" 
      End 
   End 
End 
Attribute VB_Name = "Form_Main"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

' =====================================================================
' Form_Main - code section (reconstructed from procs57/00_..39_ asm,
' Form_Main vtable slots at 0x6F8 + slot*4, event handlers 26..39).
'
' Slot map (slot = (vtoffset - 0x6F8) \ 4):
'   00 0x4A7CF0 InitLayout            20 0x4B05F0 ProcessSelection
'   01 0x4A8720 InitBoard             21 0x4B09C0 LevelClear
'   02 0x4A9080 ClearSelection        22 0x4B0F90 DeadlockHandler
'   03 0x4A90B0 ShuffleRemaining      23 0x4B11C0 TimeoutOver
'   04 0x4A94B0 ShiftUp               24 0x4B1500 InitHighScoreFile
'   05 0x4A9710 ShiftDown             25 0x4B1710 IsHighScore
'   06 0x4A9980 ShiftLeft             26 0x4B18E0 Form_Load
'   07 0x4A9BE0 ShiftRight            27 0x4B1DB0 Form_QueryUnload
'   08 0x4A9E50 ShiftOutwardH         28 0x4B21D0 Mem_Author_Click
'   09 0x4AA4F0 ShiftInwardH          29 0x4B22C0 Mem_Exit_Click
'   10 0x4AABA0 RedrawBoard           30 0x4B23A0 Mem_Explain_Click
'   11 0x4AC390 AnimateLinkAndRemove  31 0x4B2490 Mem_Music_Click
'   12 0x4ADF80 DrawTitleScreen       32 0x4B26B0 Mem_NewGame_Click
'   13 0x4AE4D0 HasValidMove          33 0x4B27B0 Mem_QuitGame_Click
'   14 0x4AE690 CheckLink             34 0x4B2820 Mem_Record_Click
'   15 0x4AE760 CheckLink0            35 0x4B2910 Mem_Sound_Click
'   16 0x4AEAE0 CheckLink1            36 0x4B2A30 Pic_Show_MouseDown
'   17 0x4AEE30 CheckLink2            37 0x4B2D70 Pic_Show_MouseMove
'   18 0x4AF7F0 CheckLink3            38 0x4B2F20 PlayMidi
'   19 0x4B01B0 GameLoop              39 0x4B30D0 Timer_PlayMidi_Timer
'
' Extra Private helpers (no vtable slot of their own; the original
' inlines these): DrawHUD, RowClear, ColClear.
'
' Notes:
'  - Sel1C/Sel1R hold column/row of the first selected cell and
'    Sel2C/Sel2R the second (g_4B6058..g_4B6064 in Mainbas).
'  - Board(c, r): c = 0..17, r = 0..10; active cells c = 1..16,
'    r = 1..9; Occupied = -1 tile / 0 empty (border ring stays 0 so
'    exterior link paths route through it).
'  - Pic_MjData sheet layout: faces 9 per row (40x50) at rows 0..4,
'    face masks at rows 5..9, shell strip 46x56 (6 palette variants +
'    AND mask at x=276) at y=500, selected shell variants at y=556.
' =====================================================================

Private Const SRCCOPY As Long = &HCC0020
Private Const SRCAND As Long = &H8800C6
Private Const SRCPAINT As Long = &HEE0086

' === slot 00 (0x4A7CF0) =============================================
Private Sub InitLayout()
    TwipsX = Screen.TwipsPerPixelX
    TwipsY = Screen.TwipsPerPixelY
    Me.Width = Me.Width - Me.ScaleWidth * TwipsX + 768 * TwipsX
    Me.Height = Me.Height - Me.ScaleHeight * TwipsY + 550 * TwipsY
    If Screen.Width \ TwipsX <= 800 Then
        Me.Top = 0
    Else
        Me.Top = (Screen.Height - Me.Height) / 2
    End If
    Me.Left = (Screen.Width - Me.Width) / 2
    Pic_Show.Height = 550
    Pic_Show.Width = 768
    Pic_Show.Top = 0
    Pic_Show.Left = 0
    Pic_Show.Enabled = False
    Pic_Buffer.Height = Pic_Show.ScaleHeight
    Pic_Buffer.Width = Pic_Show.ScaleWidth
    OffX = (Pic_Show.ScaleWidth - 726) \ 2
    OffY = (Pic_Show.ScaleHeight - 550) \ 2 + 6
    Mem_NewGame.Enabled = True
    Mem_QuitGame.Enabled = False
    PaletteOff = 0
End Sub

' === slot 01 (0x4A8720) =============================================
Private Sub InitBoard()
    Dim tmp(0 To 143) As BoardCell
    Dim t As Integer
    Dim r As Integer
    Dim c As Integer
    Dim k As Integer
    Dim emptyCell As BoardCell
    emptyCell.Occupied = 0
    emptyCell.TileId = 0
    emptyCell.MatchId = 0
    m_BoardActive = -1
    ClearSelection
    PaletteOff = (Int(Rnd() * 5) + PaletteOff + 1) Mod 6
    TilesLeft = 144
    For t = 0 To 143
        tmp(t).Occupied = -1
        If t < 136 Then
            tmp(t).TileId = t \ 4
        Else
            tmp(t).TileId = t - 102
        End If
        tmp(t).MatchId = t \ 4
    Next t
    k = 0
    For r = 0 To 10
        For c = 0 To 17
            If r >= 1 And r <= 9 And c >= 1 And c <= 16 Then
                Board(c, r) = tmp(k)
                k = k + 1
            Else
                Board(c, r) = emptyCell
            End If
        Next c
    Next r
    Set Pic_Buffer.Picture = LoadPicture("")
    BitBltRet = BitBlt(Pic_Buffer.hDC, (Pic_Buffer.ScaleWidth - Pic_Level(Level).ScaleWidth) / 2, (Pic_Buffer.ScaleHeight - Pic_Level(Level).ScaleHeight) / 2, Pic_Level(Level).ScaleWidth, Pic_Level(Level).ScaleHeight, Pic_Level(Level).hDC, 0, 0, SRCCOPY)
    Set Pic_Buffer.Picture = Pic_Buffer.Image
    Pic_Buffer.Cls
End Sub

' === slot 02 (0x4A9080) =============================================
Private Sub ClearSelection()
    Sel1C = -1
    Sel1R = -1
    Sel2C = -1
    Sel2R = -1
    HoverC = -1
    HoverR = -1
End Sub

' === slot 03 (0x4A90B0) =============================================
Private Sub ShuffleRemaining()
    Dim tmp(0 To 143) As BoardCell
    Dim swp As BoardCell
    Dim r As Integer
    Dim c As Integer
    Dim n As Integer
    Dim i As Integer
    Dim j As Integer
    Dim k As Integer
    For i = 0 To 143
        tmp(i).Occupied = 0
        tmp(i).TileId = 0
        tmp(i).MatchId = 0
    Next i
    n = 0
    For r = 0 To 10
        For c = 0 To 17
            If Board(c, r).Occupied = -1 Then
                tmp(n) = Board(c, r)
                n = n + 1
            End If
        Next c
    Next r
    n = n - 1
    For i = 0 To 79
        For j = 0 To n
            k = Int(Rnd() * n)
            swp = tmp(k)
            tmp(k) = tmp(j)
            tmp(j) = swp
        Next j
    Next i
    n = 0
    For r = 0 To 10
        For c = 0 To 17
            If Board(c, r).Occupied = -1 Then
                Board(c, r) = tmp(n)
                n = n + 1
            End If
        Next c
    Next r
End Sub

' === slot 04 (0x4A94B0): compact every column toward row 1 ==========
Private Sub ShiftUp()
    Dim cells(1 To 9) As BoardCell
    Dim emptyCell As BoardCell
    Dim c As Integer
    Dim r As Integer
    Dim n As Integer
    Dim m As Integer
    emptyCell.Occupied = 0
    emptyCell.TileId = 0
    emptyCell.MatchId = 0
    For c = 1 To 16
        n = 0
        For r = 1 To 9
            If Board(c, r).Occupied = -1 Then
                n = n + 1
                cells(n) = Board(c, r)
            End If
        Next r
        If n < 9 And n > 0 Then
            m = 1
            For r = 1 To 9
                If r > n Then
                    Board(c, r) = emptyCell
                Else
                    Board(c, r) = cells(m)
                    m = m + 1
                End If
            Next r
        End If
    Next c
End Sub

' === slot 05 (0x4A9710): compact every column toward row 9 ==========
Private Sub ShiftDown()
    Dim cells(1 To 9) As BoardCell
    Dim emptyCell As BoardCell
    Dim c As Integer
    Dim r As Integer
    Dim n As Integer
    Dim m As Integer
    emptyCell.Occupied = 0
    emptyCell.TileId = 0
    emptyCell.MatchId = 0
    For c = 1 To 16
        n = 0
        For r = 1 To 9
            If Board(c, r).Occupied = -1 Then
                n = n + 1
                cells(n) = Board(c, r)
            End If
        Next r
        If n < 9 And n > 0 Then
            m = 1
            For r = 1 To 9
                If r <= 9 - n Then
                    Board(c, r) = emptyCell
                Else
                    Board(c, r) = cells(m)
                    m = m + 1
                End If
            Next r
        End If
    Next c
End Sub

' === slot 06 (0x4A9980): compact every row toward column 1 ==========
Private Sub ShiftLeft()
    Dim cells(1 To 16) As BoardCell
    Dim emptyCell As BoardCell
    Dim r As Integer
    Dim c As Integer
    Dim n As Integer
    Dim m As Integer
    emptyCell.Occupied = 0
    emptyCell.TileId = 0
    emptyCell.MatchId = 0
    For r = 1 To 9
        n = 0
        For c = 1 To 16
            If Board(c, r).Occupied = -1 Then
                n = n + 1
                cells(n) = Board(c, r)
            End If
        Next c
        If n < 16 And n > 0 Then
            m = 1
            For c = 1 To 16
                If c > n Then
                    Board(c, r) = emptyCell
                Else
                    Board(c, r) = cells(m)
                    m = m + 1
                End If
            Next c
        End If
    Next r
End Sub

' === slot 07 (0x4A9BE0): compact every row toward column 16 =========
Private Sub ShiftRight()
    Dim cells(1 To 16) As BoardCell
    Dim emptyCell As BoardCell
    Dim r As Integer
    Dim c As Integer
    Dim n As Integer
    Dim m As Integer
    emptyCell.Occupied = 0
    emptyCell.TileId = 0
    emptyCell.MatchId = 0
    For r = 1 To 9
        n = 0
        For c = 1 To 16
            If Board(c, r).Occupied = -1 Then
                n = n + 1
                cells(n) = Board(c, r)
            End If
        Next c
        If n < 16 And n > 0 Then
            m = 1
            For c = 1 To 16
                If c <= 16 - n Then
                    Board(c, r) = emptyCell
                Else
                    Board(c, r) = cells(m)
                    m = m + 1
                End If
            Next c
        End If
    Next r
End Sub

' === slot 08 (0x4A9E50): halves compact away from the centre ========
Private Sub ShiftOutwardH()
    Dim cells(1 To 16) As BoardCell
    Dim emptyCell As BoardCell
    Dim r As Integer
    Dim c As Integer
    Dim n As Integer
    Dim m As Integer
    emptyCell.Occupied = 0
    emptyCell.TileId = 0
    emptyCell.MatchId = 0
    For r = 1 To 9
        n = 0
        For c = 1 To 16 / 2
            If Board(c, r).Occupied = -1 Then
                n = n + 1
                cells(n) = Board(c, r)
            End If
        Next c
        If n < 16 / 2 And n > 0 Then
            m = 1
            For c = 1 To 16 / 2
                If c <= n Then
                    Board(c, r) = cells(m)
                    m = m + 1
                Else
                    Board(c, r) = emptyCell
                End If
            Next c
        End If
    Next r
    For r = 1 To 9
        n = 0
        For c = 16 / 2 + 1 To 16
            If Board(c, r).Occupied = -1 Then
                n = n + 1
                cells(n) = Board(c, r)
            End If
        Next c
        If n < 16 / 2 And n > 0 Then
            m = 1
            For c = 1 To 16 / 2
                If c > 16 / 2 - n Then
                    Board(16 / 2 + c, r) = cells(m)
                    m = m + 1
                Else
                    Board(16 / 2 + c, r) = emptyCell
                End If
            Next c
        End If
    Next r
End Sub

' === slot 09 (0x4AA4F0): halves compact toward the centre ===========
Private Sub ShiftInwardH()
    Dim cells(1 To 16) As BoardCell
    Dim emptyCell As BoardCell
    Dim r As Integer
    Dim c As Integer
    Dim n As Integer
    Dim m As Integer
    emptyCell.Occupied = 0
    emptyCell.TileId = 0
    emptyCell.MatchId = 0
    For r = 1 To 9
        n = 0
        For c = 1 To 16 / 2
            If Board(c, r).Occupied = -1 Then
                n = n + 1
                cells(n) = Board(c, r)
            End If
        Next c
        If n < 16 / 2 And n > 0 Then
            m = 1
            For c = 1 To 16 / 2
                If c > 16 / 2 - n Then
                    Board(c, r) = cells(m)
                    m = m + 1
                Else
                    Board(c, r) = emptyCell
                End If
            Next c
        End If
    Next r
    For r = 1 To 9
        n = 0
        For c = 16 / 2 + 1 To 16
            If Board(c, r).Occupied = -1 Then
                n = n + 1
                cells(n) = Board(c, r)
            End If
        Next c
        If n < 16 / 2 And n > 0 Then
            m = 1
            For c = 1 To 16 / 2
                If c <= n Then
                    Board(16 / 2 + c, r) = cells(m)
                    m = m + 1
                Else
                    Board(16 / 2 + c, r) = emptyCell
                End If
            Next c
        End If
    Next r
End Sub

' === slot 10 (0x4AABA0) =============================================
Private Sub RedrawBoard()
    Dim i As Integer
    Dim anchor As Long
    Dim bar As Single
    Dim s As String
    Dim r As Integer
    Dim c As Integer
    Dim t1 As Integer
    Dim t2 As Integer
    Dim px As Long
    Dim py As Long
    Pic_Buffer.Cls
    anchor = (Pic_Buffer.ScaleWidth - 300) \ 2
    If Lives > 0 Then
        BitBltRet = BitBlt(Pic_Buffer.hDC, 15, 10, 20, 30, Pic_Data.hDC, 0, 25, SRCCOPY)
        s = CStr(Lives)
        For i = 1 To Len(s)
            BitBltRet = BitBlt(Pic_Buffer.hDC, i * 15 + 25, 10, 15, 25, Pic_Data.hDC, Val(Mid$(s, i, 1)) * 15, 0, SRCCOPY)
        Next i
    End If
    Pic_Buffer.Line (anchor - 2, 6)-(anchor + 301, 24), QBColor(14), B
    bar = (1 - LastTick / (TimeLeft * 1000)) * 300
    If bar > 0 Then
        BitBltRet = BitBlt(Pic_Buffer.hDC, anchor, 8, CInt(bar), 15, Pic_Data.hDC, 20, 25, SRCCOPY)
    End If
    s = CStr(Score)
    For i = 1 To Len(s)
        BitBltRet = BitBlt(Pic_Buffer.hDC, Pic_Buffer.ScaleWidth - 100 + i * 15, 10, 15, 25, Pic_Data.hDC, Val(Mid$(s, i, 1)) * 15, 0, SRCCOPY)
    Next i
    For r = 0 To 10
        For c = 0 To 17
            If Board(c, r).Occupied = -1 Then
                t1 = Board(c, r).TileId Mod 9
                t2 = Board(c, r).TileId \ 9
                px = OffX + c * 40
                py = OffY + r * 50
                BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 46, 56, Pic_MjData.hDC, 276, 500, SRCAND)
                If c = Sel1C And r = Sel1R Then
                    BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 46, 56, Pic_MjData.hDC, 46 * PaletteOff, 556, SRCPAINT)
                Else
                    BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 46, 56, Pic_MjData.hDC, 46 * PaletteOff, 500, SRCPAINT)
                End If
                If m_BoardActive = -1 Then
                    BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 40, 50, Pic_MjData.hDC, t1 * 40, (t2 + 5) * 50, SRCAND)
                    BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 40, 50, Pic_MjData.hDC, t1 * 40, t2 * 50, SRCPAINT)
                End If
                If c = HoverC And r = HoverR Then
                    Pic_Buffer.Line (px + 1, py + 1)-(px + 39, py + 49), QBColor(12), B
                    Pic_Buffer.Line (px + 2, py + 2)-(px + 38, py + 48), QBColor(14), B
                    Pic_Buffer.Line (px + 3, py + 3)-(px + 37, py + 47), QBColor(12), B
                End If
            End If
        Next c
    Next r
    Pic_Show.Cls
    BitBltRet = BitBlt(Pic_Show.hDC, 0, 0, Pic_Show.ScaleWidth, Pic_Show.ScaleHeight, Pic_Buffer.hDC, 0, 0, SRCCOPY)
    Pic_Show.Refresh
End Sub

' === slot 11 (0x4AC390) =============================================
Private Sub AnimateLinkAndRemove()
    Dim anchor As Long
    Dim bar As Single
    Dim s As String
    Dim r As Integer
    Dim c As Integer
    Dim t1 As Integer
    Dim t2 As Integer
    Dim px As Long
    Dim py As Long
    Dim i As Integer
    Dim lc(1 To 3) As Integer
    Dim savedW As Integer
    Dim x0 As Single
    Dim y0 As Single
    Dim x1 As Single
    Dim y1 As Single
    Dim x2 As Single
    Dim y2 As Single
    Dim x3 As Single
    Dim y3 As Single
    Pic_Show.Enabled = False
    Pic_Buffer.Cls
    anchor = (Pic_Buffer.ScaleWidth - 300) \ 2
    If Lives > 0 Then
        BitBltRet = BitBlt(Pic_Buffer.hDC, 15, 10, 20, 30, Pic_Data.hDC, 0, 25, SRCCOPY)
        s = CStr(Lives)
        For i = 1 To Len(s)
            BitBltRet = BitBlt(Pic_Buffer.hDC, i * 15 + 25, 10, 15, 25, Pic_Data.hDC, Val(Mid$(s, i, 1)) * 15, 0, SRCCOPY)
        Next i
    End If
    Pic_Buffer.Line (anchor - 2, 6)-(anchor + 301, 24), QBColor(14), B
    bar = (1 - LastTick / (TimeLeft * 1000)) * 300
    If bar > 0 Then
        BitBltRet = BitBlt(Pic_Buffer.hDC, anchor, 8, CInt(bar), 15, Pic_Data.hDC, 20, 25, SRCCOPY)
    End If
    s = CStr(Score)
    For i = 1 To Len(s)
        BitBltRet = BitBlt(Pic_Buffer.hDC, Pic_Buffer.ScaleWidth - 100 + i * 15, 10, 15, 25, Pic_Data.hDC, Val(Mid$(s, i, 1)) * 15, 0, SRCCOPY)
    Next i
    For r = 1 To 9
        For c = 1 To 16
            If Board(c, r).Occupied = -1 Then
                t1 = Board(c, r).TileId Mod 9
                t2 = Board(c, r).TileId \ 9
                px = OffX + c * 40
                py = OffY + r * 50
                BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 46, 56, Pic_MjData.hDC, 276, 500, SRCAND)
                BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 46, 56, Pic_MjData.hDC, 46 * PaletteOff, 500, SRCPAINT)
                If c = Sel1C And r = Sel1R Then
                    BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 46, 56, Pic_MjData.hDC, 276, 500, SRCAND)
                    BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 46, 56, Pic_MjData.hDC, 46 * PaletteOff, 556, SRCPAINT)
                End If
                If c = Sel2C And r = Sel2R Then
                    BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 46, 56, Pic_MjData.hDC, 276, 500, SRCAND)
                    BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 46, 56, Pic_MjData.hDC, 46 * PaletteOff, 556, SRCPAINT)
                End If
                BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 40, 50, Pic_MjData.hDC, t1 * 40, (t2 + 5) * 50, SRCAND)
                BitBltRet = BitBlt(Pic_Buffer.hDC, px, py, 40, 50, Pic_MjData.hDC, t1 * 40, t2 * 50, SRCPAINT)
            End If
        Next c
    Next r
    x0 = LinkX1 * 40 + OffX + 40 / 2
    y0 = LinkY1 * 50 + OffY + 50 / 2
    x1 = LinkX2 * 40 + OffX + 40 / 2
    y1 = LinkY2 * 50 + OffY + 50 / 2
    x2 = LinkX3 * 40 + OffX + 40 / 2
    y2 = LinkY3 * 50 + OffY + 50 / 2
    x3 = LinkX4 * 40 + OffX + 40 / 2
    y3 = LinkY4 * 50 + OffY + 50 / 2
    lc(1) = 14
    lc(2) = 12
    lc(3) = 15
    savedW = Pic_Buffer.DrawWidth
    For i = 3 To 1 Step -1
        Pic_Buffer.DrawWidth = i * 2
        Select Case LinkN
            Case 3
                Pic_Buffer.Line (x0, y0)-(x1, y1), QBColor(lc(i)), B
                Pic_Buffer.Line (x1, y1)-(x2, y2), QBColor(lc(i)), B
                Pic_Buffer.Line (x2, y2)-(x3, y3), QBColor(lc(i)), B
            Case 2
                Pic_Buffer.Line (x0, y0)-(x1, y1), QBColor(lc(i)), B
                Pic_Buffer.Line (x1, y1)-(x2, y2), QBColor(lc(i)), B
            Case 1
                Pic_Buffer.Line (x0, y0)-(x1, y1), QBColor(lc(i)), B
        End Select
    Next i
    Pic_Buffer.DrawWidth = savedW
    Pic_Show.Cls
    BitBltRet = BitBlt(Pic_Show.hDC, 0, 0, Pic_Show.ScaleWidth, Pic_Show.ScaleHeight, Pic_Buffer.hDC, 0, 0, SRCCOPY)
    Pic_Show.Refresh
    DelayMs 250
    Pic_Show.Enabled = True
End Sub

' === slot 12 (0x4ADF80) =============================================
Private Sub DrawTitleScreen()
    Pic_Show.Cls
    BitBltRet = BitBlt(Pic_Show.hDC, (Pic_Show.ScaleWidth - Pic_Title.ScaleWidth) \ 2, (Pic_Show.ScaleHeight - Pic_Title.ScaleHeight) \ 2 - 20, Pic_Title.ScaleWidth, Pic_Title.ScaleHeight, Pic_Title.hDC, 0, 0, SRCCOPY)
    BitBltRet = BitBlt(Pic_Show.hDC, (Pic_Show.ScaleWidth - 290) \ 2, Pic_Show.ScaleHeight - 100, 290, 25, Pic_Data.hDC, 220, 0, SRCCOPY)
    Pic_Show.Refresh
    Pic_Buffer.Cls
End Sub

' === slot 13 (0x4AE4D0) =============================================
Private Function HasValidMove() As Boolean
    Dim r As Integer
    Dim c As Integer
    Dim idx As Integer
    Dim k As Integer
    Dim c2 As Integer
    Dim r2 As Integer
    HasValidMove = False
    idx = -1
    For r = 0 To 10
        For c = 0 To 17
            idx = idx + 1
            If Board(c, r).Occupied = -1 Then
                If idx < 197 Then
                    For k = idx + 1 To 197
                        c2 = k Mod 18
                        r2 = k \ 18
                        If Board(c2, r2).Occupied = -1 Then
                            If Board(c, r).MatchId = Board(c2, r2).MatchId Then
                                If CheckLink(c, r, c2, r2) Then
                                    HasValidMove = True
                                    Exit Function
                                End If
                            End If
                        End If
                    Next k
                End If
            End If
        Next c
    Next r
End Function

' === slot 14 (0x4AE690) =============================================
Private Function CheckLink(ByVal x1 As Long, ByVal y1 As Long, ByVal x2 As Long, ByVal y2 As Long) As Boolean
    Dim t As Long
    CheckLink = False
    If x1 > x2 Then
        t = x1
        x1 = x2
        x2 = t
        t = y1
        y1 = y2
        y2 = t
    End If
    If x1 = x2 Then
        If y1 > y2 Then
            t = y1
            y1 = y2
            y2 = t
        End If
        CheckLink = CheckLink0(x1, y1, x2, y2)
    End If
    If x1 < x2 Then
        If y1 = y2 Then
            CheckLink = CheckLink1(x1, y1, x2, y2)
        End If
        If y1 < y2 Then
            CheckLink = CheckLink2(x1, y1, x2, y2)
        End If
        If y1 > y2 Then
            CheckLink = CheckLink3(x1, y1, x2, y2)
        End If
    End If
End Function

' === slot 15 (0x4AE760): same column (x1 = x2, y1 <= y2) ============
Private Function CheckLink0(ByVal x1 As Long, ByVal y1 As Long, ByVal x2 As Long, ByVal y2 As Long) As Boolean
    Dim i As Long
    Dim m As Long
    Dim cnt As Long
    CheckLink0 = False
    cnt = 0
    For i = y1 To y2
        If Board(x1, i).Occupied = -1 Then cnt = cnt + 1
    Next i
    If cnt = 2 Then
        LinkN = 1
        LinkX1 = x1
        LinkY1 = y1
        LinkX2 = x2
        LinkY2 = y2
        CheckLink0 = True
        Exit Function
    End If
    For m = x1 - 1 To 0 Step -1
        cnt = 0
        For i = m To x1
            If Board(i, y1).Occupied = -1 Then cnt = cnt + 1
            If Board(i, y2).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = y1 To y2
            If Board(m, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = m
            LinkY2 = y1
            LinkX3 = m
            LinkY3 = y2
            LinkX4 = x2
            LinkY4 = y2
            CheckLink0 = True
            Exit Function
        End If
    Next m
    For m = x1 + 1 To 17
        cnt = 0
        For i = x1 To m
            If Board(i, y1).Occupied = -1 Then cnt = cnt + 1
            If Board(i, y2).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = y1 To y2
            If Board(m, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = m
            LinkY2 = y1
            LinkX3 = m
            LinkY3 = y2
            LinkX4 = x2
            LinkY4 = y2
            CheckLink0 = True
            Exit Function
        End If
    Next m
End Function

' === slot 16 (0x4AEAE0): same row (y1 = y2, x1 < x2) ================
Private Function CheckLink1(ByVal x1 As Long, ByVal y1 As Long, ByVal x2 As Long, ByVal y2 As Long) As Boolean
    Dim i As Long
    Dim m As Long
    Dim cnt As Long
    CheckLink1 = False
    cnt = 0
    For i = x1 To x2
        If Board(i, y1).Occupied = -1 Then cnt = cnt + 1
    Next i
    If cnt = 2 Then
        LinkN = 1
        LinkX1 = x1
        LinkY1 = y1
        LinkX2 = x2
        LinkY2 = y2
        CheckLink1 = True
        Exit Function
    End If
    For m = y1 - 1 To 0 Step -1
        cnt = 0
        For i = m To y1
            If Board(x1, i).Occupied = -1 Then cnt = cnt + 1
            If Board(x2, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = x1 To x2
            If Board(i, m).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = x1
            LinkY2 = m
            LinkX3 = x2
            LinkY3 = m
            LinkX4 = x2
            LinkY4 = y2
            CheckLink1 = True
            Exit Function
        End If
    Next m
    For m = y1 + 1 To 10
        cnt = 0
        For i = y1 To m
            If Board(x1, i).Occupied = -1 Then cnt = cnt + 1
            If Board(x2, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = x1 To x2
            If Board(i, m).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = x1
            LinkY2 = m
            LinkX3 = x2
            LinkY3 = m
            LinkX4 = x2
            LinkY4 = y2
            CheckLink1 = True
            Exit Function
        End If
    Next m
End Function

' === slot 17 (0x4AEE30): x1 < x2, y1 < y2 ===========================
Private Function CheckLink2(ByVal x1 As Long, ByVal y1 As Long, ByVal x2 As Long, ByVal y2 As Long) As Boolean
    Dim i As Long
    Dim m As Long
    Dim cnt As Long
    CheckLink2 = False
    ' 1-corner path through (x2, y1)
    cnt = 0
    For i = x1 To x2
        If Board(i, y1).Occupied = -1 Then cnt = cnt + 1
    Next i
    For i = y1 To y2
        If Board(x2, i).Occupied = -1 Then cnt = cnt + 1
    Next i
    If cnt = 2 Then
        LinkN = 2
        LinkX1 = x1
        LinkY1 = y1
        LinkX2 = x2
        LinkY2 = y1
        LinkX3 = x2
        LinkY3 = y2
        CheckLink2 = True
        Exit Function
    End If
    ' 1-corner path through (x1, y2)
    cnt = 0
    For i = y1 To y2
        If Board(x1, i).Occupied = -1 Then cnt = cnt + 1
    Next i
    For i = x1 To x2
        If Board(i, y2).Occupied = -1 Then cnt = cnt + 1
    Next i
    If cnt = 2 Then
        LinkN = 2
        LinkX1 = x1
        LinkY1 = y1
        LinkX2 = x1
        LinkY2 = y2
        LinkX3 = x2
        LinkY3 = y2
        CheckLink2 = True
        Exit Function
    End If
    ' Stage 1: interior columns x1 + 1 .. x2 - 1
    If x2 - x1 > 1 Then
        For m = x1 + 1 To x2 - 1
            cnt = 0
            For i = x1 To m
                If Board(i, y1).Occupied = -1 Then cnt = cnt + 1
            Next i
            For i = m To x2
                If Board(i, y2).Occupied = -1 Then cnt = cnt + 1
            Next i
            For i = y1 To y2
                If Board(m, i).Occupied = -1 Then cnt = cnt + 1
            Next i
            If cnt = 2 Then
                LinkN = 3
                LinkX1 = x1
                LinkY1 = y1
                LinkX2 = m
                LinkY2 = y1
                LinkX3 = m
                LinkY3 = y2
                LinkX4 = x2
                LinkY4 = y2
                CheckLink2 = True
                Exit Function
            End If
        Next m
    End If
    ' Stage 2: interior rows y1 + 1 .. y2 - 1
    If y2 - y1 > 1 Then
        For m = y1 + 1 To y2 - 1
            cnt = 0
            For i = y1 To m
                If Board(x1, i).Occupied = -1 Then cnt = cnt + 1
            Next i
            For i = m To y2
                If Board(x2, i).Occupied = -1 Then cnt = cnt + 1
            Next i
            For i = x1 To x2
                If Board(i, m).Occupied = -1 Then cnt = cnt + 1
            Next i
            If cnt = 2 Then
                LinkN = 3
                LinkX1 = x1
                LinkY1 = y1
                LinkX2 = x1
                LinkY2 = m
                LinkX3 = x2
                LinkY3 = m
                LinkX4 = x2
                LinkY4 = y2
                CheckLink2 = True
                Exit Function
            End If
        Next m
    End If
    ' Stage 3: rows above y1 (y1 - 1 down to 0)
    For m = y1 - 1 To 0 Step -1
        cnt = 0
        For i = m To y1
            If Board(x1, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = m To y2
            If Board(x2, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = x1 To x2
            If Board(i, m).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = x1
            LinkY2 = m
            LinkX3 = x2
            LinkY3 = m
            LinkX4 = x2
            LinkY4 = y2
            CheckLink2 = True
            Exit Function
        End If
    Next m
    ' Stage 4: rows below y2 (y2 + 1 up to 10)
    For m = y2 + 1 To 10
        cnt = 0
        For i = y1 To m
            If Board(x1, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = y2 To m
            If Board(x2, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = x1 To x2
            If Board(i, m).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = x1
            LinkY2 = m
            LinkX3 = x2
            LinkY3 = m
            LinkX4 = x2
            LinkY4 = y2
            CheckLink2 = True
            Exit Function
        End If
    Next m
    ' Stage 5: columns left of x1 (x1 - 1 down to 0)
    For m = x1 - 1 To 0 Step -1
        cnt = 0
        For i = m To x1
            If Board(i, y1).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = m To x2
            If Board(i, y2).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = y1 To y2
            If Board(m, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = m
            LinkY2 = y1
            LinkX3 = m
            LinkY3 = y2
            LinkX4 = x2
            LinkY4 = y2
            CheckLink2 = True
            Exit Function
        End If
    Next m
    ' Stage 6: columns right of x2 (x2 + 1 up to 17)
    For m = x2 + 1 To 17
        cnt = 0
        For i = x1 To m
            If Board(i, y1).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = x2 To m
            If Board(i, y2).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = y1 To y2
            If Board(m, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = m
            LinkY2 = y1
            LinkX3 = m
            LinkY3 = y2
            LinkX4 = x2
            LinkY4 = y2
            CheckLink2 = True
            Exit Function
        End If
    Next m
End Function

' === slot 18 (0x4AF7F0): x1 < x2, y1 > y2 ===========================
Private Function CheckLink3(ByVal x1 As Long, ByVal y1 As Long, ByVal x2 As Long, ByVal y2 As Long) As Boolean
    Dim i As Long
    Dim m As Long
    Dim cnt As Long
    CheckLink3 = False
    ' 1-corner path through (x2, y1)
    cnt = 0
    For i = x1 To x2
        If Board(i, y1).Occupied = -1 Then cnt = cnt + 1
    Next i
    For i = y2 To y1
        If Board(x2, i).Occupied = -1 Then cnt = cnt + 1
    Next i
    If cnt = 2 Then
        LinkN = 2
        LinkX1 = x1
        LinkY1 = y1
        LinkX2 = x2
        LinkY2 = y1
        LinkX3 = x2
        LinkY3 = y2
        CheckLink3 = True
        Exit Function
    End If
    ' 1-corner path through (x1, y2)
    cnt = 0
    For i = y2 To y1
        If Board(x1, i).Occupied = -1 Then cnt = cnt + 1
    Next i
    For i = x1 To x2
        If Board(i, y2).Occupied = -1 Then cnt = cnt + 1
    Next i
    If cnt = 2 Then
        LinkN = 2
        LinkX1 = x1
        LinkY1 = y1
        LinkX2 = x1
        LinkY2 = y2
        LinkX3 = x2
        LinkY3 = y2
        CheckLink3 = True
        Exit Function
    End If
    ' Stage 1: interior columns x1 + 1 .. x2 - 1
    If x2 - x1 > 1 Then
        For m = x1 + 1 To x2 - 1
            cnt = 0
            For i = x1 To m
                If Board(i, y1).Occupied = -1 Then cnt = cnt + 1
            Next i
            For i = m To x2
                If Board(i, y2).Occupied = -1 Then cnt = cnt + 1
            Next i
            For i = y2 To y1
                If Board(m, i).Occupied = -1 Then cnt = cnt + 1
            Next i
            If cnt = 2 Then
                LinkN = 3
                LinkX1 = x1
                LinkY1 = y1
                LinkX2 = m
                LinkY2 = y1
                LinkX3 = m
                LinkY3 = y2
                LinkX4 = x2
                LinkY4 = y2
                CheckLink3 = True
                Exit Function
            End If
        Next m
    End If
    ' Stage 2: interior rows y2 + 1 .. y1 - 1
    If y1 - y2 > 1 Then
        For m = y2 + 1 To y1 - 1
            cnt = 0
            For i = m To y1
                If Board(x1, i).Occupied = -1 Then cnt = cnt + 1
            Next i
            For i = y2 To m
                If Board(x2, i).Occupied = -1 Then cnt = cnt + 1
            Next i
            For i = x1 To x2
                If Board(i, m).Occupied = -1 Then cnt = cnt + 1
            Next i
            If cnt = 2 Then
                LinkN = 3
                LinkX1 = x1
                LinkY1 = y1
                LinkX2 = x1
                LinkY2 = m
                LinkX3 = x2
                LinkY3 = m
                LinkX4 = x2
                LinkY4 = y2
                CheckLink3 = True
                Exit Function
            End If
        Next m
    End If
    ' Stage 3: rows above y2 (y2 - 1 down to 0)
    For m = y2 - 1 To 0 Step -1
        cnt = 0
        For i = m To y1
            If Board(x1, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = m To y2
            If Board(x2, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = x1 To x2
            If Board(i, m).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = x1
            LinkY2 = m
            LinkX3 = x2
            LinkY3 = m
            LinkX4 = x2
            LinkY4 = y2
            CheckLink3 = True
            Exit Function
        End If
    Next m
    ' Stage 4: rows below y1 (y1 + 1 up to 10)
    For m = y1 + 1 To 10
        cnt = 0
        For i = y1 To m
            If Board(x1, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = y2 To m
            If Board(x2, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = x1 To x2
            If Board(i, m).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = x1
            LinkY2 = m
            LinkX3 = x2
            LinkY3 = m
            LinkX4 = x2
            LinkY4 = y2
            CheckLink3 = True
            Exit Function
        End If
    Next m
    ' Stage 5: columns left of x1 (x1 - 1 down to 0)
    For m = x1 - 1 To 0 Step -1
        cnt = 0
        For i = m To x1
            If Board(i, y1).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = m To x2
            If Board(i, y2).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = y2 To y1
            If Board(m, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = m
            LinkY2 = y1
            LinkX3 = m
            LinkY3 = y2
            LinkX4 = x2
            LinkY4 = y2
            CheckLink3 = True
            Exit Function
        End If
    Next m
    ' Stage 6: columns right of x2 (x2 + 1 up to 17)
    For m = x2 + 1 To 17
        cnt = 0
        For i = x1 To m
            If Board(i, y1).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = x2 To m
            If Board(i, y2).Occupied = -1 Then cnt = cnt + 1
        Next i
        For i = y2 To y1
            If Board(m, i).Occupied = -1 Then cnt = cnt + 1
        Next i
        If cnt = 2 Then
            LinkN = 3
            LinkX1 = x1
            LinkY1 = y1
            LinkX2 = m
            LinkY2 = y1
            LinkX3 = m
            LinkY3 = y2
            LinkX4 = x2
            LinkY4 = y2
            CheckLink3 = True
            Exit Function
        End If
    Next m
End Function

' === slot 19 (0x4B01B0) =============================================
Private Sub GameLoop()
    Dim t1 As Long
    Dim t2 As Long
    Level = 0
    Score = 0
    Lives = 10
    TimeLeft = 600
    m_QuitFlag = 0
    Do
        InitBoard
        Do
            ShuffleRemaining
            DoEvents
        Loop While Not HasValidMove()
        RedrawBoard
        Pic_Show.Enabled = True
        GameState = 0
        LastTick = 0
        t1 = GetTickCount()
        Do
            DelayMs 15
            t2 = GetTickCount()
            LastTick = LastTick + t2 - t1
            t1 = t2
            ProcessSelection
            RedrawBoard
            If LastTick >= TimeLeft * 1000 Then GameState = 3
            If GameState = 3 Then
                If SoundOn = -1 Then PlayWave 204, 1
                TimeoutOver
            ElseIf GameState = 2 Then
                If SoundOn = -1 Then PlayWave 206, 1
                DeadlockHandler
            ElseIf GameState = 1 Then
                If SoundOn = -1 Then PlayWave 205, 1
                LevelClear
            End If
            DoEvents
            If m_BoardActive = 0 Then
                Exit Do
            ElseIf m_QuitFlag = -1 Then
                Exit Do
            End If
        Loop
        DoEvents
    Loop While m_QuitFlag <> -1
    Pic_Show.Enabled = False
    Pic_Show.Cls
    Pic_Buffer.Cls
    Mem_NewGame.Enabled = True
    Mem_QuitGame.Enabled = False
    DrawTitleScreen
End Sub

' === slot 20 (0x4B05F0) =============================================
Private Sub ProcessSelection()
    Dim c1 As Integer
    Dim r1 As Integer
    Dim c2 As Integer
    Dim r2 As Integer
    If Sel2C > 0 Then
        c1 = CInt(Sel1C)
        r1 = CInt(Sel1R)
        c2 = CInt(Sel2C)
        r2 = CInt(Sel2R)
        If Board(c1, r1).MatchId = Board(c2, r2).MatchId Then
            If CheckLink(c1, r1, c2, r2) Then
                If SoundOn = -1 Then PlayWave 203, 1
                AnimateLinkAndRemove
                Board(c1, r1).Occupied = 0
                Board(c2, r2).Occupied = 0
                ClearSelection
                Select Case Level
                    Case 1: ShiftUp
                    Case 2: ShiftDown
                    Case 3: ShiftLeft
                    Case 4: ShiftRight
                    Case 5: ShiftOutwardH
                    Case 6: ShiftInwardH
                End Select
                Score = Score + 20
                TilesLeft = TilesLeft - 2
                If TilesLeft < 1 Then
                    GameState = 1
                    ClearSelection
                ElseIf Not HasValidMove() Then
                    GameState = 2
                    ClearSelection
                Else
                    ClearSelection
                End If
            Else
                If SoundOn = -1 Then PlayWave 204, 1
                ClearSelection
            End If
        Else
            If SoundOn = -1 Then PlayWave 204, 1
            ClearSelection
        End If
    End If
End Sub

' === slot 21 (0x4B09C0) =============================================
Private Sub LevelClear()
    Dim bonus As Integer
    Level = Level + 1
    Lives = Lives + 1
    If Level > 6 Then
        bonus = CInt(TimeLeft - LastTick \ 1000 + Lives * 50)
        Score = Score + bonus
        MsgBox "真是厲害!!恭喜完成所有關卡。" & vbCrLf & "剩餘時間及生命能量可以獲得 " & CStr(bonus) & " 分" & vbCrLf & "累計總得分為 " & CStr(Score) & " 分", vbExclamation, "恭喜破關"
        m_BoardActive = 0
        m_QuitFlag = -1
        NewScoreIdx = IsHighScore(Score)
        If NewScoreIdx = -1 Then Form_Table.Show 1
        GameState = 0
    Else
        bonus = CInt(TimeLeft - LastTick \ 1000)
        Score = Score + bonus
        MsgBox "過關成功，請進入第 " & CStr(Level + 1) & " 關。" & vbCrLf & "剩餘時間能量可以獲得 " & CStr(bonus) & " 分", vbExclamation, "成功"
        m_BoardActive = 0
        GameState = 0
    End If
End Sub

' === slot 22 (0x4B0F90) =============================================
Private Sub DeadlockHandler()
    Lives = Lives - 1
    If Lives >= 0 Then
        Do
            ShuffleRemaining
            DoEvents
        Loop While Not HasValidMove()
        GameState = 0
    Else
        MsgBox "已無牌可消除，遊戲結束。", vbExclamation, "殘局"
        m_QuitFlag = -1
        NewScoreIdx = IsHighScore(Score)
        If NewScoreIdx = -1 Then Form_Table.Show 1
        GameState = 0
    End If
End Sub

' === slot 23 (0x4B11C0) =============================================
Private Sub TimeoutOver()
    Dim bonus As Integer
    bonus = Lives * 50
    Score = Score + bonus
    MsgBox "時間能量用盡，遊戲結束。" & vbCrLf & "剩餘生命能量可以獲得 " & CStr(bonus) & " 分" & vbCrLf & "累計總得分為 " & CStr(Score) & " 分", vbExclamation, "殘局"
    m_QuitFlag = -1
    NewScoreIdx = IsHighScore(Score)
    If NewScoreIdx = -1 Then Form_Table.Show 1
    GameState = 0
End Sub

' === slot 24 (0x4B1500) =============================================
Private Sub InitHighScoreFile()
    Dim recs(0 To 9) As RecordType
    Dim i As Integer
    Dim fno As Integer
    For i = 0 To 9
        recs(i).Name = ""
        recs(i).Score = 0
    Next i
    fno = FreeFile
    Open HisFile For Binary As #fno
    For i = 0 To 9
        recs(i).Name = Scramble(recs(i).Name)
        Put #fno, , recs(i)
    Next i
    Close #fno
End Sub

' === slot 25 (0x4B1710) =============================================
Private Function IsHighScore(ByVal sc As Single) As Boolean
    Dim recs(0 To 9) As RecordType
    Dim i As Integer
    Dim fno As Integer
    On Error GoTo ErrHandler
    If Dir$(HisFile) <> "" Then
        fno = FreeFile
        Open HisFile For Binary As #fno
        For i = 0 To 9
            Get #fno, , recs(i)
        Next i
        Close #fno
        If sc > recs(9).Score Then
            IsHighScore = True
        Else
            IsHighScore = False
        End If
    Else
        IsHighScore = False
    End If
    Exit Function
ErrHandler:
    IsHighScore = False
End Function

' === slot 26 (0x4B18E0) =============================================
Private Sub Form_Load()
    Dim buf As String
    Dim n As Long
    If App.PrevInstance Then End
    buf = Space$(255)
    n = GetWindowsDirectoryA(buf, 255)
    WinDirPath = Left$(buf, n)
    HisFile = WinDirPath & "\d4s.hst"
    If Dir$(HisFile) = "" Then InitHighScoreFile
    NewScoreIdx = 0
    PlayerName = "Player"
    buf = Space$(255)
    n = GetTempPathA(255, buf)
    TempDir = Left$(buf, n)
    If HasSoundCard() Then
        m_HasSound = -1
        SoundOn = -1
        Mem_Sound.Checked = True
        MidiPath = WinDirPath & "\D4S.MID"
        m_MidiTrack = 101
        If ExtractMidi(m_MidiTrack, "MIDI", MidiPath) Then
            PlayMidi MidiPath
            Timer_PlayMidi.Enabled = True
            MusicOn = -1
            Mem_Music.Checked = True
        End If
    Else
        Mem_BackGround.Enabled = False
        MusicOn = 0
        SoundOn = 0
    End If
    Randomize Timer
    InitLayout
    DrawTitleScreen
End Sub

' === slot 27 (0x4B1DB0) =============================================
Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
    Dim ret As String
    m_QuitFlag = -1
    DelayMs 100
    Pic_Buffer.Cls
    Pic_Show.Cls
    Set Pic_Buffer.Picture = LoadPicture("")
    Set Pic_Show.Picture = LoadPicture("")
    If m_HasSound = -1 Then
        If Timer_PlayMidi.Enabled = True Then
            Timer_PlayMidi.Enabled = False
            mciSendStringA "close MyMidi", ret, 0, 0
        End If
        If Dir$(MidiPath) <> "" Then Kill MidiPath
    End If
    End
End Sub

' === slot 28 (0x4B21D0) =============================================
Private Sub Mem_Author_Click()
    Form_Author.Show 1
End Sub

' === slot 29 (0x4B22C0) =============================================
Private Sub Mem_Exit_Click()
    Unload Me
End Sub

' === slot 30 (0x4B23A0) =============================================
Private Sub Mem_Explain_Click()
    Form_Explain.Show 1
End Sub

' === slot 31 (0x4B2490) =============================================
Private Sub Mem_Music_Click()
    MusicOn = Not MusicOn
    Mem_Music.Checked = Not Mem_Music.Checked
    If MusicOn = -1 Then
        PlayMidi MidiPath
        Timer_PlayMidi.Enabled = True
    Else
        Timer_PlayMidi.Enabled = False
        mciSendStringA "close MyMidi", "", 0, 0
    End If
End Sub

' === slot 32 (0x4B26B0) =============================================
Private Sub Mem_NewGame_Click()
    Mem_NewGame.Enabled = False
    Mem_QuitGame.Enabled = True
    GameLoop
End Sub

' === slot 33 (0x4B27B0) =============================================
Private Sub Mem_QuitGame_Click()
    m_QuitFlag = -1
End Sub

' === slot 34 (0x4B2820) =============================================
Private Sub Mem_Record_Click()
    Form_Table.Show 1
End Sub

' === slot 35 (0x4B2910) =============================================
Private Sub Mem_Sound_Click()
    SoundOn = Not SoundOn
    Mem_Sound.Checked = Not Mem_Sound.Checked
End Sub

' === slot 36 (0x4B2A30) =============================================
Private Sub Pic_Show_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
    Dim dx As Single
    Dim dy As Single
    If Button = 1 Then
        dx = X - OffX
        dy = Y - OffY
        If dx > 0 And dx < 720 And dy > 0 And dy < 550 Then
            dx = dx \ 40
            dy = dy \ 50
        End If
        If Sel1C < 0 Then
            If Board(dx, dy).Occupied = -1 Then
                Sel1C = dx
                Sel1R = dy
                If SoundOn = -1 Then PlayWave 202, 1
                RedrawBoard
            End If
        ElseIf Sel2C < 0 Then
            If Board(dx, dy).Occupied = -1 Then
                If Not (dx = Sel1C And dy = Sel1R) Then
                    Sel2C = dx
                    Sel2R = dy
                End If
            End If
        End If
    Else
        If Sel1C > 0 Then
            Sel1C = -1
            Sel1R = -1
            Sel2C = -1
            Sel2R = -1
            If SoundOn = -1 Then PlayWave 201, 1
        End If
    End If
End Sub

' === slot 37 (0x4B2D70) =============================================
Private Sub Pic_Show_MouseMove(Button As Integer, Shift As Integer, X As Single, Y As Single)
    Dim dx As Single
    Dim dy As Single
    dx = X - OffX
    dy = Y - OffY
    If dx > 0 And dx < 720 And dy > 0 And dy < 550 Then
        dx = dx \ 40
        dy = dy \ 50
        If Board(dx, dy).Occupied = -1 Then
            HoverC = dx
            HoverR = dy
        Else
            HoverC = -1
            HoverR = -1
        End If
    Else
        HoverC = -1
        HoverR = -1
    End If
End Sub

' === slot 38 (0x4B2F20) =============================================
Private Sub PlayMidi(ByVal pth As String)
    Dim ret As String
    mciSendStringA "close MyMidi", ret, 0, 0
    mciSendStringA "open " & pth & " alias MyMidi", ret, 0, 0
    mciSendStringA "play MyMidi", ret, 0, 0
End Sub

' === slot 39 (0x4B30D0) =============================================
' Original v1.00-cn kills the .MID while the stopped mcimidi device still holds
' it open (Win9x allowed that; NT/Wine raises VB error 70).
Private Sub Timer_PlayMidi_Timer()
    Dim st As String * 80
    Dim ret As String
    mciSendStringA "status MyMidi mode", st, Len(st), 0
    If Left$(st, 7) = "stopped" Or Left$(st, 2) = "停止" Then
        m_MidiTrack = m_MidiTrack + 1
        If m_MidiTrack > 103 Then m_MidiTrack = 101
        If Dir$(MidiPath) <> "" Then Kill MidiPath
        If ExtractMidi(m_MidiTrack, "MIDI", MidiPath) Then PlayMidi MidiPath
    End If
End Sub
