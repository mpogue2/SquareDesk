program Speakers;

uses
  Forms,
  MainFrm in 'MainFrm.pas' {Form1},
  bass in '..\bass.pas',
  bassmix in '..\bassmix.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
