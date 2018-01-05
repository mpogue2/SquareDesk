// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:3; fill-column:88 -*-

// SD -- square dance caller's helper.
//
//    Copyright (C) 1990-2005  William B. Ackerman.
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

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#pragma comment(lib, "comdlg32")
#endif

#include "deploy.h"     // This takes the place of the Visual C++-generated resource.h.

enum state_type {
   MAYBE_CHOOSING_DIR,
   SAVING_OLD,
   SAVE_QUERYING,
   JUST_WRITING,
   FINISHED,
   FAILED};

state_type state;

char const *file_list[] = {
   "sd.exe",
   "sdtty.exe",
   "mkcalls.exe",
   "sdlib.dll",
   "sd_calls.txt",
   "sd_calls.dat",
   "SD.lnk",
   "SD plain.lnk",
   "SDTTY.lnk",
   "SDTTY plain.lnk",
   "SD nocheckers.lnk",
   "SD couple.lnk",
   "SD hidecouple.lnk",
   "Edit sd.ini.lnk",
   "Sd manual.lnk",
   "Release Notes.lnk",
   "Faq.lnk",
   "sd_doc.html",
   "relnotes.html",
   "appnote1.html",
   "appnote2.html",
   "appnote3.html",
   "appnote4.html",
   "faq.html",
   "COPYING.txt",
   (char const *) 0};

char const *save_list[] = {
   "sd.exe",
   "sdtty.exe",
   "mkcalls.exe",
   "sdlib.dll",
   "sd_calls.txt",
   "sd_calls.dat",
   (char const *) 0};

char const *shortcut_list[] = {
   "SD.lnk",
   "SD plain.lnk",
   "SDTTY.lnk",
   "SDTTY plain.lnk",
   "SD nocheckers.lnk",
   "SD couple.lnk",
   "SD hidecouple.lnk",
   "Edit sd.ini.lnk",
   "Sd manual.lnk",
   "Release Notes.lnk",
   "Faq.lnk",
   (char const *) 0};

bool InstallDirExists;
char szInstallDir[1000];
char tempstring[1000];
char szCurDir[1000];

void do_install(HWND hwnd)
{
   // This holds random file paths.
   char szStringBuf[500];

   // Copy the required files.

   char const **file_ptr;
   for (file_ptr = file_list ; *file_ptr ; file_ptr++) {
      lstrcpy(szStringBuf, szInstallDir);
      lstrcat(szStringBuf, "\\");
      lstrcat(szStringBuf, *file_ptr);

      if (!CopyFile(*file_ptr, szStringBuf, false)) {
         lstrcpy(szStringBuf, "ERROR!!  Can't copy file  ");
         lstrcat(szStringBuf, *file_ptr);
         lstrcat(szStringBuf, "\nThe installation has failed.");
         SetDlgItemText(hwnd, IDC_MAINCAPTION, szStringBuf);
         ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_HIDE);
         SetDlgItemText(hwnd, IDOK, "Exit");
         state = FAILED;
         return;
      }
   }

   // This holds the Windows directory, e.g. "C:\WINNT"
   char szSysDirBuf[501];

   // This has to hold the full path of the start menu group.
   char szStartMenuBuf[1000];

   // This one has to be able to hold the full pathname of the Explorer
   // and its argument, which is the full path of the start menu group.
   char szSecondBuf[1500];

   unsigned int yyy = GetWindowsDirectory(szSysDirBuf, 500);
   if (yyy > 500 || yyy == 0) {
      SetDlgItemText(hwnd, IDC_MAINCAPTION,
                     "ERROR!!  Can't determine system directory.\n"
                     "The installation has failed.");
      ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_HIDE);
      SetDlgItemText(hwnd, IDOK, "Exit");
      state = FAILED;
      return;
   }

   // Create Sd directory in the "programs" subdirectory of the start menu.
   // First, find the user's Start Menu and Programs subdirectory.  Use
   // the registry.  We used to do this directly from the "USERPROFILE"
   // environment variable, appending "Start Menu\Programs" to it.  That
   // doesn't always work on Windows XP Home.

   HKEY hKey;

   int xxx = RegOpenKeyEx(HKEY_CURRENT_USER,
                          "Software\\Microsoft\\Windows\\CurrentVersion"
                          "\\Explorer\\User Shell Folders",
                          0, KEY_QUERY_VALUE, &hKey);

   if (xxx == ERROR_SUCCESS) {
      unsigned long int buffersize = 999;
      xxx = RegQueryValueEx(hKey, "Programs", 0, 0,
                             (unsigned char *) szSecondBuf, &buffersize);
      RegCloseKey( hKey );
   }

   yyy = 0;

   if (xxx == ERROR_SUCCESS)
      yyy = ExpandEnvironmentStrings(szSecondBuf, szStartMenuBuf, 900);

   if (yyy == 0) {
      // Somehow, it didn't work.  So we fall back on to the old method.
      int zzz = GetEnvironmentVariable("USERPROFILE", szStartMenuBuf, 500);
      if (zzz == 0) {
         // This isn't workstation NT.  Use the system directory.
         lstrcpy(szStartMenuBuf, szSysDirBuf);
      }
      else if (zzz > 500) {
         SetDlgItemText(hwnd, IDC_MAINCAPTION,
                        "ERROR!!  Environment string too large.\n" \
                        "The installation has failed.");
         ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_HIDE);
         SetDlgItemText(hwnd, IDOK, "Exit");
         state = FAILED;
         return;
      }

      lstrcat(szStartMenuBuf, "\\Start Menu\\Programs");
   }

   // We now have the Programs subdirectory.
   lstrcat(szStartMenuBuf, "\\Sd");

   // See if the start folder exists.
   DWORD sd_att = GetFileAttributes(szStartMenuBuf);
   if (sd_att == ~0U || !(sd_att & FILE_ATTRIBUTE_DIRECTORY)) {
      // It doesn't exist -- create it.
      if (!CreateDirectory(szStartMenuBuf, 0)) {
         lstrcpy(szSecondBuf, "ERROR!!  Can't create Start Menu folder\n\n");
         lstrcat(szSecondBuf, szStartMenuBuf);
         lstrcat(szSecondBuf, "\n\nThe installation has failed.");
         SetDlgItemText(hwnd, IDC_MAINCAPTION, szSecondBuf);
         ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_HIDE);
         SetDlgItemText(hwnd, IDOK, "Exit");
         state = FAILED;
         return;
      }
   }

   // Copy the icons to the start menu.

   for (file_ptr = shortcut_list ; *file_ptr ; file_ptr++) {
      lstrcpy(szSecondBuf, szStartMenuBuf);
      lstrcat(szSecondBuf, "\\");
      lstrcat(szSecondBuf, *file_ptr);

      if (!CopyFile(*file_ptr, szSecondBuf, false)) {
         lstrcpy(szStringBuf, "ERROR!!  Can't copy file  ");
         lstrcat(szStringBuf, *file_ptr);
         lstrcat(szStringBuf, "\nThe installation has failed.");
         SetDlgItemText(hwnd, IDC_MAINCAPTION, szStringBuf);
         ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_HIDE);
         SetDlgItemText(hwnd, IDOK, "Exit");
         state = FAILED;
         return;
      }
   }

   // Display same in explorer.

   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   memset(&si, 0, sizeof(STARTUPINFO));
   memset(&pi, 0, sizeof(PROCESS_INFORMATION));
   GetStartupInfo(&si);

   lstrcpy(szSecondBuf, szSysDirBuf);
   lstrcat(szSecondBuf, "\\explorer.exe ");
   lstrcat(szSecondBuf, szStartMenuBuf);
   CreateProcess(0, szSecondBuf, 0, 0, false, 0, 0, 0, &si, &pi);
   // Now pi has pi.hProcess, pi.hThread, dwProcessId, dwThreadId

   SetDlgItemText(hwnd, IDC_MAINCAPTION, "Installation complete.");
   ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_HIDE);
   SetDlgItemText(hwnd, IDOK, "Exit");
   state = FINISHED;
}

void create_and_install(HWND hwnd)
{
   if (!CreateDirectory(szInstallDir, 0)) {
      lstrcpy(tempstring, "ERROR!!  Can't create ");
      lstrcat(tempstring, szInstallDir);
      lstrcat(tempstring, ".\nThe installation has failed.");
      SetDlgItemText(hwnd, IDC_MAINCAPTION, tempstring);
      ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_HIDE);
      SetDlgItemText(hwnd, IDOK, "Exit");
      state = FAILED;
   }
   else {
      do_install(hwnd);
   }
}

void exists_check_and_install(HWND hwnd)
{
   char const **file_ptr;

   for (file_ptr = save_list ; *file_ptr ; file_ptr++) {
      char szFilenameBuf[1000];
      lstrcpy(szFilenameBuf, "C:\\Sd\\");
      lstrcat(szFilenameBuf, *file_ptr);
      if (GetFileAttributes(szFilenameBuf) != ~0U) {
         // We have a pre-existing program.  Try to get the database version.

         char Buffer[200];
         DWORD dwNumRead;

         lstrcpy(szFilenameBuf, "The directory C:\\Sd exists, and has an Sd program.\n\n");

         HANDLE hFile = CreateFile("C:\\Sd\\sd_calls.dat",
                                   GENERIC_READ, FILE_SHARE_READ, 0,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

         if (hFile != INVALID_HANDLE_VALUE &&
             ReadFile(hFile, Buffer, 100, &dwNumRead, 0)) {
            int size = (((WORD) Buffer[8]) << 8) | ((WORD) Buffer[9]);
            Buffer[10+size] = 0;
            Buffer[40] = 0;    // In case of disaster.
            lstrcat(szFilenameBuf, "The database version appears to be ");
            lstrcat(szFilenameBuf, &Buffer[10]);
            lstrcat(szFilenameBuf, ".\n\n");
         }

         CloseHandle(hFile);

         lstrcat(szFilenameBuf,
                 "Press \"Save old version\" to save the existing "
                 "software before loading the new.\n\n");
         lstrcat(szFilenameBuf, "Press \"Overwrite\" to overwrite the existing software.");

         SetDlgItemText(hwnd, IDC_MAINCAPTION, szFilenameBuf);
         SetDlgItemText(hwnd, IDC_BUTTON1, "Save old version");
         ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_SHOW);
         SetFocus(GetDlgItem(hwnd, IDC_BUTTON1));
         SetDlgItemText(hwnd, IDOK, "Overwrite");
         state = SAVE_QUERYING;
         return;
      }
   }

   SetDlgItemText(hwnd, IDC_MAINCAPTION,
                  "The directory C:\\Sd exists, but has no Sd program.\n\n"
                  "Press OK to install Sd and Sdtty there.");
   state = JUST_WRITING;
}



LRESULT CALLBACK MainWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   switch (iMsg) {
   case WM_COMMAND:
      if (wParam == IDOK) {
         switch (state) {
         case FAILED:
         case FINISHED:
            iMsg = WM_CLOSE;
            break;
         case MAYBE_CHOOSING_DIR:
            // User declined to choose the directory.  Use the name in szFilenameBuf.
            if (InstallDirExists)
               exists_check_and_install(hwnd);
            else
               create_and_install(hwnd);
            return 0;
         case SAVE_QUERYING:    // In querying mode, the "OK" button means overwrite.
         case JUST_WRITING:
            do_install(hwnd);
            return 0;
         }
      }
      else if (wParam == IDCANCEL) {
         iMsg = WM_CLOSE;
      }
      else if (wParam == IDC_BUTTON1) {
         if (state == MAYBE_CHOOSING_DIR) {
            // User wants to choose the install directory.

            OPENFILENAME ofn;
            szInstallDir[0] = 0;
            memset(&ofn, 0, sizeof(OPENFILENAME));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.lpstrInitialDir = "";
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = 0;
            ofn.lpstrFile = szInstallDir;
            ofn.nMaxFile = _MAX_PATH;
            ofn.lpstrFileTitle = 0;
            ofn.nMaxFileTitle = 0;
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_CREATEPROMPT | OFN_HIDEREADONLY;
            ofn.lpstrDefExt = "";

            if (!GetSaveFileName(&ofn)) {
               SetDlgItemText(hwnd, IDC_MAINCAPTION, "ERROR!!  Can't get save location.\n"
                              "The installation has failed.");
               ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_HIDE);
               SetDlgItemText(hwnd, IDOK, "Exit");
               state = FAILED;
               return 0;
            }

            // GetSaveFileName set the working directory.  We don't want that.  Set it back.
            SetCurrentDirectory(szCurDir);

            // Now see if this directory exists, and get permission to create.

            lstrcpy(tempstring, "The directory ");
            lstrcat(tempstring, szInstallDir);

            DWORD sd_att = GetFileAttributes(szInstallDir);
            if (sd_att != ~0U && (sd_att & FILE_ATTRIBUTE_DIRECTORY)) {
               InstallDirExists = true;
               lstrcat(tempstring, " exists.\n\n");
               lstrcat(tempstring, "Press OK to install Sd and Sdtty there.");
            }
            else {
               InstallDirExists = false;
               lstrcat(tempstring, " does not exist.\n\n");
               lstrcat(tempstring, "Press OK to create it and install Sd and Sdtty there.");
            }

            state = MAYBE_CHOOSING_DIR;
            SetDlgItemText(hwnd, IDC_MAINCAPTION, tempstring);
            SetDlgItemText(hwnd, IDOK, "OK");
            ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_HIDE);
         }
         else if (state == SAVE_QUERYING) {
            // Put up the dialog box to get the save directory.

            char szSaveDir[1000];
            OPENFILENAME ofn;
            szSaveDir[0] = 0;
            memset(&ofn, 0, sizeof(OPENFILENAME));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.lpstrInitialDir = "C:";
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = 0;
            ofn.lpstrFile = szSaveDir;
            ofn.nMaxFile = _MAX_PATH;
            ofn.lpstrFileTitle = 0;
            ofn.nMaxFileTitle = 0;
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_CREATEPROMPT | OFN_HIDEREADONLY;
            ofn.lpstrDefExt = "";

            if (!GetSaveFileName(&ofn)) {
               SetDlgItemText(hwnd, IDC_MAINCAPTION, "ERROR!!  Can't get save location.\n"
                              "The installation has failed.");
               ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_HIDE);
               SetDlgItemText(hwnd, IDOK, "Exit");
               state = FAILED;
               return 0;
            }

            // GetSaveFileName set the working directory.  We don't want that.  Set it back.
            SetCurrentDirectory(szCurDir);

            if (!CreateDirectory(szSaveDir, 0)) {
               SetDlgItemText(hwnd, IDC_MAINCAPTION,
                              "ERROR!!  Can't create save folder.\n"
                              "The installation has failed.");
               ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_HIDE);
               SetDlgItemText(hwnd, IDOK, "Exit");
               state = FAILED;
               return 0;
            }

            char szFromName[1000];
            char szToName[1000];
            char const **file_ptr;

            for (file_ptr = save_list ; *file_ptr ; file_ptr++) {
               lstrcpy(szFromName, "C:\\Sd\\");
               lstrcat(szFromName, *file_ptr);
               lstrcpy(szToName, szSaveDir);
               lstrcat(szToName, "\\");
               lstrcat(szToName, *file_ptr);
               CopyFile(szFromName, szToName, false);
            }

            do_install(hwnd);
         }

         return 0;
      }
      break;
   case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
   }

   return DefWindowProc(hwnd, iMsg, wParam, lParam);
}



int WINAPI WinMain(
   HINSTANCE hInstance,
   HINSTANCE hPrevInstance,
   PSTR szCmdLine,
   int iCmdShow)
{
   GetCurrentDirectory(999, szCurDir);

   MSG Msg;
   static char szAppName[] = "deploy";
   WNDCLASSEX wndclass;

   // Create and register the class for the main window.

   wndclass.cbSize = sizeof(wndclass);
   wndclass.style = CS_HREDRAW | CS_VREDRAW;
   wndclass.lpfnWndProc = MainWndProc;
   wndclass.cbClsExtra = 0;
   wndclass.cbWndExtra = DLGWINDOWEXTRA;
   wndclass.hInstance = hInstance;
   wndclass.hIcon = LoadIcon(hInstance, szAppName);
   wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground  = (HBRUSH) (COLOR_BTNFACE+1);
   wndclass.lpszMenuName = 0;
   wndclass.lpszClassName = szAppName;
   wndclass.hIconSm = wndclass.hIcon;
   RegisterClassEx(&wndclass);

   HWND hwnd = CreateDialog(hInstance, szAppName, 0, 0);

   ShowWindow(hwnd, iCmdShow);
   SetFocus(GetDlgItem(hwnd, IDOK));

   lstrcpy(szInstallDir, "C:\\Sd");

   DWORD sd_att = GetFileAttributes(szInstallDir);
   if (sd_att != ~0U && (sd_att & FILE_ATTRIBUTE_DIRECTORY)) {
      InstallDirExists = true;
      SetDlgItemText(hwnd, IDC_MAINCAPTION,
                     "The directory C:\\Sd exists.\n\n"
                     "Press OK to install Sd and Sdtty there.\n\n"
                     "Press Choose Directory to install in a different directory.");
   }
   else {
      InstallDirExists = false;
      SetDlgItemText(hwnd, IDC_MAINCAPTION,
                     "The directory C:\\Sd does not exist.\n\n"
                     "Press OK to create C:\\Sd and install Sd and Sdtty there.\n\n"
                     "Press Choose Directory to install in a different directory.");
   }
   SetDlgItemText(hwnd, IDOK, "OK");
   SetDlgItemText(hwnd, IDC_BUTTON1, "Choose Directory");
   ShowWindow(GetDlgItem(hwnd, IDC_BUTTON1), SW_SHOW);
   state = MAYBE_CHOOSING_DIR;

   while (GetMessage(&Msg, NULL, 0, 0)) {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
   }

   return Msg.wParam;
}
