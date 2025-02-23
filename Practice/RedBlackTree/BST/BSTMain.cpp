#include <iostream>
#include "BinarySearchTree.h"

int main()
{
    BinarySearchTree bst;
    
    bst.insert(50);
    bst.insert(15);
    bst.insert(70);
    bst.insert(10);
    bst.insert(30);
    bst.insert(60);
    
    bst.erase(70);

    bst.printAllNode();
}

