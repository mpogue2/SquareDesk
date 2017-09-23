// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2004  William B. Ackerman.
//    Copyright (C) 1996 Charles Petzold.
//
//    This file is part of "Sd".
//
//    Sd is free software; you can redistribute it and/or modify it
//    under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    Sd is distributed in the hope that it will be useful, but WITHOUT
//    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
//    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
//    License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Sd; if not, write to the Free Software Foundation, Inc.,
//    59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//    This is for version 36.


// This file is used in print utilities other than Sd.

/* This defines the following functions:
   windows_init_printer
   windows_choose_font
   windows_print_this
   windows_print_any
*/

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdlib.h>
#include <string.h>

// Note that we do *NOT* include resource.h here.  The client will pass
// the necessary items from resource.h in the "print_default_info" structure.

#include "sdprint.h"


#if defined(_MSC_VER)
#pragma comment(lib, "comctl32")
#endif



struct printer_innards {
   HINSTANCE hInstance;
   HWND hwnd;
   int IDD_PRINTING_DIALOG;
   int IDC_FILENAME;
   LOGFONT lf;
   CHOOSEFONT cf;
   DOCINFO di;
   PRINTDLG pd;
   char szPrintDir[_MAX_PATH];
   OPENFILENAME ofn;
};



static HWND hDlgPrint;
static BOOL bUserAbort;

printer::printer(HINSTANCE hInstance, HWND hwnd, const print_default_info & info)
{
   innards = new printer_innards;
   ZeroMemory(innards, sizeof(printer_innards));

   innards->hwnd = hwnd;
   innards->hInstance = hInstance;
   innards->IDD_PRINTING_DIALOG = info.IDD_PRINTING_DIALOG;
   innards->IDC_FILENAME = info.IDC_FILENAME;

   // Initialize the default file directory and type.
   innards->ofn.lStructSize = sizeof(OPENFILENAME);
   innards->ofn.lpstrFilter = info.filter;
   innards->ofn.lpstrDefExt = "txt";

   // We need to figure out the "logical size" that will give a 14 point
   // font.  And we haven't opened the printer, so we have to do it in terms
   // of the display.  The LOGPIXELSY number for the display will give "logical
   // units" (whatever they are -- approximately pixels, but it doesn't matter)
   // per inch.  A point is 1/72nd of an inch, so we do a little math.
   // Later, when the user chooses a font, the system will do its calculations
   // based on the display (we still won't have opened the printer, so we
   // just let the system think we're choosing a display font instead of a
   // printer font.)

   innards->cf.lStructSize = sizeof(CHOOSEFONT);
   innards->cf.hwndOwner = innards->hwnd;
   innards->cf.lpLogFont = &innards->lf;
   innards->cf.Flags = CF_NOVECTORFONTS | CF_BOTH | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS;

   // Here is where we set the initial default font and point size.
   lstrcpy(innards->lf.lfFaceName, info.font);
   set_point_size(info.pointsize);
   innards->lf.lfWeight = info.bold ? FW_BOLD : FW_NORMAL;
   innards->lf.lfItalic = info.italic;
}

printer::~printer()
{
   delete innards;
}


void printer::set_point_size(int size)
{
   innards->cf.iPointSize = size * 10;
   innards->lf.lfHeight = -((innards->cf.iPointSize*GetDeviceCaps(GetDC(innards->hwnd), LOGPIXELSY)+360)/720);

   // At all times, "lf" has the data for the current font (unfortunately,
   // calibrated for the display) and "cf.iPointSize" has the actual point
   // size times 10.  All manipulations during the "choose font" operations
   // will be in terms of this.  When it comes time to print, we will create
   // a new font for the display.  All the "lf" stuff will be incorrect for
   // it, and we will have to recompute the parameters of the logical font
   // based on the printer calibration.  We will use the field "cf.iPointSize"
   // (the only thing that is invariant) to recompute it.
}

void printer::choose_font()
{
   // This operation will take place in the context of the display
   // rather than the printer, but we have to do it that way, because
   // we don't want to open the printer just yet.  The problem that this
   // creates is that the "lfHeight" size will be calibrated wrong.

   LOGFONT lfsave = innards->lf;
   CHOOSEFONT cfsave = innards->cf;

   if (!ChooseFont(&innards->cf)) {
      // Windows is occasionally lacking in common sense.
      // It modifies the "cf" and "lf" structures as we
      // make selections, even if we later cancel the whole thing.
      innards->lf = lfsave;
      innards->cf = cfsave;
   }

   // Now "lf" has all the info, though it is, unfortunately, calibrated
   // for the display.  Also, "cf.iPointSize" has the point size times 10,
   // which is, fortunately, invariant.
}


BOOL CALLBACK PrintDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG:
      EnableMenuItem(GetSystemMenu(hDlg, FALSE), SC_CLOSE, MF_GRAYED);
      return TRUE;
   case WM_COMMAND:
      bUserAbort = TRUE;
      EnableWindow (GetParent(hDlg), TRUE);
      DestroyWindow(hDlg);
      hDlgPrint = NULL;
      return TRUE;
   }
   return FALSE;
}

BOOL CALLBACK PrintAbortProc(HDC hPrinterDC, int iCode)
{
   MSG msg;

   while (!bUserAbort && PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
      if (!hDlgPrint || !IsDialogMessage (hDlgPrint, &msg)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }
   return !bUserAbort;
}


// We make this really large.  Memory is cheap.  Bugs aren't.
#define PRINT_LINE_LENGTH 500


void printer::print_this(const char *szFileName, char *szMainTitle, bool pagenums)
{
   BOOL            bSuccess;
   TEXTMETRIC      tm;

   // Invoke Print common dialog box

   ZeroMemory(&innards->pd, sizeof(PRINTDLG));   // Don't keep selections around.
   innards->pd.lStructSize = sizeof(PRINTDLG);
   innards->pd.hwndOwner = innards->hwnd;
   innards->pd.Flags = PD_ALLPAGES | PD_NOPAGENUMS | PD_RETURNDC |
      PD_NOSELECTION | PD_DISABLEPRINTTOFILE | PD_HIDEPRINTTOFILE;
   innards->pd.nCopies = 1;

   if (!PrintDlg(&innards->pd)) return;

   HDC hdcPrn = innards->pd.hDC;

   // Now the "logical font" structure "lf" has the selected font, except for
   // one problem -- its "lfHeight" field is calibrated for the display.
   // We need to recalibrate it for the printer.  So we go through the same
   // procedure as at initialization, this time using the printer device context.
   // We get the font size, in invariant form (10*point size) from "cf.iPointSize".

   LOGFONT printerlf = innards->lf;   // All other fields are good.
   printerlf.lfHeight = -((innards->cf.iPointSize*GetDeviceCaps(hdcPrn, LOGPIXELSY)+360)/720);
   SelectObject(hdcPrn, CreateFontIndirect(&printerlf));

   // Font is now ready.  Find out how big it is.

   GetTextMetrics(hdcPrn, &tm);
   int iPixelLineHeight = tm.tmHeight + tm.tmExternalLeading;

   int iPixelLeftOfPage = tm.tmAveCharWidth*2-GetDeviceCaps(hdcPrn, PHYSICALOFFSETX);

   // This is where we choose to start printing.  If it were zero,
   // we would start right at the top (less whatever margin the system
   // provides.)  We set it to iPixelLineHeight to give us an effective
   // blank line at the top of the page, taking into account the page margin
   // for common printers, which seems to be 108.  That seems to look right.
   int iPixelTopOfPage = iPixelLineHeight + 108 - GetDeviceCaps(hdcPrn, PHYSICALOFFSETY );

   // If printer can't print as close to the edge of the page as we would like, do the best we can.
   if (iPixelLeftOfPage < 0) iPixelLeftOfPage = 0;
   if (iPixelTopOfPage < 0) iPixelTopOfPage = 0;

   // This is where we choose to stop printing.  It must not be greater than
   // "GetDeviceCaps(hdcPrn, VERTRES)-iPixelLineHeight", that is, we must subtract
   // iPixelLineHeight.  If we set it to exactly that value, we print to the bottom
   // of the page.  So we subtract twice that value to get a reasonable
   // bottom margin.
   int iPixelBottomOfPage = GetDeviceCaps(hdcPrn, VERTRES)-iPixelLineHeight*2;

   // If doing page numbers, cut out another two lines at the bottom.
   if (pagenums) iPixelBottomOfPage -= iPixelLineHeight*2;

   // Display the printing dialog box

   EnableWindow(innards->hwnd, FALSE);

   bSuccess = TRUE;
   bUserAbort = FALSE;

   hDlgPrint = CreateDialog(innards->hInstance,
                            MAKEINTRESOURCE(innards->IDD_PRINTING_DIALOG),
                            innards->hwnd, (DLGPROC) PrintDlgProc);

   SetDlgItemText(hDlgPrint, innards->IDC_FILENAME, szFileName);
   SetAbortProc(hdcPrn, PrintAbortProc);

   ZeroMemory(&innards->di, sizeof(DOCINFO));
   innards->di.cbSize = sizeof(DOCINFO);
   innards->di.lpszDocName = szMainTitle;

   FILE *fildes = fopen(szFileName, "r");
   if (!fildes) {
      bSuccess = FALSE;
      goto print_failed;
   }

   fpos_t fposStart;

   if (fgetpos(fildes, &fposStart)) {
      bSuccess = FALSE;
      goto print_failed;
   }

   // Start the printer

   if (StartDoc (hdcPrn, &innards->di) > 0) {

      int iNumCopiesOfFile = (innards->pd.Flags & PD_COLLATE) ? innards->pd.nCopies : 1;
      int iNumCopiesOfEachPage = (innards->pd.Flags & PD_COLLATE) ? 1 : innards->pd.nCopies;

      // Scan across the required number of complete copies of the file.
      // If not collating, this loop cycles only once.

      for (int iFileCopies = 0 ; iFileCopies < iNumCopiesOfFile ; iFileCopies++) {

         if (fsetpos(fildes, &fposStart)) {   // Seek to beginning.
            bSuccess = FALSE;
            break;
         }

         int pagenum = 1;

         // Scan the file.

         for ( BOOL bEOF = false ; !bEOF ; ) {
            fpos_t fposPageStart;

            // Save the current place so that we can do manual
            // uncollated copies of the page if necessary.

            if (fgetpos(fildes, &fposPageStart)) {
               bSuccess = FALSE;
               goto print_failed;
            }

            // Scan across the number of times we need to print this page.
            // If printing multiple copies and not collating, this loop
            // will cycle multiple times, printing the same page repeatedly.
            for (int iPageCopies = 0 ; iPageCopies < iNumCopiesOfEachPage; iPageCopies++) {

               // Go back to our saved position.

               if (fsetpos(fildes, &fposPageStart)) {
                  bSuccess = FALSE;
                  goto print_failed;
               }

               // Don't open the page unless there is actually a line of text to print.
               // That way, we don't embarrass ourselves by printing a blank page if
               // we see a formfeed after an exact integral number of sheets of paper.

               bool bPageIsOpen = false;
               char pstrBuffer[PRINT_LINE_LENGTH+1];

               for (int iRasterPos = iPixelTopOfPage;
                    iRasterPos <= iPixelBottomOfPage;
                    iRasterPos += iPixelLineHeight) {

                  fpos_t fposLineStart;
                  if (fgetpos(fildes, &fposLineStart))
                     break;

                  if (!fgets(pstrBuffer, PRINT_LINE_LENGTH, fildes)) {
                     bEOF = true;
                     break;
                  }

                  // Strip off any <NEWLINE> -- we don't want it.  Note that, since
                  // we are using a POSIX call to read the file, we get POSIX-style
                  // line breaks -- just '\n'.

                  int j = strlen(pstrBuffer);
                  if (j>0 && pstrBuffer[j-1] == '\n')
                     pstrBuffer[j-1] = '\0';

                  // If we get a form-feed, break out of the loop, but reset
                  // the file location to just after the form-feed -- leave the
                  // rest of the line.

                  if (pstrBuffer[0] == '\f') {
                     (void) fsetpos(fildes, &fposLineStart);
                     (void) fgetc(fildes);   // pass over the form-feed.
                     break;
                  }

                  if (!bPageIsOpen) {
                     if (StartPage(hdcPrn) < 0) {
                        bSuccess = FALSE;
                        goto print_failed;
                     }
                     bPageIsOpen = true;
                  }

                  TextOut (hdcPrn, iPixelLeftOfPage, iRasterPos, pstrBuffer, strlen(pstrBuffer));
               }

               // If doing page numbers, we have two more lines of space at the bottom.

               if (pagenums && bPageIsOpen) {
                  wsprintf(pstrBuffer, "                   %s   Page %d", szFileName, pagenum);
                  TextOut (hdcPrn, iPixelLeftOfPage, iPixelBottomOfPage + 2*iPixelLineHeight, pstrBuffer, strlen(pstrBuffer));
               }

               if (bPageIsOpen) {
                  if (EndPage (hdcPrn) < 0) {
                     bSuccess = FALSE;
                     goto print_failed;
                  }
               }

               if (bUserAbort)
                  break;
            }

            if (!bSuccess || bUserAbort)
               break;

            pagenum++;
         }

         if (!bSuccess || bUserAbort)
            break;
      }
   }
   else
      bSuccess = FALSE;

   if (bSuccess)
      EndDoc(hdcPrn);

 print_failed:

   if (!bUserAbort) {
      EnableWindow(innards->hwnd, TRUE);
      DestroyWindow(hDlgPrint);
   }

   if (fildes) (void) fclose(fildes);
   DeleteDC(hdcPrn);

   if (!(bSuccess && !bUserAbort)) {
     char szBuffer[MAX_PATH + 64];
     wsprintf(szBuffer, "Could not print file %s", szFileName);
     MessageBox(innards->hwnd, szBuffer, "Error", MB_OK | MB_ICONEXCLAMATION);
   }
}



void printer::print_any(char *szMainTitle, bool pagenums)
{
   char szCurDir[_MAX_PATH];
   char szFileToPrint[_MAX_PATH];

   (void) GetCurrentDirectory(_MAX_PATH, szCurDir);

   // Put up the dialog box to get the file to print.

   // If we have a directory saved from an earlier command, use that as the default.
   // If not, use the default directory chosen by the system.
   if (innards->szPrintDir[0])
      innards->ofn.lpstrInitialDir = innards->szPrintDir;
   else
      innards->ofn.lpstrInitialDir = "";
   innards->ofn.hwndOwner = innards->hwnd;
   szFileToPrint[0] = 0;
   innards->ofn.lpstrFile = szFileToPrint;
   innards->ofn.nMaxFile = _MAX_PATH;
   innards->ofn.lpstrFileTitle = 0;
   innards->ofn.nMaxFileTitle = 0;
   innards->ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

   if (!GetOpenFileName(&innards->ofn))
      return;     // Take no action if we didn't get a file name.

   // GetOpenFileName changed the working directory.
   // We don't want that.  Set it back, after saving it for next print command.
   (void) GetCurrentDirectory(_MAX_PATH, innards->szPrintDir);
   (void) SetCurrentDirectory(szCurDir);

   print_this(szFileToPrint, szMainTitle, pagenums);
}
