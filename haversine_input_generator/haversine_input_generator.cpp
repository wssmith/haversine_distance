#include <cstddef>
#include <exception>
#include <iostream>

#include "random_generator.hpp"

namespace
{

}

int main()
{
    try
    {
        std::cout << "Hello World!\n";
    }
    catch (std::exception& ex)
    {
        std::cout << "ERROR!! " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cout << "UNKNOWN ERROR!!\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
