#pragma once

struct Node
{
    int x;
    int y;
    int g;  //출발점부터의 거리
    int h;  //도착점부터의 거리
    int w;  //g + h 값
    bool isObstacle;
    Node* parent;    //부모 노드를 가리키는 값
};

struct cmp {
    bool operator()(Node* a, Node* b) {
        return a->w > b->w;
    }
};