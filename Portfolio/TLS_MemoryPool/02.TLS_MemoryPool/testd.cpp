#include <iostream>
#include <memory>

//* ���� ǥ���� ���� ����

//���� ���� ���ø��� ������ ������ �Ʒ��� ���� ���� ������ �Լ��� �����ε��ؾ� ����
template <typename T>
auto Multiply(T a, T b)
{
    return a * b;
}

template <typename T>
auto Multiply(T a, T b, T c)
{
    return a * b * c;
}

template <typename T>
auto Multiply(T a, T b, T c, T d)
{
    return a * b * c * d;
}

//���� ���� ���ø� ���� �Ŀ��� ������� ȣ��� ���� ���ڸ� ó���� �� �־���
//��� ȣ���� ���� ����
auto VariadicMultiply()
{
    return 1;
}

template<typename T1, typename...T>
auto VariadicMultiply(T1 t, T... ts)
{
    //������� ȣ��
    return t * VariadicMultiply(ts...);
}

//���� ǥ������ Ȱ���� ���� ���� ���� (�����ε峪 ���� ������ ���� ����. �Ƹ����...)
template <typename... Args>
auto FoldMultiply(Args... args)
{
    return (args * ... * 1);
    //     pack  op    op init
    // 4���� ���ڰ� ���Դٰ� �����ϸ�
    // p1 op (p2 op (p3 op (p4 op init)))�� ��ȯ�ȴ�!!
    // 
    //return (args * ...) ���ε� ���� => p1 * ( p2 * (p3 * p4))�� �ȴ�.
}

int main() 
{
    //���� ���� ���ø� ��ͷ� ������ ���
    std::cout << VariadicMultiply<int, int>(10, 10, 10, 10) << '\n';
    /*
        ���� ����
        VariadicMultiply<int, int>(10, 10, 10, 10);
        return 10 * VariadicMultiply(10, 10, 10);
        return 10 * 10 * VariadicMultiply(10, 10);
        return 10 * 10 * 10 * VariadicMultiply(10);
        return 10 * 10 * 10 * 10 * VariadicMultiply(); <- ���� ���� ���� ���ڰ� ���� �Լ��� 1�� ���� 
        ��� 10 * 10 * 10 * 10 * 1 = 10,000 ���
    */


    //���� ǥ������ ����� ���
    std::cout << FoldMultiply(10, 10, 10, 10) << '\n'; //Prints 10,000
    return 0;
}