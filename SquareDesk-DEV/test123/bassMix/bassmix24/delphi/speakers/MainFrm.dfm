object Form1: TForm1
  Left = 0
  Top = 0
  BorderStyle = bsDialog
  Caption = 'BASSmix Multi-Speaker Example'
  ClientHeight = 64
  ClientWidth = 322
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnClose = FormClose
  OnCreate = FormCreate
  DesignSize = (
    322
    64)
  PixelsPerInch = 96
  TextHeight = 13
  object lblSpeakers: TLabel
    Left = 24
    Top = 39
    Width = 69
    Height = 13
    Caption = 'Speaker Pairs:'
  end
  object btnOpen: TButton
    Left = 8
    Top = 8
    Width = 306
    Height = 25
    Anchors = [akLeft, akTop, akRight]
    Caption = 'click to open a file...'
    TabOrder = 0
    OnClick = btnOpenClick
    ExplicitWidth = 344
  end
  object cbPair1: TCheckBox
    Left = 116
    Top = 39
    Width = 33
    Height = 17
    Anchors = [akTop]
    Caption = '1'
    Checked = True
    State = cbChecked
    TabOrder = 1
    OnClick = cbPair1Click
    ExplicitLeft = 112
  end
  object cbPair2: TCheckBox
    Left = 156
    Top = 39
    Width = 33
    Height = 17
    Anchors = [akTop]
    Caption = '2'
    Checked = True
    State = cbChecked
    TabOrder = 2
    OnClick = cbPair1Click
    ExplicitLeft = 151
  end
  object cbPair3: TCheckBox
    Left = 197
    Top = 39
    Width = 33
    Height = 17
    Anchors = [akTop]
    Caption = '3'
    Checked = True
    State = cbChecked
    TabOrder = 3
    OnClick = cbPair1Click
    ExplicitLeft = 190
  end
  object cbPair4: TCheckBox
    Left = 237
    Top = 39
    Width = 33
    Height = 17
    Anchors = [akTop]
    Caption = '4'
    Checked = True
    State = cbChecked
    TabOrder = 4
    OnClick = cbPair1Click
    ExplicitLeft = 229
  end
  object OpenDlg: TOpenDialog
    Filter = 'Streamable Files|*.mp3;*.mp2;*.mp1;*.ogg;*.wav|All Files|*'
  end
end
