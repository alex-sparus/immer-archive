//
//  main.cpp
//  immer-test
//
//  Created by Alex Shabalin on 11/10/2023.
//

#include <iostream>

#include <immer/algorithm.hpp>
#include <immer/vector.hpp>

namespace {

void qwe()
{
    // immer::for_each(immer::vector<int>{1}, [](){});
}

} // namespace

int main(int argc, const char* argv[])
{
    // insert code here...
    std::cout << "Hello, World!\n";

    auto vec = immer::vector<int>{1};
    auto v2  = vec.push_back(2);

    std::cout << v2.size() << '\n';

    return 0;
}
