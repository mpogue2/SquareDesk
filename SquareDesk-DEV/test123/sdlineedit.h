/****************************************************************************
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

#ifndef SDLINEEDIT_H
#define SDLINEEDIT_H
 
#include <QLineEdit> 

class MainWindow;

class SDLineEdit: public QLineEdit
{
  Q_OBJECT
  
private:
  MainWindow *mainWindow;
  
public:
SDLineEdit() : mainWindow(NULL)
  {
  };
  SDLineEdit(QWidget* parent) : mainWindow(NULL)
  {
      this->setParent(parent); 
  };
  virtual ~SDLineEdit()
  {};

  void setMainWindow(MainWindow *mw)
  {
      this->mainWindow = mw;
  }

public:
  bool event(QEvent *) override;
};
#endif

