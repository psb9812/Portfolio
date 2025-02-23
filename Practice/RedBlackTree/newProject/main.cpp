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
        

        //삽입 테스트
        for (int insertLoop = 0; insertLoop < insertNum; insertLoop++)
        {
            int val = rand() % insertNum;

            //중복된 값은 거르려는 의도
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
                cout << "크기 불일치" << endl;
                return 1;
            }
            if (*myrbtiter != *mapiter)
            {
                cout << "오류 발생" << endl;
                return 1;
            }

            myrbtiter++;
            mapiter++;
        }

        cout << "완료 수치 :" << mainLoop + 1 << " / " << 10 << endl;
    }
    ProfileDataOutText(L"profile.txt");
    

    /*while (1)
    {
        int option;
        int inputVal;
        std::cout << "삽입 하나요 삭제하나요?\n 삽입: 1\n 삭제: 2\n>>";
        std::cin >> option;

        switch (option)
        {
        case 1:
            std::cout << "레드 블랙 트리에 넣을 값을 입력해주세요(exit : -1) >> ";
            std::cin >> inputVal;
            if (inputVal == -1)
                break;

            rbt.insert(inputVal);
            rbt.printAllNode();
            std::cout << std::endl;
            break;
        case 2:
            std::cout << "레드 블랙 트리에 삭제할 값을 입력해주세요(exit : -1) >> ";
            std::cin >> inputVal;
            if (inputVal == -1)
                break;
            if (!rbt.erase(inputVal))
            {
                std::cout << "제거할 수 없는 값입니다" << std::endl;
            }
            rbt.printAllNode();
            std::cout << std::endl;
            break;

        default:
            std::cout << "유효하지 않은 값 입니다.\n";
            break;
        }
    }*/

}
