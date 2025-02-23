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

        //������ openList�� �ְ� ��ã�� ����
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

        //���� ����Ʈ���� w�� ���� ���� ���� �ϳ� �̴´�.
        Node* curNode = _openList.top();
        _openList.pop();
        _closeList.push_back(curNode); //openList���� ���� ���� closeList�� �־��ش�.
        _openList_Point.erase(std::remove(_openList_Point.begin(), _openList_Point.end(), Point{ curNode->x, curNode->y }), _openList_Point.end());

        if (curNode->x == endPos.x && curNode->y == endPos.y)
        {
            //�������� ã�� ���
            //���������� ������ �ִܰŸ� ǥ���ϱ�
            while (curNode->parent != NULL)
            {
                HPEN oldPen = (HPEN)SelectObject(hdc, _finalRoutePen);
                //�θ� ��忡 ȭ��ǥ ǥ���ϱ�
                MoveToEx(hdc, (curNode->x - gridPosX) * gridSize + gridSize / 2, (curNode->y - gridPosY) * gridSize + gridSize / 2, NULL);
                LineTo(hdc, (curNode->parent->x - gridPosX) * gridSize + gridSize / 2, (curNode->parent->y - gridPosY) * gridSize + gridSize / 2);
                SelectObject(hdc, oldPen);
                curNode = curNode->parent;
            }
            PathFindEndProcess();
            Sleep(1000);
            return;
        }
        //���� ����� 8�������� �̵��� �� �ִ� ��带 openList�� �ִ´�.
        for (int idx = 0; idx < 8; idx++)
        {
            bool isSkip = false;
            int nextNodeXIdx = curNode->x + dx[idx];
            int nextNodeYIdx = curNode->y + dy[idx];
            if (nextNodeYIdx < 0 || nextNodeYIdx >= GRID_HEIGHT || nextNodeXIdx < 0 || nextNodeXIdx >= GRID_WIDTH)
                continue;
            Node* nextNode = &tileMap[nextNodeYIdx][nextNodeXIdx];

            if (nextNode->isObstacle == true)  continue;     //��ֹ��̶�� ��ŵ
            //openList���� ������ ��ġ�� ���� �ִ��� Ȯ�� �� �����Ѵٸ� ��ŵ
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
            //closeList���� ������ ��ġ�� ���� �ִ��� Ȯ�� �� �����Ѵٸ� ��ŵ
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
            //���� ������ ��Ŭ���� �Ÿ�
            if (abs(dx[idx]) + abs(dy[idx]) == 2)
            {
                //�밢������ ���ٸ�
                nextNode->g = curNode->g + 14;
            }
            else
            {
                nextNode->g = curNode->g + 10;
            }
            //������������ ����ư �Ÿ�
            nextNode->h = (abs(endPos.x - nextNode->x) + abs(endPos.y - nextNode->y)) * 100;
            //���� ��(h�� 10�踦 ���༭ ��ǥ������ ����� ��带 �� ���� ��ȣ�ϰ� �Ѵ�.
            nextNode->w = nextNode->g + nextNode->h;
            nextNode->parent = curNode;

            //��ŵ���� �ʾ����� openList�� �߰��Ѵ�.
            _openList.push(nextNode);
            _openList_Point.push_back(Point{ nextNode->x, nextNode->y });
        }
    }
}

void AStarAlgorithm::PathFindAll(HWND hWnd, HDC hdc, Node(&tileMap)[GRID_HEIGHT][GRID_WIDTH], int gridSize, int gridPosX, int gridPosY, Point startPos, Point endPos)
{
    _isFinding = true;
    //���� ��� ����
    Node startNode = tileMap[startPos.y][startPos.x];
    startNode.h = (abs(startPos.x - endPos.x) + abs(startPos.y - endPos.y)) * 10;
    startNode.w = startNode.h + startNode.g;

    //������ openList�� �ְ� ��ã�� ����
    _openList.push(&tileMap[startPos.y][startPos.x]);
    _openList_Point.push_back(Point{ startNode.x, startNode.y });

    while (!_openList.empty())
    {
        _searchCount++;
        if (_searchCount >= _maxSearchCount)
        {
            break;
        }

        //���� ����Ʈ���� w�� ���� ���� ���� �ϳ� �̴´�.
        Node* curNode = _openList.top();
        _openList.pop();
        _closeList.push_back(curNode); //openList���� ���� ���� closeList�� �־��ش�.
        _openList_Point.erase(std::remove(_openList_Point.begin(), _openList_Point.end(), Point{ curNode->x, curNode->y }), _openList_Point.end());

        if (curNode->x == endPos.x && curNode->y == endPos.y)
        {
            //�������� ã�� ���
            //���������� ������ �ִܰŸ� ǥ���ϱ�
            while (curNode->parent != NULL)
            {
                HPEN oldPen = (HPEN)SelectObject(hdc, _finalRoutePen);
                //�θ� ��忡 ȭ��ǥ ǥ���ϱ�
                MoveToEx(hdc, (curNode->x - gridPosX) * gridSize + gridSize / 2, (curNode->y - gridPosY) * gridSize + gridSize / 2, NULL);
                LineTo(hdc, (curNode->parent->x - gridPosX) * gridSize + gridSize / 2, (curNode->parent->y - gridPosY) * gridSize + gridSize / 2);
                SelectObject(hdc, oldPen);
                curNode = curNode->parent;
            }
            break;
        }
        //���� ����� 8�������� �̵��� �� �ִ� ��带 openList�� �ִ´�.
        for (int idx = 0; idx < 8; idx++)
        {
            bool isSkip = false;
            int nextNodeXIdx = curNode->x + dx[idx];
            int nextNodeYIdx = curNode->y + dy[idx];
            if (nextNodeYIdx < 0 || nextNodeYIdx >= GRID_HEIGHT || nextNodeXIdx < 0 || nextNodeXIdx >= GRID_WIDTH)
                continue;
            Node* nextNode = &tileMap[nextNodeYIdx][nextNodeXIdx];

            if (nextNode->isObstacle == true)  continue;     //��ֹ��̶�� ��ŵ
            //openList���� ������ ��ġ�� ���� �ִ��� Ȯ�� �� �����Ѵٸ� ��ŵ
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
            //closeList���� ������ ��ġ�� ���� �ִ��� Ȯ�� �� �����Ѵٸ� ��ŵ
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
            //���� ������ ��Ŭ���� �Ÿ�
            if (abs(dx[idx]) + abs(dy[idx]) == 2)
            {
                //�밢������ ���ٸ�
                nextNode->g = curNode->g + 14;
            }
            else
            {
                nextNode->g = curNode->g + 10;
            }
            //������������ ����ư �Ÿ�
            nextNode->h = (abs(endPos.x - nextNode->x) + abs(endPos.y - nextNode->y)) * 100;
            //���� ��h�� 10�踦 ���༭ ��ǥ������ ����� ��带 �� ���� ��ȣ�ϰ� �Ѵ�.
            nextNode->w = nextNode->g + nextNode->h;
            nextNode->parent = curNode;

            //��ŵ���� �ʾ����� openList�� �߰��Ѵ�.
            _openList.push(nextNode);
            _openList_Point.push_back(Point{ nextNode->x, nextNode->y });

            InvalidateRect(hWnd, NULL, false);
            UpdateWindow(hWnd);
        }
        Sleep(50); //������� �ϱ� ������ ���
    }
    TextOut(hdc, 0, 0, L"��ã�� ����", 6);
    PathFindEndProcess();
    Sleep(4000);
}

void AStarAlgorithm::PathFindNoRender(Node(&tileMap)[GRID_HEIGHT][GRID_WIDTH], Point startPos, Point endPos)
{
    //���� ��� ����
    Node startNode = tileMap[startPos.y][startPos.x];
    startNode.h = (abs(startPos.x - endPos.x) + abs(startPos.y - endPos.y)) * 10;
    startNode.w = startNode.h + startNode.g;

    //������ openList�� �ְ� ��ã�� ����
    _openList.push(&tileMap[startPos.y][startPos.x]);
    _openList_Point.push_back(Point{ startNode.x, startNode.y });

    while (!_openList.empty())
    {
        _searchCount++;
        if (_searchCount >= _maxSearchCount)
        {
            break;
        }

        //���� ����Ʈ���� w�� ���� ���� ���� �ϳ� �̴´�.
        Node* curNode = _openList.top();
        _openList.pop();
        _closeList.push_back(curNode); //openList���� ���� ���� closeList�� �־��ش�.
        _openList_Point.erase(std::remove(_openList_Point.begin(), _openList_Point.end(), Point{ curNode->x, curNode->y }), _openList_Point.end());

        if (curNode->x == endPos.x && curNode->y == endPos.y)
        {
            //�������� ã�� ���
            //���������� ������ �ִܰŸ� ǥ���ϱ�
            while (curNode->parent != NULL)
            {
                curNode = curNode->parent;
            }
            break;
        }
        //���� ����� 8�������� �̵��� �� �ִ� ��带 openList�� �ִ´�.
        for (int idx = 0; idx < 8; idx++)
        {
            bool isSkip = false;
            int nextNodeXIdx = curNode->x + dx[idx];
            int nextNodeYIdx = curNode->y + dy[idx];
            if (nextNodeYIdx < 0 || nextNodeYIdx >= GRID_HEIGHT || nextNodeXIdx < 0 || nextNodeXIdx >= GRID_WIDTH)
                continue;
            Node* nextNode = &tileMap[nextNodeYIdx][nextNodeXIdx];

            if (nextNode->isObstacle == true)  continue;     //��ֹ��̶�� ��ŵ
            //openList���� ������ ��ġ�� ���� �ִ��� Ȯ�� �� �����Ѵٸ� ��ŵ
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
            //closeList���� ������ ��ġ�� ���� �ִ��� Ȯ�� �� �����Ѵٸ� ��ŵ
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
            //���� ������ ��Ŭ���� �Ÿ�
            if (abs(dx[idx]) + abs(dy[idx]) == 2)
            {
                //�밢������ ���ٸ�
                nextNode->g = curNode->g + 14;
            }
            else
            {
                nextNode->g = curNode->g + 10;
            }
            //������������ ����ư �Ÿ�
            nextNode->h = (abs(endPos.x - nextNode->x) + abs(endPos.y - nextNode->y)) * 100;
            //���� ��(h�� 10�踦 ���༭ ��ǥ������ ����� ��带 �� ���� ��ȣ�ϰ� �Ѵ�.
            nextNode->w = nextNode->g + nextNode->h;
            nextNode->parent = curNode;

            //��ŵ���� �ʾ����� openList�� �߰��Ѵ�.
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
    //����� �ڷᱸ�� ����
    while (!_openList.empty()) {
        _openList.pop();
    }
    _openList_Point.clear();
    _closeList.clear();

    _isFinding = false;
}

void AStarAlgorithm::Render(HDC hdc, Node(&tileMap)[GRID_HEIGHT][GRID_WIDTH], int gridSize, int gridPosX, int gridPosY)
{
    //���� ����Ʈ�� ��� ��ĥ
    SelectObject(hdc, _openListTileBrush);
    for (auto iter = _openList_Point.begin(); iter != _openList_Point.end(); iter++)
    {
        Rectangle(hdc, (iter->x) * gridSize, (iter->y) * gridSize,
            (iter->x + 1) * gridSize, (iter->y + 1) * gridSize);
    }
    //Ŭ���� ����Ʈ�� ��� ��ĥ
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
        //�θ� ��忡 ȭ��ǥ ǥ���ϱ�
        MoveToEx(hdc, (iter->x) * gridSize + gridSize / 2, (iter->y) * gridSize + gridSize / 2, NULL);
        LineTo(hdc, ((tileMap[iter->y][iter->x]).parent->x) * gridSize + gridSize / 2,
            ((tileMap[iter->y][iter->x]).parent->y) * gridSize + gridSize / 2);
    }

    for (auto iter = _closeList.begin(); iter != _closeList.end(); iter++)
    {
        //���� �������� ����ó��
        if (tileMap[(*iter)->y][(*iter)->x].parent == NULL) continue;

        //�θ� ��忡 ȭ��ǥ ǥ���ϱ�
        MoveToEx(hdc, ((*iter)->x) * gridSize + gridSize / 2, ((*iter)->y) * gridSize + gridSize / 2, NULL);
        LineTo(hdc, ((tileMap[(*iter)->y][(*iter)->x]).parent->x) * gridSize + gridSize / 2, 
            ((tileMap[(*iter)->y][(*iter)->x]).parent->y)* gridSize + gridSize / 2);
    }

    //�� ��� ��ġ �ֱ�
    if (gridSize > 36)
    {
        int fontSize = 13; // ���ϴ� ���� ũ��
        HFONT hFont = CreateFont(
            fontSize,                // ���� ����
            0,                       // ���� �ʺ� (0�� �ڵ� ���)
            0,                       // ���� ����
            0,                       // ���̽����� ����
            FW_NORMAL,               // ���� ����
            FALSE,                   // ���Ÿ�
            FALSE,                   // ����
            FALSE,                   // ��Ҽ�
            DEFAULT_CHARSET,         // ���� ����
            OUT_DEFAULT_PRECIS,      // ��� ���е�
            CLIP_DEFAULT_PRECIS,     // Ŭ���� ���е�
            DEFAULT_QUALITY,         // ��� ǰ��
            DEFAULT_PITCH | FF_DONTCARE, // ��ġ �� �۲� ����
            TEXT("Arial"));          // �۲� �̸�

        // ������ �۲��� HDC�� ����
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

        // �۲� ���ҽ� ����
        DeleteObject(hFont);
    }

    SelectObject(hdc, oldPen);
}