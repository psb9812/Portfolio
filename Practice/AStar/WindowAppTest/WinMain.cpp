#define _CRT_SECURE_NO_WARNINGS
#include <queue>
#include <list>
#include <vector>
#include <algorithm>
#include <string>
#include "framework.h"
#include "WindowAppTest.h"
#include "windowsx.h"
#include "Node.h"
#include "Point.h"
#include "AStarAlgorithm.h"
#include "CAMapGenerator.h"
#define PROFILE
#include "Profiler.h"

#define MAX_LOADSTRING 100
//더블 버퍼링 관련 전역 변수
HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;
HDC g_hMemDC;
RECT g_MemDCRect;

//그리드 관련 전역 변수
int g_GridSize = 16;
const int g_GridWidth = 100;
const int g_GridHeight = 100;

HBRUSH g_hTileBrush;
HPEN g_hGridPen;

Node g_Tile[g_GridHeight][g_GridWidth];

CAMapGenerator g_mapGenerator(g_Tile, GRID_WIDTH, GRID_HEIGHT, 4, 4, 2);
AStarAlgorithm g_aStarAlgorithm;


bool g_bErase = false;
bool g_bDrag = false;

void RenderGrid(HDC hdc);
void RenderObstacle(HDC hdc);
void RenderStartEndPos(HDC hdc);
void RenderMenual(HDC hdc);

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    //노드 배열 초기화
    for (int iCntH = 0; iCntH < g_GridHeight; iCntH++)
    {
        for (int iCntW = 0; iCntW < g_GridWidth; iCntW++)
        {
            g_Tile[iCntH][iCntW].x = iCntW;
            g_Tile[iCntH][iCntW].y = iCntH;
        }
    }

    // TODO: 여기에 코드를 입력합니다.
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWAPPTEST));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWAPPTEST);
    wcex.lpszClassName = L"testClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"testClass", L"길찾기 알고리즘", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}
//wasd로 움직일때 보정값
int g_gridPosX = 0;
int g_gridPosY = 0;

int g_gridWidthLength = g_GridWidth * g_GridSize;
int g_gridHeightLength = g_GridHeight * g_GridSize;

Point g_startPos;//출발점 좌표
Point g_endPos;//도착점 좌표
int cursorX;
int cursorY;

bool g_isChoiceStartPos;
bool g_isChoiceEndPos;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_CREATE:
    {
        hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_gridWidthLength, g_gridHeightLength);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);

        g_hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        g_hTileBrush = CreateSolidBrush(RGB(100, 100, 100));
    }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_SIZE:
    {
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);

        hdc = GetDC(hWnd);

        g_gridWidthLength = g_GridWidth * g_GridSize;
        g_gridHeightLength = g_GridHeight * g_GridSize;
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_gridWidthLength, g_gridHeightLength);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);

        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);
    }
        break;
    case WM_LBUTTONDOWN:
        g_bDrag = true;
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            int iTileX = xPos / g_GridSize + g_gridPosX;
            int iTileY = yPos / g_GridSize + g_gridPosY;

            if (g_Tile[iTileY][iTileX].isObstacle == true)
                g_bErase = true;
            else
                g_bErase = false;
        }
        break;
    case WM_MBUTTONDOWN:
        //출발점 지정
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        int iTileX = xPos / g_GridSize + g_gridPosX;
        int iTileY = yPos / g_GridSize + g_gridPosY;

        g_startPos.x = iTileX;
        g_startPos.y = iTileY;

        g_isChoiceStartPos = true;
        InvalidateRect(hWnd, NULL, false);
    }
        break;
    case WM_RBUTTONDOWN:
        //도착점 지정
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        int iTileX = xPos / g_GridSize + g_gridPosX;
        int iTileY = yPos / g_GridSize + g_gridPosY;

        g_endPos.x = iTileX;
        g_endPos.y = iTileY;
        g_isChoiceEndPos = true;
        InvalidateRect(hWnd, NULL, false);
    }
        break;
    case WM_LBUTTONUP:
        g_bDrag = false;
        break;
    case WM_MOUSEWHEEL:
    {
        //그리드 사이즈를 변경하기 전에 커서의 위치와 커서가 올라가 있는 타일의 인덱스를 저장함.
        int cursorTileXBeforZoom = cursorX / g_GridSize + g_gridPosX;
        int cursorTileYBeforZoom = cursorY / g_GridSize + g_gridPosY;

        //휠이 조금 움직일 때마다 120이 변경됨.
        //너무 많이 변경되므로 4만큼 바뀌게 했음
        int delta = GET_WHEEL_DELTA_WPARAM(wParam) /30 ;
        
        g_GridSize += delta;
        g_GridSize = max(12, g_GridSize);

        //줌을 했을 떄(확대 혹은 축소) 기존의 커서가 있던 타일의 위치가 달라지면 기존의 타일 위에 있도록 조정해줌.
        int cursorTileXAfterZoom = cursorX / g_GridSize + g_gridPosX;
        int cursorTileYAfterZoom = cursorY / g_GridSize + g_gridPosY;


        int correctionValueOfX = (cursorTileXBeforZoom - cursorTileXAfterZoom);   //X축으로 몇 칸 이동시켜줘야 하는지를 판단하는 보정치
        g_gridPosX += correctionValueOfX;
        int correctionValueOfY = (cursorTileYBeforZoom - cursorTileYAfterZoom);   //Y축으로 몇 칸 이동시켜줘야 하는지를 판단하는 보정치
        g_gridPosY += correctionValueOfY;

        g_gridPosY = max(0, g_gridPosY);
        g_gridPosX = max(0, g_gridPosX);
        g_gridPosY = min(g_GridHeight - g_MemDCRect.bottom / g_GridSize, g_gridPosY);
        g_gridPosX = min(g_GridWidth - g_MemDCRect.right / g_GridSize, g_gridPosX);

        //바뀐 그리드 사이즈 만큼 다시 비트맵을 그려준다.
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);

        hdc = GetDC(hWnd);

        g_gridWidthLength = g_GridWidth * g_GridSize;
        g_gridHeightLength = g_GridHeight * g_GridSize;
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_gridWidthLength, g_gridHeightLength);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);

        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);

        InvalidateRect(hWnd, NULL, false);
    }
        break;
    case WM_MOUSEMOVE:
        {
            cursorX = GET_X_LPARAM(lParam);
            cursorY = GET_Y_LPARAM(lParam);
            if (g_bDrag)
            {
                int iTileX = cursorX / g_GridSize + g_gridPosX;
                int iTileY = cursorY / g_GridSize + g_gridPosY;

                g_Tile[iTileY][iTileX].isObstacle = !g_bErase;
                InvalidateRect(hWnd, NULL, false);
            }
        }
        break;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case 'W':
            g_gridPosY = max(0, g_gridPosY - 1);
            break;
        case 'A':
            g_gridPosX = max(0, g_gridPosX - 1);
            break;
        case 'S':
            g_gridPosY = min(g_GridHeight - g_MemDCRect.bottom / g_GridSize, g_gridPosY + 1);
            break;
        case 'D':
            g_gridPosX = min(g_GridWidth - g_MemDCRect.right / g_GridSize, g_gridPosX + 1);
            break;
        case ' ':
            if (g_isChoiceEndPos && g_isChoiceStartPos)
            {
                if (!g_aStarAlgorithm.isFinding())
                {
                    HDC hdc = GetDC(hWnd);
                    g_aStarAlgorithm.PathFindAll(hWnd, hdc, g_Tile, g_GridSize, g_gridPosX, g_gridPosY, g_startPos, g_endPos);
                    ReleaseDC(hWnd, hdc);
                }
            }
            break;
        case 'Z':
            if (g_isChoiceEndPos && g_isChoiceStartPos)
            {
                HDC hdc = GetDC(hWnd);
                g_aStarAlgorithm.PathFind(hWnd, hdc, g_Tile, g_GridSize, g_gridPosX, g_gridPosY, g_startPos, g_endPos);
                ReleaseDC(hWnd, hdc);
            }
            break;
        case 'N':
            if (g_isChoiceEndPos && g_isChoiceStartPos)
            {
                {
                    START_PROFILEING(aStarAlgorithm, "aStarAlgorithm");
                    g_aStarAlgorithm.PathFindNoRender(g_Tile, g_startPos, g_endPos);
                }
            }
            break;
        case 'R':
            //시작, 도착점 리셋
            g_isChoiceEndPos = false;
            g_isChoiceStartPos = false;
            break;
        case 'H':
            ProfileDataOutText(_T("AStarAlgoritmProfile.txt"));
            break;
        case 'M':
            g_mapGenerator.GenerateMap();
            break;
        }

        InvalidateRect(hWnd, NULL, false);
        break;
    case WM_PAINT:
        {
            if (g_aStarAlgorithm.isFinding())
            {
                //break;
            }
            int startXPos = g_gridPosX * g_GridSize;
            int startYPos = g_gridPosY * g_GridSize;

            PatBlt(g_hMemDC, 0, 0, g_gridWidthLength, g_gridHeightLength, WHITENESS);
            RenderObstacle(g_hMemDC);

            if (g_isChoiceEndPos || g_isChoiceStartPos)
            {
                RenderStartEndPos(g_hMemDC);
            }
            if (g_aStarAlgorithm.isFinding())
            {
                g_aStarAlgorithm.Render(g_hMemDC, g_Tile, g_GridSize, g_gridPosX, g_gridPosY);
            }
                
            RenderGrid(g_hMemDC); 

            hdc = BeginPaint(hWnd, &ps);

            BitBlt(hdc, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom,
                g_hMemDC, startXPos, startYPos, SRCCOPY);

            //RenderMenual(hdc);

            EndPaint(hWnd, &ps);
            ReleaseDC(hWnd, hdc);
        }
        break;
    case WM_DESTROY:
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);
        DeleteObject(g_hTileBrush);
        DeleteObject(g_hGridPen);
        
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

void RenderGrid(HDC hdc)
{
    int iX = 0;
    int iY = 0;
    HPEN hOldPen = (HPEN)SelectObject(hdc, g_hGridPen);
    
    for (int iCntW = 0; iCntW <= g_GridWidth; iCntW++)
    {
        MoveToEx(hdc, iX, 0, NULL);
        LineTo(hdc, iX, g_GridHeight * g_GridSize);
        iX += g_GridSize;
    }
    for (int iCntH = 0; iCntH <= g_GridHeight; iCntH++)
    {
        MoveToEx(hdc, 0, iY, NULL);
        LineTo(hdc, g_GridWidth * g_GridSize, iY);
        iY += g_GridSize;
    }
    SelectObject(hdc, hOldPen);
}

void RenderObstacle(HDC hdc)
{
    int iX = 0;
    int iY = 0;
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, g_hTileBrush);
    SelectObject(hdc, GetStockObject(NULL_PEN));

    for (int iCntW = 0; iCntW < g_GridWidth; iCntW++)
    {
        for (int iCntH = 0; iCntH < g_GridHeight; iCntH++)
        {
            if (g_Tile[iCntH][iCntW].isObstacle)
            {
                iX = iCntW * g_GridSize;
                iY = iCntH * g_GridSize;
                Rectangle(hdc, iX, iY, iX + g_GridSize + 2, iY + g_GridSize + 2);
            }
        }
    }
    SelectObject(hdc, hOldBrush);
}

void RenderStartEndPos(HDC hdc)
{
    if (g_isChoiceStartPos)
    {
        HBRUSH startBrush = CreateSolidBrush(RGB(255, 0, 0));

        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, startBrush);
        int startPosX = g_startPos.x * g_GridSize;
        int startPosY = g_startPos.y * g_GridSize;
        Rectangle(hdc, startPosX, startPosY, startPosX + g_GridSize + 2, startPosY + g_GridSize + 2);

        SelectObject(hdc, hOldBrush);
        DeleteObject(startBrush);
    }
    if (g_isChoiceEndPos)
    {
        HBRUSH endBrush = CreateSolidBrush(RGB(0, 0, 255));

        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, endBrush);
        int endPosX = g_endPos.x * g_GridSize;
        int endPosY = g_endPos.y * g_GridSize;
        Rectangle(hdc, endPosX, endPosY, endPosX + g_GridSize + 2, endPosY + g_GridSize + 2);

        SelectObject(hdc, hOldBrush);
        DeleteObject(endBrush);
    }
}

void RenderMenual(HDC hdc)
{
    TextOut(hdc, 1000, 5, L"휠 버튼 클릭 : 출발점 설정", 16);
    TextOut(hdc, 1000, 25, L"오른쪽 버튼 클릭 : 도착점 설정", 18);
    TextOut(hdc, 1000, 45, L"스페이스 바 : 길찾기 시작", 15);
    TextOut(hdc, 1000, 65, L"엔터키 : 길찾기 종료", 12);

    std::wstring cursorXStr = std::to_wstring(cursorX /g_GridSize + g_gridPosX);
    TextOut(hdc, 1000, 85, cursorXStr.c_str(), cursorXStr.length());
    std::wstring cursorYStr = std::to_wstring(cursorY / g_GridSize + g_gridPosY);
    TextOut(hdc, 1000, 105, cursorYStr.c_str(), cursorYStr.length());

}