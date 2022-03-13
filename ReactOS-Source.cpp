 /*
  * @implemented
  */
 HWND
 WINAPI
 DECLSPEC_HOTPATCH
 CreateWindowExW(DWORD dwExStyle,
                 LPCWSTR lpClassName,
                 LPCWSTR lpWindowName,
                 DWORD dwStyle,
                 int x,
                 int y,
                 int nWidth,
                 int nHeight,
                 HWND hWndParent,
                 HMENU hMenu,
                 HINSTANCE hInstance,
                 LPVOID lpParam)
 {
     MDICREATESTRUCTW mdi;
     HWND hwnd;
 
     if (!RegisterDefaultClasses)
     {
        ERR("CreateWindowExW RegisterSystemControls\n");
        RegisterSystemControls();
     }
 
     if (dwExStyle & WS_EX_MDICHILD)
     {
         POINT mPos[2];
         UINT id = 0;
         HWND top_child;
         PWND pWndParent;
 
         pWndParent = ValidateHwnd(hWndParent);
 
         if (!pWndParent) return NULL;
 
         if (pWndParent->fnid != FNID_MDICLIENT)
         {
            WARN("WS_EX_MDICHILD, but parent %p is not MDIClient\n", hWndParent);
            goto skip_mdi;
         }
 
         /* lpParams of WM_[NC]CREATE is different for MDI children.
         * MDICREATESTRUCT members have the originally passed values.
         */
         mdi.szClass = lpClassName;
         mdi.szTitle = lpWindowName;
         mdi.hOwner = hInstance;
         mdi.x = x;
         mdi.y = y;
         mdi.cx = nWidth;
         mdi.cy = nHeight;
         mdi.style = dwStyle;
         mdi.lParam = (LPARAM)lpParam;
 
         lpParam = (LPVOID)&mdi;
 
         if (pWndParent->style & MDIS_ALLCHILDSTYLES)
         {
             if (dwStyle & WS_POPUP)
             {
                 WARN("WS_POPUP with MDIS_ALLCHILDSTYLES is not allowed\n");
                 return(0);
             }
             dwStyle |= (WS_CHILD | WS_CLIPSIBLINGS);
         }
         else
         {
             dwStyle &= ~WS_POPUP;
             dwStyle |= (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION |
                 WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
         }
 
         top_child = GetWindow(hWndParent, GW_CHILD);
 
         if (top_child)
         {
             /* Restore current maximized child */
             if((dwStyle & WS_VISIBLE) && IsZoomed(top_child))
             {
                 TRACE("Restoring current maximized child %p\n", top_child);
                 SendMessageW( top_child, WM_SETREDRAW, FALSE, 0 );
                 ShowWindow(top_child, SW_RESTORE);
                 SendMessageW( top_child, WM_SETREDRAW, TRUE, 0 );
             }
         }
 
         MDI_CalcDefaultChildPos(hWndParent, -1, mPos, 0, &id);
 
         if (!(dwStyle & WS_POPUP)) hMenu = UlongToHandle(id);
 
         if (dwStyle & (WS_CHILD | WS_POPUP))
         {
             if (x == CW_USEDEFAULT || x == CW_USEDEFAULT16)
             {
                 x = mPos[0].x;
                 y = mPos[0].y;
             }
             if (nWidth == CW_USEDEFAULT || nWidth == CW_USEDEFAULT16 || !nWidth)
                 nWidth = mPos[1].x;
             if (nHeight == CW_USEDEFAULT || nHeight == CW_USEDEFAULT16 || !nHeight)
                 nHeight = mPos[1].y;
         }
     }
 
 skip_mdi:
     hwnd = User32CreateWindowEx(dwExStyle,
                                 (LPCSTR)lpClassName,
                                 (LPCSTR)lpWindowName,
                                 dwStyle,
                                 x,
                                 y,
                                 nWidth,
                                 nHeight,
                                 hWndParent,
                                 hMenu,
                                 hInstance,
                                 lpParam,
                                 0);
     return hwnd;
 }









  HWND WINAPI
 User32CreateWindowEx(DWORD dwExStyle,
                      LPCSTR lpClassName,
                      LPCSTR lpWindowName,
                      DWORD dwStyle,
                      int x,
                      int y,
                      int nWidth,
                      int nHeight,
                      HWND hWndParent,
                      HMENU hMenu,
                      HINSTANCE hInstance,
                      LPVOID lpParam,
                      DWORD dwFlags)
 {
     LARGE_STRING WindowName;
     LARGE_STRING lstrClassName, *plstrClassName;
     LARGE_STRING lstrClassVersion, *plstrClassVersion;
     UNICODE_STRING ClassName;
     UNICODE_STRING ClassVersion;
     WNDCLASSEXA wceA;
     WNDCLASSEXW wceW;
     HMODULE hLibModule = NULL;
     DWORD dwLastError;
     BOOL Unicode, ClassFound = FALSE;
     HWND Handle = NULL;
     LPCWSTR lpszClsVersion;
     LPCWSTR lpLibFileName = NULL;
     HANDLE pCtx = NULL;
     DWORD dwFlagsVer;
 
 #if 0
     DbgPrint("[window] User32CreateWindowEx style %d, exstyle %d, parent %d\n", dwStyle, dwExStyle, hWndParent);
 #endif
 
     dwFlagsVer = RtlGetExpWinVer( hInstance ? hInstance : GetModuleHandleW(NULL) );
     TRACE("Module Version %x\n",dwFlagsVer);
 
     if (!RegisterDefaultClasses)
     {
         TRACE("RegisterSystemControls\n");
         RegisterSystemControls();
     }
 
     Unicode = !(dwFlags & NUCWE_ANSI);
 
     if (IS_ATOM(lpClassName))
     {
         plstrClassName = (PVOID)lpClassName;
     }
     else
     {
         if (Unicode)
         {
             RtlInitUnicodeString(&ClassName, (PCWSTR)lpClassName);
         }
         else
         {
             if (!RtlCreateUnicodeStringFromAsciiz(&ClassName, (PCSZ)lpClassName))
             {
                 SetLastError(ERROR_OUTOFMEMORY);
                 return NULL;
             }
         }
 
         /* Copy it to a LARGE_STRING */
         lstrClassName.Buffer = ClassName.Buffer;
         lstrClassName.Length = ClassName.Length;
         lstrClassName.MaximumLength = ClassName.MaximumLength;
         plstrClassName = &lstrClassName;
     }
 
     /* Initialize a LARGE_STRING */
     RtlInitLargeString(&WindowName, lpWindowName, Unicode);
 
     // HACK: The current implementation expects the Window name to be UNICODE
     if (!Unicode)
     {
         NTSTATUS Status;
         PSTR AnsiBuffer = WindowName.Buffer;
         ULONG AnsiLength = WindowName.Length;
 
         WindowName.Length = 0;
         WindowName.MaximumLength = AnsiLength * sizeof(WCHAR);
         WindowName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                             0,
                                             WindowName.MaximumLength);
         if (!WindowName.Buffer)
         {
             SetLastError(ERROR_OUTOFMEMORY);
             goto cleanup;
         }
 
         Status = RtlMultiByteToUnicodeN(WindowName.Buffer,
                                         WindowName.MaximumLength,
                                         &WindowName.Length,
                                         AnsiBuffer,
                                         AnsiLength);
         if (!NT_SUCCESS(Status))
         {
             goto cleanup;
         }
     }
 
     if (!hMenu && (dwStyle & (WS_OVERLAPPEDWINDOW | WS_POPUP)))
     {
         if (Unicode)
         {
             wceW.cbSize = sizeof(wceW);
             if (GetClassInfoExW(hInstance, (LPCWSTR)lpClassName, &wceW) && wceW.lpszMenuName)
             {
                 hMenu = LoadMenuW(hInstance, wceW.lpszMenuName);
             }
         }
         else
         {
             wceA.cbSize = sizeof(wceA);
             if (GetClassInfoExA(hInstance, lpClassName, &wceA) && wceA.lpszMenuName)
             {
                 hMenu = LoadMenuA(hInstance, wceA.lpszMenuName);
             }
         }
     }
 
     if (!Unicode) dwExStyle |= WS_EX_SETANSICREATOR;
 
     lpszClsVersion = ClassNameToVersion(lpClassName, NULL, &lpLibFileName, &pCtx, !Unicode);
     if (!lpszClsVersion)
     {
         plstrClassVersion = plstrClassName;
     }
     else
     {
         RtlInitUnicodeString(&ClassVersion, lpszClsVersion);
         lstrClassVersion.Buffer = ClassVersion.Buffer;
         lstrClassVersion.Length = ClassVersion.Length;
         lstrClassVersion.MaximumLength = ClassVersion.MaximumLength;
         plstrClassVersion = &lstrClassVersion;
     }
 
     for (;;)
     {
         Handle = NtUserCreateWindowEx(dwExStyle,
                                       plstrClassName,
                                       plstrClassVersion,
                                       &WindowName,
                                       dwStyle,
                                       x,
                                       y,
                                       nWidth,
                                       nHeight,
                                       hWndParent,
                                       hMenu,
                                       hInstance,
                                       lpParam,
                                       dwFlagsVer,
                                       pCtx );
         if (Handle) break;
         if (!lpLibFileName) break;
         if (!ClassFound)
         {
             dwLastError = GetLastError();
             if (dwLastError == ERROR_CANNOT_FIND_WND_CLASS)
             {
                 ClassFound = VersionRegisterClass(ClassName.Buffer, lpLibFileName, pCtx, &hLibModule);
                 if (ClassFound) continue;
             }
         }
         if (hLibModule)
         {
             dwLastError = GetLastError();
             FreeLibrary(hLibModule);
             SetLastError(dwLastError);
             hLibModule = NULL;
         }
         break;
     }
 
 #if 0
     DbgPrint("[window] NtUserCreateWindowEx() == %d\n", Handle);
 #endif
 
 cleanup:
     if (!Unicode)
     {
         if (!IS_ATOM(lpClassName))
             RtlFreeUnicodeString(&ClassName);
 
         RtlFreeLargeString(&WindowName);
     }
 
     return Handle;
 }