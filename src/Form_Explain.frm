VERSION 5.00
Begin VB.Form Form_Explain 
      ClientLeft      =   45 
      ClientTop       =   270 
      ClientWidth     =   4680 
      ClientHeight    =   3195 
   Caption         =   "¹CÀ¸»¡©ú" 
   BackColor       =   &H00000000& 
   ScaleMode       =   3 
   AutoRedraw      =   0 'False 
   FontTransparent =   -1 'True 
   BorderStyle     =   4 
   LinkTopic       =   "Form1" 
   MaxButton       =   0 'False 
   MinButton       =   0 'False 
   ShowInTaskbar   =   0 'False 
   Picture         =   "Form_Explain.frx":0000 
   StartUpPosition =   2 
End 
Attribute VB_Name = "Form_Explain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

' Form_Explain code - reconstructed from procs57 55 (0x4B55E0)
' Form_Load normalizes the client area back to the original 640x376 pixel
' design size (0x280/0x178 constants in the asm) using Mainbas TwipsX/TwipsY
' (g_4B6028/g_4B602C, set by Form_Main). The asm references the form through
' its global instance (g_4B6138), hence "Form_Explain." instead of "Me.".

Private Sub Form_Load()
    If App.PrevInstance Then End
    Form_Explain.Width = Form_Explain.Width - Form_Explain.ScaleWidth * TwipsX + 640 * TwipsX
    Form_Explain.Height = Form_Explain.Height - Form_Explain.ScaleHeight * TwipsY + 376 * TwipsY
End Sub
