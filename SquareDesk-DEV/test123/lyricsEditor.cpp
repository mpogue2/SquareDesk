#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utility.h"

// START LYRICS EDITOR STUFF

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


void MainWindow::showHTML(QString fromWhere) {
    Q_UNUSED(fromWhere)
    qDebug() << "***** showHTML(" << fromWhere << "):\n";

    QString editedCuesheet = ui->textBrowserCueSheet->toHtml();
    QString tEditedCuesheet = tidyHTML(editedCuesheet);
    QString pEditedCuesheet = postProcessHTMLtoSemanticHTML(tEditedCuesheet);
    qDebug().noquote() << "***** Post-processed HTML will be:\n" << pEditedCuesheet;
}

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

// TODO: can't make a doc from scratch yet.

void MainWindow::on_pushButtonCueSheetClearFormatting_clicked()
{
//    qDebug() << "on_pushButtonCueSheetClearFormatting_clicked";
        SelectionRetainer retainer(ui->textBrowserCueSheet);
        QTextCursor cursor = ui->textBrowserCueSheet->textCursor();

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

//        if (ui->toolButtonEditLyrics->isChecked()) {
//            ui->pushButtonCueSheetEditTitle->setChecked(c == TitleChars);
//            ui->pushButtonCueSheetEditLabel->setChecked(c == LabelChars);
//            ui->pushButtonCueSheetEditArtist->setChecked(c == ArtistChars);
//            ui->pushButtonCueSheetEditHeader->setChecked(c == HeaderChars);
//            ui->pushButtonCueSheetEditLyrics->setChecked(c == LyricsChars);
//        } else {
//            ui->pushButtonCueSheetEditTitle->setChecked(false);
//            ui->pushButtonCueSheetEditLabel->setChecked(false);
//            ui->pushButtonCueSheetEditArtist->setChecked(false);
//            ui->pushButtonCueSheetEditHeader->setChecked(false);
//            ui->pushButtonCueSheetEditLyrics->setChecked(false);
//        }
//    }

    lastKnownTextCharFormat = f;  // save it away for when we unlock editing
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
            QString tEditedCuesheet = tidyHTML(editedCuesheet);
            QString postProcessedCuesheet = postProcessHTMLtoSemanticHTML(tEditedCuesheet);
            stream << postProcessedCuesheet;
            stream.flush();
        }

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
    //                qDebug().noquote() << "***** editedCuesheet to write:\n" << editedCuesheet;

                QString tEditedCuesheet = tidyHTML(editedCuesheet);
    //                qDebug().noquote() << "***** tidied editedCuesheet to write:\n" << tEditedCuesheet;

                QString postProcessedCuesheet = postProcessHTMLtoSemanticHTML(tEditedCuesheet);
//                qDebug().noquote() << "***** I AM THINKING ABOUT WRITING TO FILE postProcessed:\n" << postProcessedCuesheet;
#endif
}

void MainWindow::on_pushButtonCueSheetEditSave_clicked()
{
    QTextCursor tc = ui->textBrowserCueSheet->textCursor();  // save text cursor
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

    ui->textBrowserCueSheet->setTextCursor(tc); // Reset the cursor after a save
}

void MainWindow::on_pushButtonCueSheetEditSaveAs_clicked()
{
    QTextCursor tc = ui->textBrowserCueSheet->textCursor();  // save text cursor

    //    qDebug() << "on_pushButtonCueSheetEditSaveAs_clicked";
    saveLyricsAs();  // we probably want to save with a different name, so unlike "Save", this is always called here.

    ui->pushButtonCueSheetEditSave->hide();   // the two save buttons are now invisible
    ui->pushButtonCueSheetEditSaveAs->hide();
    ui->pushButtonEditLyrics->show();  // and the "unlock for editing" button is now visible
    ui->actionSave->setEnabled(false);  // save is disabled now
    ui->actionSave_As->setEnabled(false);  // save as... is also disabled now
    setInOutButtonState();

    ui->textBrowserCueSheet->setTextCursor(tc); // Reset the cursor after a save
}


QString MainWindow::tidyHTML(QString cuesheet) {
#define NOTIDYYET
#ifdef NOTIDYYET
    QString cuesheet_tidied = cuesheet; // FIX
#else
    qDebug() << "tidyHTML";
//    qDebug().noquote() << "************\ncuesheet in:" << cuesheet;

    // first get rid of <L> and </L>.  Those are ILLEGAL.
    cuesheet.replace("<L>","",Qt::CaseInsensitive).replace("</L>","",Qt::CaseInsensitive);

    // then get rid of <NOBR> and </NOBR>, NOT SUPPORTED BY W3C.
    cuesheet.replace("<NOBR>","",Qt::CaseInsensitive).replace("</NOBR>","",Qt::CaseInsensitive);

    // and &nbsp; too...let the layout engine do its thing.
    cuesheet.replace("&nbsp;"," ");

    // convert to a c_string, for HTML-TIDY
    char* tidyInput;
    string csheet = cuesheet.toStdString();
    tidyInput = new char [csheet.size()+1];
    strcpy( tidyInput, csheet.c_str() );

////    qDebug().noquote() << "\n***** TidyInput:\n" << QString((char *)tidyInput);

    TidyBuffer output;// = {0};
    TidyBuffer errbuf;// = {0};
    tidyBufInit(&output);
    tidyBufInit(&errbuf);
    int rc = -1;
    Bool ok;

    // TODO: error handling here...using GOTO!

    TidyDoc tdoc = tidyCreate();
    ok = tidyOptSetBool( tdoc, TidyHtmlOut, yes );  // Convert to XHTML
    if (ok) {
        ok = tidyOptSetBool( tdoc, TidyUpperCaseTags, yes );  // span -> SPAN
    }
//    if (ok) {
//        ok = tidyOptSetInt( tdoc, TidyUpperCaseAttrs, TidyUppercaseYes );  // href -> HREF
//    }
    if (ok) {
        ok = tidyOptSetBool( tdoc, TidyDropEmptyElems, yes );  // Discard empty elements
    }
    if (ok) {
        ok = tidyOptSetBool( tdoc, TidyDropEmptyParas, yes );  // Discard empty p elements
    }
    if (ok) {
        ok = tidyOptSetInt( tdoc, TidyIndentContent, TidyYesState );  // text/block level content indentation
    }
    if (ok) {
        ok = tidyOptSetInt( tdoc, TidyWrapLen, 150 );  // text/block level content indentation
    }
    if (ok) {
        ok = tidyOptSetBool( tdoc, TidyMark, no);  // do not add meta element indicating tidied doc
    }
    if (ok) {
        ok = tidyOptSetBool( tdoc, TidyLowerLiterals, yes);  // Folds known attribute values to lower case
    }
    if (ok) {
        ok = tidyOptSetInt( tdoc, TidySortAttributes, TidySortAttrAlpha);  // Sort attributes
    }
    if ( ok )
        rc = tidySetErrorBuffer( tdoc, &errbuf );      // Capture diagnostics
    if ( rc >= 0 )
        rc = tidyParseString( tdoc, tidyInput );           // Parse the input
    if ( rc >= 0 )
        rc = tidyCleanAndRepair( tdoc );               // Tidy it up!
    if ( rc >= 0 )
        rc = tidyRunDiagnostics( tdoc );               // Kvetch
    if ( rc > 1 )                                    // If error, force output.
        rc = ( tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1 );
    if ( rc >= 0 )
        rc = tidySaveBuffer( tdoc, &output );          // Pretty Print

    QString cuesheet_tidied;
    if ( rc >= 0 )
    {
        if ( rc > 0 ) {
//            qDebug().noquote() << "\n***** Diagnostics:" << QString((char*)errbuf.bp);
//            qDebug().noquote() << "\n***** TidyOutput:\n" << QString((char*)output.bp);
        }
        cuesheet_tidied = QString(reinterpret_cast<char *>(output.bp));
    }
    else {
        qDebug() << "***** Severe error:" << rc;
    }

    tidyBufFree( &output );
    tidyBufFree( &errbuf );
    tidyRelease( tdoc );

//    // get rid of TIDY cruft
////    cuesheet_tidied.replace("<META NAME=\"generator\" CONTENT=\"HTML Tidy for HTML5 for Mac OS X version 5.5.31\">","");

//    qDebug().noquote() << "************\ncuesheet out:" << cuesheet_tidied;
#endif

    return(cuesheet_tidied);
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
    QString cuesheet3 = tidyHTML(cuesheet);

    // now the semantic replacement.
    // assumes that QTextEdit spits out spans in a consistent way
    // TODO: allow embedded NL (due to line wrapping)
    // NOTE: background color is optional here, because I got rid of the the spec for it in BODY
    // <SPAN style="font-family:'Verdana'; font-size:x-large; color:#ff0002; background-color:#ffffe0;">
    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:x-large; color:#ff0002;[\\s\n]*(background-color:#ffffe0;)*\">"),
                             "<SPAN class=\"hdr\">");
    // <SPAN style="font-family:'Verdana'; font-size:large; color:#030303; background-color:#ffc0cb;">
    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:large; color:#030303; background-color:#ffc0cb;\">"),  // background-color required for lyrics
                             "<SPAN class=\"lyrics\">");
    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:medium; color:#60c060;[\\s\n]*(background-color:#ffffe0;)*\">"),
                             "<SPAN class=\"label\">");
    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:medium; color:#0000ff;[\\s\n]*(background-color:#ffffe0;)*\">"),
                             "<SPAN class=\"artist\">");
    // <SPAN style="font-family:'Arial Black'; font-size:x-large; color:#010101; background-color:#ffffe0;">
    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Arial Black'; font-size:x-large;[\\s\n]*(font-weight:600;)*[\\s\n]*color:#010101;[\\s\n]*(background-color:#ffffe0;)*\">"),
                             "<SPAN class=\"title\">");
    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:large;[\\s\n]*font-weight:600;[\\s\n]*color:#000000;[\\s\n]*(background-color:#ffffe0;)*\">"),
                                     "<SPAN style=\"font-weight: Bold;\">");
    // <SPAN style="font-family:'Verdana'; font-size:large; font-style:italic; color:#000000;">
    cuesheet3.replace(QRegularExpression("<SPAN style=[\\s\n]*\"font-family:'Verdana'; font-size:large;[\\s\n]*font-style:italic;[\\s\n]*color:#000000;[\\s\n]*(background-color:#ffffe0;)*\">"),
                                     "<SPAN style=\"font-style: Italic;\">");

    cuesheet3.replace("<P style=\"\">","<P>");
    cuesheet3.replace("<P style=\"background-color:#ffffe0;\">","<P>");  // background color already defaults via the BODY statement
    cuesheet3.replace("<BODY bgcolor=\"#FFFFE0\" style=\"font-family:'.SF NS Text'; font-size:13pt; font-weight:400; font-style:normal;\">","<BODY>");  // must go back to USER'S choices in cuesheet2.css

    // now replace <SPAN bold...>foo</SPAN> --> <B>foo</B>
    QRegularExpression boldRegExp("<SPAN style=\"font-weight: Bold;\">(.*)</SPAN>", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption);
//    boldRegExp.setMinimal(true);
//    boldRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    cuesheet3.replace(boldRegExp,"<B>\\1</B>");  // don't be greedy, and replace <SPAN bold> -> <B>

    // now do the same for Italic...
    // "<SPAN style=\"font-style: Italic;\">"
    QRegularExpression italicRegExp("<SPAN style=\"font-style: Italic;\">(.*)</SPAN>", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption);
//    italicRegExp.setMinimal(true);
//    italicRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    cuesheet3.replace(italicRegExp,"<I>\\1</I>");  // don't be greedy, and replace <SPAN italic> -> <B>


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

    // put the <link rel="STYLESHEET" type="text/css" href="cuesheet2.css"> back in
    if (!cuesheet3.contains("<link",Qt::CaseInsensitive)) {
//        qDebug() << "Putting the <LINK> back in...";
        cuesheet3.replace("</TITLE>","</TITLE><LINK rel=\"STYLESHEET\" type=\"text/css\" href=\"cuesheet2.css\">");
    }
    // tidy it one final time before writing it to a file (gets rid of blank SPAN's, among other things)
    QString cuesheet4 = tidyHTML(cuesheet3);
//    qDebug().noquote() << "***** postProcess 2: " << cuesheet4;
    return(cuesheet4);
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
    QString css = "/* For correct operation of the Lyrics editor, the colors must be distinct from each other */\n"
                  "body, p, font { font-size: large;   font-weight: Normal; color: #000000; line-height: 100%; background: #FFFFE0; }\n\n"
                  ".title        { font-family: Arial Black; font-size: x-large; font-weight: Normal; color: #010101;}\n"
                  ".label        { font-size: medium;  font-weight: Normal; color: #60C060;}\n"
                  ".artist       { font-size: medium;  font-weight: Normal; color: #0000FF;}\n"
                  ".hdr          { font-size: x-large; font-weight: Normal; color: #FF0002;}\n"
                  ".lyrics       { font-size: large;   font-weight: Normal; color: #030303; background-color: #FFC0CB;}\n";
    ui->textBrowserCueSheet->document()->setDefaultStyleSheet(css);
}

void MainWindow::loadCuesheet(const QString &cuesheetFilename)
{
    // THIS IS NOT QUITE RIGHT YET.  NORMAL SAVE TRIGGERS IT.  But, I'm leaving it here as
    //   a partial implementation for later.
//    if ( ui->pushButtonEditLyrics->isChecked()
//            // && ui->textBrowserCueSheet->document()->isModified()  // TODO: Hmmm...seems like it's always modified...
//       ) {
//        qDebug() << "YOU HAVE A CUESHEET OPEN FOR EDITING. ARE YOU SURE YOU WANT TO SWITCH TO A DIFFERENT CUESHEET (changes will be lost)?";
//    }

    loadedCuesheetNameWithPath = ""; // nothing loaded yet

    QUrl cuesheetUrl(QUrl::fromLocalFile(cuesheetFilename));  // NOTE: can contain HTML that references a customer's cuesheet2.css
    if (cuesheetFilename.endsWith(".txt", Qt::CaseInsensitive)) {
        // text files are read in, converted to HTML, and sent to the Lyrics tab
        QFile f1(cuesheetFilename);
        f1.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream in(&f1);
        QString html = txtToHTMLlyrics(in.readAll(), cuesheetFilename);
        ui->textBrowserCueSheet->setText(html);
        loadedCuesheetNameWithPath = cuesheetFilename;
        f1.close();
    }
    else if (cuesheetFilename.endsWith(".mp3", Qt::CaseInsensitive)) {
//        qDebug() << "loadCuesheet():";
        QString embeddedID3Lyrics = loadLyrics(cuesheetFilename);
//        qDebug() << "embLyrics:" << embeddedID3Lyrics;
        if (embeddedID3Lyrics != "") {
            QString HTMLlyrics = txtToHTMLlyrics(embeddedID3Lyrics, cuesheetFilename);
//            qDebug() << "HTML:" << HTMLlyrics;
            QString html(HTMLlyrics);  // embed CSS, if found, since USLT is plain text
            ui->textBrowserCueSheet->setHtml(html);
            loadedCuesheetNameWithPath = cuesheetFilename;
        }
    } else {

        // read in the HTML for the cuesheet

        QFile f1(cuesheetFilename);
        QString cuesheet;
        if ( f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f1);
            cuesheet = in.readAll();  // read the entire CSS file, if it exists

            // FIX: Qt6.2 does not support QTextCodec anymore.  Commenting out for now.
//            if (cuesheet.contains("charset=windows-1252") || cuesheetFilename.contains("GP 956")) {  // WARNING: HACK HERE
//                // this is very likely to be an HTML file converted from MS WORD,
//                //   and it still uses windows-1252 encoding.

//                f1.seek(0);  // go back to the beginning of the file

//                QByteArray win1252bytes(f1.readAll());  // and read it again (as bytes this time)
//                QTextCodec *codec = QTextCodec::codecForName("windows-1252");  // FROM win-1252 bytes
//                cuesheet = codec->toUnicode(win1252bytes);                     // TO Unicode QString
//            }

//            qDebug() << "Cuesheet: " << cuesheet;
            cuesheet.replace("\xB4","'");  // replace wacky apostrophe, which doesn't display well in QEditText
            // NOTE: o-umlaut is already translated (incorrectly) here to \xB4, too.  There's not much we
            //   can do with non UTF-8 HTML files that aren't otherwise marked as to encoding.

//#if defined(Q_OS_MAC) || defined(Q_OS_LINUX) || defined(Q_OS_WIN)
            // HTML-TIDY IT ON INPUT *********
            QString cuesheet_tidied = tidyHTML(cuesheet);
//#else
//            QString cuesheet_tidied = cuesheet;  // LINUX, WINDOWS
//#endif

            // ----------------------
            // set the HTML for the cuesheet itself (must set CSS first)
//            ui->textBrowserCueSheet->setHtml(cuesheet);
//            qDebug() << "tidied: " << cuesheet_tidied;
            ui->textBrowserCueSheet->setHtml(cuesheet_tidied);
            loadedCuesheetNameWithPath = cuesheetFilename;
            f1.close();
//            showHTML(__FUNCTION__);  // DEBUG DEBUG DEBUG
        }

    }
    ui->textBrowserCueSheet->document()->setModified(false);

    // -----------
    QTextCursor cursor = ui->textBrowserCueSheet->textCursor();     // cursor for this document
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor); // select entire document

    QTextBlockFormat fmt;       // = cursor.blockFormat(); // get format of current block
    fmt.setTopMargin(0.0);
    fmt.setBottomMargin(0.0);   // modify it

    cursor.mergeBlockFormat(fmt); // set margins to zero for all blocks

    cursor.movePosition(QTextCursor::Start);  // move cursor back to the start of the document

    ui->pushButtonEditLyrics->setChecked(false);  // locked for editing, in case this is just a change in the dropdown

    ui->pushButtonCueSheetEditSave->hide();   // the two save buttons are now invisible
    ui->pushButtonCueSheetEditSaveAs->hide();
    ui->pushButtonEditLyrics->show();  // and the "unlock for editing" button shows up!
    ui->actionSave->setEnabled(false);  // save is disabled to start out
    ui->actionSave_As->setEnabled(false);  // save as... is also disabled at the start
    setInOutButtonState();
}

// END LYRICS EDITOR STUFF
