#include "../sdlib/sdui.h"
#include "../sdlib/sdbase.h"
#include <QDebug>
#include "sdinterface.h"
#include "mainwindow.h"
#include <QInputDialog>
#include <QLineEdit>
 



class SquareDesk_iofull : public iobase {
public:
    SquareDesk_iofull(SDThread *thread, MainWindow *mw, QWaitCondition *waitCondition, QMutex &mutex)
        : sdthread(thread), mw(mw), waitCondition(waitCondition),
          WaitingForCommand(false), mutexMessageLoop(mutex)
    {
        mutexMessageLoop.lock();
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
    void show_match(int frequency_to_show);
    const char *version_string();
    uims_reply_thing get_resolve_command();
    bool choose_font();
    bool print_this();
    bool print_any();
    bool help_manual();
    bool help_faq();
    popup_return get_popup_string(Cstring prompt1, Cstring prompt2, Cstring final_inline_prompt,
                                  Cstring seed, char *dest);
    void fatal_error_exit(int code, Cstring s1=0, Cstring s2=0);
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
    void add_string_input(const char *str);

    
private:
    void EnterMessageLoop();

    SDThread *sdthread;
    MainWindow *mw;
    QWaitCondition *waitCondition;
    QMutex &mutexMessageLoop;
    char szResolveWndTitle [MAX_TEXT_LINE_LENGTH];

    bool WaitingForCommand;
    int nLastOne;
#define ui_undefined -999
    uims_reply_kind MenuKind;
    popup_return PopupStatus;

    void ShowListBox(int);
    void UpdateStatusBar(const char *);
    bool do_popup(int nWhichOne);

};

void SDThread::on_user_input(QString str)
{
    QByteArray inUtf8 = str.toUtf8();
    const char *data = inUtf8.constData();
    iofull->add_string_input(data);
}

void SDThread::stop()
{
    abort = true;
    qDebug() << "SDThread stop";
    eventLoopWaitCond.wakeAll();
}


void SquareDesk_iofull::add_string_input(const char *s)
{
    mutexMessageLoop.lock();
    matcher_class &matcher = *gg77->matcher_p;

    matcher.copy_to_user_input(s);
    int matches = matcher.match_user_input(nLastOne, false, false, false);

    if ((matches == 1 || matches - matcher.m_yielding_matches == 1 || matcher.m_final_result.exact) &&
        ((!matcher.m_final_result.match.packed_next_conc_or_subcall &&
          !matcher.m_final_result.match.packed_secondary_subcall) ||
         matcher.m_final_result.match.kind == ui_call_select ||
         matcher.m_final_result.match.kind == ui_concept_select))
    {
        mutexMessageLoop.unlock();
        waitCondition->wakeAll();
    }
    else
    {
        qDebug() << "No Match";
        mutexMessageLoop.unlock();
        
    }

    //See the    use_computed_match: case
}

void SquareDesk_iofull::UpdateStatusBar(const char *s)
{
    emit sdthread->on_sd_update_status_bar(QString(s));
}


// SquareDesk_iofull
void SquareDesk_iofull::EnterMessageLoop()
{
 
    gg77->matcher_p->m_active_result.valid = false;
    gg77->matcher_p->erase_matcher_input();
    WaitingForCommand = true;
    qDebug() << "Emitting on_sd_awainting_input()";
    emit sdthread->on_sd_awaiting_input();
    qDebug() << "Waiting on message loop: " << WaitingForCommand;

    waitCondition->wait(&mutexMessageLoop);
    qDebug() << "Out of message loop: " << WaitingForCommand;
    mutexMessageLoop.unlock();

    if (WaitingForCommand)
    {
        general_final_exit(0);
        return;
    }
}


int SquareDesk_iofull::do_abort_popup()
{
    QMessageBox::StandardButton reply;

    reply = QMessageBox::question(mw, "Confirmation",
                                  "The current sequence will be aborted. Do you really want to abort it?",
                                  QMessageBox::Yes | QMessageBox::No);
    return (reply == QMessageBox::Yes) ? POPUP_ACCEPT : POPUP_DECLINE;
}

void SquareDesk_iofull::prepare_for_listing()
{
    // For multi-page listing, we don't need it.
}


void SquareDesk_iofull::set_window_title(char s[])
{
    qDebug() << "SquareDesk_iofull::set_window_title(char s[]);";
    emit sdthread->on_sd_set_window_title(QString(s));
}

void SquareDesk_iofull::add_new_line(const char the_line[], uint32 drawing_picture)
{
    emit sdthread->on_sd_add_new_line(QString(the_line), drawing_picture);
}

void SquareDesk_iofull::no_erase_before_n(int /* n */)
{
    // This is for maintaining crucial lines on the screen.
//    qDebug() << "SquareDesk_iofull::no_erase_before_n(int n);";
}

void SquareDesk_iofull::reduce_line_count(int /* n */)
{
    // This is for culling old lines. We can probably leave it doing nothing.
//    qDebug() << "SquareDesk_iofull::reduce_line_count(int " << n << ");";
}

void SquareDesk_iofull::update_resolve_menu(command_kind goal, int cur, int max,
                                            resolver_display_state state)
{
    create_resolve_menu_title(goal, cur, max, state, szResolveWndTitle);
    UpdateStatusBar(szResolveWndTitle);

    // Put it in the transcript area also, where it's easy to see.

    get_utils_ptr()->writestuff(szResolveWndTitle);
    get_utils_ptr()->newline();
//    qDebug() << "SquareDesk_iofull::update_resolve_menu(command_kind goal, int cur, int max,";
}

void SquareDesk_iofull::show_match(int frequency_to_show)
{
//    qDebug() << "SquareDesk_iofull::show_match(int frequency_to_show);";
    get_utils_ptr()->show_match_item(frequency_to_show);
}

const char *SquareDesk_iofull::version_string()
{
    return "SquareDesk-" VERSIONSTRING;
//    qDebug() << "SquareDesk_iofull::char *version_string();";
}

void SquareDesk_iofull::ShowListBox(int nWhichOne) {
    QStringList options;
    qDebug() << "ShowListBox(" << nWhichOne << ")";

    if (nWhichOne != nLastOne)
    {
        nLastOne = nWhichOne;
        // This if/else block should be a switch, but there are some range checks
        if (nWhichOne == matcher_class::e_match_number)
        {
            for (int i = 0; i < NUM_CARDINALS; ++i)
            {
                options.append(cardinals[i]);
            }
            UpdateStatusBar("<number>");
        }
        else if (nWhichOne == matcher_class::e_match_circcer)
        {
            for (unsigned int i = 0; i < number_of_circcers; ++i)
            {
                options.append(get_call_menu_name(circcer_calls[i]));
            }
            UpdateStatusBar("<circulate replacement>");
        }
        else if (nLastOne >= matcher_class::e_match_taggers &&
                 nLastOne < matcher_class::e_match_taggers+NUM_TAGGER_CLASSES) {
            int tagclass = nLastOne - matcher_class::e_match_taggers;
            UpdateStatusBar("<tagging call>");
        
            for (unsigned int iu=0 ; iu<number_of_taggers[tagclass] ; iu++)
                options.append(get_call_menu_name(tagger_calls[tagclass][iu]));
        }
        else if (nLastOne == matcher_class::e_match_startup_commands) {
            UpdateStatusBar("<startup>");
        
            for (int i=0 ; i<num_startup_commands ; i++)
            {
                options.append(startup_commands[i]);
            }
        }
        else if (nLastOne == matcher_class::e_match_resolve_commands) {
            for (int i=0 ; i<number_of_resolve_commands ; i++)
                options.append(resolve_command_strings[i]);
        }
        else if (nLastOne == matcher_class::e_match_directions) {
            UpdateStatusBar("<direction>");
          
            for (int i=0 ; i<last_direction_kind ; i++)
                options.append(direction_menu_list[i+1]);
        }
        else if (nLastOne == matcher_class::e_match_selectors) {
            UpdateStatusBar("<selector>");

            // Menu is shorter than it appears, because we are skipping first item.
            for (int i=0 ; i<selector_INVISIBLE_START-1 ; i++)
                options.append(selector_menu_list[i]);
        }
        else {
            int i;
        
            UpdateStatusBar(menu_names[nLastOne]);

            for (i=0; i<number_of_calls[nLastOne]; i++)
                options.append(get_call_menu_name(main_call_lists[nLastOne][i]));

            short int *item;
            int menu_length;

            index_list *list_to_use = allowing_all_concepts ?
                &gg77->matcher_p->m_concept_list :
                &gg77->matcher_p->m_level_concept_list;
            item = list_to_use->the_list;
            menu_length = list_to_use->the_list_size;

            for (i=0 ; i<menu_length ; i++)
                options.append(get_concept_menu_name(access_concept_descriptor_table(item[i])));

            for (i=0 ;  ; i++) {
                Cstring name = command_menu[i].command_name;
                if (!name) break;
                options.append(name);
            }
        }
        emit sdthread->on_sd_set_matcher_options(options);
    }
}


uims_reply_thing SquareDesk_iofull::get_resolve_command()
{
//    qDebug() << "SquareDesk_iofull::get_resolve_command();";
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
//    qDebug() << "SquareDesk_iofull::choose_font();";
    return true;
}

bool SquareDesk_iofull::print_this()
{
//    qDebug() << "SquareDesk_iofull::print_this();";
    // ZZZZZ TODO: PRINT!
    return true;
}

bool SquareDesk_iofull::print_any()
{
//    qDebug() << "SquareDesk_iofull::print_any();";
    // ZZZZZ TODO: PRINT!
    return true;
}

bool SquareDesk_iofull::help_manual()
{
//    qDebug() << "SquareDesk_iofull::help_manual();";
//ZZZZZZZZZ   ShellExecute(NULL, "open", "c:\\sd\\sd_doc.html", NULL, NULL, SW_SHOWNORMAL);
    return TRUE;
    
}

bool SquareDesk_iofull::help_faq()
{
//    qDebug() << "SquareDesk_iofull::help_faq();";
// ZZZZZZZZZZ    ShellExecute(NULL, "open", "c:\\sd\\faq.html", NULL, NULL, SW_SHOWNORMAL);
    return TRUE;
}

popup_return SquareDesk_iofull::get_popup_string(Cstring prompt1, Cstring prompt2,
                                                 Cstring final_inline_prompt,
                                                 Cstring /* seed */, char *dest)
{
    QString prompt;
//    qDebug() << "SquareDesk_iofull::get_popup_string(Cstring prompt1, Cstring prompt2, Cstring final_inline_prompt,";
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
    bool ok = false;
    QString sdest = QInputDialog::getText(mw, QString(final_inline_prompt), prompt,
                                            QLineEdit::Normal,
                                            /* const QString &text = */ QString(),
                                            &ok);

    memset(dest, '\0', MAX_TEXT_LINE_LENGTH);
    strncpy(dest, sdest.toStdString().c_str(), MAX_TEXT_LINE_LENGTH - 1);
    
    return ok ? POPUP_ACCEPT_WITH_STRING : POPUP_DECLINE;
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
    qDebug() << "SquareDesk_iofull::fatal_error_exit(int code, Cstring s1=0, Cstring s2=0);";
    session_index = 0;
    general_final_exit(code);
}

void SquareDesk_iofull::serious_error_print(Cstring /* s1 */)
{
    qDebug() << "SquareDesk_iofull::serious_error_print(Cstring s1);";
    // ZZZZZZZ
}

void SquareDesk_iofull::create_menu(call_list_kind /* cl */)
{
    // This is empty in the Windows version, so leaving it empty here
//    qDebug() << "SquareDesk_iofull::create_menu(call_list_kind cl (" << cl << "));";
}

selector_kind SquareDesk_iofull::do_selector_popup(matcher_class &/* matcher */)
{
    qDebug() << "SquareDesk_iofull::do_selector_popup(matcher_class &matcher);";
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
    qDebug() << "SquareDesk_iofull::do_direction_popup(matcher_class &matcher);";
    // ZZZZZ
    return direction_uninitialized;
}

int SquareDesk_iofull::do_circcer_popup()
{
    qDebug() << "SquareDesk_iofull::do_circcer_popup();";
    return true;
}

int SquareDesk_iofull::do_tagger_popup(int /* tagger_class */)
{
    qDebug() << "SquareDesk_iofull::do_tagger_popup(int tagger_class);";
    return true;
}

int SquareDesk_iofull::yesnoconfirm(Cstring /* title */, Cstring line1, Cstring line2, bool /* excl */, bool /* info */)
{
    QString prompt("");
    if (line1 && line1[0])
    {
        prompt = line1;
        prompt += "\n";
    }

    prompt += line2;
//    uint32 flags = MB_YESNO | MB_DEFBUTTON2;
//    if (excl) flags |= MB_ICONEXCLAMATION;
//    if (info) flags |= MB_ICONINFORMATION;
// 
//    if (MessageBox(hwndMain, finalline, title, flags) == IDYES)
//       return POPUP_ACCEPT;
//    else
//       return POPUP_DECLINE;
    return POPUP_ACCEPT;
}

void SquareDesk_iofull::set_pick_string(Cstring string)
{
    qDebug() << "SquareDesk_iofull::set_pick_string(Cstring string);";
    emit sdthread->on_sd_set_pick_string(QString(string));
}

uint32 SquareDesk_iofull::get_one_number(matcher_class &/* matcher */)
{
    qDebug() << "SquareDesk_iofull::get_one_number(matcher_class &matcher);";
    return true;
}

uims_reply_thing SquareDesk_iofull::get_call_command()
{
    qDebug() << "SquareDesk_iofull::get_call_command();";
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
        bool retval = deposit_call_tree(&matcher.m_final_result.match, (parse_block *) 0, 2);
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
    qDebug() << "SquareDesk_iofull::dispose_of_abbreviation(const char *linebuff);";
    emit sdthread->on_sd_dispose_of_abbreviation(QString(linebuff));
    WaitingForCommand = false;
}

void SquareDesk_iofull::display_help()
{
    qDebug() << "SquareDesk_iofull::display_help();";
}

void SquareDesk_iofull::terminate(int /* code */)
{
    qDebug() << "SquareDesk_iofull::terminate(int code);";
}

void SquareDesk_iofull::process_command_line(int */* argcp */, char ***/* argvp */)
{
    qDebug() << "SquareDesk_iofull::process_command_line(int *argcp, char ***argvp);";
}

void SquareDesk_iofull::bad_argument(Cstring /* s1 */, Cstring /* s2 */, Cstring /* s3 */)
{
    qDebug() << "SquareDesk_iofull::bad_argument(Cstring s1, Cstring s2, Cstring s3);";
}

void SquareDesk_iofull::final_initialize()
{
    ui_options.use_escapes_for_drawing_people = 0;

    qDebug() << "SquareDesk_iofull::final_initialize();";
}

bool SquareDesk_iofull::init_step(init_callback_state s, int n)
{
    switch (s) 
    {
    case get_session_info:
        qDebug() << "get_session_info: " << s << " " << n;
        break;
        
    case final_level_query:
        qDebug() << "final_level_query: " << s << " " << n;
        break;
        
    case init_database1:
        qDebug() << "init_database1: " << s << " " << n;
        break;
        
        // update status "Reading database"
    case init_database2:
        qDebug() << "init_database2: " << s << " " << n;
        break;
        
        // update status "Creating menus"
    case calibrate_tick:
        qDebug() << "calibrate_tick: " << s << " " << n;
        break;
       
    case do_tick:
        qDebug() << "do_tick: " << s << " " << n;
        break;
       
    case tick_end:
        qDebug() << "tick_end: " << s << " " << n;
        break;
       
    case do_accelerator:
        qDebug() << "do_accelerator: " << s << " " << n;
        break;
       
    default:
        qDebug() << "init step " << s << " " << n;
        break;
    }
    qDebug() << "SquareDesk_iofull::init_step(init_callback_state s, int n);";
    return false;
}

void SquareDesk_iofull::set_utils_ptr(ui_utils *utils_ptr)
{
    m_ui_utils_ptr = utils_ptr;
    qDebug() << "SquareDesk_iofull::set_utils_ptr(ui_utils *utils_ptr);";
}

ui_utils *SquareDesk_iofull::get_utils_ptr()
{
    qDebug() << "SquareDesk_iofull::*get_utils_ptr();";
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
//   ZZZZZZZZZZ

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



SDThread::SDThread(MainWindow *mw)
    : QThread(mw),
      mw(mw),
      eventLoopWaitCond(),
      mutex(),
      abort(false)
{
    mutex.lock();
    QObject::connect(this, &SDThread::on_sd_update_status_bar, mw, &MainWindow::on_sd_update_status_bar);
    QObject::connect(this, &SDThread::on_sd_awaiting_input, mw, &MainWindow::on_sd_awaiting_input);
    QObject::connect(this, &SDThread::on_sd_set_window_title, mw, &MainWindow::on_sd_set_window_title);
    QObject::connect(this, &SDThread::on_sd_add_new_line, mw, &MainWindow::on_sd_add_new_line);
    QObject::connect(this, &SDThread::on_sd_set_matcher_options, mw, &MainWindow::on_sd_set_matcher_options);
    QObject::connect(this, &SDThread::on_sd_set_pick_string, mw, &MainWindow::on_sd_set_pick_string);
    QObject::connect(this, &SDThread::on_sd_dispose_of_abbreviation, mw, &MainWindow::on_sd_dispose_of_abbreviation);

}

SDThread::~SDThread()
{
    qDebug() << "~SDThread1";
    mutex.lock();
    qDebug() << "~SDThread2";
    abort = true;
    qDebug() << "~SDThread3";
    eventLoopWaitCond.wakeAll();
    qDebug() << "~SDThread4";
    mutex.unlock();
    qDebug() << "~SDThread5";
    wait();
    qDebug() << "~SDThread6";
}

void SDThread::run()
{
    SquareDesk_iofull ggg(this, mw, &eventLoopWaitCond, mutex);
    iofull = &ggg;
    char *argv[] = {const_cast<char *>("SquareDesk"), NULL};

    sdmain(1, argv, ggg);
}

void SDThread::unlock()
{
    mutex.unlock();
}
