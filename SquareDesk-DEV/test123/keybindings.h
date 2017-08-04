#ifndef KEYBINDINGS_H_INCLUDED
#define KEYBINDINGS_H_INCLUDED
#include <QVector>
#include <QHash>

namespace Ui
{
class MainWindow;
}


class MainWindow;


class KeyAction
{
public:
    KeyAction();
    virtual const char *name() = 0;
    virtual void doAction(MainWindow *) = 0;
    static QVector<KeyAction*> availableActions();
    static QVector<Qt::Key> mappableKeys();
    static QHash<Qt::Key, KeyAction *> defaultKeyToActionMappings();
    static QHash<QString, KeyAction*> actionNameToActionMappings();
};


#endif /* ifndef KEYBINDINGS_H_INCLUDED */
