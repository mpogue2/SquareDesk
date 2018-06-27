#include "sdredostack.h"

SDRedoStack::SDRedoStack()
    : sd_redo_stack(), doing_user_input(false), did_an_undo(false)
{
    initialize();
}


void SDRedoStack::set_doing_user_input()
{
    doing_user_input = true;
}

void SDRedoStack::clear_doing_user_input()
{
    did_an_undo = false;    
    doing_user_input = false;
}

void SDRedoStack::set_did_an_undo()
{
    did_an_undo = true;
}

void SDRedoStack::initialize()
{
    doing_user_input = false;
    sd_redo_stack.clear();
    sd_redo_stack.append(QStringList());
}


void SDRedoStack::add_lines_to_row(int row)
{
    while (sd_redo_stack.length() < row)
        sd_redo_stack.append(QStringList());
}

void SDRedoStack::add_command(int row, const QString &cmd)
{
    if (doing_user_input)
    {
        if (did_an_undo)
        {
            for (int i = row; i < sd_redo_stack.length(); ++i)
                sd_redo_stack[i].clear();
        }
        did_an_undo = false;       
        sd_redo_stack[row].append(cmd);
    }
}

QStringList SDRedoStack::get_redo_commands(int row)
{
    if (row < sd_redo_stack.count())
    {
        return sd_redo_stack[row];
    }
    return QStringList();
}
