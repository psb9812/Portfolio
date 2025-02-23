#include "AStarAlgorithm.h"
#include "Node.h"
#include "Point.h"

AStarAlgorithm::AStarAlgorithm()
    :_isFinding(false), _maxSearchCount(1000000), _searchCount(0)
{
    _closeListTileBrush = CreateSolidBrush(RGB(255, 255, 0));
    _openListTileBrush = CreateSolidBrush(RGB(0, 255, 0));
    _redPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    _finalRoutePen = CreatePen(PS_SOLID, 2, RGB(128, 0, 128));
}

AStarAlgorithm::~AStarAlgorithm()
{
    DeleteObject(_openListTileBrush);
    DeleteObject(_closeListTileBrush);
    DeleteObject(_redPen);
    DeleteObject(_finalRoutePen);
}

void AStarAlgorithm::PathFind(HWND hWnd, HDC hdc, Node(&tileMap)[GRID_HEIGHT][GRID_WIDTH], int gridSize, int gridPosX, int gridPosY, Point startPos, Point endPos)
{
    if (!_isFinding)
    {
        _isFinding = true;

        Node startNode = tileMap[startPos.y][startPos.x];
        startNode.h = (abs(startPos.x - endPos.x) + abs(startPos.y - endPos.y)) * 10;
        startNode.w = startNode.h + startNode.g;

        //시작점 openList에 넣고 길찾기 시작
        _openList.push(&tileMap[startPos.y][startPos.x]);
        _openList_Point.push_back(Point{ startNode.x, startNode.y });
    }
    else
    {
        _searchCount++;
        if (_searchCount >= _maxSearchCount || _openList.empty())
        {
            PathFindEndProcess();
            return;
        }

        //오픈 리스트에서 w가 가장 작은 값을 하나 뽑는다.
        Node* curNode = _openList.top();
        _openList.pop();
        _closeList.push_back(curNode); //openList에서 꺼낸 노드는 closeList에 넣어준다.
        _openList_Point.erase(std::remove(_openList_Point.begin(), _openList_Point.end(), Point{ curNode->x, curNode->y }), _openList_Point.end());

        if (curNode->x == endPos.x && curNode->y == endPos.y)
        {
            //목적지를 찾은 경우
            //도착지에서 역으로 최단거리 표시하기
            while (curNode->parent != NULL)
            {
                HPEN oldPen = (HPEN)SelectObject(hdc, _finalRoutePen);
                //부모 노드에 화살표 표시하기
                MoveToEx(hdc, (curNode->x - gridPosX) * gridSize + gridSize / 2, (curNode->y - gridPosY) * gridSize + gridSize / 2, NULL);
                LineTo(hdc, (curNode->parent->x - gridPosX) * gridSize + gridSize / 2, (curNode->parent->y - gridPosY) * gridSize + gridSize / 2);
                SelectObject(hdc, oldPen);
                curNode = curNode->parent;
            }
            PathFindEndProcess();
            Sleep(1000);
            return;
        }
        //꺼낸 노드의 8방향으로 이동할 수 있는 노드를 openList에 넣는다.
        for (int idx = 0; idx < 8; idx++)
        {
            bool isSkip = false;
            int nextNodeXIdx = curNode->x + dx[idx];
            int nextNodeYIdx = curNode->y + dy[idx];
            if (nextNodeYIdx < 0 || nextNodeYIdx >= GRID_HEIGHT || nextNodeXIdx < 0 || nextNodeXIdx >= GRID_WIDTH)
                continue;
            Node* nextNode = &tileMap[nextNodeYIdx][nextNodeXIdx];

            if (nextNode->isObstacle == true)  continue;     //장애물이라면 스킵
            //openList에서 동일한 위치의 값이 있는지 확인 후 존재한다면 스킵
            for (std::list<Point>::iterator iter = _openList_Point.begin(); iter != _openList_Point.end(); iter++)
            {
                if (nextNode->x == iter->x && nextNode->y == iter->y)
                {
                    int gValueFromcurNode;

                    if (abs(dx[idx]) + abs(dy[idx]) == 2)
                        gValueFromcurNode = curNode->g + 14;
                    else
                        gValueFromcurNode = curNode->g + 10;

                    if (gValueFromcurNode < tileMap[iter->y][iter->x].g)
                    {
                        nextNode->parent = curNode;
                        nextNode->g = gValueFromcurNode;
                        nextNode->w = nextNode->g + nextNode->h;

                        _openList.push(nextNode);
                    }
                    isSkip = true;
                    break;
                }
            }
            //closeList에서 동일한 위치의 값이 있는지 확인 후 존재한다면 스킵
            for (std::list<Node*>::iterator iter = _closeList.begin(); iter != _closeList.end(); iter++)
            {
                if (nextNode->x == (*iter)->x && nextNode->y == (*iter)->y)
                {
                    isSkip = true;
                    break;
                }
            }
            if (isSkip) continue;

            int xLengthOfStartPosToNode = abs(startPos.x - nextNode->x);
            int yLengthOfStartPosToNode = abs(startPos.y - nextNode->y);
            //뽑은 노드와의 유클리드 거리
            if (abs(dx[idx]) + abs(dy[idx]) == 2)
            {
                //대각선으로 갔다면
                nextNode->g = curNode->g + 14;
            }
            else
            {
                nextNode->g = curNode->g + 10;
            }
            //종료지점과의 맨해튼 거리
            nextNode->h = (abs(endPos.x - nextNode->x) + abs(endPos.y - nextNode->y)) * 100;
            //둘의 합(h에 10배를 해줘서 목표지점과 가까운 노드를 더 많이 선호하게 한다.
            nextNode->w = nextNode->g + nextNode->h;
            nextNode->parent = curNode;

            //스킵되지 않았으면 openList에 추가한다.
            _openList.push(nextNode);
            _openList_Point.push_back(Point{ nextNode->x, nextNode->y });
        }
    }
}

void AStarAlgorithm::PathFindAll(HWND hWnd, HDC hdc, Node(&tileMap)[GRID_HEIGHT][GRID_WIDTH], int gridSize, int gridPosX, int gridPosY, Point startPos, Point endPos)
{
    _isFinding = true;
    //시작 노드 세팅
    Node startNode = tileMap[startPos.y][startPos.x];
    startNode.h = (abs(startPos.x - endPos.x) + abs(startPos.y - endPos.y)) * 10;
    startNode.w = startNode.h + startNode.g;

    //시작점 openList에 넣고 길찾기 시작
    _openList.push(&tileMap[startPos.y][startPos.x]);
    _openList_Point.push_back(Point{ startNode.x, startNode.y });

    while (!_openList.empty())
    {
        _searchCount++;
        if (_searchCount >= _maxSearchCount)
        {
            break;
        }

        //오픈 리스트에서 w가 가장 작은 값을 하나 뽑는다.
        Node* curNode = _openList.top();
        _openList.pop();
        _closeList.push_back(curNode); //openList에서 꺼낸 노드는 closeList에 넣어준다.
        _openList_Point.erase(std::remove(_openList_Point.begin(), _openList_Point.end(), Point{ curNode->x, curNode->y }), _openList_Point.end());

        if (curNode->x == endPos.x && curNode->y == endPos.y)
        {
            //목적지를 찾은 경우
            //도착지에서 역으로 최단거리 표시하기
            while (curNode->parent != NULL)
            {
                HPEN oldPen = (HPEN)SelectObject(hdc, _finalRoutePen);
                //부모 노드에 화살표 표시하기
                MoveToEx(hdc, (curNode->x - gridPosX) * gridSize + gridSize / 2, (curNode->y - gridPosY) * gridSize + gridSize / 2, NULL);
                LineTo(hdc, (curNode->parent->x - gridPosX) * gridSize + gridSize / 2, (curNode->parent->y - gridPosY) * gridSize + gridSize / 2);
                SelectObject(hdc, oldPen);
                curNode = curNode->parent;
            }
            break;
        }
        //꺼낸 노드의 8방향으로 이동할 수 있는 노드를 openList에 넣는다.
        for (int idx = 0; idx < 8; idx++)
        {
            bool isSkip = false;
            int nextNodeXIdx = curNode->x + dx[idx];
            int nextNodeYIdx = curNode->y + dy[idx];
            if (nextNodeYIdx < 0 || nextNodeYIdx >= GRID_HEIGHT || nextNodeXIdx < 0 || nextNodeXIdx >= GRID_WIDTH)
                continue;
            Node* nextNode = &tileMap[nextNodeYIdx][nextNodeXIdx];

            if (nextNode->isObstacle == true)  continue;     //장애물이라면 스킵
            //openList에서 동일한 위치의 값이 있는지 확인 후 존재한다면 스킵
            for (std::list<Point>::iterator iter = _openList_Point.begin(); iter != _openList_Point.end(); iter++)
            {
                if (nextNode->x == iter->x && nextNode->y == iter->y)
                {
                    int gValueFromcurNode;

                    if (abs(dx[idx]) + abs(dy[idx]) == 2)
                        gValueFromcurNode = curNode->g + 14;
                    else
                        gValueFromcurNode = curNode->g + 10;

                    if (gValueFromcurNode < tileMap[iter->y][iter->x].g)
                    {
                        nextNode->parent = curNode;
                        nextNode->g = gValueFromcurNode;
                        nextNode->w = nextNode->g + nextNode->h;

                        _openList.push(nextNode);
                    }
                    isSkip = true;
                    break;
                }
            }
            //closeList에서 동일한 위치의 값이 있는지 확인 후 존재한다면 스킵
            for (std::list<Node*>::iterator iter = _closeList.begin(); iter != _closeList.end(); iter++)
            {
                if (nextNode->x == (*iter)->x && nextNode->y == (*iter)->y)
                {
                    isSkip = true;
                    break;
                }
            }
            if (isSkip) continue;

            int xLengthOfStartPosToNode = abs(startPos.x - nextNode->x);
            int yLengthOfStartPosToNode = abs(startPos.y - nextNode->y);
            //뽑은 노드와의 유클리드 거리
            if (abs(dx[idx]) + abs(dy[idx]) == 2)
            {
                //대각선으로 갔다면
                nextNode->g = curNode->g + 14;
            }
            else
            {
                nextNode->g = curNode->g + 10;
            }
            //종료지점과의 맨해튼 거리
            nextNode->h = (abs(endPos.x - nextNode->x) + abs(endPos.y - nextNode->y)) * 100;
            //둘의 합h에 10배를 해줘서 목표지점과 가까운 노드를 더 많이 선호하게 한다.
            nextNode->w = nextNode->g + nextNode->h;
            nextNode->parent = curNode;

            //스킵되지 않았으면 openList에 추가한다.
            _openList.push(nextNode);
            _openList_Point.push_back(Point{ nextNode->x, nextNode->y });

            InvalidateRect(hWnd, NULL, false);
            UpdateWindow(hWnd);
        }
        Sleep(50); //보여줘야 하기 때문에 대기
    }
    TextOut(hdc, 0, 0, L"길찾기 종료", 6);
    PathFindEndProcess();
    Sleep(4000);
}

void AStarAlgorithm::PathFindNoRender(Node(&tileMap)[GRID_HEIGHT][GRID_WIDTH], Point startPos, Point endPos)
{
    //시작 노드 세팅
    Node startNode = tileMap[startPos.y][startPos.x];
    startNode.h = (abs(startPos.x - endPos.x) + abs(startPos.y - endPos.y)) * 10;
    startNode.w = startNode.h + startNode.g;

    //시작점 openList에 넣고 길찾기 시작
    _openList.push(&tileMap[startPos.y][startPos.x]);
    _openList_Point.push_back(Point{ startNode.x, startNode.y });

    while (!_openList.empty())
    {
        _searchCount++;
        if (_searchCount >= _maxSearchCount)
        {
            break;
        }

        //오픈 리스트에서 w가 가장 작은 값을 하나 뽑는다.
        Node* curNode = _openList.top();
        _openList.pop();
        _closeList.push_back(curNode); //openList에서 꺼낸 노드는 closeList에 넣어준다.
        _openList_Point.erase(std::remove(_openList_Point.begin(), _openList_Point.end(), Point{ curNode->x, curNode->y }), _openList_Point.end());

        if (curNode->x == endPos.x && curNode->y == endPos.y)
        {
            //목적지를 찾은 경우
            //도착지에서 역으로 최단거리 표시하기
            while (curNode->parent != NULL)
            {
                curNode = curNode->parent;
            }
            break;
        }
        //꺼낸 노드의 8방향으로 이동할 수 있는 노드를 openList에 넣는다.
        for (int idx = 0; idx < 8; idx++)
        {
            bool isSkip = false;
            int nextNodeXIdx = curNode->x + dx[idx];
            int nextNodeYIdx = curNode->y + dy[idx];
            if (nextNodeYIdx < 0 || nextNodeYIdx >= GRID_HEIGHT || nextNodeXIdx < 0 || nextNodeXIdx >= GRID_WIDTH)
                continue;
            Node* nextNode = &tileMap[nextNodeYIdx][nextNodeXIdx];

            if (nextNode->isObstacle == true)  continue;     //장애물이라면 스킵
            //openList에서 동일한 위치의 값이 있는지 확인 후 존재한다면 스킵
            for (std::list<Point>::iterator iter = _openList_Point.begin(); iter != _openList_Point.end(); iter++)
            {
                if (nextNode->x == iter->x && nextNode->y == iter->y)
                {
                    int gValueFromcurNode;

                    if (abs(dx[idx]) + abs(dy[idx]) == 2)
                        gValueFromcurNode = curNode->g + 14;
                    else
                        gValueFromcurNode = curNode->g + 10;

                    if (gValueFromcurNode < tileMap[iter->y][iter->x].g)
                    {
                        nextNode->parent = curNode;
                        nextNode->g = gValueFromcurNode;
                        nextNode->w = nextNode->g + nextNode->h;

                        _openList.push(nextNode);
                    }
                    isSkip = true;
                    break;
                }
            }
            //closeList에서 동일한 위치의 값이 있는지 확인 후 존재한다면 스킵
            for (std::list<Node*>::iterator iter = _closeList.begin(); iter != _closeList.end(); iter++)
            {
                if (nextNode->x == (*iter)->x && nextNode->y == (*iter)->y)
                {
                    isSkip = true;
                    break;
                }
            }
            if (isSkip) continue;

            int xLengthOfStartPosToNode = abs(startPos.x - nextNode->x);
            int yLengthOfStartPosToNode = abs(startPos.y - nextNode->y);
            //뽑은 노드와의 유클리드 거리
            if (abs(dx[idx]) + abs(dy[idx]) == 2)
            {
                //대각선으로 갔다면
                nextNode->g = curNode->g + 14;
            }
            else
            {
                nextNode->g = curNode->g + 10;
            }
            //종료지점과의 맨해튼 거리
            nextNode->h = (abs(endPos.x - nextNode->x) + abs(endPos.y - nextNode->y)) * 100;
            //둘의 합(h에 10배를 해줘서 목표지점과 가까운 노드를 더 많이 선호하게 한다.
            nextNode->w = nextNode->g + nextNode->h;
            nextNode->parent = curNode;

            //스킵되지 않았으면 openList에 추가한다.
            _openList.push(nextNode);
            _openList_Point.push_back(Point{ nextNode->x, nextNode->y });

        }
    }
    PathFindEndProcess();
}


bool AStarAlgorithm::isFinding() const
{
    return _isFinding;
}

void AStarAlgorithm::PathFindEndProcess()
{
    //사용한 자료구조 비우기
    while (!_openList.empty()) {
        _openList.pop();
    }
    _openList_Point.clear();
    _closeList.clear();

    _isFinding = false;
}

void AStarAlgorithm::Render(HDC hdc, Node(&tileMap)[GRID_HEIGHT][GRID_WIDTH], int gridSize, int gridPosX, int gridPosY)
{
    //오픈 리스트의 노드 색칠
    SelectObject(hdc, _openListTileBrush);
    for (auto iter = _openList_Point.begin(); iter != _openList_Point.end(); iter++)
    {
        Rectangle(hdc, (iter->x) * gridSize, (iter->y) * gridSize,
            (iter->x + 1) * gridSize, (iter->y + 1) * gridSize);
    }
    //클로즈 리스트의 노드 색칠
    SelectObject(hdc, _closeListTileBrush);
    for (auto iter = _closeList.begin(); iter != _closeList.end(); iter++)
    {
        Rectangle(hdc, ((*iter)->x) * gridSize, ((*iter)->y) * gridSize,
            ((*iter)->x + 1) * gridSize, ((*iter)->y + 1) * gridSize);
    }

    
    HPEN oldPen = (HPEN)SelectObject(hdc, _redPen);
    for (auto iter = _openList_Point.begin(); iter != _openList_Point.end(); iter++)
    {
        if (tileMap[iter->y][iter->x].parent == NULL) continue;
        //부모 노드에 화살표 표시하기
        MoveToEx(hdc, (iter->x) * gridSize + gridSize / 2, (iter->y) * gridSize + gridSize / 2, NULL);
        LineTo(hdc, ((tileMap[iter->y][iter->x]).parent->x) * gridSize + gridSize / 2,
            ((tileMap[iter->y][iter->x]).parent->y) * gridSize + gridSize / 2);
    }

    for (auto iter = _closeList.begin(); iter != _closeList.end(); iter++)
    {
        //최초 시작점은 예외처리
        if (tileMap[(*iter)->y][(*iter)->x].parent == NULL) continue;

        //부모 노드에 화살표 표시하기
        MoveToEx(hdc, ((*iter)->x) * gridSize + gridSize / 2, ((*iter)->y) * gridSize + gridSize / 2, NULL);
        LineTo(hdc, ((tileMap[(*iter)->y][(*iter)->x]).parent->x) * gridSize + gridSize / 2, 
            ((tileMap[(*iter)->y][(*iter)->x]).parent->y)* gridSize + gridSize / 2);
    }

    //각 노드 수치 넣기
    if (gridSize > 36)
    {
        int fontSize = 13; // 원하는 글자 크기
        HFONT hFont = CreateFont(
            fontSize,                // 글자 높이
            0,                       // 글자 너비 (0은 자동 계산)
            0,                       // 기울기 각도
            0,                       // 베이스라인 각도
            FW_NORMAL,               // 글자 굵기
            FALSE,                   // 이탤릭
            FALSE,                   // 밑줄
            FALSE,                   // 취소선
            DEFAULT_CHARSET,         // 문자 집합
            OUT_DEFAULT_PRECIS,      // 출력 정밀도
            CLIP_DEFAULT_PRECIS,     // 클리핑 정밀도
            DEFAULT_QUALITY,         // 출력 품질
            DEFAULT_PITCH | FF_DONTCARE, // 피치 및 글꼴 가족
            TEXT("Arial"));          // 글꼴 이름

        // 생성된 글꼴을 HDC에 적용
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
        WCHAR buffer[20];
        for (auto iter = _openList_Point.begin(); iter != _openList_Point.end(); iter++)
        {
            _itow_s(tileMap[iter->y][iter->x].g, buffer, 10);
            TextOut(hdc, iter->x * gridSize + 5, iter->y * gridSize + 5, buffer, wcslen(buffer));
            _itow_s(tileMap[iter->y][iter->x].h, buffer, 10);
            TextOut(hdc, iter->x * gridSize + 5, iter->y * gridSize + gridSize/2 - 10, buffer, wcslen(buffer));
            _itow_s(tileMap[iter->y][iter->x].w, buffer, 10);
            TextOut(hdc, iter->x * gridSize + 5, iter->y * gridSize + (gridSize-20), buffer, wcslen(buffer));
        }
        for (auto iter = _closeList.begin(); iter != _closeList.end(); iter++)
        {
            _itow_s((*iter)->g, buffer, 10);
            TextOut(hdc, (*iter)->x * gridSize + 5, (*iter)->y * gridSize + 5, buffer, wcslen(buffer));
            _itow_s((*iter)->h, buffer, 10);
            TextOut(hdc, (*iter)->x * gridSize + 5, (*iter)->y * gridSize + gridSize / 2 - 10, buffer, wcslen(buffer));
            _itow_s((*iter)->w, buffer, 10);
            TextOut(hdc, (*iter)->x * gridSize + 5, (*iter)->y * gridSize + (gridSize - 20), buffer, wcslen(buffer));
        }
        SelectObject(hdc, oldFont);

        // 글꼴 리소스 해제
        DeleteObject(hFont);
    }

    SelectObject(hdc, oldPen);
}