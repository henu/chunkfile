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
        file.verify();
    }
}

void testAddingFirstChunk(std::string const& path)
{
    // Write to file
    {
        Chunkfile file(path);
        file.set(0, std::string("yolo"));
    }

    // Test
    {
        Chunkfile file(path);
        std::string test;
        file.get(test, 0);
        testTrue(test == std::string("yolo"));
        file.verify();
    }
}

void testAddingSecondChunk(std::string const& path)
{
    // Write to file
    {
        Chunkfile file(path);
        file.set(1, std::string("ebin"));
    }

    // Test
    {
        Chunkfile file(path);
        std::string test;
        file.get(test, 0);
        testTrue(test == std::string("yolo"));
        file.get(test, 1);
        testTrue(test == std::string("ebin"));
        file.verify();
    }
}

void testAddingBigChunk(std::string const& path)
{
    // Write to file
    {
        Chunkfile file(path);
        file.set(2, std::string("lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots of data"));
    }

    // Test
    {
        Chunkfile file(path);
        std::string test;
        file.get(test, 0);
        testTrue(test == std::string("yolo"));
        file.get(test, 1);
        testTrue(test == std::string("ebin"));
        file.get(test, 2);
        testTrue(test == std::string("lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots and lots of data"));
        file.verify();
    }
}

void testVectorSetAndGet(std::string const& path)
{
    // Write to file
    {
        Chunkfile::Bytes bytes;
        bytes.push_back('x');
        bytes.push_back('y');
        bytes.push_back('z');
        Chunkfile file(path);
        file.set(3, bytes);
    }

    // Test
    {
        Chunkfile file(path);
        std::string test = file.getString(3);
        testTrue(test == std::string("xyz"));
        file.verify();
    }
    {
        Chunkfile file(path);
        Chunkfile::Bytes test = file.getBytes(3);
        testTrue(test.size() == 3);
        testTrue(test[0] == 'x');
        testTrue(test[1] == 'y');
        testTrue(test[2] == 'z');
        file.verify();
    }
}

void testReplacingChunks(std::string const& path)
{
    // Write to file
    {
        Chunkfile file(path);
        file.set(0, std::string("a little bit bigger chunk"));
        file.set(1, std::string("another longer chunk"));
        file.set(3, std::string("and one more"));
    }

    // Test
    {
        Chunkfile file(path);
        testTrue(file.getString(0) == std::string("a little bit bigger chunk"));
        testTrue(file.getString(1) == std::string("another longer chunk"));
        testTrue(file.getString(3) == std::string("and one more"));
        file.verify();
    }
}

void testRemovingChunks(std::string const& path)
{
    // Write to file
    {
        Chunkfile file(path);
        file.del(0);
        file.del(2);
    }

    // Test
    {
        Chunkfile file(path);
        testFalse(file.exists(0));
        testTrue(file.exists(1));
        testFalse(file.exists(2));
        testTrue(file.exists(3));
        file.verify();
    }

    // Write to file
    {
        Chunkfile file(path);
        file.del(1);
    }

    // Test
    {
        Chunkfile file(path);
        testFalse(file.exists(0));
        testFalse(file.exists(1));
        testFalse(file.exists(2));
        testTrue(file.exists(3));
        file.verify();
    }

    // Write to file
    {
        Chunkfile file(path);
        file.del(3);
    }

    // Test
    {
        Chunkfile file(path);
        testFalse(file.exists(0));
        testFalse(file.exists(1));
        testFalse(file.exists(2));
        testFalse(file.exists(3));
        file.verify();
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

    std::cout << "Test adding first chunk..." << std::endl;
    testAddingFirstChunk(path);
    std::cout << "Passed!" << std::endl;

    std::cout << "Test adding second chunk..." << std::endl;
    testAddingSecondChunk(path);
    std::cout << "Passed!" << std::endl;

    std::cout << "Test adding big chunk..." << std::endl;
    testAddingBigChunk(path);
    std::cout << "Passed!" << std::endl;

    std::cout << "Test using std::vector..." << std::endl;
    testVectorSetAndGet(path);
    std::cout << "Passed!" << std::endl;

    std::cout << "Test replacing chunk..." << std::endl;
    testReplacingChunks(path);
    std::cout << "Passed!" << std::endl;

    std::cout << "Test removing chunks..." << std::endl;
    testRemovingChunks(path);
    std::cout << "Passed!" << std::endl;

    std::cout << "Test file removal..." << std::endl;
    testFileRemoval(path);
    std::cout << "Passed!" << std::endl;

    std::cout << "All tests passed!" << std::endl;

    return EXIT_SUCCESS;
}
