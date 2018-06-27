#include "sdredostack.h"

SDRedoStack::SDRedoStack()
    : sd_redo_stack(), doing_user_input(false)
{
    initialize();
}


void SDRedoStack::set_doing_user_input()
{
    doing_user_input = true;
}

void SDRedoStack::clear_doing_user_input()
{
    doing_user_input = false;
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
    if (row > 0 && doing_user_input)
        sd_redo_stack[row - 1].clear();
}

void SDRedoStack::add_command(int row, const QString &cmd)
{
    if (doing_user_input)
        sd_redo_stack[row].append(cmd);
}

QStringList SDRedoStack::get_redo_commands(int row)
{
    if (row < sd_redo_stack.count())
    {
        return sd_redo_stack[row];
    }
    return QStringList();
}
