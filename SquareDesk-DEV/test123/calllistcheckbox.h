/****************************************************************************
**
** Copyright (C) 2016-2021 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

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

