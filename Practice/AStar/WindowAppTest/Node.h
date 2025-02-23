#pragma once

struct Node
{
    int x;
    int y;
    int g;  //����������� �Ÿ�
    int h;  //������������ �Ÿ�
    int w;  //g + h ��
    bool isObstacle;
    Node* parent;    //�θ� ��带 ����Ű�� ��
};

struct cmp {
    bool operator()(Node* a, Node* b) {
        return a->w > b->w;
    }
};