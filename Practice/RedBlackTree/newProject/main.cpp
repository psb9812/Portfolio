#include <iostream>
#include <map>
#include <vector>
#include "RBTree.h"
#define PROFILE
#include "Profiler.h"

using namespace std;

int main()
{
    const int insertNum = 1000000;
    const int eraseNum = 500000;
    srand(time(NULL));

    for (int mainLoop = 0; mainLoop < 10; mainLoop++)
    {
        RBTree rbt;
        map<int, int> map;

        vector<int> values;
        vector<int> delValues;

        vector<int> myrbtVec;
        vector<int> mapVec;

        int duplicationCount = 0;
        

        //���� �׽�Ʈ
        for (int insertLoop = 0; insertLoop < insertNum; insertLoop++)
        {
            int val = rand() % insertNum;

            //�ߺ��� ���� �Ÿ����� �ǵ�
            if (find(values.begin(), values.end(), val) != values.end())
            {
                duplicationCount++;
                continue;
            }
            values.push_back(val);
            {
                START_PROFILEING(myRBTreeInsert, "RBTreeInsert");
                rbt.insert(val);
            }
            {
                START_PROFILEING(mapInsert, "MapInsert");
                map.insert(make_pair(val, val));
            }
        }

        for (int i = 0; i < eraseNum; i++)
        {
            int deleteValIdx = rand() % (insertNum - duplicationCount);
            int deleteVal = values.at(deleteValIdx);
            if (find(delValues.begin(), delValues.end(), deleteVal) != delValues.end())
            {
                continue;
            }
            delValues.push_back(deleteVal);

            {
                START_PROFILEING(myRBTreeErase, "RBTreeErase");
                rbt.erase(deleteVal);
            }

            {
                START_PROFILEING(mapErase, "MapErase");
                map.erase(deleteVal);
            }
        }

        rbt.toVector(myrbtVec);

        for (auto iter = map.begin(); iter != map.end(); iter++)
        {
            mapVec.push_back((*iter).first);
        }

        auto myrbtiter = myrbtVec.begin();
        auto mapiter = mapVec.begin();

        for (int n = 0; n < map.size(); n++)
        {
            if (map.size() != rbt.size())
            {
                cout << "ũ�� ����ġ" << endl;
                return 1;
            }
            if (*myrbtiter != *mapiter)
            {
                cout << "���� �߻�" << endl;
                return 1;
            }

            myrbtiter++;
            mapiter++;
        }

        cout << "�Ϸ� ��ġ :" << mainLoop + 1 << " / " << 10 << endl;
    }
    ProfileDataOutText(L"profile.txt");
    

    /*while (1)
    {
        int option;
        int inputVal;
        std::cout << "���� �ϳ��� �����ϳ���?\n ����: 1\n ����: 2\n>>";
        std::cin >> option;

        switch (option)
        {
        case 1:
            std::cout << "���� �� Ʈ���� ���� ���� �Է����ּ���(exit : -1) >> ";
            std::cin >> inputVal;
            if (inputVal == -1)
                break;

            rbt.insert(inputVal);
            rbt.printAllNode();
            std::cout << std::endl;
            break;
        case 2:
            std::cout << "���� �� Ʈ���� ������ ���� �Է����ּ���(exit : -1) >> ";
            std::cin >> inputVal;
            if (inputVal == -1)
                break;
            if (!rbt.erase(inputVal))
            {
                std::cout << "������ �� ���� ���Դϴ�" << std::endl;
            }
            rbt.printAllNode();
            std::cout << std::endl;
            break;

        default:
            std::cout << "��ȿ���� ���� �� �Դϴ�.\n";
            break;
        }
    }*/

}
