// Explorer_v2.cpp: определяет точку входа для приложения.
//

#include "stdafx.h"
#include "Explorer_v2.h"


#define MAX_LOADSTRING	100
#define MAX_PATH		512

// Глобальные переменные:
HINSTANCE hInst;								// текущий экземпляр
TCHAR szTitle[MAX_LOADSTRING];					// Текст строки заголовка
TCHAR szWindowClass[MAX_LOADSTRING];			// имя класса главного окна

HWND hWndListBox1, hWndListBox2;
HWND hWndEdit1, hWndEdit2;
HWND hWndProgressBar;
WNDPROC origWndProcListView;
TCHAR path1[MAX_PATH], path2[MAX_PATH];
TCHAR selectedFile1[MAX_PATH], selectedFile2[MAX_PATH];
int lastListBox = 0;	// побитно |1й листбокс|2й листбокс|файл|папка|ссылка|000|
int id_button = ID_BUTTON_START;
bool cancelCopy;		// флаг для потока копирования
LARGE_INTEGER dirSize, copySize;
//HANDLE copyThread;

typedef struct _REPARSE_DATA_BUFFER
{
	ULONG ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	union
	{
		struct
		{
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG Flags;
			WCHAR PathBuffer[1];
		}
		SymbolicLinkReparseBuffer;
		struct
		{
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		}
		MountPointReparseBuffer;
		struct
		{
			UCHAR  DataBuffer[1];
		}
		GenericReparseBuffer;
	};
}
REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

// Отправить объявления функций, включенных в этот модуль кода:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
int					FileOperation(TCHAR *from, TCHAR *to, UINT func);
HWND				CreateListBox(int x, int y, int width, int heigth, HWND hWnd, HMENU id);
void				LoadFileList(HWND hWndlistBox, TCHAR *path);
int		CALLBACK	SortUpDir(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
void				AddIconToListBox(HWND hWndListBox, int size, TCHAR c_dir[MAX_PATH]);
void				DisplayError(TCHAR *header);
LRESULT CALLBACK	WndProcListView1(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK	WndProcListView2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	DialogRename1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	DialogRename2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	DialogCreateDir1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	DialogCreateDir2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	Dialog_Copy_File(HWND, UINT, WPARAM, LPARAM);
DWORD	CALLBACK	CopyProgressRoutineForFile(
	_In_ LARGE_INTEGER TotalFileSize,
	_In_ LARGE_INTEGER TotalBytesTransferred,
	_In_ LARGE_INTEGER StreamSize,
	_In_ LARGE_INTEGER StreamBytesTransferred,
	_In_ DWORD dwStreamNumber,
	_In_ DWORD dwCallbackReason,
	_In_ HANDLE hSourceFile,
	_In_ HANDLE hDestinationFile,
	_In_opt_ LPVOID lpData
	);
DWORD WINAPI		ThreadCopyForFile(LPVOID lpParam);
LARGE_INTEGER		GetFolderSize(TCHAR path[MAX_PATH]);
INT_PTR CALLBACK	Dialog_Copy_Dir(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI		ThreadCopyForDir(LPVOID lpParam);
DWORD CALLBACK		CopyProgressRoutineForDir(
	_In_ LARGE_INTEGER TotalFileSize,
	_In_ LARGE_INTEGER TotalBytesTransferred,
	_In_ LARGE_INTEGER StreamSize,
	_In_ LARGE_INTEGER StreamBytesTransferred,
	_In_ DWORD dwStreamNumber,
	_In_ DWORD dwCallbackReason,
	_In_ HANDLE hSourceFile,
	_In_ HANDLE hDestinationFile,
	_In_opt_ LPVOID lpData
	);
bool				CopyFolder(TCHAR pathFrom[MAX_PATH], TCHAR pathTo[MAX_PATH], HWND ProgressBar);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
					   _In_opt_ HINSTANCE hPrevInstance,
					   _In_ LPTSTR    lpCmdLine,
					   _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: разместите код здесь.
	MSG msg;
	HACCEL hAccelTable;

	// Инициализация глобальных строк
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_EXPLORER_V2, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Выполнить инициализацию приложения:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EXPLORER_V2));

	// Цикл основного сообщения:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  НАЗНАЧЕНИЕ: регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EXPLORER_V2));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_EXPLORER_V2);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   НАЗНАЧЕНИЕ: сохраняет обработку экземпляра и создает главное окно.
//
//   КОММЕНТАРИИ:
//
//        В данной функции дескриптор экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится на экран главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Сохранить дескриптор экземпляра в глобальной переменной

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 1000, 800, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  НАЗНАЧЕНИЕ:  обрабатывает сообщения в главном окне.
//
//  WM_COMMAND	- обработка меню приложения
//  WM_PAINT	-Закрасить главное окно
//  WM_DESTROY	 - ввести сообщение о выходе и вернуться.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	LPNMHDR lpnmHdr = (LPNMHDR)lParam;
	LPNMLISTVIEW pnmLV = (LPNMLISTVIEW)lParam;
	TCHAR *selectedFile = 0;
	TCHAR selectedFileSize[MAX_PATH];
	TCHAR fullPathToFile[MAX_PATH];
	
	bool reloadFileList = 1;
	HWND hWndListBox = 0;
	HWND hWndEdit = 0;
	TCHAR *path = 0;
	
	switch (message)
	{
	case WM_CREATE:	
		int x, y;
		int k;
		int dx, width;
		TCHAR *disk, *disk_start;

		InitCommonControls();

		disk_start = disk = new TCHAR[256];
		memset(disk_start, 0, sizeof(disk_start));

		GetLogicalDrives();
		GetLogicalDriveStrings(256, (LPTSTR)disk);

		x = 10;
		y = 10;
		dx = 200;
		width = 170;
		k = _tcslen(disk) + 1;
		while (*disk != '\0')
		{
			disk[1] = 0;
			CreateWindow(_T("BUTTON"), disk, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
				x, y, 30, 20, hWnd, (HMENU)id_button++, NULL, NULL);

			CreateWindow(_T("BUTTON"), disk, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
				x + 490, y, 30, 20, hWnd, (HMENU)id_button++, NULL, NULL);

			x += 40;
			disk += k;
		}
		delete[]disk_start;

		x = 10;
		y += 30;


		hWndEdit1 = CreateWindow(_T("EDIT"), NULL, WS_BORDER | WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY,
			x, y, 480, 20, hWnd, (HMENU)ID_EDIT_1, NULL, NULL);

		hWndEdit2 = CreateWindow(_T("EDIT"), NULL, WS_BORDER | WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY,
			x + 490, y, 480, 20, hWnd, (HMENU)ID_EDIT_2, NULL, NULL);

		y += 30;

		hWndListBox1 = CreateListBox(
			x, y,
			480, 620,
			hWnd,
			(HMENU)ID_LISTBOX_1);

		hWndListBox2 = CreateListBox(
			x + 490, y,
			480, 620,
			hWnd,
			(HMENU)ID_LISTBOX_2);	

		origWndProcListView = (WNDPROC) SetWindowLong(hWndListBox1, 
			GWL_WNDPROC, (LONG) WndProcListView1); 

		origWndProcListView = (WNDPROC) SetWindowLong(hWndListBox2, 
			GWL_WNDPROC, (LONG) WndProcListView2); 

		y += 630;

		path1[0] = 0;
		path2[0] = 0;

		_tcscat_s(path1, _T("c:\\"));
		_tcscat_s(path2, _T("c:\\"));

		LoadFileList(hWndListBox1, path1);
		LoadFileList(hWndListBox2, path2);

		SetWindowText(hWndEdit1, path1);
		SetWindowText(hWndEdit2, path2);

		CreateWindow(_T("BUTTON"), _T("F2 Переименование"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			x, y, width, 30, hWnd, (HMENU)ID_BUTTON_RENAME, NULL, NULL);

		x += dx;

		CreateWindow(_T("BUTTON"), _T("F5 Копирование"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			x, y, width, 30, hWnd, (HMENU)ID_BUTTON_COPY, NULL, NULL);

		x += dx;

		CreateWindow(_T("BUTTON"), _T("F6 Перемещение"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			x, y, width, 30, hWnd, (HMENU)ID_BUTTON_MOVE, NULL, NULL);

		x += dx;

		CreateWindow(_T("BUTTON"), _T("F7 Каталог"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			x, y, width, 30, hWnd, (HMENU)ID_BUTTON_DIR_CREATE, NULL, NULL);

		x += dx;

		CreateWindow(_T("BUTTON"), _T("F8 Удаление"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			x, y, width, 30, hWnd, (HMENU)ID_BUTTON_DELETE, NULL, NULL);

		x += dx;

		break;
	case WM_NOTIFY:
		switch (lpnmHdr->code)
		{
		case NM_CLICK:
			switch (lpnmHdr->idFrom)
			{
			case ID_LISTBOX_1:
				hWndListBox = hWndListBox1;
				selectedFile = selectedFile1;
				lastListBox = 0x01; 
				break;
			case ID_LISTBOX_2:
				hWndListBox = hWndListBox2;
				selectedFile = selectedFile2;
				lastListBox = 0x02;
				break;
			default:
				break;
			}
			if (hWndListBox)
			{
				ListView_GetItemText(lpnmHdr->hwndFrom, pnmLV->iItem, 0, selectedFile, MAX_PATH);
				if (_tcscmp(selectedFile, _T("..")) && _tcscmp(selectedFile, _T(".")))
				{
					ListView_GetItemText(lpnmHdr->hwndFrom, pnmLV->iItem, 1, selectedFileSize, MAX_PATH);
					if (_tcscmp(selectedFileSize, _T("<Папка>")) == 0) lastListBox |= 1 << 3;
					else if (_tcscmp(selectedFileSize, _T("<Ссылка>")) == 0) lastListBox |= 1 << 4;
					else lastListBox |= 1 << 2;
				}
			}
			break;
			//		case NM_RETURN:
		case NM_DBLCLK:
			switch (lpnmHdr->idFrom)
			{
			case ID_LISTBOX_1:
				hWndListBox = hWndListBox1;
				hWndEdit = hWndEdit1;
				path = path1;
				break;
			case ID_LISTBOX_2:
				hWndListBox = hWndListBox2;
				hWndEdit = hWndEdit2;
				path = path2;
				break;
			default:
				break;
			}
			if (hWndListBox)
			{
				selectedFile = new TCHAR[MAX_PATH];
				ListView_GetItemText(lpnmHdr->hwndFrom, pnmLV->iItem, 0, selectedFile, MAX_PATH);

				if (_tcscmp(selectedFile, _T("..")) == 0)	// Вверх на одну дирректорию
				{
					path[_tcslen(path) - 1] = 0;
					for (int i = _tcslen(path) - 1; i > 0; i--)
					{
						TCHAR s;
						s = path[i];
						if (s == '\\')
						{
							k = i;
							break;
						}
					}
					path[k + 1] = 0;
				}
				else if (_tcscmp(selectedFile, _T(".")) == 0)	// В корень диска
				{
					path[3] = 0;
				}
				else
				{
					ListView_GetItemText(lpnmHdr->hwndFrom, pnmLV->iItem, 1, selectedFileSize, MAX_PATH);
					if (_tcscmp(selectedFileSize, _T("<Папка>")) == 0)
					{
						_tcscat_s(path, MAX_PATH, _T("\\"));
						path[_tcslen(path) - 1] = 0;
						_tcscat_s(path, MAX_PATH, selectedFile);
						_tcscat_s(path, MAX_PATH, _T("\\"));
					}
					else if (_tcscmp(selectedFileSize, _T("<Ссылка>")) == 0)
					{
						HANDLE reparsePoint;
						BYTE *pBuffer;
						PREPARSE_DATA_BUFFER pReparseBuffer = NULL;
						DWORD dwRetCode;

						fullPathToFile[0] = 0;
						_tcscpy_s(fullPathToFile, _T("\\??\\"));
						//_tcscpy_s(fullPathToFile, _T("\\\\.\\"));
						_tcscat_s(fullPathToFile, path);
						_tcscat_s(fullPathToFile, selectedFile);

						//MessageBox(0, fullPathToFile, L"", 0);

						reparsePoint = CreateFile(fullPathToFile,
							READ_CONTROL, 
							FILE_SHARE_READ,  
							0, 
							OPEN_EXISTING,
							FILE_FLAG_OPEN_REPARSE_POINT | 
							FILE_FLAG_BACKUP_SEMANTICS |
							0,
							0);

						if (reparsePoint == INVALID_HANDLE_VALUE)
						{
							DisplayError(_T("reParsePoint 1"));
							CloseHandle(reparsePoint);
						}
						else 
						{
							pBuffer = new BYTE[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
							pReparseBuffer = (PREPARSE_DATA_BUFFER)pBuffer;
							if (DeviceIoControl(
								reparsePoint,
								FSCTL_GET_REPARSE_POINT,
								NULL,
								0,
								pReparseBuffer,
								MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
								&dwRetCode,
								NULL))
							{
								if(pReparseBuffer->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
								{
									if (*pReparseBuffer->MountPointReparseBuffer.PathBuffer)
									{
										_tcscpy_s(path, MAX_PATH, &(pReparseBuffer->MountPointReparseBuffer.PathBuffer)[4]);
										_tcscat_s(path, MAX_PATH, _T("\\"));
									}
									else
										MessageBox(0, _T("Ошибка при получении пути по ссылке"), _T("Ошибка"), 0);
								}
								else
								{
									_tcscat_s(path, MAX_PATH, _T("\\"));
									path[_tcslen(path) - 1] = 0;
									_tcscat_s(path, MAX_PATH, selectedFile);
									_tcscat_s(path, MAX_PATH, _T("\\"));
								}
							}
							else DisplayError(_T("DeviceIoControl 1"));
							delete pBuffer;
							CloseHandle(reparsePoint);
						}
					}
					else
					{
						fullPathToFile[0] = 0;
						_tcscpy_s(fullPathToFile, path);
						_tcscat_s(fullPathToFile, selectedFile);
						ShellExecute(0, _T("open"), fullPathToFile, NULL, NULL, SW_SHOWNORMAL);
						reloadFileList = 0;
					}
				}
				if (reloadFileList)
				{
					lastListBox = 0;
					SetWindowText(hWndEdit, path);
					LoadFileList(hWndListBox, path);
					switch (lpnmHdr->idFrom)
					{
					case ID_LISTBOX_1:
						selectedFile1[0] = 0;
						break;
					case ID_LISTBOX_2:
						selectedFile2[0] = 0;
						break;
					default:
						break;
					}

				}
				delete[]selectedFile;
			}
			break;
		case NM_RCLICK:
			switch (lpnmHdr->idFrom)
			{
			case ID_LISTBOX_1:
				hWndListBox = hWndListBox1;
				hWndEdit = hWndEdit1;
				path = path1;
				break;
			case ID_LISTBOX_2:
				hWndListBox = hWndListBox2;
				hWndEdit = hWndEdit2;
				path = path2;
				break;
			default:
				break;
			}
			if (hWndListBox)
			{
				selectedFile = new TCHAR[MAX_PATH];
				ListView_GetItemText(lpnmHdr->hwndFrom, pnmLV->iItem, 0, selectedFile, MAX_PATH);

				SHELLEXECUTEINFO fileInfo;

				_tcscpy_s(fullPathToFile, path);
				_tcscat_s(fullPathToFile, selectedFile);

				ZeroMemory(&fileInfo,sizeof(SHELLEXECUTEINFO));
				fileInfo.cbSize=sizeof(SHELLEXECUTEINFO);
				fileInfo.lpVerb=_T("properties");
				fileInfo.lpFile=fullPathToFile;
				fileInfo.nShow=SW_SHOW;
				fileInfo.fMask=SEE_MASK_INVOKEIDLIST;
				ShellExecuteEx(&fileInfo);
			}
			break;
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Разобрать выбор в меню:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		case ID_BUTTON_RENAME:
			switch (lastListBox)
			{
			case 1:
				SendMessage(hWndListBox1,WM_KEYDOWN,VK_F2,0);
				break;
			case 2:
				SendMessage(hWndListBox2,WM_KEYDOWN,VK_F2,0);
				break;
			}
			break;

		case ID_BUTTON_COPY:
			switch (lastListBox)
			{
			case 1:
				SendMessage(hWndListBox1,WM_KEYDOWN,VK_F5,0);
				break;
			case 2:
				SendMessage(hWndListBox2,WM_KEYDOWN,VK_F5,0);
				break;
			}
			break;

		case ID_BUTTON_MOVE:
			switch (lastListBox)
			{
			case 1:
				SendMessage(hWndListBox1,WM_KEYDOWN,VK_F6,0);
				break;
			case 2:
				SendMessage(hWndListBox2,WM_KEYDOWN,VK_F6,0);
				break;
			}
			break;

		case ID_BUTTON_DIR_CREATE:
			switch (lastListBox)
			{
			case 1:
				SendMessage(hWndListBox1,WM_KEYDOWN,VK_F7,0);
				break;
			case 2:
				SendMessage(hWndListBox2,WM_KEYDOWN,VK_F7,0);
				break;
			}
			break;

		case ID_BUTTON_DELETE:
			switch (lastListBox)
			{
			case 1:
				SendMessage(hWndListBox1,WM_KEYDOWN,VK_F8,0);
				break;
			case 2:
				SendMessage(hWndListBox2,WM_KEYDOWN,VK_F8,0);
				break;
			}
			break;

		default:
			if (wmId >= ID_BUTTON_START && wmId < id_button)
			{
				if (wmId % 2 == 0)
				{
					hWndListBox = hWndListBox1;
					hWndEdit = hWndEdit1;
					selectedFile = selectedFile1;
					path = path1;
				}
				else
				{
					hWndListBox = hWndListBox2;
					hWndEdit = hWndEdit2;
					selectedFile = selectedFile2;
					path = path2;
				}
				GetWindowText(GetDlgItem(hWnd, wmId),path,MAX_PATH);
				_tcscat_s(path, MAX_PATH, _T(":\\"));
				SetWindowText(hWndEdit, path);
				LoadFileList(hWndListBox, path);
				lastListBox = 0;
				selectedFile[0] = 0;
			}
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: добавьте любой код отрисовки...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Обработчик сообщений для окна "О программе".
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

int FileOperation(TCHAR *from, TCHAR *to, UINT func)
{
	SHFILEOPSTRUCT shFileOpStr = {0};
	int i;

	i = 0;
	while(from[i]) i++;
	from[i+1] = 0;

	if(to)
	{
		i = 0;
		while(to[i]) i++;
		to[i+1] = 0;
	}

	shFileOpStr.hwnd = 0;
	shFileOpStr.wFunc = func;
	shFileOpStr.pFrom = from;
	shFileOpStr.pTo = to;
	shFileOpStr.fFlags = FOF_NOCONFIRMMKDIR;  
	shFileOpStr.fAnyOperationsAborted = 0;
	shFileOpStr.hNameMappings = 0;
	shFileOpStr.lpszProgressTitle = 0;

	return SHFileOperation(&shFileOpStr);
}

HWND CreateListBox(int x, int y, int width, int heigth, HWND hWnd, HMENU id)
{
	HWND hWndListBox;
	LVCOLUMN lvc;	// структура колонки

	hWndListBox = CreateWindow(WC_LISTVIEW,
		_T(""),
		WS_CHILD
		| WS_VISIBLE	// Видимый
		| WS_BORDER		// Рамка
		| ES_READONLY
		| LVS_REPORT	// Стиль таблицы
		//		| LVS_NOSORTHEADER 
		//		| WS_SIZEBOX	// Изменяемый размер
		,
		x, y,
		width, heigth,
		hWnd,
		id,
		0,
		0);

	ListView_SetExtendedListViewStyle(hWndListBox,
		ListView_GetExtendedListViewStyle(hWndListBox)
		| LVS_EX_FULLROWSELECT	// Выделение всей строки
		//		| LVS_EX_GRIDLINES		// показывать сетку таблицы
		);
	//--Настройка цветов--//
	ListView_SetBkColor(hWndListBox, 0x00000000);	//0x00bbggrr
	ListView_SetTextColor(hWndListBox, 0x0000ff00);
	ListView_SetTextBkColor(hWndListBox, 0x00000000);
	//--Добавление колонки--//
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.iSubItem = 0;			// номер колонки
	lvc.pszText = _T("Имя");	// имя колонки
	lvc.cx = 200;               // Ширина колонки
	lvc.fmt = LVCFMT_LEFT;		// Прижать влево текст
	ListView_InsertColumn(hWndListBox, 0, &lvc);

	lvc.iSubItem = 1;			// номер колонки
	lvc.pszText = _T("Размер");	// имя колонки
	lvc.cx = 90;				// Ширина колонки
	lvc.fmt = LVCFMT_RIGHT;		// Прижать вправо текст

	ListView_InsertColumn(hWndListBox, 1, &lvc);

	lvc.iSubItem = 2;			// номер колонки
	lvc.pszText = _T("Дата");	// имя колонки
	lvc.cx = 100;				// Ширина колонки
	ListView_InsertColumn(hWndListBox, 2, &lvc);

	return hWndListBox;
}

void LoadFileList(HWND hWndListBox, TCHAR *path)
{
	LVITEM lvi;					// структура текста в колонке
	WIN32_FIND_DATA fileInfo;	// переменная для загрузки данных об одном файле
	HANDLE findFile;			// Указатель на файл
	int i, j, k, iTmp;
	LARGE_INTEGER fileSize;		// Размер файла
	SYSTEMTIME fileDate;		// Дата изменения файла
	TCHAR cTmp[256], cTmp2[256];
	TCHAR path2[MAX_PATH];

	SendMessage(hWndListBox, LVM_DELETEALLITEMS, 0, 0);

	path2[0] = 0;
	_tcscat_s(path2, path);
	_tcscat_s(path2, _T("*"));

	//--Добавление текста--//
	memset(&lvi, 0, sizeof(lvi));	// Zero struct's Members
	lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;;			// Text Style
	lvi.cchTextMax = 256;			// Максимальная длина текста

	findFile = FindFirstFile(path2, &fileInfo);	// Загрузка данных о первом файле
	if (findFile != INVALID_HANDLE_VALUE)
	{
		i = 0;
		do
		{
			//--Вывод имени файла--//
			lvi.iItem = i;							// номер строки
			lvi.iImage = i;
			lvi.iSubItem = 0;						// номер колонки
			lvi.pszText = fileInfo.cFileName;		// Текст строки
			lvi.lParam = i;
			ListView_InsertItem(hWndListBox, &lvi);

			//--Вывод размера файла--//
			if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
				{
					ListView_SetItemText(hWndListBox, i, 1, _T("<Ссылка>"));
				}
				else
				{
					ListView_SetItemText(hWndListBox, i, 1, _T("<Папка>"));
				}
			}
			else
			{
				fileSize.LowPart = fileInfo.nFileSizeLow;
				fileSize.HighPart = fileInfo.nFileSizeHigh;
				_stprintf_s(cTmp, 256, _T("%lld"), fileSize);
				iTmp = _tcslen(cTmp);
				if (iTmp > 3)
				{
					_tcscpy_s(cTmp2, cTmp);
					k = 0;
					for (j = 0; j < iTmp; j++)
					{
						if ((iTmp - j) % 3 == 0 && j)
						{
							cTmp[k] = ' ';
							k++;
						}
						cTmp[k] = cTmp2[j];
						k++;
					}
					cTmp[k] = 0;
				}
				ListView_SetItemText(hWndListBox, i, 1, cTmp);
			}
			//--Вывод даты файла--//
			FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &fileDate);		// преобразование даты в другую структуру
			_stprintf_s(cTmp, 256, _T("%02d.%02d.%04d %02d:%02d"), fileDate.wDay, fileDate.wMonth, fileDate.wYear, fileDate.wHour, fileDate.wMinute);
			ListView_SetItemText(hWndListBox, i, 2, cTmp);

			i++;
		} while (FindNextFile(findFile, &fileInfo));		// Загрузка данных о следующем файле
	}
	AddIconToListBox(hWndListBox, i, path2);
	ListView_SortItemsEx(hWndListBox, SortUpDir, hWndListBox);
}

int CALLBACK SortUpDir(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	TCHAR word1[256], word2[256];
	LVFINDINFO find;
	int i1, i2;

	if (lParam1 == 0) return -1;
	else if (lParam2 == 0) return 1;

	find.flags = LVFI_PARAM;
	find.lParam = lParam1;
	ListView_GetItemText((HWND)lParamSort,
		i1 = ListView_FindItem((HWND)lParamSort, -1, &find),
		1, word1, 256);

	find.lParam = lParam2;
	ListView_GetItemText((HWND)lParamSort,
		i2 = ListView_FindItem((HWND)lParamSort, -1, &find),
		1, word2, 256);

	if (word1[0] == '<' && word2[0] == '<')
		//	if ((_tcscmp(word1, _T("<Папка>")) == 0 || _tcscmp(word1, _T("<Ссылка>")) == 0) &&
			//		(_tcscmp(word2, _T("<Папка>")) == 0 || _tcscmp(word2, _T("<Ссылка>")) == 0))
	{
		return 0;
	}
	else if (word1[0] == '<')
		//	else if (_tcscmp(word1, _T("<Папка>")) == 0 || _tcscmp(word1, _T("<Ссылка>")) == 0)
	{
		return -1;
	}
	else if (word2[0] == '<')
		//	else if (_tcscmp(word2, _T("<Папка>")) == 0 || _tcscmp(word2, _T("<Ссылка>")) == 0)
	{
		return 1;
	}
	else return 0;
}

void AddIconToListBox(HWND hWndListBox, int size, TCHAR c_dir[MAX_PATH])
{
	HIMAGELIST hSmall;
	SHFILEINFO lp;
	TCHAR buf1[MAX_PATH];
	DWORD num;

	hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK | ILC_COLOR32, size + 2, 1);

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	hFind = FindFirstFile(c_dir, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		MessageBox(0, _T("Ошибка"), _T("Не найден"), MB_OK | MB_ICONWARNING);
	}
	else
	{
		do 
		{//присваеваем атрибуты
			if (_tcscmp(FindFileData.cFileName, _T(".")) == 0)
			{//если диск
				_tcscpy_s(buf1, c_dir);
				_tcscat_s(buf1, FindFileData.cFileName);
				SHGetFileInfo(_T(""), FILE_ATTRIBUTE_DEVICE, &lp, sizeof(lp), SHGFI_ICONLOCATION | SHGFI_ICON | SHGFI_SMALLICON);

				ImageList_AddIcon(hSmall, lp.hIcon);
				DestroyIcon(lp.hIcon);

			}
			if (_tcscmp(FindFileData.cFileName, _T("..")) == 0)
			{//если фаилы,папки
				_tcscpy_s(buf1, c_dir);
				_tcscat_s(buf1, FindFileData.cFileName);
				SHGetFileInfo(_T(""), FILE_ATTRIBUTE_DIRECTORY, &lp, sizeof(lp), SHGFI_ICONLOCATION | SHGFI_ICON | SHGFI_SMALLICON);

				ImageList_AddIcon(hSmall, lp.hIcon);
				DestroyIcon(lp.hIcon);
			}
			//присваеваем иконки
			_tcscpy_s(buf1, c_dir);
			buf1[_tcslen(buf1) - 1] = 0;

			_tcscat_s(buf1, FindFileData.cFileName);
			num = GetFileAttributes(buf1);
			SHGetFileInfo(buf1, num, &lp, sizeof(lp), SHGFI_ICONLOCATION | SHGFI_ICON | SHGFI_SMALLICON);

			ImageList_AddIcon(hSmall, lp.hIcon);
			DestroyIcon(lp.hIcon);

		} while (FindNextFile(hFind, &FindFileData) != 0);

		FindClose(hFind);
	}
	ListView_SetImageList(hWndListBox, hSmall, LVSIL_SMALL);
}

void DisplayError(TCHAR *header)
{
	TCHAR message[512];
	TCHAR buf[8];
	LPVOID lpvMessageBuffer;

	_itot_s(GetLastError(), buf, 10);
	_tcscpy_s(message, _T("Ошибка "));
	_tcscat_s(message, buf);
	_tcscat_s(message, _T(": "));

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpvMessageBuffer, 0, NULL);

	_tcscat_s(message, (LPWSTR)lpvMessageBuffer);
	MessageBox(0, message, header, 0);
	LocalFree(lpvMessageBuffer);
}

LRESULT CALLBACK WndProcListView1(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{		
	switch(message)
	{
	case WM_KEYDOWN:
		TCHAR from[MAX_PATH], to[MAX_PATH];

		_tcscpy_s(from, path1);
		_tcscat_s(from, selectedFile1);
		_tcscpy_s(to, path2);
		_tcscat_s(to, selectedFile1);

		switch(wParam)
		{
		case VK_F2:
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_RENAME), hWnd, DialogRename1) == 0)
			{
				LoadFileList(hWndListBox1, path1);
			}
			break;

		case VK_F5:
			switch(lastListBox >> 2)
			{
			case 1:		// Файл
				if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_COPY_THREAD), hWnd, Dialog_Copy_File) != 0)
				{
					LoadFileList(hWndListBox2, path2);
				}
				break;
			case 2:		// папка
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_COPY_THREAD), hWnd, Dialog_Copy_Dir);
				LoadFileList(hWndListBox2, path2);
				break;
			}
			break;

		case VK_F6:
			if(FileOperation(from,to,FO_MOVE) == 0)
			{
				LoadFileList(hWndListBox1, path1);
				LoadFileList(hWndListBox2, path2);
			}
			break;
	
		case VK_F7:
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_CREATE_DIR), hWnd, DialogCreateDir1) == 1)
			{
				LoadFileList(hWndListBox1, path1);
			}
			break;

		case VK_DELETE:
		case VK_F8:
			if(FileOperation(from,0,FO_DELETE) == 0)
			{
				LoadFileList(hWndListBox1, path1);
			}
			break;
		default:
			break;
		}	
	default:
		return CallWindowProc(origWndProcListView, hWnd, message, wParam, lParam); 
	}
	return 0;
}

LRESULT CALLBACK WndProcListView2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{		
	switch(message)
	{
	case WM_RBUTTONUP:

		break;
	case WM_KEYDOWN:
		TCHAR from[MAX_PATH], to[MAX_PATH];

		_tcscpy_s(from, path2);
		_tcscat_s(from, selectedFile2);
		_tcscpy_s(to, path1);
		_tcscat_s(to, selectedFile2);

		switch(wParam)
		{
		case VK_F2:
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_RENAME), hWnd, DialogRename2) == 0)
			{
				LoadFileList(hWndListBox2, path2);
			}
			break;

		case VK_F5:
			switch(lastListBox >> 2)
			{
			case 1:		// Файл
				if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_COPY_THREAD), hWnd, Dialog_Copy_File) != 0)
				{
					LoadFileList(hWndListBox1, path1);
				}
				break;
			case 2:		// папка
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_COPY_THREAD), hWnd, Dialog_Copy_Dir);
				LoadFileList(hWndListBox1, path1);
				break;
			}
			break;

		case VK_F6:
			if(FileOperation(from,to,FO_MOVE) == 0)
			{
				LoadFileList(hWndListBox1, path1);
				LoadFileList(hWndListBox2, path2);
			}
			break;

		case VK_F7:
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_CREATE_DIR), hWnd, DialogCreateDir2) == 1)
			{
				LoadFileList(hWndListBox2, path2);
			}
			break;

		case VK_DELETE:
		case VK_F8:
			if(FileOperation(from,0,FO_DELETE) == 0)
			{
				LoadFileList(hWndListBox2, path2);
			}
			break;

		default:
			break;
		}	
	default:
		return CallWindowProc(origWndProcListView, hWnd, message, wParam, lParam); 
	}
	return 0;
}

INT_PTR CALLBACK DialogRename1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(hDlg, ID_DEDIT),selectedFile1);
		return (INT_PTR)TRUE;
	
	case WM_COMMAND:
		switch(wParam) 
		{
		case IDOK:
			TCHAR from[MAX_PATH], to[MAX_PATH], buf[MAX_PATH];

			buf[GetWindowText(GetDlgItem(hDlg, ID_DEDIT),buf,MAX_PATH)] = 0;

			_tcscpy_s(from, path1);
			_tcscat_s(from, selectedFile1);
			_tcscpy_s(to, path1);
			_tcscat_s(to, buf);

			EndDialog(hDlg, LOWORD(FileOperation(from,to,FO_RENAME)));
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK DialogRename2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(hDlg, ID_DEDIT),selectedFile2);
		return (INT_PTR)TRUE;
	
	case WM_COMMAND:
		switch(wParam) 
		{
		case IDOK:
			TCHAR from[MAX_PATH], to[MAX_PATH], buf[MAX_PATH];

			buf[GetWindowText(GetDlgItem(hDlg, ID_DEDIT),buf,MAX_PATH)] = 0;

			_tcscpy_s(from, path2);
			_tcscat_s(from, selectedFile2);
			_tcscpy_s(to, path2);
			_tcscat_s(to, buf);

			EndDialog(hDlg, LOWORD(FileOperation(from,to,FO_RENAME)));
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK DialogCreateDir1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	
	case WM_COMMAND:
		switch(wParam) 
		{
		case IDOK:
			TCHAR to[MAX_PATH], buf[MAX_PATH];

			buf[GetWindowText(GetDlgItem(hDlg, ID_DEDIT),buf,MAX_PATH)] = 0;

			_tcscpy_s(to, path1);
			_tcscat_s(to, buf);

			EndDialog(hDlg, LOWORD(CreateDirectory(to,0)));
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(0));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK DialogCreateDir2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	
	case WM_COMMAND:
		switch(wParam) 
		{
		case IDOK:
			TCHAR to[MAX_PATH], buf[MAX_PATH];

			buf[GetWindowText(GetDlgItem(hDlg, ID_DEDIT),buf,MAX_PATH)] = 0;

			_tcscpy_s(to, path2);
			_tcscat_s(to, buf);

			EndDialog(hDlg, LOWORD(CreateDirectory(to,0)));
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(0));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_Copy_File(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		CreateThread(
			0,	// default security attributes
			0, // use default stack size
			ThreadCopyForFile,	// thread function name
			hDlg,	// argument to thread function
			0, // use default creation flags
			0);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDCANCEL:
			cancelCopy = 1;
			EndDialog(hDlg, LOWORD(0));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

DWORD WINAPI ThreadCopyForFile(LPVOID lpParam)
{
	TCHAR lpExistingFileName[MAX_PATH];
	TCHAR lpNewFileName[MAX_PATH];
	DWORD r = 0;
	bool copy;
	switch (lastListBox & 0x03)
	{
	case 0:
		copy = 0;
		EndDialog((HWND)lpParam, LOWORD(IDCANCEL));
		break;
	case 1:
		copy = 1;
		_tcscpy_s(lpExistingFileName, path1);
		_tcscat_s(lpExistingFileName, selectedFile1);
		_tcscpy_s(lpNewFileName, path2);
		_tcscat_s(lpNewFileName, selectedFile1);
		break;
	case 2:
		copy = 1;
		_tcscpy_s(lpExistingFileName, path2);
		_tcscat_s(lpExistingFileName, selectedFile2);
		_tcscpy_s(lpNewFileName, path1);
		_tcscat_s(lpNewFileName, selectedFile2);
		break;
	default:
		copy = 0;
	}
	if (copy)
	{
		cancelCopy = 0;
		r = CopyFileEx(lpExistingFileName, lpNewFileName, CopyProgressRoutineForFile, GetDlgItem((HWND)lpParam, ID_DPROGRESSBAR), (LPBOOL)&cancelCopy, COPY_FILE_FAIL_IF_EXISTS);
		if(!r)	 DisplayError(_T("Ошибка при копировании."));
	}
	EndDialog((HWND)lpParam, LOWORD(r));
	return r;
}

DWORD CALLBACK CopyProgressRoutineForFile(
	_In_ LARGE_INTEGER TotalFileSize,
	_In_ LARGE_INTEGER TotalBytesTransferred,
	_In_ LARGE_INTEGER StreamSize,
	_In_ LARGE_INTEGER StreamBytesTransferred,
	_In_ DWORD dwStreamNumber,
	_In_ DWORD dwCallbackReason,
	_In_ HANDLE hSourceFile,
	_In_ HANDLE hDestinationFile,
	_In_opt_ LPVOID lpData
	)
{
	switch (dwCallbackReason)
	{
	case CALLBACK_CHUNK_FINISHED:
		SendMessage((HWND)lpData, PBM_SETPOS, (TotalBytesTransferred.QuadPart * 100 / TotalFileSize.QuadPart), 0);
		UpdateWindow((HWND)lpData);
		break;
	}
	return PROGRESS_CONTINUE;
}

LARGE_INTEGER GetFolderSize(TCHAR path[MAX_PATH])
{
	HANDLE Handle;  
	WIN32_FIND_DATA FindData;
	LARGE_INTEGER Result;
	LARGE_INTEGER tmp;
	Result.QuadPart = 0;
	_tcscat_s(path, MAX_PATH, _T("*"));
	Handle = FindFirstFile(path, &FindData);
	if (Handle == INVALID_HANDLE_VALUE)
	{		
		return Result;
	}		
	do
	{
		if(_tcscmp(FindData.cFileName,L".") && _tcscmp(FindData.cFileName,L".."))
		{
			if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{				
				tmp.QuadPart = 0;
				TCHAR path2[MAX_PATH];
				_tcscpy_s(path2,path);
				path2[_tcslen(path2)-1] = 0;
				_tcscat_s(path2, FindData.cFileName);
				_tcscat_s(path2, _T("\\")); 
				tmp = GetFolderSize(path2);
				Result.QuadPart = Result.QuadPart + tmp.QuadPart;	
			}
			else
			{
				tmp.QuadPart = 0;
				tmp.QuadPart = ((DWORDLONG)FindData.nFileSizeHigh<<32) + FindData.nFileSizeLow;
				Result.QuadPart = Result.QuadPart + tmp.QuadPart;				
			}
		}
	}
	while(FindNextFile(Handle, &FindData) != 0);
	FindClose(Handle);
	return Result;
}

INT_PTR CALLBACK Dialog_Copy_Dir(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		CreateThread(
			0,	// default security attributes
			0, // use default stack size
			ThreadCopyForDir,	// thread function name
			hDlg,	// argument to thread function
			0, // use default creation flags
			0);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDCANCEL:
			cancelCopy = 1;
			EndDialog(hDlg, LOWORD(0));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

DWORD WINAPI ThreadCopyForDir(LPVOID lpParam)
{
	TCHAR lpExistingFileName[MAX_PATH];
	TCHAR lpNewFileName[MAX_PATH];
	DWORD r = 0;
	bool copy;
	switch (lastListBox & 0x03)
	{
	case 0:
		copy = 0;
		EndDialog((HWND)lpParam, LOWORD(IDCANCEL));
		break;
	case 1:
		copy = 1;
		_tcscpy_s(lpExistingFileName, path1);
		_tcscat_s(lpExistingFileName, selectedFile1);
		_tcscat_s(lpExistingFileName, _T("\\"));
		_tcscpy_s(lpNewFileName, path2);
		_tcscat_s(lpNewFileName, selectedFile1);
		break;
	case 2:
		copy = 1;
		_tcscpy_s(lpExistingFileName, path2);
		_tcscat_s(lpExistingFileName, selectedFile2);
		_tcscat_s(lpExistingFileName, _T("\\"));
		_tcscpy_s(lpNewFileName, path1);
		_tcscat_s(lpNewFileName, selectedFile2);
		break;
	default:
		copy = 0;
	}
	if (copy)
	{
		cancelCopy = 0;		
		copySize.QuadPart = 0;
		dirSize = GetFolderSize(lpExistingFileName);
		
		r = CopyFolder(lpExistingFileName, lpNewFileName,GetDlgItem((HWND)lpParam, ID_DPROGRESSBAR));
	}
	EndDialog((HWND)lpParam, LOWORD(r));
	return r;
}

DWORD CALLBACK CopyProgressRoutineForDir(
	_In_ LARGE_INTEGER TotalFileSize,
	_In_ LARGE_INTEGER TotalBytesTransferred,
	_In_ LARGE_INTEGER StreamSize,
	_In_ LARGE_INTEGER StreamBytesTransferred,
	_In_ DWORD dwStreamNumber,
	_In_ DWORD dwCallbackReason,
	_In_ HANDLE hSourceFile,
	_In_ HANDLE hDestinationFile,
	_In_opt_ LPVOID lpData
	)
{
	switch (dwCallbackReason)
	{
	case CALLBACK_CHUNK_FINISHED:
		SendMessage((HWND)lpData, PBM_SETPOS, ((TotalBytesTransferred.QuadPart + copySize.QuadPart) * 100 / dirSize.QuadPart), 0);
		UpdateWindow((HWND)lpData);
		break;
	}
	return PROGRESS_CONTINUE;
}

bool CopyFolder(TCHAR pathFrom[MAX_PATH], TCHAR pathTo[MAX_PATH], HWND ProgressBar)
{
	HANDLE Handle;  
	WIN32_FIND_DATA FindData;

	CreateDirectory(pathTo, 0);
	_tcscat_s(pathTo, MAX_PATH, _T("\\")); 
	
	Handle = FindFirstFile(pathFrom, &FindData);
	if (Handle == INVALID_HANDLE_VALUE)
	{		
		return 0;
	}		
	do
	{
		if(_tcscmp(FindData.cFileName,L".") && _tcscmp(FindData.cFileName,L".."))
		{
			if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{				
				TCHAR pathFrom2[MAX_PATH], pathTo2[MAX_PATH];
				_tcscpy_s(pathFrom2,pathFrom);				
				pathFrom2[_tcslen(pathFrom2)-1] = 0;
				_tcscat_s(pathFrom2, FindData.cFileName);
				_tcscat_s(pathFrom2, _T("\\*")); 
				
				_tcscpy_s(pathTo2, pathTo);
				_tcscat_s(pathTo2, FindData.cFileName);
				//_tcscat_s(pathTo2, _T("\\")); 
				
				CopyFolder(pathFrom2, pathTo2, ProgressBar);
			}
			else
			{
				TCHAR lpExistingFileName[MAX_PATH];
				TCHAR lpNewFileName[MAX_PATH];

				_tcscpy_s(lpExistingFileName, pathFrom);
				lpExistingFileName[_tcslen(lpExistingFileName) - 1] = 0;
				_tcscat_s(lpExistingFileName, FindData.cFileName);
				_tcscpy_s(lpNewFileName, pathTo);
				_tcscat_s(lpNewFileName, FindData.cFileName);

				if(CopyFileEx(lpExistingFileName, lpNewFileName, CopyProgressRoutineForDir, ProgressBar, (LPBOOL)&cancelCopy, COPY_FILE_FAIL_IF_EXISTS))
					copySize.QuadPart = copySize.QuadPart + ((DWORDLONG)FindData.nFileSizeHigh<<32) + FindData.nFileSizeLow;				
			}
		}
	}
	while(FindNextFile(Handle, &FindData) != 0);
	FindClose(Handle);
	return 1;
}