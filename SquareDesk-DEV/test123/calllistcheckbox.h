#ifndef CALLLISTCHECKBOX_H
#define CALLLISTCHECKBOX_H
 
#include <QCheckBox> 

class MainWindow;

class CallListCheckBox: public QCheckBox
{
  Q_OBJECT
  
private:
  MainWindow *mainWindow;
  int row;
  
public:    
CallListCheckBox() : mainWindow(NULL), row(-1)
  {
      connect(this , SIGNAL(stateChanged(int)),this,SLOT(checkBoxStateChanged(int)));
  };
  CallListCheckBox(QWidget* parent) : mainWindow(NULL), row(-1)
  {
      this->setParent(parent);
      connect(this , SIGNAL(stateChanged(int)),this,SLOT(checkBoxStateChanged(int)));
  };
  ~CallListCheckBox()
  {};

  void setRow(int row)
  {
      this->row = row;
  }
  void setMainWindow(MainWindow *mw)
  {
      this->mainWindow = mw;
  }
	
  public slots:
      //Slot that is called when CheckBox is checked or unchecked
      void checkBoxStateChanged(int state);
  
};
#endif

