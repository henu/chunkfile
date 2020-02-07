#include "chunkfile.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

void testTrue(bool b)
{
    if (!b) {
        throw std::runtime_error("Value is not true!");
    }
}

void testFalse(bool b)
{
    if (b) {
        throw std::runtime_error("Value is not false!");
    }
}

void testFileCreation(std::string const& path)
{
    // Make sure file does not exist
    std::ifstream f(path.c_str());
    testFalse(f.good());

    // Create new file
    {
        Chunkfile file(path);
    }

    // Test if file was created successfully
    f.open(path.c_str());
    testTrue(f.good());

    // Test if file opens nicely
    {
        Chunkfile file(path);
    }
}

void testFileRemoval(std::string const& path)
{
    testFalse(::remove(path.c_str()));
}

int main()
{
    // Get random path
    srand(time(NULL));
    std::string path = "/tmp/chunkfile_test_" + std::to_string(rand());

    std::cout << "Test file creation..." << std::endl;
    testFileCreation(path);
    std::cout << "Passed!" << std::endl;

    std::cout << "Test file removal..." << std::endl;
    testFileRemoval(path);
    std::cout << "Passed!" << std::endl;

    std::cout << "All tests passed!" << std::endl;

    return EXIT_SUCCESS;
}