/****************************************************************************
**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
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

//#include "../sdlib/sdui.h"
//#include "../sdlib/sdbase.h"
#include "../sdlib/sd.h"
#include <QDebug>
#include "sdinterface.h"
#include "mainwindow.h"

#include <stdio.h>

typedef unsigned int uint32;

static int numberOfSDThreadsActive = 0;

class SquareDesk_iofull : public iobase {
public:
    SquareDesk_iofull(SDThread *thread, MainWindow *mw, QWaitCondition *waitCondSDAwaitingInput, QMutex *mutex,
                      QWaitCondition *waitCondAckToMainThread,
                      QMutex *mutexAckToMainThread
        )
        : sdthread(thread), mw(mw), waitCondSDAwaitingInput(waitCondSDAwaitingInput),
          WaitingForCommand(false), mutexSDAwaitingInput(mutex),
          waitCondAckToMainThread(waitCondAckToMainThread),
          mutexAckToMainThread(mutexAckToMainThread),
          answerYesToEverything(false), seenAFormation(false),
          currentInputState(SDThread::InputStateNormal),
          currentInputText(),
          currentInputYesNo(false)

    {
    }

    virtual ~SquareDesk_iofull()
    {
        // base class has virtual members, so this class should have a virtual destructor
    }

    int do_abort_popup();
    void prepare_for_listing();
    uims_reply_thing get_startup_command();
    void set_window_title(char s[]);
    void add_new_line(const char the_line[], uint32 drawing_picture);
    void no_erase_before_n(int n);
    void reduce_line_count(int n);
    void update_resolve_menu(command_kind goal, int cur, int max,
                             resolver_display_state state);
//    void show_match(int frequency_to_show);
    void show_match();
    const char *version_string();
    uims_reply_thing get_resolve_command();
    bool choose_font();
    bool print_this();
    bool print_any();
    bool help_manual();
    bool help_faq();
    popup_return get_popup_string(Cstring prompt1, Cstring prompt2, Cstring final_inline_prompt,
                                  Cstring seed, char *dest);
    void fatal_error_exit(int code, Cstring s1 = nullptr, Cstring s2 = nullptr);
    void serious_error_print(Cstring s1);
    void create_menu(call_list_kind cl);
    selector_kind do_selector_popup(matcher_class &matcher);
    direction_kind do_direction_popup(matcher_class &matcher);
    int do_circcer_popup();
    int do_tagger_popup(int tagger_class);
    int yesnoconfirm(Cstring title, Cstring line1, Cstring line2, bool excl, bool info);
    void set_pick_string(Cstring string);
    uint32 get_one_number(matcher_class &matcher);
    uims_reply_thing get_call_command();
    void dispose_of_abbreviation(const char *linebuff);
    void display_help();
    void terminate(int code);
    void process_command_line(int *argcp, char ***argvp);
    void bad_argument(Cstring s1, Cstring s2, Cstring s3);
    void final_initialize();
    bool init_step(init_callback_state s, int n);
    void set_utils_ptr(ui_utils *utils_ptr);
    ui_utils *get_utils_ptr();

    ui_utils *m_ui_utils_ptr;

public :
    bool add_string_input(const char *str);


private:
    void add_new_line(const QString &the_line, uint32 drawing_picture = 0);
    void wait_for_input();
    void EnterMessageLoop();

    SDThread *sdthread;
    MainWindow *mw;
    QWaitCondition *waitCondSDAwaitingInput;
    bool WaitingForCommand;
    QMutex *mutexSDAwaitingInput;

    QWaitCondition *waitCondAckToMainThread;
    QMutex *mutexAckToMainThread;
    char szResolveWndTitle [MAX_TEXT_LINE_LENGTH];

    int nLastOne;
#define ui_undefined -999
    uims_reply_kind MenuKind;
    popup_return PopupStatus;

    friend class SDThread;
    bool answerYesToEverything;
    bool seenAFormation;

    SDThread::CurrentInputState currentInputState;
    QString currentInputText;
    bool currentInputYesNo;

    void ShowListBox(int);
    void UpdateStatusBar(const char *);
    bool do_popup(int nWhichOne);

    dance_level find_dance_program(QString call);
};

void SDThread::set_dance_program(dance_level dance_program)
{
    calling_level = dance_program;
}

dance_level SDThread::find_dance_program(QString call)
{
    return iofull->find_dance_program(call);
}

dance_level SquareDesk_iofull::find_dance_program(QString call)
{
    dance_level dance_program(l_nonexistent_concept);
    int length_of_longest_match = 0;
    
    for (int i=0; i<number_of_calls[nLastOne]; i++)
    {
        QString this_name(get_call_menu_name(main_call_lists[nLastOne][i]));

        if (call.endsWith(this_name, Qt::CaseInsensitive))
        {
            if (dance_program == l_nonexistent_concept
                || (length_of_longest_match < this_name.length()
                    && dance_program > (dance_level)(main_call_lists[nLastOne][i]->the_defn.level)))
            {
                length_of_longest_match = this_name.length();
                dance_program = (dance_level)(main_call_lists[nLastOne][i]->the_defn.level);
            }
        }
    }
    return dance_program;
}


SDThread::CurrentInputState SDThread::currentInputState()
{
    return iofull->currentInputState;
}

bool SDThread::do_user_input(QString str)
{
    if (on_user_input(str))
    {
        waitCondAckToMainThread.wait(&mutexAckToMainThread);
        return true;
    }
    return false;
}

bool SDThread::on_user_input(QString str)
{
    QByteArray inUtf8 = str.simplified().toUtf8();
    const char *data = inUtf8.constData();
    return iofull->add_string_input(data);
}

bool SquareDesk_iofull::add_string_input(const char *s)
{
    bool woke(false);
    // So this code should only execute when we're in the condition
    // variable wait, because that's when the mutex is unlocked.
    QMutexLocker locker(mutexSDAwaitingInput);

    switch (currentInputState)
    {
    case SDThread::InputStateYesNo:
        currentInputYesNo = 'Y' == *s || 'y' == *s;
        waitCondSDAwaitingInput->wakeAll();
        woke = true;
        break;
    case SDThread::InputStateText:
        currentInputYesNo = 0 != *s;
        currentInputText = s;
        waitCondSDAwaitingInput->wakeAll();
        woke = true;
        break;
//    default:  // all the possible cases are explicity here, so default: is not needed
//        qWarning() << "Unknown input state: " << currentInputState;
    case SDThread::InputStateNormal:
        int len = strlen(s);
             matcher_class &matcher = *gg77->matcher_p;

        if (s[len - 1] == '!'
            || s[len - 1] == '?')
        {
            bool question_mark = s[len - 1] == '?';
            char ach[MAX_TEXT_LINE_LENGTH + 1];
            memset(ach, '\0', sizeof(ach));
            if (len > MAX_TEXT_LINE_LENGTH)
                len = MAX_TEXT_LINE_LENGTH;
            strncpy(ach,s,len);
            ach[len - 1] = '\0';
            matcher.copy_to_user_input(ach);
            emit sdthread->sd_begin_available_call_list_output();
            matcher.match_user_input(nLastOne, true, question_mark, false);
            emit sdthread->sd_end_available_call_list_output();
            break;
        }


        matcher.copy_to_user_input(s);
        int matches = matcher.match_user_input(nLastOne, false, false, false);

        if ((matches == 1 || matches - matcher.m_yielding_matches == 1 || matcher.m_final_result.exact) &&
            ((!matcher.m_final_result.match.packed_next_conc_or_subcall &&
              !matcher.m_final_result.match.packed_secondary_subcall) ||
             matcher.m_final_result.match.kind == ui_call_select ||
             matcher.m_final_result.match.kind == ui_concept_select))
        {
            WaitingForCommand = false;
            waitCondSDAwaitingInput->wakeAll();
            woke = true;
        }
        else if (matches > 0)
        {
            QString s(QString("  (%1 matches, type ! or ? for list)").arg(matches));
            add_new_line(s);
        }
        else
        {
            add_new_line("  (no matches)");
#if 0
            qWarning() << "No Match: " << s;
            qWarning() << "  matcher.m_yielding_matches: " << matcher.m_yielding_matches;
            qWarning() << "  matcher.m_final_result.exact: " << matcher.m_final_result.exact;
            qWarning() << "  matcher.m_final_result.match.packed_next_conc_or_subcall: " << matcher.m_final_result.match.packed_next_conc_or_subcall;
            qWarning() << "  matcher.m_final_result.match.packed_secondary_subcall: " << matcher.m_final_result.match.packed_secondary_subcall;
            qWarning() << "  matcher.m_final_result.match.kind: " << matcher.m_final_result.match.kind;
            qWarning() << "  ui_call_select: " << ui_call_select;
            qWarning() << "  matcher.m_final_result.match.kind: " << matcher.m_final_result.match.kind;
            qWarning() << "  ui_concept_select: " << ui_concept_select;
#endif
        }
    }

    //See the    use_computed_match: case
    return woke;
}

void SquareDesk_iofull::UpdateStatusBar(const char *s)
{
    emit sdthread->on_sd_update_status_bar(QString(s));
}


void SquareDesk_iofull::wait_for_input()
{
    emit sdthread->on_sd_awaiting_input();
    waitCondSDAwaitingInput->wait(mutexSDAwaitingInput);

    if (1)
    {
        QMutexLocker locker(mutexAckToMainThread);
        waitCondAckToMainThread->wakeAll();
    }
}

// SquareDesk_iofull
void SquareDesk_iofull::EnterMessageLoop()
{

    gg77->matcher_p->m_active_result.valid = false;
    gg77->matcher_p->erase_matcher_input();
    WaitingForCommand = true;
    wait_for_input();
    if (WaitingForCommand)
    {
        general_final_exit(0);
        return;
    }
}


int SquareDesk_iofull::do_abort_popup()
{
    if (answerYesToEverything)
        return POPUP_ACCEPT;
    yesnoconfirm("Confirmation",
                 "The current sequence will be aborted. Do you really want to abort it? (Y/N):",
                 nullptr, // NULL,
                 false, false);

    if (currentInputYesNo)
        seenAFormation = false;
    return currentInputYesNo ? POPUP_ACCEPT : POPUP_DECLINE;
}

void SquareDesk_iofull::prepare_for_listing()
{
    // For multi-page listing, we don't need it.
}


void SquareDesk_iofull::set_window_title(char s[])
{
//    qWarning() << "SquareDesk_iofull::set_window_title(" << s << ");";
    emit sdthread->on_sd_set_window_title(QString(s));
}

void SquareDesk_iofull::add_new_line(const QString &the_line, uint32 drawing_picture)
{
    if (drawing_picture)
        seenAFormation = true;
    emit sdthread->on_sd_add_new_line(the_line, drawing_picture);
}
void SquareDesk_iofull::add_new_line(const char the_line[], uint32 drawing_picture)
{
    if (drawing_picture)
        seenAFormation = true;
    emit sdthread->on_sd_add_new_line(QString(the_line), drawing_picture);
}

void SquareDesk_iofull::no_erase_before_n(int /* n */)
{
    // This is for maintaining crucial lines on the screen.
//    qWarning() << "SquareDesk_iofull::no_erase_before_n(int n);";
}

void SquareDesk_iofull::reduce_line_count(int /* n */)
{
    // This is for culling old lines. We can probably leave it doing nothing.
//    qWarning() << "SquareDesk_iofull::reduce_line_count(int " << n << ");";
}

void SquareDesk_iofull::update_resolve_menu(command_kind goal, int cur, int max,
                                            resolver_display_state state)
{
    create_resolve_menu_title(goal, cur, max, state, szResolveWndTitle);
    UpdateStatusBar(szResolveWndTitle);

    // Put it in the transcript area also, where it's easy to see.

    get_utils_ptr()->writestuff(szResolveWndTitle);
    get_utils_ptr()->newline();
//    qWarning() << "SquareDesk_iofull::update_resolve_menu(command_kind goal, int cur, int max,";
}

//void SquareDesk_iofull::show_match(int frequency_to_show)
void SquareDesk_iofull::show_match()
{
//    qWarning() << "SquareDesk_iofull::show_match(int frequency_to_show);";
//    get_utils_ptr()->show_match_item(frequency_to_show);
    get_utils_ptr()->show_match_item();
}

const char *SquareDesk_iofull::version_string()
{
    return "SquareDesk-" VERSIONSTRING;
//    qWarning() << "SquareDesk_iofull::char *version_string();";
}


void SquareDesk_iofull::ShowListBox(int nWhichOne) {
    QStringList options;
    QStringList dance_levels;
//    qWarning() << "ShowListBox(" << nWhichOne << ")";

    if (nWhichOne != nLastOne)
    {
        nLastOne = nWhichOne;
        // This if/else block should be a switch, but there are some range checks
        if (nWhichOne == matcher_class::e_match_number)
        {
            for (int i = 0; i < NUM_CARDINALS; ++i)
            {
                options.append(cardinals[i]);
                dance_levels.append(QString("%1").arg(0));
            }
            UpdateStatusBar("<number>");
        }
        else if (nWhichOne == matcher_class::e_match_circcer)
        {
            for (unsigned int i = 0; i < number_of_circcers; ++i)
            {
                // options.append(get_call_menu_name(circcer_calls[i])); // FIX: Dan, please fix this!
                dance_levels.append(QString("%1").arg(0));
            }
            UpdateStatusBar("<circulate replacement>");
        }
        else if (nLastOne >= matcher_class::e_match_taggers &&
                 nLastOne < matcher_class::e_match_taggers+NUM_TAGGER_CLASSES) {
            int tagclass = nLastOne - matcher_class::e_match_taggers;
            UpdateStatusBar("<tagging call>");

            for (unsigned int iu=0 ; iu<number_of_taggers[tagclass] ; iu++)
            {
                options.append(get_call_menu_name(tagger_calls[tagclass][iu]));
                dance_levels.append(QString("%1").arg(0));
            }
        }
        else if (nLastOne == matcher_class::e_match_startup_commands) {
            UpdateStatusBar("<startup>");

            for (int i=0 ; i<num_startup_commands ; i++)
            {
                options.append(startup_commands[i]);
                dance_levels.append(QString("%1").arg(0));
            }
        }
        else if (nLastOne == matcher_class::e_match_resolve_commands) {
            for (int i=0 ; i<number_of_resolve_commands ; i++)
            {
                options.append(resolve_command_strings[i]);
                dance_levels.append(QString("%1").arg(0));
            }
        }
        else if (nLastOne == matcher_class::e_match_directions) {
            UpdateStatusBar("<direction>");

            for (int i=0 ; i<last_direction_kind ; i++)
            {
                options.append(direction_menu_list[i+1]);
                dance_levels.append(QString("%1").arg(0));
            }
        }
        else if (nLastOne == matcher_class::e_match_selectors) {
            UpdateStatusBar("<selector>");

            // Menu is shorter than it appears, because we are skipping first item.
            for (int i=0 ; i<selector_INVISIBLE_START-1 ; i++)
            {
                options.append(selector_menu_list[i]);
                dance_levels.append(QString("%1").arg(0));
            }
        }
        else {
            int i;

            UpdateStatusBar(menu_names[nLastOne]);

            for (i=0; i<number_of_calls[nLastOne]; i++)
            {
                options.append(get_call_menu_name(main_call_lists[nLastOne][i]));
                dance_levels.append(QString("%1").arg(main_call_lists[nLastOne][i]->the_defn.level));
            }

            short int *item;
            int menu_length;

            index_list *list_to_use = allowing_all_concepts ?
                &gg77->matcher_p->m_concept_list :
                &gg77->matcher_p->m_level_concept_list;
            item = list_to_use->the_list;
            menu_length = list_to_use->the_list_size;

            for (i=0 ; i<menu_length ; i++)
            {
                options.append(get_concept_menu_name(access_concept_descriptor_table(item[i])));
                dance_levels.append(QString("%1").arg(0 + kSDCallTypeConcepts));
            }

            for (i=0 ;  ; i++) {
                Cstring name = command_menu[i].command_name;
                if (!name) break;
                options.append(name);
                dance_levels.append(QString("%1").arg(0 + kSDCallTypeCommands));
            }
            options.append("abort this sequence");
            dance_levels.append(nullptr); // 0);

            options.append("square your sets");
            dance_levels.append(nullptr); // 0);
        }
        emit sdthread->on_sd_set_matcher_options(options, dance_levels);
   }
}


uims_reply_thing SquareDesk_iofull::get_resolve_command()
{
//    qWarning() << "SquareDesk_iofull::get_resolve_command();";
    matcher_class &matcher = *gg77->matcher_p;
    UpdateStatusBar(szResolveWndTitle);

    nLastOne = ui_undefined;
    MenuKind = ui_resolve_select;
    ShowListBox(matcher_class::e_match_resolve_commands);
    EnterMessageLoop();

    if (matcher.m_final_result.match.index < 0)
        // Special encoding from a function key.
        return uims_reply_thing(matcher.m_final_result.match.kind, -1-matcher.m_final_result.match.index);
    else
        return uims_reply_thing(matcher.m_final_result.match.kind, (int) resolve_command_values[matcher.m_final_result.match.index]);
}

bool SquareDesk_iofull::choose_font()
{
//    qWarning() << "SquareDesk_iofull::choose_font();";
    return true;
}

bool SquareDesk_iofull::print_this()
{
//    qWarning() << "SquareDesk_iofull::print_this();";
    // ZZZZZ TODO: PRINT!
    return true;
}

bool SquareDesk_iofull::print_any()
{
//    qWarning() << "SquareDesk_iofull::print_any();";
    // ZZZZZ TODO: PRINT!
    return true;
}

bool SquareDesk_iofull::help_manual()
{
//    qWarning() << "SquareDesk_iofull::help_manual();";
//ZZZZZZZZZ   ShellExecute(NULL, "open", "c:\\sd\\sd_doc.html", NULL, NULL, SW_SHOWNORMAL);
    return true;

}

bool SquareDesk_iofull::help_faq()
{
//    qWarning() << "SquareDesk_iofull::help_faq();";
// ZZZZZZZZZZ    ShellExecute(NULL, "open", "c:\\sd\\faq.html", NULL, NULL, SW_SHOWNORMAL);
    return true;
}

popup_return SquareDesk_iofull::get_popup_string(Cstring prompt1, Cstring prompt2,
                                                 Cstring final_inline_prompt,
                                                 Cstring /* seed */, char *dest)
{
    QString prompt;
    if (prompt1 && prompt1[0] && prompt1[0] == '*')
    {
        prompt = QString(prompt1 + 1);
    }
    else if (prompt1)
    {
        prompt = prompt1;
    }

    if (prompt2 && prompt2[0] && prompt2[0] == '*')
    {
        prompt = QString(prompt2 + 1);
    }
    else
    {
        prompt = prompt2;
    }


    QStringList options;
    QStringList dance_levels;
    emit sdthread->on_sd_set_matcher_options(options, dance_levels);
    add_new_line(prompt + "\n" + QString(final_inline_prompt));

    currentInputState = SDThread::InputStateText;
    wait_for_input();

    if (currentInputState != SDThread::InputStateText)
    {
        qWarning() << "Text state issue" << currentInputState;
    }

    currentInputState = SDThread::InputStateNormal;

    QString sdest = currentInputText;
    memset(dest, '\0', MAX_TEXT_LINE_LENGTH);
    strncpy(dest, sdest.toStdString().c_str(), MAX_TEXT_LINE_LENGTH - 1);

    return currentInputYesNo ? POPUP_ACCEPT_WITH_STRING : POPUP_DECLINE;
}


void SquareDesk_iofull::fatal_error_exit(int code, Cstring s1, Cstring s2)
{
    QString message(s1);

    if (s2 && s2[0])
    {
        message += ": ";
        message += s2;
    }

    // ZZZZZZZZZZZ
    qWarning() << "SquareDesk_iofull::fatal_error_exit(" << code << ", " << s1 << ", " << s2 << ");";
    session_index = 0;
    general_final_exit(code);
}

void SquareDesk_iofull::serious_error_print(Cstring /* s1 */)
{
    qWarning() << "SquareDesk_iofull::serious_error_print(Cstring s1);";
    // ZZZZZZZ
}

void SquareDesk_iofull::create_menu(call_list_kind /* cl */)
{
    // This is empty in the Windows version, so leaving it empty here
//    qWarning() << "SquareDesk_iofull::create_menu(call_list_kind cl (" << cl << "));";
}

selector_kind SquareDesk_iofull::do_selector_popup(matcher_class &/* matcher */)
{
    matcher_class &matcher = *gg77->matcher_p;
    match_result saved_match = matcher.m_final_result;
   // We add 1 to the menu position to get the actual selector enum; the enum effectively starts at 1.
   // Item zero in the enum is selector_uninitialized, which we return if the user cancelled.
    selector_kind retval = do_popup((int) matcher_class::e_match_selectors) ?
        (selector_kind) (matcher.m_final_result.match.index+1) : selector_uninitialized;
    matcher.m_final_result = saved_match;
    return retval;
}

direction_kind SquareDesk_iofull::do_direction_popup(matcher_class &/* matcher */)
{
    qWarning() << "SquareDesk_iofull::do_direction_popup(matcher_class &matcher);";
    // ZZZZZ
    return direction_uninitialized;
}

int SquareDesk_iofull::do_circcer_popup()
{
    matcher_class &matcher = *gg77->matcher_p;
    uint32 retval = 0;

    if (interactivity == interactivity_verify) {
        retval = verify_options.circcer;
        if (retval == 0) retval = 1;
    }
    else if (!matcher.m_final_result.valid || (matcher.m_final_result.match.call_conc_options.circcer == 0)) {
        match_result saved_match = matcher.m_final_result;
        if (do_popup((int) matcher_class::e_match_circcer))
            retval = matcher.m_final_result.match.call_conc_options.circcer;
        matcher.m_final_result = saved_match;
    }
    else {
        retval = matcher.m_final_result.match.call_conc_options.circcer;
        matcher.m_final_result.match.call_conc_options.circcer = 0;
    }

    return retval;
}

int SquareDesk_iofull::do_tagger_popup(int /* tagger_class */)
{
    qWarning() << "SquareDesk_iofull::do_tagger_popup(int tagger_class);";
    return true;
}

int SquareDesk_iofull::yesnoconfirm(Cstring title , Cstring line1, Cstring line2, bool /* excl */, bool /* info */)
{
    if (answerYesToEverything)
        return POPUP_ACCEPT;

    QString prompt("");
    if (line1 && line1[0])
    {
        prompt = line1;
        prompt += "\n";
    }

    prompt += line2;
    prompt += " (Y/N):";
    currentInputState = SDThread::InputStateYesNo;
    QStringList options;
    QStringList dance_levels;
    options.append("yes");
    dance_levels.append("0");
    options.append("no");
    dance_levels.append("0");

    emit sdthread->on_sd_set_matcher_options(options, dance_levels);
    add_new_line(QString(title) + "\n" +  prompt);
    wait_for_input();

    if (currentInputState != SDThread::InputStateYesNo)
    {
        qWarning() << "YesNo state issue" << currentInputState;
    }
    currentInputState = SDThread::InputStateNormal;

    return currentInputYesNo ? POPUP_ACCEPT : POPUP_DECLINE;
}

void SquareDesk_iofull::set_pick_string(Cstring string)
{
    emit sdthread->on_sd_set_pick_string(QString(string));
}

uint32 SquareDesk_iofull::get_one_number(matcher_class &/* matcher */)
{
    QStringList options;
    QStringList dance_levels;
    for (int i = 1; 0 <= 36; ++i)
    {
        options.append(QString("%1").arg(i));
        dance_levels.append("0");
    }

    emit sdthread->on_sd_set_matcher_options(options, dance_levels);
    add_new_line("How many? (Type a number between 0 and 36):");
    currentInputState = SDThread::InputStateText;

    wait_for_input();

    if (currentInputState != SDThread::InputStateText)
    {
        qWarning() << "Text state issue" << currentInputState;
    }

    currentInputState = SDThread::InputStateNormal;
    if (!currentInputYesNo) return ~0U;
    else {
        return currentInputText.toInt();
    }


//    return true;  // will never be executed
}

uims_reply_thing SquareDesk_iofull::get_call_command()
{
    matcher_class &matcher = *gg77->matcher_p;
startover:
    if (allowing_modifications)
        parse_state.call_list_to_use = call_list_any;

//    SetTitle();
    nLastOne = ui_undefined;    /* Make sure we get a new menu,
                                   in case concept levels were toggled. */
    MenuKind = ui_call_select;
    // ZZZZZ show call list menu
    ShowListBox(parse_state.call_list_to_use);
    EnterMessageLoop();

    int index = matcher.m_final_result.match.index;

    if (index < 0) {
        // Special encoding from a function key.
        return uims_reply_thing(matcher.m_final_result.match.kind, -1-index);
    }
    else if (matcher.m_final_result.match.kind == ui_command_select) {
        // Translate the command.
        return uims_reply_thing(matcher.m_final_result.match.kind, (int) command_command_values[index]);
    }
    else if (matcher.m_final_result.match.kind == ui_special_concept) {
        return uims_reply_thing(matcher.m_final_result.match.kind, index);
    }
    else {
        // Reject off-level concept accelerator key presses.
        if (!allowing_all_concepts && matcher.m_final_result.match.kind == ui_concept_select &&
            get_concept_level(matcher.m_final_result.match.concept_ptr) > calling_level)
            goto startover;

        call_conc_option_state save_stuff = matcher.m_final_result.match.call_conc_options;
        there_is_a_call = false;
        uims_reply_kind my_reply = matcher.m_final_result.match.kind;
//        bool retval = deposit_call_tree(&matcher.m_final_result.match, (parse_block *) 0, 2);
        bool retval = deposit_call_tree(&matcher.m_final_result.match, static_cast<parse_block *>(nullptr), 2);
        matcher.m_final_result.match.call_conc_options = save_stuff;
        if (there_is_a_call) {
            parse_state.topcallflags1 = the_topcallflags;
            my_reply = ui_call_select;
        }

        return uims_reply_thing(retval ? ui_user_cancel : my_reply, index);
    }
}

void SquareDesk_iofull::dispose_of_abbreviation(const char *linebuff)
{
    emit sdthread->on_sd_dispose_of_abbreviation(QString(linebuff));
    WaitingForCommand = false;
}

void SquareDesk_iofull::display_help()
{
}

void SquareDesk_iofull::terminate(int code)
{
    Q_UNUSED(code)
//    qWarning() << "SquareDesk_iofull::terminate(" <<  code << ");";
}

void SquareDesk_iofull::process_command_line(int */* argcp */, char ***/* argvp */)
{
//    qWarning() << "SquareDesk_iofull::process_command_line(int *argcp, char ***argvp);";
}

void SquareDesk_iofull::bad_argument(Cstring /* s1 */, Cstring /* s2 */, Cstring /* s3 */)
{
    qWarning() << "SquareDesk_iofull::bad_argument(Cstring s1, Cstring s2, Cstring s3);";
}

void SquareDesk_iofull::final_initialize()
{
    ui_options.use_escapes_for_drawing_people = 0;

//    qWarning() << "SquareDesk_iofull::final_initialize();";
}

bool SquareDesk_iofull::init_step(init_callback_state s, int /* n */)
{
    switch (s)
    {
    case get_session_info:
        // qWarning() << "get_session_info: " << s << " " << n;
        break;

    case final_level_query:
        // qWarning() << "final_level_query: " << s << " " << n;
        break;

    case init_database1:
        // qWarning() << "init_database1: " << s << " " << n;
        break;

        // update status "Reading database"
    case init_database2:
        // qWarning() << "init_database2: " << s << " " << n;
        break;

        // update status "Creating menus"
    case calibrate_tick:
        // qWarning() << "calibrate_tick: " << s << " " << n;
        break;

    case do_tick:
        // qWarning() << "do_tick: " << s << " " << n;
        break;

    case tick_end:
        // qWarning() << "tick_end: " << s << " " << n;
        break;

    case do_accelerator:
        // qWarning() << "do_accelerator: " << s << " " << n;
        break;

//    default:  // all the cases are explicitly covered, so default is not needed
//        // qWarning() << "init step " << s << " " << n;
//        break;
    }
    // qWarning() << "SquareDesk_iofull::init_step(init_callback_state s, int n);";
    return false;
}

void SquareDesk_iofull::set_utils_ptr(ui_utils *utils_ptr)
{
    m_ui_utils_ptr = utils_ptr;
}

ui_utils *SquareDesk_iofull::get_utils_ptr()
{
    return m_ui_utils_ptr;
}


bool SquareDesk_iofull::do_popup(int nWhichOne)
{
//   uims_reply_kind SavedMenuKind = MenuKind;
   nLastOne = ui_undefined;
//   MenuKind = ui_call_select;

   ShowListBox(nWhichOne);
   EnterMessageLoop();
//   MenuKind = SavedMenuKind;
   // A value of -1 means that the user hit the "cancel" button.
   return (gg77->matcher_p->m_final_result.match.index >= 0);
}


uims_reply_thing SquareDesk_iofull::get_startup_command()
{
    matcher_class &matcher = *gg77->matcher_p;
    nLastOne = ui_undefined;
    MenuKind = ui_start_select;

    ShowListBox(matcher_class::e_match_startup_commands);

    EnterMessageLoop();

    int index = matcher.m_final_result.match.index;

    if (index < 0)
        // Special encoding from a function key.
        return uims_reply_thing(matcher.m_final_result.match.kind, -1-index);
    else if (matcher.m_final_result.match.kind == ui_command_select) {
        // Translate the command.
        return uims_reply_thing(matcher.m_final_result.match.kind, (int) command_command_values[index]);
    }
    else if (matcher.m_final_result.match.kind == ui_start_select) {
        // Translate the command.
        return uims_reply_thing(matcher.m_final_result.match.kind, (int) startup_command_values[index]);
    }

    return uims_reply_thing(matcher.m_final_result.match.kind, index);
}

void SDThread::add_selectors_to_list_widget(QListWidget *listWidget)
{
    // Skip ???
    for (size_t i = 1; selector_list[i].name; ++i)
    {
        listWidget->addItem(selector_list[i].name);
    }
}

void SDThread::add_directions_to_list_widget(QListWidget *listWidget)
{
    // Skip "???" and "(no direction)"
    for (size_t i = 2; direction_names[i].name; ++i)
    {
        listWidget->addItem(direction_names[i].name);
    }
}

SDThread::SDThread(MainWindow *mw, dance_level dance_program, QString dance_program_name)
    : QThread(mw),
      dance_program_name(dance_program_name),
      mw(mw),
      waitCondSDAwaitingInput(),
      mutexSDAwaitingInput(),
      mutexThreadRunning(),
      abort(false)
{
    // return SD to its original glory
    last_direction_kind = direction_ENUM_EXTENT;
    direction_names[direction_zigzag].name =  "zig-zag";
    direction_names[direction_zigzag].name_uc = "ZIG-ZAG";
    direction_names[direction_the_music].name = "the music";
    direction_names[direction_the_music].name_uc = "THE MUSIC";


    
    set_dance_program(dance_program);
    // We should expand these elsewhere for autocomplete stuff

    // Build our leading selector list, static to this file:
    for (size_t i = 0; selector_list[i].name; ++i)
    {
        selectors.append(selector_list[i].name);
        selectors.append(selector_list[i].sing_name);
    }
    // These are in mkcalls.cpp so we don't have a way to extract this list separately
    // altdeftabh
    selectors.append("diamond");
    selectors.append("reverse");
    selectors.append("left");
    selectors.append("funny");
    selectors.append("interlocked");
    selectors.append("magic");
    selectors.append("grand");
    selectors.append("12matrix");
    selectors.append("16matrix");
    selectors.append("cross");
    selectors.append("single");
    selectors.append("singlefile");
    selectors.append("half");
    selectors.append("rewind");
    selectors.append("straight");
    selectors.append("twisted");
    selectors.append("lasthalf");
    selectors.append("fractal");
    selectors.append("fast");

    // const char *yoyotabplain[] = {
    selectors.append("yoyo");
    selectors.append("generous");
    selectors.append("stingy");
    // const char *mxntabplain[] = {
    selectors.append("1x2");
    selectors.append("2x1");
    selectors.append("1x3");
    selectors.append("3x1");
    selectors.append("0x3");
    selectors.append("3x0");
    selectors.append("0x4");
    selectors.append("4x0");
    selectors.append("6x2");
    selectors.append("3x2");

    // const char *reverttabplain[] = {
    selectors.append("revert");
    selectors.append("reflect");
    selectors.append("revertreflect");
    selectors.append("reflectrevert");
    selectors.append("revertreflectrevert");
    selectors.append("reflectrevertreflect");
    selectors.append("reflectreflect");


    // this will cause the thread startup to block until this
    // unlocked, which happens through SDThread::unlock() at the end
    // of the MainWindow constructor.
    mutexSDAwaitingInput.lock();
    mutexAckToMainThread.lock();
    QObject::connect(this, &SDThread::on_sd_update_status_bar, mw, &MainWindow::on_sd_update_status_bar);
    QObject::connect(this, &SDThread::on_sd_awaiting_input, mw, &MainWindow::on_sd_awaiting_input);
    QObject::connect(this, &SDThread::on_sd_set_window_title, mw, &MainWindow::on_sd_set_window_title);
    QObject::connect(this, &SDThread::on_sd_add_new_line, mw, &MainWindow::on_sd_add_new_line);
    QObject::connect(this, &SDThread::on_sd_set_matcher_options, mw, &MainWindow::on_sd_set_matcher_options);
    QObject::connect(this, &SDThread::on_sd_set_pick_string, mw, &MainWindow::on_sd_set_pick_string);
    QObject::connect(this, &SDThread::on_sd_dispose_of_abbreviation, mw, &MainWindow::on_sd_dispose_of_abbreviation);
    QObject::connect(this, &SDThread::sd_begin_available_call_list_output, mw, &MainWindow::sd_begin_available_call_list_output);
    QObject::connect(this, &SDThread::sd_end_available_call_list_output, mw, &MainWindow::sd_end_available_call_list_output);

}

void SDThread::resetSDState() {
    iofull->answerYesToEverything = true;
    abort = true;
//    mutexSDAwaitingInput.lock();
    while (iofull->currentInputState != InputStateNormal)
    {
//        mutexSDAwaitingInput.unlock();
        do_user_input("Yes");
//        mutexSDAwaitingInput.lock();
    }
//    mutexSDAwaitingInput.unlock();

}

void SDThread::resetAndExecute(QStringList &commands)
{
    resetSDState();
    iofull->answerYesToEverything = false;
    abort = false;
    if (iofull->seenAFormation)
    {
//        qDebug() << "resetAndExecute: aborting";
        do_user_input("abort this sequence");
        do_user_input("yes");
    }
    
    for (QString &cmd : commands)
    {
//        qDebug() << "resetAndExecute: executing " << cmd;
        do_user_input(cmd);
    }
}

void SDThread::finishAndShutdownSD()
{
    resetSDState();

//    if (!iofull->seenAFormation)
    {
        do_user_input("abort the search");
        do_user_input("heads start");  // THIS SEEMS WRONG, DOESN'T IT? But it stops crash on SDesk quit
        do_user_input("square thru 4");
    }
    do_user_input("quit");
}

SDThread::~SDThread()
{
    mutexSDAwaitingInput.unlock();
    mutexAckToMainThread.unlock();
    QMutexLocker locker(&mutexThreadRunning);
    if (!wait(250))
    {
        qWarning() << "Thread unable to stop, calling terminate";
        terminate();
    }
    numberOfSDThreadsActive--;
}

void SDThread::run()
{
    numberOfSDThreadsActive++;    
    QMutexLocker locker(&mutexThreadRunning);
    mutexSDAwaitingInput.lock();
    SquareDesk_iofull ggg(this, mw, &waitCondSDAwaitingInput, &mutexSDAwaitingInput,
                          &waitCondAckToMainThread, &mutexAckToMainThread);
    iofull = &ggg;
    QString executableDir = QCoreApplication::applicationDirPath(); // this is where the executable is (not necessarily the current working directory)
    QString sdCallsFilename = executableDir + "/sd_calls.dat";

#if !(defined(Q_OS_MAC) || defined(Q_OS_WIN))
    // if it's Linux, look in /usr/share/SquareDesk
    QFileInfo check_file(sdCallsFilename);
    if (!(check_file.exists() && check_file.isFile()))
    {
        sdCallsFilename = "/usr/share/SquareDesk/sd_calls.dat";
    }
#endif
    std::string str = sdCallsFilename.toStdString();
    const char* p = str.c_str();
    //    qDebug() << "database:" << p;

    char *levelString = strdup(const_cast<char *>(dance_program_name.toStdString().c_str())); // make a copy of the string

    // where sd should write files
    QString sdDir = mw->musicRootPath + "/sd/";
    std::string str2 = sdDir.toStdString();
    const char* p2 = str2.c_str();
//    qDebug() << "SD musicRootPath: " << p2;

    char *argv[] = {const_cast<char *>("SquareDesk"),
                    const_cast<char *>("-db"), const_cast<char *>(p),               // location of sd_calls.dat database
                    const_cast<char *>("-output_prefix"), const_cast<char *>(p2),   // fully-qualified dir name for files that sd writes
                    const_cast<char *>("-minigrand_getouts"),
                    const_cast<char *>("-bend_line_home_getouts"),
//            const_cast<char *>(dance_program_name.toStdString().c_str()),
                    levelString,
                    nullptr};

    sdmain(sizeof(argv) / sizeof(*argv) - 1, argv, ggg);  // note: manually set argc to match number of argv arguments...
}

void SDThread::unlock()
{
    // This is at the end of the mainwindow constructor, and should
    // let the thread run, because the thread attempts a lock at the
    // start of SDThread::run()

    mutexSDAwaitingInput.unlock();
}


QString SDThread::sd_strip_leading_selectors(QString originalText)
{


    for (QString name : selectors)
    {
        if (originalText.startsWith(name, Qt::CaseInsensitive))
        {
            originalText.remove(0, name.length());
            originalText = originalText.simplified();
        }
    }
    return originalText;
}
