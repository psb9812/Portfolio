#include <iostream>
#include <memory>

//* 폴딩 표현식 공부 예제

//가변 인자 템플릿이 나오기 전에는 아래와 같이 여러 버전의 함수를 오버로딩해야 했음
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

//가변 인자 템플릿 등장 후에는 재귀적인 호출로 가변 인자를 처리할 수 있었음
//재귀 호출의 종료 조건
auto VariadicMultiply()
{
    return 1;
}

template<typename T1, typename...T>
auto VariadicMultiply(T1 t, T... ts)
{
    //재귀적인 호출
    return t * VariadicMultiply(ts...);
}

//폴드 표현식을 활용한 가변 인자 곱셈 (오버로드나 종료 조건이 딱히 없다. 아름답다...)
template <typename... Args>
auto FoldMultiply(Args... args)
{
    return (args * ... * 1);
    //     pack  op    op init
    // 4개의 인자가 들어왔다고 가정하면
    // p1 op (p2 op (p3 op (p4 op init)))로 변환된다!!
    // 
    //return (args * ...) 으로도 가능 => p1 * ( p2 * (p3 * p4))가 된다.
}

int main() 
{
    //가변 인자 템플릿 재귀로 구현한 방식
    std::cout << VariadicMultiply<int, int>(10, 10, 10, 10) << '\n';
    /*
        내부 동작
        VariadicMultiply<int, int>(10, 10, 10, 10);
        return 10 * VariadicMultiply(10, 10, 10);
        return 10 * 10 * VariadicMultiply(10, 10);
        return 10 * 10 * 10 * VariadicMultiply(10);
        return 10 * 10 * 10 * 10 * VariadicMultiply(); <- 종료 조건 도달 인자가 없는 함수는 1을 리턴 
        결과 10 * 10 * 10 * 10 * 1 = 10,000 출력
    */


    //폴드 표현식을 사용한 방식
    std::cout << FoldMultiply(10, 10, 10, 10) << '\n'; //Prints 10,000
    return 0;
}