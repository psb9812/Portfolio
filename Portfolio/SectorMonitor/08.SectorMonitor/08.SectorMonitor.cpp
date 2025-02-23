#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windowsx.h>
#include "framework.h"
#include "08.SectorMonitor.h"
#include "RingBuffer.h"
#include "NetworkProtocol.h"
#include "../../CommonProtocol.h"
#include "Message.h"
#include "CommonDefine.h"
#include <string>

#define MAX_LOADSTRING 100
#define SERVERPORT  3000
#define UM_NETWORK  WM_USER + 1
#define REQUEST_TIMER   1


wchar_t LServerIP[256];
bool isExit = false;
SOCKET g_ServerSocket;
RingBuffer sendQ;
RingBuffer recvQ;

USHORT g_Sectors[MAX_SECTOR_Y][MAX_SECTOR_X];
UINT32 g_totalUserNum;

HPEN g_hGridPen;
HBRUSH g_hRedBrush;
HBRUSH g_hOrangeBrush;
HBRUSH g_hGreenBrush;
HBRUSH g_hBlueBrush;
HBRUSH g_hWhiteBrush;
HFONT g_hFont;

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, const wchar_t* windowClass, const wchar_t* title, int width, int height);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK IPWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void SendProc(HWND hWnd, char* buf, int size);
void WriteEvent(HWND hWnd);
void ReadEvent(HWND hWnd);
void CloseEvent(HWND hWnd);
void ErrorBox(HWND hWnd, const WCHAR* info);
void Render(HDC hdc);
GRID_COLOR GetGridColor(USHORT userNum);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, NULL, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

#pragma region IPWindowClass
    WNDCLASSEXW IPWcex;

    IPWcex.cbSize = sizeof(WNDCLASSEX);

    IPWcex.style = CS_HREDRAW | CS_VREDRAW;
    IPWcex.lpfnWndProc = IPWndProc;
    IPWcex.cbClsExtra = 0;
    IPWcex.cbWndExtra = 0;
    IPWcex.hInstance = hInstance;
    IPWcex.hIcon = LoadIcon(hInstance, NULL);
    IPWcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    IPWcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    IPWcex.lpszMenuName = nullptr;
    IPWcex.lpszClassName = L"IPInputClass";
    IPWcex.hIconSm = LoadIcon(IPWcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassExW(&IPWcex);
#pragma endregion IPWindowClass

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance(hInstance, nCmdShow, L"IPInputClass", L"서버의 IP를 입력하세요", 300, 140))
    {
        return FALSE;
    }


    MSG IPWindowMsg;
    // 기본 메시지 루프입니다:
    while (GetMessage(&IPWindowMsg, nullptr, 0, 0))
    {
        TranslateMessage(&IPWindowMsg);
        DispatchMessage(&IPWindowMsg);
    }

    if (isExit)
        return -1;

    //소켓 설정
    WSAData wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return -1;

    int retOfConnect;
    int retOfAsyncSelect;

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    InetPton(AF_INET, LServerIP, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(SERVERPORT);

    // 애플리케이션 초기화를 수행합니다:
    hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

    HWND hWnd = CreateWindowW(L"그냥 창", szTitle, WS_OVERLAPPEDWINDOW,
        0, 0, 600, 600, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        int err = GetLastError();
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    g_ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_ServerSocket == INVALID_SOCKET)
    {
        __debugbreak();
    }
    retOfAsyncSelect = WSAAsyncSelect(g_ServerSocket, hWnd, UM_NETWORK, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
    if (retOfAsyncSelect != 0)
    {
        ErrorBox(hWnd, L"WSAAsyncSelect()");
    }
    retOfConnect = connect(g_ServerSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (retOfConnect == SOCKET_ERROR)
    {
        if (GetLastError() != WSAEWOULDBLOCK)
        {
            ErrorBox(hWnd, L"connect()");
        }
    }


    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, NULL);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"그냥 창";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, const wchar_t* windowClass, const wchar_t* title, int width, int height)
{
    hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

    HWND hWnd = CreateWindowW(windowClass, title, WS_OVERLAPPEDWINDOW,
        0, 0, width, height, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK IPWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndEdit, hwndConnectButton, hwndCancelButton;

    switch (message)
    {
    case WM_CREATE:
        hwndEdit = CreateWindowEx(
            0, L"EDIT",   // Predefined class
            L"127.0.0.1", // Default text
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, // Styles
            10, 10, 260, 20, // x, y, width, height
            hWnd,        // Parent window
            NULL,        // No menu
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);       // Pointer not needed

        hwndConnectButton = CreateWindow(
            L"BUTTON",  // Predefined class
            L"접속",    // Button text
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
            50, 40, 80, 30, // x, y, width, height
            hWnd,     // Parent window
            (HMENU)1, // Button ID
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);    // Pointer not needed

        hwndCancelButton = CreateWindow(
            L"BUTTON",  // Predefined class
            L"취소",    // Button text
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
            150, 40, 80, 30, // x, y, width, height
            hWnd,     // Parent window
            (HMENU)2, // Button ID
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);    // Pointer not needed
        break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // 메뉴 선택을 구문 분석합니다:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case 1:
            //접속을 누른 경우
            //기존 창을 날리고 그림을 그릴 수 있는 창 생성.

            GetWindowText(hwndEdit, LServerIP, sizeof(LServerIP) / sizeof(wchar_t));
            DestroyWindow(hWnd);
            break;
        case 2:
            //취소를 누른 경우 프로세스를 종료한다.
            DestroyWindow(hWnd);
            isExit = true;
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


bool g_isDrag = false;
int g_oldX, g_oldY, g_curX, g_curY;
HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;
HDC g_hMemDC;
RECT g_MemDCRect;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        g_hGridPen = CreatePen(PS_SOLID, 3, RGB(0, 0, 0));
        g_hRedBrush = CreateSolidBrush(RGB(255, 0, 0));
        g_hOrangeBrush = CreateSolidBrush(RGB(255, 165, 0));
        g_hGreenBrush = CreateSolidBrush(RGB(0, 255, 0));
        g_hBlueBrush = CreateSolidBrush(RGB(0, 0, 255));
        g_hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        g_hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Arial");


        HDC hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);

        //유저 분포 데이터 요청 패킷을 보내는 타이머를 설정한다.
        SetTimer(hWnd, REQUEST_TIMER, 1000, NULL);
    }
        break;
    //윈도우 사이즈 변경 시 메모리 DC 크기 갱신
    case WM_SIZE:
    {
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);

        HDC hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);

    }
        break;
    case WM_LBUTTONDOWN:        //마우스 클릭했을 때
        g_isDrag = true;
        g_curX = GET_X_LPARAM(lParam);
        g_curY = GET_Y_LPARAM(lParam);
        break;
    case WM_LBUTTONUP:          //마우스 뗏을 때
        g_isDrag = false;
        break;
    case WM_MOUSEMOVE:
        break;
    case WM_TIMER:
    {
        Message* pMessage = Message::Alloc();
        *pMessage << (WORD)en_PACKET_CS_CHAT_REQ_USER_DISTRIBUTION;
        pMessage->WriteHeader<Net>((unsigned char)0x77);
        pMessage->Encode(ENCODE_STATIC_KEY);

        //1초에 한 번 요청을 보낸다.
        SendProc(hWnd, (char*)pMessage->GetHeaderPtr(), pMessage->GetDataSize() + pMessage->GetHeaderSize());
        Message::Free(pMessage);
    }
        break;
    case UM_NETWORK:
        //에러 체크
        if (WSAGETSELECTERROR(lParam))
        {
            ErrorBox(hWnd, L"UM_NETWOR ERROR");
        }

        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_CONNECT:
        {
            int error = WSAGETSELECTERROR(lParam);
            if (error != 0) 
            {
                ErrorBox(hWnd, L"CONNECT_ERROR");
            }
        }
            break;
        case FD_WRITE:
            WriteEvent(hWnd);
            break;
        case FD_READ:
            ReadEvent(hWnd);
            break;
        case FD_CLOSE:
            CloseEvent(hWnd);
            break;
        }
        break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // 메뉴 선택을 구문 분석합니다:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        
        //메모리 DC 클리어
        PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);

        //그리고 싶은 것을 메모리 DC에 출력
        Render(g_hMemDC);

        //메모리 DC에 랜더링이 끝나면, 메모리 DC -> 윈도우 DC로 출력
        HDC hdc = BeginPaint(hWnd, &ps);
        BitBlt(hdc, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, g_hMemDC, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);
        DeleteObject(g_hGridPen);
        DeleteObject(g_hRedBrush);
        DeleteObject(g_hOrangeBrush);
        DeleteObject(g_hGreenBrush);
        DeleteObject(g_hBlueBrush);
        DeleteObject(g_hWhiteBrush);
        DeleteObject(g_hFont);

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
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

void SendProc(HWND hWnd, char* buf, int size)
{
    int retOfEnqueue = 0;
    //구조적 통일성을 위해 무조건 보낼 것은 send큐에 넣고 빼면서 보낸다.
    retOfEnqueue = sendQ.Enqueue(buf, size);
    //이하의 WriteEvemt()랑 같은 로직
    WriteEvent(hWnd);
    //Wouldblock이 안 뜬다면 sendQ에 있는 것을 모두 다 비운다.
    //왜냐하면 지금 안하면 보낼 수 없기 때문이다.
}
void WriteEvent(HWND hWnd)
{
    int retOfSend = 0;

    if (sendQ.GetUseSize() > sendQ.DirectDequeueSize()) //데이터가 경계에 걸쳐있는 상황
    {
        retOfSend = send(g_ServerSocket, sendQ.GetReadBufferPtr(), sendQ.DirectDequeueSize(), 0);
        if (retOfSend == SOCKET_ERROR)
        {
            if (GetLastError() != WSAEWOULDBLOCK)
            {
                CloseEvent(hWnd);
                return;
            }
        }
        sendQ.MoveRead(retOfSend);
        retOfSend = send(g_ServerSocket, sendQ.GetReadBufferPtr(), sendQ.GetUseSize(), 0);
        if (retOfSend == SOCKET_ERROR)
        {
            if (GetLastError() != WSAEWOULDBLOCK)
            {
                CloseEvent(hWnd);
                return;
            }
        }
        sendQ.MoveRead(retOfSend);
    }
    else
    {
        retOfSend = send(g_ServerSocket, sendQ.GetReadBufferPtr(), sendQ.GetUseSize(), 0);
        if (retOfSend == SOCKET_ERROR)
        {
            if (GetLastError() != WSAEWOULDBLOCK)
            {
                CloseEvent(hWnd);
                return;
            }
        }
        sendQ.MoveRead(retOfSend);
    }

}

void ReadEvent(HWND hWnd)
{
    HDC hDC;
    int retOfRecv = 0;
    //1. 먼저 recv한 번으로 링버퍼로 데이터 긁어온다.
    retOfRecv = recv(g_ServerSocket, recvQ.GetWriteBufferPtr(), recvQ.DirectEnqueueSize(), 0);
    if (retOfRecv == SOCKET_ERROR || retOfRecv == 0)
    {
        CloseEvent(hWnd);
        return;
    }
    recvQ.MoveWrite(retOfRecv);

    //2. 반복문 돌면서 메시지를 받으면서 처리한다.
    while (recvQ.GetUseSize() > sizeof(NetHeader))
    {
        NetHeader header;
        recvQ.Peek((char*)&header, sizeof(NetHeader));

        //헤더의 길이만큼 데이터가 없다면 반복문 탈출
        if (header.len > recvQ.GetUseSize() - sizeof(NetHeader))
            break;

        recvQ.MoveRead(sizeof(NetHeader));

        Message* pMessage = Message::Alloc();
        //레퍼런스 카운트 수동 증가
        pMessage->AddRefCount();
        //인코딩 되어있음을 명시함.
        pMessage->SetIsEncode(true);
        //버퍼에 있는 값을 직렬화 버퍼에 넣기
        recvQ.Dequeue(pMessage->GetBufferPtr(), header.len);
        pMessage->MoveWritePos(header.len);

        //체크섬도 디코딩의 대상이므로 직렬화 버퍼에 헤더부분에 직접 넣는다.
        NetHeader* pMessageHeader = reinterpret_cast<NetHeader*>(pMessage->GetHeaderPtr());
        pMessageHeader->checkSum = header.checkSum;

        //메시지 복호화
        bool decodeOK = pMessage->Decode(header.randKey, ENCODE_STATIC_KEY);
        if (!decodeOK)
        {
            ErrorBox(hWnd, L"디코드 실패");
        }
        
        WORD type = 0;

        *pMessage >> type;
        if (type != en_PACKET_CS_CHAT_RES_USER_DISTRIBUTION)
        {
            ErrorBox(hWnd, L"알 수 없는 타입");
        }
        
        for (int i = 0; i < MAX_SECTOR_Y; i++)
        {
            //전역 섹터 배열에 수신 받은 데이터를 그대로 ㄴㅓd
            pMessage->GetData((char*)g_Sectors[i], MAX_SECTOR_X * sizeof(USHORT));
        }
        Message::Free(pMessage);

        g_totalUserNum = 0;
        //총 유저 수 구하기
        for (int y = 0; y < MAX_SECTOR_Y; y++)
        {
            for (int x = 0; x < MAX_SECTOR_X; x++)
            {
                g_totalUserNum += g_Sectors[y][x];
            }
        }
        

        //그린다.
        hDC = GetDC(hWnd);
        InvalidateRect(hWnd, NULL, true);
        ReleaseDC(hWnd, hDC);
    }
}

void CloseEvent(HWND hWnd)
{
    MessageBox(hWnd, L"연결이 종료되었습니다.", L"연결 종료", MB_OK);
    PostQuitMessage(0);
}

void ErrorBox(HWND hWnd, const WCHAR* info)
{
    int errorCode = GetLastError();
    WCHAR errBuf[126];
    wsprintf(errBuf, L"에러코드 : %d\n %s", errorCode, info);
    MessageBox(hWnd, errBuf, L"에러 발생", MB_OK);
}

void Render(HDC hdc)
{
    int x = 0;
    int y = 0;
    HPEN hOldPen = (HPEN)SelectObject(hdc, g_hGridPen);

    //그리드 그리기
    for (int w = 0; w <= MAX_SECTOR_X; w++)
    {
        MoveToEx(hdc, x, 0, NULL);
        LineTo(hdc, x, MAX_SECTOR_Y * GRID_SIZE);
        x += GRID_SIZE;
    }
    for (int h = 0; h <= MAX_SECTOR_Y; h++)
    {
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, MAX_SECTOR_X * GRID_SIZE, y);
        y += GRID_SIZE;
    }

    SelectObject(hdc, hOldPen);

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, g_hGreenBrush);
    x = 0;
    y = 0;
    //칸에 색과 수치 칠하기
    for (int h = 0; h < MAX_SECTOR_Y; h++)
    {
        for (int w = 0; w < MAX_SECTOR_X; w++)
        {
            x = w * GRID_SIZE;
            y = h * GRID_SIZE;
            switch (GetGridColor(g_Sectors[h][w]))
            {
            case RED:
                SelectObject(hdc, g_hRedBrush);
                break;
            case ORANGE:
                SelectObject(hdc, g_hOrangeBrush);
                break;
            case GREEN:
                SelectObject(hdc, g_hGreenBrush);
                break;
            case BLUE:
                SelectObject(hdc, g_hBlueBrush);
                break;
            case WHITE:
                SelectObject(hdc, g_hWhiteBrush);
                break;
            default:
                break;
            }
            //테두리가 있으므로 +2 한다.
            Rectangle(hdc, x, y, x + GRID_SIZE + 2, y + GRID_SIZE + 2);

            SelectObject(hdc, g_hFont);

            // 배경 투명 설정
            SetBkMode(hdc, TRANSPARENT);

            // 숫자 출력 (칸 중심)
            std::wstring number = std::to_wstring(g_Sectors[h][w]);
            int textX = x + GRID_SIZE / 4;  // 중앙 정렬 보정
            int textY = y + GRID_SIZE / 4;
            TextOut(hdc, textX, textY, number.c_str(), number.length());
        }
    }

    //총 인원과 기준 명시
    WCHAR totalUserStr[20] = { 0, };
    wsprintf(totalUserStr, L"접속 유저 수 : %d", g_totalUserNum);
    TextOut(hdc, 2, MAX_SECTOR_X * GRID_SIZE + 10, totalUserStr, wcslen(totalUserStr));
}

GRID_COLOR GetGridColor(USHORT userNum)
{
    if (userNum == 0)
    {
        return WHITE;
    }
    else if (0 < userNum && userNum <= 6)
    {
        return BLUE;
    }
    else if (6 < userNum && userNum <= 12)
    {
        return GREEN;
    }
    else if (12 < userNum && userNum <= 16)
    {
        return ORANGE;
    }
    else
    {
        return RED;
    }
}