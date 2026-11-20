VERSION 5.00
Begin VB.Form Form_Author 
      ClientLeft      =   45 
      ClientTop       =   270 
      ClientWidth     =   6240 
      ClientHeight    =   4830 
   Caption         =   "§@ªÌ" 
   ScaleMode       =   3 
   AutoRedraw      =   0 'False 
   FontTransparent =   -1 'True 
   BorderStyle     =   4 
   LinkTopic       =   "Form1" 
   MaxButton       =   0 'False 
   MinButton       =   0 'False 
   ShowInTaskbar   =   0 'False 
   Picture         =   "Form_Author.frx":0000 
   StartUpPosition =   2 
End 
Attribute VB_Name = "Form_Author"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

' Form_Author code - reconstructed from procs57 56 (0x4B58D0)
' Form_Load normalizes the client area back to the original 370x310 pixel
' design size (0x172/0x136 constants in the asm) using Mainbas TwipsX/TwipsY
' (g_4B6028/g_4B602C, set by Form_Main). The asm references the form through
' its global instance (g_4B6124), hence "Form_Author." instead of "Me.".

Private Sub Form_Load()
    If App.PrevInstance Then End
    Form_Author.Width = Form_Author.Width - Form_Author.ScaleWidth * TwipsX + 370 * TwipsX
    Form_Author.Height = Form_Author.Height - Form_Author.ScaleHeight * TwipsY + 310 * TwipsY
End Sub
