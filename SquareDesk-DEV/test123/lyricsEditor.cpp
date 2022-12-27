#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utility.h"

// START LYRICS EDITOR STUFF

// RESOURCE FILES ----------------------------------------------
// get a resource file, and return as string or "" if not found
QString MainWindow::getResourceFile(QString s) {
#if defined(Q_OS_MAC)
    QString appPath = QApplication::applicationFilePath();
    QString patterTemplatePath = appPath + "/Contents/Resources/" + s;
    patterTemplatePath.replace("Contents/MacOS/SquareDesk/","");
#elif defined(Q_OS_WIN32)
    // TODO: There has to be a better way to do this.
    QString appPath = QApplication::applicationFilePath();
    QString patterTemplatePath = appPath + "/" + s;
    patterTemplatePath.replace("SquareDesk.exe/","");
#else
    QString patterTemplatePath = s;
    // Linux
    QStringList paths;
    paths.append(QApplication::applicationFilePath());
    paths.append("/usr/share/SquareDesk");
    paths.append(".");

    for (auto path : paths)
    {
        QString filename(path + "/" + s);
        QFileInfo check_file(filename);
        if (check_file.exists() && check_file.isFile())
        {
            patterTemplatePath = s;
            break;
        }
    }
#endif

    QString fileContents;

    QFile file(patterTemplatePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open '" + s + "' file.";
        qDebug() << "looked here:" << patterTemplatePath;
        return("");  // NOTE: early return, couldn't find template file
    } else {
        fileContents = file.readAll();
        file.close();
    }
    return(fileContents);
}

// DEBUG -------------------------------------------------------
void MainWindow::showHTML(QString fromWhere) {
    Q_UNUSED(fromWhere)
    qDebug() << "***** showHTML(" << fromWhere << "):\n";

    QString editedCuesheet = ui->textBrowserCueSheet->toHtml();
    QString pEditedCuesheet = postProcessHTMLtoSemanticHTML(editedCuesheet);
    qDebug().noquote() << "***** Post-processed HTML will be:\n" << pEditedCuesheet;
}

// BUTTON HANDLING ----------------------------------------------
void MainWindow::setInOutButtonState() {
    bool checked = ui->pushButtonEditLyrics->isChecked();
    if (!prefsManager.GetInOutEditingOnlyWhenLyricsUnlocked())
        checked = true;

    ui->pushButtonSetIntroTime->setEnabled(checked);
    ui->pushButtonSetOutroTime->setEnabled(checked);
    ui->dateTimeEditIntroTime->setEnabled(checked);
    ui->dateTimeEditOutroTime->setEnabled(checked);
}

void MainWindow::on_pushButtonEditLyrics_toggled(bool checkState)
{
//    qDebug() << "on_pushButtonEditLyrics_toggled" << checkState;
    bool checked = (checkState != Qt::Unchecked);

    ui->pushButtonCueSheetEditTitle->setEnabled(checked);
    ui->pushButtonCueSheetEditLabel->setEnabled(checked);
    ui->pushButtonCueSheetEditArtist->setEnabled(checked);
    ui->pushButtonCueSheetEditHeader->setEnabled(checked);
    ui->pushButtonCueSheetEditLyrics->setEnabled(checked);

    ui->pushButtonCueSheetEditBold->setEnabled(checked);
    ui->pushButtonCueSheetEditItalic->setEnabled(checked);

    ui->actionBold->setEnabled(checked);
    ui->actionItalic->setEnabled(checked);
    ui->actionAuto_format_Lyrics->setEnabled(checked);

    ui->pushButtonCueSheetClearFormatting->setEnabled(checked);

    if (checked) {
        // unlocked now, so must set up button state, too
//        qDebug() << "setting up button state using lastKnownTextCharFormat...";
        on_textBrowserCueSheet_currentCharFormatChanged(lastKnownTextCharFormat);
        ui->textBrowserCueSheet->setFocusPolicy(Qt::ClickFocus);  // now it can get focus
        ui->pushButtonCueSheetEditSave->show();   // the two save buttons are now visible
        ui->pushButtonCueSheetEditSaveAs->show();
        ui->pushButtonEditLyrics->hide();  // and this button goes away!
        ui->actionSave->setEnabled(true);  // save is enabled now
        ui->actionSave_As->setEnabled(true);  // save as... is enabled now

    } else {
        ui->textBrowserCueSheet->clearFocus();  // if the user locks the editor, remove focus
        ui->textBrowserCueSheet->setFocusPolicy(Qt::NoFocus);  // and don't allow it to get focus
    }

    setInOutButtonState();
}

void MainWindow::on_textBrowserCueSheet_selectionChanged()
{
    if (ui->pushButtonEditLyrics->isChecked()) {
        // if editing is enabled:
        if (ui->textBrowserCueSheet->textCursor().hasSelection()) {
            // if it has a non-empty selection, then set the buttons based on what the selection contains
            int selContains = currentSelectionContains();
//            qDebug() << "currentSelectionContains: " << selContains;

            ui->pushButtonCueSheetEditTitle->setChecked(selContains & titleBit);
            ui->pushButtonCueSheetEditLabel->setChecked(selContains & labelBit);
            ui->pushButtonCueSheetEditArtist->setChecked(selContains & artistBit);
            ui->pushButtonCueSheetEditHeader->setChecked(selContains & headerBit);
            ui->pushButtonCueSheetEditLyrics->setChecked(selContains & lyricsBit);
        } else {
            // else, base the button state on the charformat at the current cursor position
            QTextCharFormat tcf = ui->textBrowserCueSheet->textCursor().charFormat();

            //        qDebug() << "tell me about: " << tcf.font();
            //        qDebug() << "foreground: " << tcf.foreground();
            //        qDebug() << "background: " << tcf.background();
            //        qDebug() << "family: " << tcf.font().family();
            //        qDebug() << "point size: " << tcf.font().pointSize();
            //        qDebug() << "pixel size: " << tcf.font().pixelSize();

            charsType c = FG_BG_to_type(tcf.foreground().color(), tcf.background().color());
//            qDebug() << "empty selection, charsType: " << c;

            ui->pushButtonCueSheetEditTitle->setChecked(c == TitleChars);
            ui->pushButtonCueSheetEditLabel->setChecked(c == LabelChars);
            ui->pushButtonCueSheetEditArtist->setChecked(c == ArtistChars);
            ui->pushButtonCueSheetEditHeader->setChecked(c == HeaderChars);
            ui->pushButtonCueSheetEditLyrics->setChecked(c == LyricsChars);
        }
    } else {
        // if editing is disabled, all the buttons are disabled.
//        qDebug() << "selection, but editing is disabled.";
        ui->pushButtonCueSheetEditTitle->setChecked(false);
        ui->pushButtonCueSheetEditLabel->setChecked(false);
        ui->pushButtonCueSheetEditArtist->setChecked(false);
        ui->pushButtonCueSheetEditHeader->setChecked(false);
        ui->pushButtonCueSheetEditLyrics->setChecked(false);
    }
    setInOutButtonState();
}

static void setSelectedTextToClass(QTextEdit *editor, QString blockClass)
{
//    qDebug() << "setSelectedTextToClass: " << blockClass;
    SelectionRetainer retainer(editor);
    QTextCursor cursor = editor->textCursor();

    if (!cursor.hasComplexSelection())
    {
        // TODO: remove <SPAN class="title"></SPAN> from entire rest of the document (title is a singleton)
        QString selectedText = cursor.selectedText();
        if (selectedText.isEmpty())
        {
            // if cursor is not selecting any text, make the change apply to the entire line (vs block)
            cursor.movePosition(QTextCursor::StartOfLine);
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            selectedText = cursor.selectedText();
//            qDebug() << "setSelectedTestToClass: " << selectedText;
        }

        if (!selectedText.isEmpty())  // this is not redundant.
        {
            cursor.beginEditBlock(); // start of grouping for UNDO purposes
            cursor.removeSelectedText();
            QString newText = "<SPAN class=\"" + blockClass + "\">" + selectedText.toHtmlEscaped() + "</SPAN>";
//            qDebug() << "newText before: " << newText;
            newText = newText.replace(QChar(0x2028),"</SPAN><BR/><SPAN class=\"" + blockClass + "\">");  // 0x2028 = Unicode Line Separator
//            qDebug() << "newText after: " << newText;
            cursor.insertHtml(newText);
            cursor.endEditBlock(); // end of grouping for UNDO purposes
        }


    } else {
        // this should only happen for tables, which we really don't support yet.
        qDebug() << "Sorry, on_pushButtonCueSheetEdit...: " + blockClass + ": Title_toggled has complex selection...";
    }
}

void MainWindow::on_pushButtonCueSheetEditHeader_clicked(bool /* checked */)
{
    if (!cuesheetEditorReactingToCursorMovement)
    {
        setSelectedTextToClass(ui->textBrowserCueSheet, "hdr");
    }
//    showHTML(__FUNCTION__);
}

void MainWindow::on_pushButtonCueSheetEditItalic_toggled(bool checked)
{
    if (!cuesheetEditorReactingToCursorMovement)
    {
        ui->textBrowserCueSheet->setFontItalic(checked);
    }
//    showHTML(__FUNCTION__);
}

void MainWindow::on_pushButtonCueSheetEditBold_toggled(bool checked)
{
    if (!cuesheetEditorReactingToCursorMovement)
    {
        ui->textBrowserCueSheet->setFontWeight(checked ? QFont::Bold : QFont::Normal);
    }
//    showHTML(__FUNCTION__);
}


void MainWindow::on_pushButtonCueSheetEditTitle_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (!cuesheetEditorReactingToCursorMovement)
    {
        setSelectedTextToClass(ui->textBrowserCueSheet, "title");
    }
//    showHTML(__FUNCTION__);
}

void MainWindow::on_pushButtonCueSheetEditArtist_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (!cuesheetEditorReactingToCursorMovement)
    {
        setSelectedTextToClass(ui->textBrowserCueSheet, "artist");
    }
//    showHTML(__FUNCTION__);
}

void MainWindow::on_pushButtonCueSheetEditLabel_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (!cuesheetEditorReactingToCursorMovement)
    {
        setSelectedTextToClass(ui->textBrowserCueSheet, "label");
    }
//    showHTML(__FUNCTION__);
}

void MainWindow::on_pushButtonCueSheetEditLyrics_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (!cuesheetEditorReactingToCursorMovement)
    {
        setSelectedTextToClass(ui->textBrowserCueSheet, "lyrics");
    }
//    showHTML(__FUNCTION__);
}

int MainWindow::currentSelectionContains() {
    int result = 0;
    QTextCursor cursor = ui->textBrowserCueSheet->textCursor();
    QString theHTML = cursor.selection().toHtml();

//    qDebug() << "\n*****selection (rich text):\n " << theHTML << "\n";

    // NOTE: This code depends on using a special cuesheet2.css file...
    if (theHTML.contains("color:#010101")) {
        result |= titleBit;
    }
    if (theHTML.contains("color:#60c060")) {
        result |= labelBit;
    }
    if (theHTML.contains("color:#0000ff")) {
        result |= artistBit;
    }
    if (theHTML.contains("color:#ff0002")) {
        result |= headerBit;
    }
    if (theHTML.contains("color:#030303")) {
        result |= lyricsBit;
    }
    if (theHTML.contains("color:#000000")) {
        result |= noneBit;
    }

    return (result);
}

void MainWindow::on_pushButtonCueSheetClearFormatting_clicked()
{
//    qDebug() << "on_pushButtonCueSheetClearFormatting_clicked";
        SelectionRetainer retainer(ui->textBrowserCueSheet);
        QTextCursor cursor = ui->textBrowserCueSheet->textCursor();

//        qDebug() << "\n***** CURSOR: " << cursor.selectionStart() << cursor.selectionEnd() << cursor.position() << cursor.atEnd();

        // now look at it as HTML
        QString selected = cursor.selection().toHtml();
//        qDebug() << "\n***** initial selection (HTML): " << selected;

        // Qt gives us a whole HTML doc here.  Strip off all the parts we don't want.
        QRegularExpression startSpan("<span.*>", QRegularExpression::InvertedGreedinessOption);  // don't be greedy!
//        startSpan.setMinimal(true);  // don't be greedy!

        selected.replace(QRegularExpression("<.*<!--StartFragment-->"),"")
                .replace(QRegularExpression("<!--EndFragment-->.*</html>"),"")
                .replace(startSpan,"")
                .replace("</span>","")
                ;
//        qDebug() << "current replacement: " << selected;

        // SUPER CLEAN -------
        if (cursor.selectionStart() == 0 && cursor.atEnd()) {
            // selection starts at beginning of file (position 0), and cursor is at end of file, means everything in the file is selected
            //   if this is the case, let's do a SUPER CLEAN to get rid of all formatting from say MS Word or OpenOffice.
//            qDebug() << "BEFORE CLEANING: '" << selected << "'";
            selected
                    .replace(QRegularExpression("<HEAD>.*</HEAD>", QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption), "") // delete <head> section
                    .replace(QRegularExpression("<BR *>",  QRegularExpression::CaseInsensitiveOption), "|BRBRBR|") // preserve line breaks
                    .replace(QRegularExpression("<BR */>", QRegularExpression::CaseInsensitiveOption), "|BRBRBR|")
                    .replace(QRegularExpression("<.*>", QRegularExpression::InvertedGreedinessOption), "") // delete ALL TAGS
                    .replace("|BRBRBR|", "<BR>") // restore line breaks
                    .replace(QRegularExpression("^(<BR>)+"), "")    // delete extra BR's at the start of the file
                    .replace(QRegularExpression("^\n+"), "")        // delete extra NL's at the start of the file
                    .replace("\n", "<BR>\n")     // restore line breaks
                    ;
//            qDebug() << "SUPER CLEANED: '" << selected << "'";
        }

        // WARNING: this has a dependency on internal cuesheet2.css's definition of BODY text.
        QString HTMLreplacement =
                "<span style=\" font-family:'Verdana'; font-size:large; color:#000000;\">" +
                selected +
                "</span>";

//        qDebug() << "\n***** HTMLreplacement: " << HTMLreplacement;

        cursor.beginEditBlock(); // start of grouping for UNDO purposes
        cursor.removeSelectedText();  // remove the rich text...
//        cursor.insertText(selected);  // ...and put back in the stripped-down text
        cursor.insertHtml(HTMLreplacement);  // ...and put back in the stripped-down text
        cursor.endEditBlock(); // end of grouping for UNDO purposes
}

// Knowing what the FG and BG colors are (from internal cuesheet2.css) allows us to determine the character type
// The parsing code below looks at FG and BG in a very specific order.  Be careful if making changes.
// The internal cuesheet2.css file is also now fixed.  If you change colors there, you'll break editing.
//

MainWindow::charsType MainWindow::FG_BG_to_type(QColor fg, QColor bg) {
    Q_UNUSED(bg)
//    qDebug() << "fg: " << fg.blue() << ", bg: " << bg.blue();
    return(static_cast<charsType>(fg.blue()));  // the blue channel encodes the type
}

void MainWindow::on_textBrowserCueSheet_currentCharFormatChanged(const QTextCharFormat & f)
{
//    qDebug() << "on_textBrowserCueSheet_currentCharFormatChanged" << f;

    RecursionGuard guard(cuesheetEditorReactingToCursorMovement);

    ////    ui->pushButtonCueSheetEditHeader->setChecked(f.fontPointSize() == 14);

    ui->pushButtonCueSheetEditItalic->setChecked(f.fontItalic());
    ui->pushButtonCueSheetEditBold->setChecked(f.fontWeight() == QFont::Bold);

//    if (f.isCharFormat()) {
//        QTextCharFormat tcf = (QTextCharFormat)f;
////        qDebug() << "tell me about: " << tcf.font();
////        qDebug() << "foreground: " << tcf.foreground();
////        qDebug() << "background: " << tcf.background();
////        qDebug() << "family: " << tcf.font().family();
////        qDebug() << "point size: " << tcf.font().pointSize();
////        qDebug() << "pixel size: " << tcf.font().pixelSize();

//        charsType c = FG_BG_to_type(tcf.foreground().color(), tcf.background().color());
////        qDebug() << "charsType: " << c;

    lastKnownTextCharFormat = f;  // save it away for when we unlock editing
}



static bool isFileInPathStack(QList<QString> *pathStack, const QString &checkFilename)
{
    QListIterator<QString> iter(*pathStack);
    while (iter.hasNext()) {
        QString s = iter.next();
        QStringList sl1 = s.split("#!#");
        QString type = sl1[0];  // the type (of original pathname, before following aliases)
        QString filename = sl1[1];  // everything else
        if (filename == checkFilename)
            return true;
    }
    return false;
}

// This function is called to write out the tidied/semantically-processed HTML to a file.
// If SAVE or SAVE AS..., then the file is read in again from disk, in case there are round-trip problems.
//

// NOTE: must match the one in mainwindow.cpp (I'll fix this later)...
static const char *cuesheet_file_extensions[3] = { "htm", "html", "txt" };          // NOTE: must use Qt::CaseInsensitive compares for these

void MainWindow::writeCuesheet(QString filename)
{
//    qDebug() << "writeCuesheet: " << filename;
    bool needs_extension = true;
    for (size_t i = 0; i < (sizeof(cuesheet_file_extensions) / sizeof(*cuesheet_file_extensions)); ++i)
    {
        // qDebug() << "writeCuesheet(): " << cuesheet_file_extensions[i];  // DEBUG
        QString ext(".");
        ext.append(cuesheet_file_extensions[i]);
        if (filename.endsWith(ext, Qt::CaseInsensitive))
        {
            needs_extension = false;
            break;
        }
    }
    if (needs_extension)
    {
        filename += ".html";
    }

    QFile file(filename);
    QDir d = QFileInfo(file).absoluteDir();
    QString directoryName = d.absolutePath();  // directory of the saved filename

    lastCuesheetSavePath = directoryName;

#define WRITETHEMODIFIEDLYRICSFILE
#ifdef WRITETHEMODIFIEDLYRICSFILE
    if ( file.open(QIODevice::WriteOnly) )
    {
        filewatcherShouldIgnoreOneFileSave = true;  // when we save it will trigger filewatcher, which
        //  will reorder the songTable.  We are adding the file to the pathStack manually, so there
        //  really is no need to trigger the filewatcher.

        // Make sure the destructor gets called before we try to load this file...
        {
            QTextStream stream( &file );
            QString editedCuesheet = ui->textBrowserCueSheet->toHtml();
            QString postProcessedCuesheet = postProcessHTMLtoSemanticHTML(editedCuesheet);
            stream << postProcessedCuesheet;
            stream.flush();
        }

        QTextCursor cursor = ui->textBrowserCueSheet->textCursor();
        cursor.movePosition(QTextCursor::Start);  // move cursor to the start of the file after a save

        if (!isFileInPathStack(pathStack, filename))
        {
            QFileInfo fi(filename);
            QStringList section = fi.path().split("/");
            QString type = section[section.length()-1];  // must be the last item in the path
//            qDebug() << "writeCuesheet() adding " + type + "#!#" + filename + " to pathStack";
            pathStack->append(type + "#!#" + filename);
        }
    }
#else
                qDebug() << "************** SAVE FILE ***************";
                showHTML(__FUNCTION__);

                QString editedCuesheet = ui->textBrowserCueSheet->toHtml();
                qDebug().noquote() << "***** editedCuesheet to write:\n" << editedCuesheet;

//                QString postProcessedCuesheet = postProcessHTMLtoSemanticHTML(editedCuesheet);
//                qDebug().noquote() << "***** I AM THINKING ABOUT WRITING TO FILE postProcessed:\n" << postProcessedCuesheet;
#endif
}

void MainWindow::on_pushButtonCueSheetEditSave_clicked()
{
//    QTextCursor tc = ui->textBrowserCueSheet->textCursor();  // save text cursor
//    qDebug() << "on_pushButtonCueSheetEditSave_clicked";
//    if (ui->textBrowserCueSheet->document()->isModified()) {  // FIX: document is always modified.

        saveLyrics();

//        ui->textBrowserCueSheet->document()->setModified(false);  // has not been modified now
//    }

    ui->pushButtonCueSheetEditSave->hide();   // the two save buttons are now invisible
    ui->pushButtonCueSheetEditSaveAs->hide();
    ui->pushButtonEditLyrics->show();  // and the "unlock for editing" button is now visible
    ui->actionSave->setEnabled(false);  // save is disabled now
    ui->actionSave_As->setEnabled(false);  // save as... is also disabled now
    setInOutButtonState();

//    ui->textBrowserCueSheet->setTextCursor(tc); // Reset the cursor after a save  // NOTE: THis doesn't seem to work in recent Qt versions. Always sets to end.
}

void MainWindow::on_pushButtonCueSheetEditSaveAs_clicked()
{
//    QTextCursor tc = ui->textBrowserCueSheet->textCursor();  // save text cursor

    //    qDebug() << "on_pushButtonCueSheetEditSaveAs_clicked";
    saveLyricsAs();  // we probably want to save with a different name, so unlike "Save", this is always called here.

    ui->pushButtonCueSheetEditSave->hide();   // the two save buttons are now invisible
    ui->pushButtonCueSheetEditSaveAs->hide();
    ui->pushButtonEditLyrics->show();  // and the "unlock for editing" button is now visible
    ui->actionSave->setEnabled(false);  // save is disabled now
    ui->actionSave_As->setEnabled(false);  // save as... is also disabled now

    setInOutButtonState();

//    ui->textBrowserCueSheet->setTextCursor(tc); // Reset the cursor after a save
}

// ------------------------
QString MainWindow::postProcessHTMLtoSemanticHTML(QString cuesheet) {
//    qDebug() << "postProcessHTMLtoSemanticHTML";

    // margin-top:12px;
    // margin-bottom:12px;
    // margin-left:0px;
    // margin-right:0px;
    // -qt-block-indent:0;
    // text-indent:0px;
    // line-height:100%;
    // KEEP: background-color:#ffffe0;
    cuesheet
            .replace(QRegularExpression("margin-top:[0-9]+px;"), "")
            .replace(QRegularExpression("margin-bottom:[0-9]+px;"), "")
            .replace(QRegularExpression("margin-left:[0-9]+px;"), "")
            .replace(QRegularExpression("margin-right:[0-9]+px;"), "")
            .replace(QRegularExpression("text-indent:[0-9]+px;"), "")
            .replace(QRegularExpression("line-height:[0-9]+%;"), "")
            .replace(QRegularExpression("-qt-block-indent:[0-9]+;"), "")
            ;

    // get rid of unwanted QTextEdit tags
    QRegularExpression styleRegExp("(<STYLE.*</STYLE>)|(<META.*>)", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
//    styleRegExp.setMinimal(true);
//    styleRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    cuesheet.replace(styleRegExp,"");  // don't be greedy

//    qDebug().noquote() << "***** postProcess 1: " << cuesheet;
    QString cuesheet3 = cuesheet; // NOTE: no longer using libtidy here

    cuesheet3.replace("Arial Black", "Verdana"); // get rid of that!
    cuesheet3.replace(".SF NS Text", "Verdana"); // get rid of that one, too!
    cuesheet3.replace("Helvetica Neue", "Verdana"); // yep.
    cuesheet3.replace("Minion", "Verdana");  // where did this one come from?
    cuesheet3.replace("Arial,Helvetica,sans-serif", "Verdana");
    cuesheet3.replace("arial,sans-serif", "Verdana");
    cuesheet3.replace("Menlo", "Verdana");
    cuesheet3.replace("ArialMT", "Verdana");

    cuesheet3.replace("font-family:'Verdana';", "");  // eliminate all the fonts (CSS will take care of this)

    // now the semantic replacement.
    // assumes that QTextEdit spits out spans in a consistent way
    // TODO: allow embedded NL (due to line wrapping)
    // NOTE: background color is optional here, because I got rid of the the spec for it in BODY
    // <SPAN style="font-family:'Verdana'; font-size:x-large; color:#ff0002; background-color:#ffffe0;">

    // HEADER --------
//    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:x-large; color:#ff0002;[\\s\n]*(background-color:#ffffe0;)*\">"),
//                             "<SPAN class=\"hdr\">");

    // <span    style=" font-size:x-large; color:#ff0002;    ">
    // <span style="  font-size:x-large; color:#ff0002;">
    // <span style=" font-size:25pt; color:#ff0002;">
    // <span style=" font-size:25pt; color:#ff0002; background-color:#ffffe0;">
    QRegularExpression HeaderRegExp1("<SPAN[\\s\n]+style=[\\s\n]*\"[\\s\n]+font-size:(large|x-large|\\d+pt);[\\s\n]+color:#ff0002;[\\s\n]*\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(HeaderRegExp1, "<SPAN class=\"hdr\">");
    QRegularExpression HeaderRegExp2("<SPAN[\\s\n]+style=[\\s\n]*\"[\\s\n]+font-size:(large|x-large|\\d+pt);[\\s\n]+color:#ff0002;[\\s\n]+background-color:#ffffe0;\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(HeaderRegExp2, "<SPAN class=\"hdr\">");

    // LABEL ---------
//    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:medium; color:#60c060;[\\s\n]*(background-color:#ffffe0;)*\">"),
//                             "<SPAN class=\"label\">");

    // <span    style=" font-size:medium; color:#60c060;   ">
    // <span style=" font-size:25pt; color:#60c060;">
    // <span style=" font-size:25pt; color:#60c060; background-color:#ffffe0;">
    // <span style="  font-size:medium; color:#60c060;">
    QRegularExpression LabelRegExp("<SPAN[\\s\n]+style=[\\s\n]*\"[\\s\n]+font-size:(medium|\\d+pt);[\\s\n]+color:#60c060;[\\s\n]*\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(LabelRegExp, "<SPAN class=\"label\">");
    QRegularExpression LabelRegExp2("<SPAN[\\s\n]*style=[\\s\n]*\"[\\s\n]+font-size:(medium|\\d+pt);[\\s\n]+color:#60c060;[\\s\n]+background-color:#ffffe0;\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(LabelRegExp2, "<SPAN class=\"label\">");

    // ARTIST ---------
//    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:medium; color:#0000ff;[\\s\n]*(background-color:#ffffe0;)*\">"),
//                             "<SPAN class=\"artist\">");

    // <span    style=" font-size:medium; color:#0000ff;   ">
    // <span style=" font-size:25pt; color:#0000ff;">
    // <span style=" font-size:25pt; color:#0000ff; background-color:#ffffe0;">
    // <span style="  font-size:medium; color:#0000ff;">
    QRegularExpression ArtistRegExp("<SPAN[\\s\n]+style=[\\s\n]*\"[\\s\n]+font-size:(medium|\\d+pt);[\\s\n]+color:#0000ff;[\\s\n]*\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(ArtistRegExp, "<SPAN class=\"artist\">");
    QRegularExpression ArtistRegExp2("<SPAN[\\s\n]*style=[\\s\n]*\"[\\s\n]+font-size:(medium|\\d+pt);[\\s\n]+color:#0000ff;[\\s\n]*background-color:#ffffe0;\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(ArtistRegExp2, "<SPAN class=\"artist\">");

    // TITLE ---------
    // <SPAN style="font-family:'Arial Black'; font-size:x-large; color:#010101; background-color:#ffffe0;">
//    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Arial Black'; font-size:x-large;[\\s\n]*(font-weight:600;)*[\\s\n]*color:#010101;[\\s\n]*(background-color:#ffffe0;)*\">"),
//                             "<SPAN class=\"title\">");

    // <span    style=" font-family:'Verdana'; font-size:x-large; font-weight:699; color:#010101;   ">
    // <span style=" font-size:25pt; font-weight:699; color:#010101;">
    // <span style=" font-family:'Arial Black'; font-size:25pt; color:#010101;">
    // <span style="  font-size:25pt; color:#010101;">
//    QRegularExpression TitleRegExp("<SPAN[\\s\n]*style=[\\s\n]*\" font-family:'Verdana'; font-size:x-large; font-weight:699; color:#010101;[\\s\n]*\">",
//                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    QRegularExpression TitleRegExp("<SPAN[\\s\n]*style=[\\s\n]*\".* font-weight:699; color:#010101;[\\s\n]*\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(TitleRegExp, "<SPAN class=\"title\">");
    QRegularExpression TitleRegExp2("<SPAN[\\s\n]*style=[\\s\n]*\".* font-size:\\d+pt; color:#010101;[\\s\n]*\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(TitleRegExp2, "<SPAN class=\"title\">");

    // BOLD ---------
//    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:large;[\\s\n]*font-weight:600;[\\s\n]*color:#000000;[\\s\n]*(background-color:#ffffe0;)*\">"),
//                                     "<SPAN style=\"font-weight: Bold;\">");

    // <span   style=" font-size:large; font-weight:700; color:#000000;">
    // <span style="  font-size:large; font-weight:700; color:#000000;">
    // <span style=" font-size:large; font-weight:700; color:#000000;">
    // <span style="  font-size:large; font-weight:700; color:#000000;">
    // <span style="  font-size:25pt; font-weight:600; color:#000000; background-color:#ffffe0;">
    QRegularExpression BoldRegExp("<SPAN[\\s\n]+style=[\\s\n]*\"[\\s\n]*font-size:large; font-weight:700; color:#000000;[\\s\n]*\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(BoldRegExp, "<SPAN style=\"font-weight: Bold;\">");

    // now replace <SPAN bold...>foo</SPAN> --> <B>foo</B>
    QRegularExpression boldRegExp1("<SPAN style=\"font-weight: Bold;\">(.*)</SPAN>", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption);
    cuesheet3.replace(boldRegExp1,"<B>\\1</B>");  // don't be greedy, and replace <SPAN bold> -> <B>

    // ITALIC ---------
    // <SPAN style="font-family:'Verdana'; font-size:large; font-style:italic; color:#000000;">
//    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:large;[\\s\n]*font-style:italic;[\\s\n]*color:#000000;[\\s\n]*(background-color:#ffffe0;)*\">"),
//                                     "<SPAN style=\"font-style: Italic;\">");

    // <span  style=" font-size:large; font-style:italic; color:#000000;">
    QRegularExpression ItalicRegExp("<SPAN[\\s\n]+style=[\\s\n]*\"[\\s\n]*font-size:large; font-style:italic; color:#000000;[\\s\n]*\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(ItalicRegExp, "<SPAN style=\"font-style: Italic;\">");

    // now do the same for Italic...
    // "<SPAN style=\"font-style: Italic;\">"
    QRegularExpression italicRegExp("<SPAN style=\"font-style: Italic;\">(.*)</SPAN>", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption);
//    italicRegExp.setMinimal(true);
//    italicRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    cuesheet3.replace(italicRegExp,"<I>\\1</I>");  // don't be greedy, and replace <SPAN italic> -> <B>

    // BOLD ITALIC -------
    // TODO: <span style=" font-size:large; font-weight:700; font-style:italic; color:#000000;">

    // LYRICS ---------
    // <SPAN style="font-family:'Verdana'; font-size:large; color:#030303; background-color:#ffc0cb;">
//    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:large; color:#030303; background-color:#ffc0cb;\">"),  // background-color required for lyrics
//                             "<SPAN class=\"lyrics\">");

    // <span    style=" font-size:large; color:#030303; background-color:#ffc0cb;">
    // <span    style=" font-size:large; color:#030303; background-color:#ffc0cb;">
    // <span style=" font-size:25pt; color:#030303; background-color:#ffc0cb;">
    // <span style="  font-size:large; color:#030303; background-color:#ffc0cb;">
    QRegularExpression LyricsRegExp("<SPAN[\\s\n]+style=[\\s\n]*\"[\\s\n]+font-size:(large|\\d+pt);[\\s\n]+color:#030303;[\\s\n]+background-color:#ffc0cb;[\\s\n]*\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(LyricsRegExp, "<SPAN class=\"lyrics\">");


    // BLANK STYLES ON <P> --------

    QRegularExpression PRegExp("<P style=\"[\\s\n]*\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(PRegExp, "<P>\n    ");

    // <BR/> --> <BR/>\n for readability --------
    cuesheet3.replace("<br />", "<br />\n    ");
    cuesheet3.replace(QRegularExpression("<br />[\\s\n]*</span>", QRegularExpression::CaseInsensitiveOption), "<BR/></span>");
    cuesheet3.replace(QRegularExpression("white-space: pre-wrap;"), "white-space:normal;");  // messes up line spacing
    cuesheet3.replace("<BR/><", "<BR/>\n    <");
    cuesheet3.replace("<br />", "<BR/>");
//    cuesheet3.replace("<P>\n", "");   // there is only one of these, and it's not needed.
//    cuesheet3.replace("</p>", "");  // there is only one of these, and it's not needed.

    // <span style=" font-size:large; color:#000000;">
    // <span style="  font-size:large; color:#000000;">
    // <span style="  font-size:large; color:#000000;">
    // <span style="  font-size:large; color:#000000;">
    cuesheet3.replace(QRegularExpression("<span[\\s\n]+style=\"[\\s\n]+font-size:large;[\\s\n]+color:#000000;\">",QRegularExpression::CaseInsensitiveOption), "<span>");
    cuesheet3.replace(QRegularExpression("<span[\\s\n]+style=\"[\\s\n]+font-family:'Verdana';[\\s\n]+font-size:large;[\\s\n]+color:#000000;[\\s\n]+background-color:#ffffe0;\">",QRegularExpression::CaseInsensitiveOption), "<span>"); // more crap
    cuesheet3.replace(QRegularExpression("<span[\\s\n]+style=\"[\\s\n]+font-family:'Verdana';[\\s\n]+font-size:large;[\\s\n]+color:#000000;\">",QRegularExpression::CaseInsensitiveOption), "<span>");  // kludge at this point
    cuesheet3.replace(QRegularExpression("<span style=\"[\\s\n]+font-size:\\d+pt;[\\s\n]+color:#000000;[\\s\n]+background-color:#ffffe0;\">",
                                         QRegularExpression::CaseInsensitiveOption), "<span>");  // more kludge
    // <span style=" font-size:large; color:#000000; background-color:#ffffe0;">
    cuesheet3.replace(QRegularExpression("<span[\\s\n]+style=\"[\\s\n]+font-size:(large|\\d+pt);[\\s\n]+color:#000000;[\\s\n]+background-color:#ffffe0;\">",
                                         QRegularExpression::CaseInsensitiveOption), "<span>");  // kludge at this point
    // <span style=" font-size:25pt; color:#000000;">
    // <span style=" font-size:25pt; color:#000000;">
    // <span style=" font-size:21pt; color:#000000;">
    cuesheet3.replace(QRegularExpression("<span[\\s\n]+style=\"[\\s\n]+font-size:\\d+pt;[\\s\n]+color:#000000;\">",
                                         QRegularExpression::CaseInsensitiveOption), "<span>");  // kludge at this point

    QRegularExpression spanRegExp("<span>(.*)</span>", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    cuesheet3.replace(spanRegExp,"\\1");  // don't be greedy, and replace <span>anything</span> with anything.  This gets rid of the crappy color:#000000 stuff.

    // <p style="-qt-paragraph-type:empty;        font-size:large; color:#000000; background-color:#ffffe0;"><BR/></p>
    // <p style="-qt-paragraph-type:empty;       font-size:large; color:#000000;"><BR/>    </p>
    // <p style="-qt-paragraph-type:empty;      "><BR/>   </p>
    // <p style="-qt-paragraph-type:empty;        font-family:'Verdana'; font-size:large; color:#000000;">
    // <p style="-qt-paragraph-type:empty;        font-size:25pt; color:#000000; background-color:#ffffe0;">
    cuesheet3.replace("font-family:'Verdana';", ""); // this is specified elsewhere
    QRegularExpression emptyRegExp1("<p style=\"-qt-paragraph-type:empty;[\\s\n]*font-size:(large|\\d+pt); color:#000000; background-color:#ffffe0;\">(.*)</p>", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    cuesheet3.replace(emptyRegExp1,"\\2");  // don't be greedy, and replace <span>anything</span> with anything.  This gets rid of the crappy empty paragraph stuff.
    QRegularExpression emptyRegExp2("<p style=\"-qt-paragraph-type:empty;[\\s\n]*font-size:large; color:#000000;\">(.*)</p>", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    cuesheet3.replace(emptyRegExp2,"\\1");  // don't be greedy, and replace <span>anything</span> with anything.  This gets rid of the crappy empty paragraph stuff.
    QRegularExpression emptyRegExp3("<p style=\"-qt-paragraph-type:empty;[\\s\n]*\">(.*)</p>", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    cuesheet3.replace(emptyRegExp3,"\\1");  // don't be greedy, and replace <span>anything</span> with anything.  This gets rid of the crappy empty paragraph stuff.

    // -------------
    // <p style="        background-color:#ffffe0;">
    cuesheet3.replace(QRegularExpression("<P style=\"[\\s\n]*background-color:#ffffe0;\">", QRegularExpression::CaseInsensitiveOption),"<P>");  // background color already defaults via the BODY statement

    // BODY -----------
    // <body style=" font-family:'.AppleSystemUIFont'; font-size:23pt; font-weight:400; font-style:normal;" bgcolor="#ffffe0">
//    cuesheet3.replace("<BODY bgcolor=\"#FFFFE0\" style=\"font-family:'.SF NS Text'; font-size:13pt; font-weight:400; font-style:normal;\">","<BODY>");  // must go back to USER'S choices in cuesheet2.css

    QRegularExpression BodyRegExp("<body style=\" font-family:'.AppleSystemUIFont'; font-size:2[0-9]pt; font-weight:400; font-style:normal;\" bgcolor=\"#ffffe0\">",
                                    QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption );
    cuesheet3.replace(BodyRegExp, "\n\n<BODY>");  // must go back to USER'S choices in cuesheet2.css
    cuesheet3.replace("</body>", "\n</BODY>");
    cuesheet3.replace("<head>", "\n<HEAD>\n");
    cuesheet3.replace("</head>", "</HEAD>");
    cuesheet3.replace("<html>", "\n<HTML>");
    cuesheet3.replace("</html>", "\n</HTML>");
    cuesheet3.replace("<title>", "    <TITLE>");
    cuesheet3.replace("</title>", "</TITLE>");
    cuesheet3.replace("<style", "\n<STYLE");
    cuesheet3.replace("</style>", "\n</STYLE>");
    cuesheet3.replace("</span>", "</SPAN>");

    cuesheet3.replace(QRegularExpression("<STYLE type=\"text/css\">.*</STYLE>", QRegularExpression::InvertedGreedinessOption | QRegularExpression::DotMatchesEverythingOption), "");  // delete entire STYLE section

    // multi-step replacement
//    qDebug().noquote() << "***** REALLY BEFORE:\n" << cuesheet3;
    //      <SPAN style="font-family:'Verdana'; font-size:large; color:#000000; background-color:#ffffe0;">
    cuesheet3.replace(QRegularExpression("\"font-family:'Verdana'; font-size:large; color:#000000;( background-color:#ffffe0;)*\""),"\"XXXXX\""); // must go back to USER'S choices in cuesheet2.css
//    qDebug().noquote() << "***** BEFORE:\n" << cuesheet3;
    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"XXXXX\">"),"<SPAN>");
//    qDebug().noquote() << "***** AFTER:\n" << cuesheet3;

    // now replace null SPAN tags
    QRegularExpression nullStyleRegExp("<SPAN>(.*)</SPAN>", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption);
//    nullStyleRegExp.setMinimal(true);
//    nullStyleRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    cuesheet3.replace(nullStyleRegExp,"\\1");  // don't be greedy, and replace <SPAN>foo</SPAN> with foo

    // TODO: bold -- <SPAN style="font-family:'Verdana'; font-size:large; font-weight:600; color:#000000;">
    // TODO: italic -- TBD
    // TODO: get rid of style="background-color:#ffffe0;, yellowish, put at top once
    // TODO: get rid of these, use body: <SPAN style="font-family:'Verdana'; font-size:large; color:#000000;">

    QRegularExpression P2BRRegExp("<p>(.*)</p>", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    cuesheet3.replace(P2BRRegExp,"\\1<BR/>");  // don't be greedy, and replace the unneeded <P>...</P>
    QRegularExpression BRTableRegExp("<BR/>[\\s\n]*(</?t[ard])", QRegularExpression::CaseInsensitiveOption);
    cuesheet3.replace(BRTableRegExp,"\\1");  // replace the unneeded <BR/> before <table or <td or <tr
    cuesheet3.replace("<BR/>\n    \n", "<BR/>\n");
    cuesheet3.replace("<BR/>\n\n", "<BR/>\n");
    cuesheet3.replace("<BODY>\n\n", "<BODY>\n");

    cuesheet3.replace("<SPAN class=\"hdr\">", "\n    <SPAN class=\"hdr\">"); // a little extra space to make it easier to read
    cuesheet3.replace("    <BR/>\n</BODY>", "</BODY>"); // cleanup on aisle 7
    cuesheet3.replace("<BR/>\n</BODY>", "\n</BODY>"); // get rid of that last NL-equivalent at the end of the file (is this OK?)

    // Let's update the TITLE
    QRegularExpressionMatch match2;
    QRegularExpression oldTitle("<TITLE>(.*)</TITLE>", QRegularExpression::DotMatchesEverythingOption);
    bool b2 = cuesheet3.contains(oldTitle, &match2);
//    if (b2) {
//        qDebug() << "Found TITLE: " << match2.captured(1);
//    }

    QString userTitle;
    QRegularExpressionMatch match3;
    bool b3 = cuesheet3.contains(QRegularExpression("<SPAN class=\"title\">(.*)</SPAN>",
                                                    QRegularExpression::InvertedGreedinessOption),  // don't be greedy
                                 &match3); // find user-selected Title
    if (b3) {
        userTitle = match3.captured(1);
//        qDebug() << "Found userTitle: " << userTitle;
    }

    QString userLabel;
    QRegularExpressionMatch match4;
    bool b4 = cuesheet3.contains(QRegularExpression("<SPAN class=\"label\">(.*)</SPAN>",
                                                    QRegularExpression::InvertedGreedinessOption),  // don't be greedy
                                 &match4); // find user-selected Label
    if (b4) {
        userLabel= match4.captured(1);
//        qDebug() << "Found userLabel: " << userLabel;
    }

    QString newTitleString = QString("<TITLE>") + userTitle + " " + userLabel + QString("</TITLE>");
    if (b2) {
        // if there was a TITLE section, replace it
        cuesheet3.replace(oldTitle, newTitleString);
    } else {
        // else, add in a new TITLE section
        cuesheet3.replace("<HEAD>", QString("<HEAD>\n    ") + newTitleString + "\n");
    }

    // put the <link rel="STYLESHEET" type="text/css" href="cuesheet2.css"> back in
    // TODO: replace this with inline STYLE inside HEAD
    if (!cuesheet3.contains("<link",Qt::CaseInsensitive)) {
//        qDebug() << "Putting the <LINK> back in...";
        cuesheet3.replace("</TITLE>","</TITLE>\n    <LINK rel=\"STYLESHEET\" type=\"text/css\" href=\"cuesheet2.css\">");
    }

    // remove the link now, since the stylesheet is made internal to each file (and removed and replaced when edited)
    QRegularExpression linkRegex("<LINK[\\s\n]+rel=\"STYLESHEET\"[\\s\n]+type=\"text/css\"[\\s\n]+href=\"cuesheet2.css\">", QRegularExpression::CaseInsensitiveOption);
    cuesheet3.replace(linkRegex, "");

    // let's always stick in a version number when we write, just in case we need to debug.
//    qDebug() << "cuesheetSquareDeskVersion: " << cuesheetSquareDeskVersion; // version number of SquareDesk that wrote the file we read in
    QString newVersionString = QString("<!-- squaredesk:version = ") + QString(VERSIONSTRING) + QString(" -->");
//    qDebug() << "newVersionString: " << newVersionString; // current version of SquareDesk
    cuesheet3.replace("<HEAD>", QString("<HEAD> ") + newVersionString); // always write version number

    // Now, insert the stylesheet into the file, so that it can be viewed standalone in a browser.
    QString cssStylesheet = ui->textBrowserCueSheet->document()->defaultStyleSheet();
    cssStylesheet.replace("\n", "\n        ");
    QRegularExpression endTitle1("</TITLE>\n");
    QString newCSS = QString("</TITLE>\n    <STYLE>\n        ") + cssStylesheet + "\n    </STYLE>";
    cuesheet3.replace(endTitle1, newCSS);

    return(cuesheet3);
}

void MainWindow::maybeLoadCSSfileIntoTextBrowser() {
    // makes the /lyrics directory, if it doesn't exist already
    QString musicDirPath = prefsManager.GetmusicPath();
    QString lyricsDir = musicDirPath + "/lyrics";

    // if the lyrics directory doesn't exist, create it
    QDir dir(lyricsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // ------------------------------------------------------------------
    // NOTE: For correct operation of the Lyrics editor, the colors must be distinct from each other
    QString css = "body, p, font { font-family: Verdana; font-size: large; font-weight: Normal; color: #000000; background: #FFFFE0; margin-top: 0px; margin-bottom: 0px;}\n"
                  ".title        { font-size: x-large; font-weight: 699;    color: #010101;}\n"
                  ".label        { font-size: medium;  font-weight: Normal; color: #60C060;}\n"
                  ".artist       { font-size: medium;  font-weight: Normal; color: #0000FF;}\n"
                  ".hdr          { font-size: x-large; font-weight: Normal; color: #FF0002;}\n"
                  ".lyrics       { font-size: large;   font-weight: Normal; color: #030303; background-color: #FFC0CB;}\n"
                  ".bold         { font-weight: bold; }\n"
                  ".italic       { font-style: italic; }";
    ui->textBrowserCueSheet->document()->setDefaultStyleSheet(css);
}

// Convert .txt file to .html string -------------------
QString MainWindow::txtToHTMLlyrics(QString text, QString filePathname) {
    Q_UNUSED(filePathname)

    // get internal CSS file (we no longer let users change it in individual folders)
    QString css = getResourceFile("cuesheet2.css");

    text = text.toHtmlEscaped();  // convert ">" to "&gt;" etc
    text = text.replace(QRegularExpression("[\r|\n]"),"<br/>\n");

    QString HTML;
    HTML += "<HTML>\n";
    HTML += "<HEAD><STYLE>" + css + "</STYLE></HEAD>\n";
    HTML += "<BODY>\n" + text + "</BODY>\n";
    HTML += "</HTML>\n";

    return(HTML);
}

// LOAD A CUESHEET INTO THE EDITOR -------------------------------------------
void MainWindow::loadCuesheet(const QString &cuesheetFilename)
{
    loadedCuesheetNameWithPath = ""; // nothing loaded yet

    QUrl cuesheetUrl(QUrl::fromLocalFile(cuesheetFilename));
    if (cuesheetFilename.endsWith(".txt", Qt::CaseInsensitive)) {
        // text files are read in, converted to HTML, and sent to the Lyrics tab --------
        QFile f1(cuesheetFilename);
        f1.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream in(&f1);
        QString html = txtToHTMLlyrics(in.readAll(), cuesheetFilename);
        ui->textBrowserCueSheet->setText(html);
        loadedCuesheetNameWithPath = cuesheetFilename;
        f1.close();
    }
    else if (cuesheetFilename.endsWith(".mp3", Qt::CaseInsensitive)) {
        // lyrics embedded in an MP3 file -------------
        QString embeddedID3Lyrics = loadLyrics(cuesheetFilename);
        if (embeddedID3Lyrics != "") {
            QString HTMLlyrics = txtToHTMLlyrics(embeddedID3Lyrics, cuesheetFilename);
            QString html(HTMLlyrics);  // embed CSS, if found, since USLT is plain text
            ui->textBrowserCueSheet->setHtml(html);
            loadedCuesheetNameWithPath = cuesheetFilename;
        }
    } else {
        // regular HTML cuesheet -------------
        QFile f1(cuesheetFilename);
        QString cuesheet;
        if ( f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f1);
            cuesheet = in.readAll();  // read the entire HTML file, assuming it exists

//            qDebug() << "============ Cuesheet ===============\n" << cuesheet;

            cuesheet.replace("\xB4","'");  // replace wacky apostrophe, which doesn't display well in QEditText

            // NOTE: o-umlaut is already translated (incorrectly) here to \xB4, too.  There's not much we
            //   can do with non UTF-8 HTML files that aren't otherwise marked as to encoding.

            // detect that this was a SquareDesk cuesheet
            QRegularExpressionMatch match1;
            bool containsSquareDeskVersion = cuesheet.contains(QRegularExpression("<!-- squaredesk:version = (\\d+.\\d+.\\d+) -->"), &match1);
            Q_UNUSED(containsSquareDeskVersion)
            cuesheetSquareDeskVersion = match1.captured(1);
//            qDebug() << "contains: " << containsSquareDeskVersion << match1.captured(1);

            // set the HTML for the cuesheet itself (must set CSS first)
            ui->textBrowserCueSheet->setHtml(cuesheet);
            loadedCuesheetNameWithPath = cuesheetFilename;
            f1.close();
//            showHTML(__FUNCTION__);  // DEBUG DEBUG DEBUG
        }

    }

    ui->textBrowserCueSheet->document()->setModified(false);

//    qDebug() << "scrolling to top now...";
//    int minScroll = ui->textBrowserCueSheet->verticalScrollBar()->minimum();
//    ui->textBrowserCueSheet->verticalScrollBar()->setValue(static_cast<int>(minScroll));
//    qDebug() << "VERTICAL SCROLL VALUE: " << ui->textBrowserCueSheet->verticalScrollBar()->value();

    // NEW: Let's NOT reset the margins, and let the embedded CSS do that....

//    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor); // select entire document
//    QTextBlockFormat fmt;       // = cursor.blockFormat(); // get format of current block
//    fmt.setTopMargin(0.0);
//    fmt.setBottomMargin(0.0);   // modify it
//    cursor.mergeBlockFormat(fmt); // set margins to zero for all blocks
//    cursor.movePosition(QTextCursor::Start);  // move cursor back to the start of the document

    ui->pushButtonEditLyrics->setChecked(false);  // locked for editing, in case this is just a change in the dropdown
    ui->pushButtonCueSheetEditSave->hide();   // the two save buttons are now invisible
    ui->pushButtonCueSheetEditSaveAs->hide();
    ui->pushButtonEditLyrics->show();  // and the "unlock for editing" button shows up!

    int index = ui->tabWidget->currentIndex();
    if (ui->tabWidget->tabText(index) == "Lyrics" || ui->tabWidget->tabText(index) == "*Lyrics" ||
                   ui->tabWidget->tabText(index) == "Patter" || ui->tabWidget->tabText(index) == "*Patter") {
        // only do this if we are on the Lyrics/Patter tab.  If on MusicPlayer, playlist load will turn ON Save As.
        ui->actionSave->setEnabled(false);  // save is disabled to start out
        ui->actionSave_As->setEnabled(false);  // save as... is also disabled at the start
    }

    setInOutButtonState();
}

// SAVE LYRICS ------------------------------
void MainWindow::saveLyrics()
{
    // Save cuesheet to the current cuesheet filename...
    RecursionGuard dialog_guard(inPreferencesDialog);

    QString cuesheetFilename = ui->comboBoxCuesheetSelector->itemData(ui->comboBoxCuesheetSelector->currentIndex()).toString();
    if (!cuesheetFilename.isNull())
    {
        // this needs to be done BEFORE the actual write, because the reload will cause a bogus "are you sure" message
        ui->pushButtonCueSheetEditSave->hide();   // the two save buttons are now invisible
        ui->pushButtonCueSheetEditSaveAs->hide();
        ui->pushButtonEditLyrics->show();  // and the "unlock for editing" button shows up!
        ui->actionSave->setEnabled(false);  // save is disabled to start out
        ui->actionSave_As->setEnabled(false);  // save as... is also disabled at the start

        filewatcherShouldIgnoreOneFileSave = true;  // set flag
        writeCuesheet(cuesheetFilename);            // this will NOT trigger FileWatcher (one time)

        loadCuesheets(currentMP3filenameWithPath, cuesheetFilename);
        saveCurrentSongSettings();
    }
    setInOutButtonState();

}


void MainWindow::saveLyricsAs()
{
    // Ask me where to save it...
    RecursionGuard dialog_guard(inPreferencesDialog);
    QFileInfo fi(currentMP3filenameWithPath);

    if (lastCuesheetSavePath.isEmpty()) {
        lastCuesheetSavePath = musicRootPath + "/lyrics";
    }

    loadedCuesheetNameWithPath = lastCuesheetSavePath + "/" + fi.baseName() + ".html";

    QString maybeFilename = loadedCuesheetNameWithPath;
    QFileInfo fi2(loadedCuesheetNameWithPath);
    if (fi2.exists()) {
        // choose the next name in the series (this won't be done, if we came from a template)
        QString cuesheetExt = loadedCuesheetNameWithPath.split(".").last();
        QString cuesheetBase = loadedCuesheetNameWithPath
                .replace(QRegularExpression(cuesheetExt + "$"),"")  // remove extension, e.g. ".html"
                .replace(QRegularExpression("[0-9]+\\.$"),"");      // remove .<number>, e.g. ".2"

        // find an appropriate not-already-used filename to save to
        bool done = false;
        int which = 2;  // I suppose we could be smarter than this at some point.
        while (!done) {
            maybeFilename = cuesheetBase + QString::number(which) + "." + cuesheetExt;
            QFileInfo maybeFile(maybeFilename);
            done = !maybeFile.exists();  // keep going until a proposed filename does not exist (don't worry -- it won't spin forever)
            which++;
        }
    }

    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save"), // TODO: this could say Lyrics or Patter
                                                    maybeFilename,
                                                    tr("HTML (*.html *.htm)"));
    if (!filename.isNull())
    {
        // this needs to be done BEFORE the actual write, because the reload will cause a bogus "are you sure" message
        ui->pushButtonCueSheetEditSave->hide();   // the two save buttons are now invisible
        ui->pushButtonCueSheetEditSaveAs->hide();
        ui->pushButtonEditLyrics->show();  // and the "unlock for editing" button shows up!
        ui->actionSave->setEnabled(false);  // save is disabled to start out
        ui->actionSave_As->setEnabled(false);  // save as... is also disabled at the start

        filewatcherShouldIgnoreOneFileSave = true;  // set flag so that Filewatcher is NOT triggered (one time)
        writeCuesheet(filename);            // this will NOT trigger FileWatcher

        loadCuesheets(currentMP3filenameWithPath, filename);
        saveCurrentSongSettings();
    }
}

// Automatically format a cuesheet:
void MainWindow::on_actionAuto_format_Lyrics_triggered()
{
    qDebug() << "AUTO FORMAT CUESHEET";

    SelectionRetainer retainer(ui->textBrowserCueSheet);
    QTextCursor cursor = ui->textBrowserCueSheet->textCursor();

    // now look at it as HTML
    QString selected = cursor.selection().toHtml();
//    qDebug() << "***** STEP 1 *****\n" << selected << "\n***** DONE *****";

    // Qt gives us a whole HTML doc here.  Strip off all the parts we don't want.
    QRegularExpression startSpan("<span.*>", QRegularExpression::InvertedGreedinessOption);  // don't be greedy!

    selected.replace(QRegularExpression("<.*<!--StartFragment-->"),"")
            .replace(QRegularExpression("<!--EndFragment-->.*</html>"),"")
            .replace(startSpan,"")
            .replace("</span>","")
            ;

//    qDebug() << "***** STEP 2 *****\n" << selected << "\n***** DONE *****";

    // SUPER CLEAN -------
//    if (cursor.selectionStart() == 0 && cursor.atEnd()) {
    if (true) {
        // selection starts at beginning of file (position 0), and cursor is at end of file, means everything in the file is selected
        //   if this is the case, let's do a SUPER CLEAN to get rid of all formatting from say MS Word or OpenOffice.
//            qDebug() << "BEFORE CLEANING: '" << selected << "'";
        selected
                .replace(QRegularExpression("<!DOCTYPE.*>"), "") // DOCTYPE needs to be gone, or extra /n at beginning of file will happen
                .replace(QRegularExpression("<HEAD>.*</HEAD>", QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption), "") // delete <head> section
                .replace(QRegularExpression("<BR *>",  QRegularExpression::CaseInsensitiveOption), "|BRBRBR|") // preserve line breaks
                .replace(QRegularExpression("<BR */>", QRegularExpression::CaseInsensitiveOption), "|BRBRBR|")
//                .replace(QRegularExpression("<.*>", QRegularExpression::InvertedGreedinessOption), "") // delete ALL TAGS
                .replace("|BRBRBR|", "<BR>") // restore line breaks
                .replace(QRegularExpression("^(<BR>)+"), "")    // delete extra BR's at the start of the file
                .replace(QRegularExpression("^\n+"), "")        // delete extra NL's at the start of the file
                .replace("\n", "<BR>\n")     // restore line breaks
                ;
//            qDebug() << "SUPER CLEANED: '" << selected << "'";
    }


    // DO THE HARD WORK HERE ---------------
//    qDebug() << "***** STEP 3 *****\n" << selected << "\n***** DONE *****";

    QString headerStart("<span style=\" font-family:'Verdana'; font-size:x-large; color:#ff0002;\">"); // should that 25 be 23?
    QString headerEnd("</span>");

    selected
            .replace(QRegularExpression("<BR>OPENER",       QRegularExpression::CaseInsensitiveOption),   "<BR>" + headerStart + "OPENER" + headerEnd)
            .replace(QRegularExpression("<BR>FIGURE 1",     QRegularExpression::CaseInsensitiveOption),   "<BR>" + headerStart + "FIGURE 1" + headerEnd)
            .replace(QRegularExpression("<BR>FIGURE 2",     QRegularExpression::CaseInsensitiveOption),   "<BR>" + headerStart + "FIGURE 2" + headerEnd)
            .replace(QRegularExpression("<BR>BREAK",        QRegularExpression::CaseInsensitiveOption),   "<BR>" + headerStart + "MIDDLE BREAK" + headerEnd)
            .replace(QRegularExpression("<BR>MIDDLE BREAK", QRegularExpression::CaseInsensitiveOption),   "<BR>" + headerStart + "MIDDLE BREAK" + headerEnd)
            .replace(QRegularExpression("<BR>FIGURE 3",     QRegularExpression::CaseInsensitiveOption),   "<BR>" + headerStart + "FIGURE 3" + headerEnd)
            .replace(QRegularExpression("<BR>FIGURE 4",     QRegularExpression::CaseInsensitiveOption),   "<BR>" + headerStart + "FIGURE 4" + headerEnd)
            .replace(QRegularExpression("<BR>CLOSER",       QRegularExpression::CaseInsensitiveOption),   "<BR>" + headerStart + "CLOSER" + headerEnd)
            .replace(QRegularExpression("<BR>TAG",          QRegularExpression::CaseInsensitiveOption),   "<BR>" + headerStart + "TAG"    + headerEnd)
            .replace(QRegularExpression("HEADS ",           QRegularExpression::CaseInsensitiveOption),   "HEADS ")
            .replace(QRegularExpression("SIDES ",           QRegularExpression::CaseInsensitiveOption),   "SIDES ")
            .replace(QRegularExpression("BOYS ",            QRegularExpression::CaseInsensitiveOption),   "BOYS ")
            .replace(QRegularExpression("GIRLS ",           QRegularExpression::CaseInsensitiveOption),   "GIRLS ")
            .replace(QRegularExpression("<html><body><BR>\n", QRegularExpression::CaseInsensitiveOption), "<html><body>") // get rid of extra NL at start
            ;

//    qDebug() << "***** STEP 4 *****\n" << selected << "\n***** DONE *****";

    // WARNING: this has a dependency on internal cuesheet2.css's definition of BODY text.
    QString HTMLreplacement =
            "<span style=\" font-family:'Verdana'; font-size:large; color:#000000;\">" +
            selected +
            "</span>";

    cursor.beginEditBlock(); // start of grouping for UNDO purposes
        cursor.removeSelectedText();  // remove the rich text...
        cursor.insertHtml(HTMLreplacement);  // ...and put back in the stripped-down text
    cursor.endEditBlock(); // end of grouping for UNDO purposes
}
