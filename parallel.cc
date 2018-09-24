#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <random>
#include <regex>
#include <tuple>

#include "omp.h"

/**********************************************************************
 * generates a random point outside of the radius of the crystal
***********************************************************************/
std::tuple<int, int> generatePoint(std::default_random_engine& generator, std::vector<std::vector<char>>& grid, const int gridSize, const int center, const int radius) {
    std::uniform_int_distribution<int> distribution(0, gridSize - 1);
    int x, y;
    #pragma omp critical
    {
        char tempChar;
        do {
            x = distribution(generator);
            y = distribution(generator);
            // #pragma omp atomic read
            tempChar = grid[x][y];
        } while ((abs(center - x) <= radius + 1 && abs(center - y) <= radius + 1) || tempChar != 0);
        // grid[x][y] = 'p';
    }
    return std::make_tuple(x, y);
}

/**********************************************************************
 * calculates the next random move for a particle
 * 
 * note: the next move could cause particle to leave the lattice
***********************************************************************/
std::tuple<int, int> nextMove(std::default_random_engine& generator) {
    std::uniform_int_distribution<int> distribution(-1, 1);
    int dx, dy;
    dx = distribution(generator);
    dy = distribution(generator);
    return std::make_tuple(dx, dy);
}

/* determines if the current particle should stick to the crystal */
bool shouldStick(std::vector<std::vector<char>>& grid, const int gridSize, const int x, const int y) {
    bool returnBool = false;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            const int newX = x + dx;
            const int newY = y + dy;
            if (newX >= 0 && newX < gridSize && newY >= 0 && newY < gridSize) {
                if (grid[newX][newY] == 'X') {
                    // #pragma omp atomic write
                    #pragma omp critical
                    {
                    grid[x][y] = 'X';
                    }
                    returnBool = true;
                }
                if (returnBool) {
                    return returnBool;
                }
            }
        }
    }
    return returnBool;
}

/**********************************************************************
 * walks particle until it leaves lattice or sticks to the crystal
***********************************************************************/
void walkParticle(std::default_random_engine& generator, std::vector<std::vector<char>>& grid, const int gridSize, int& x, int& y) {
    while (x >= 0 && x < gridSize && y >= 0 && y < gridSize) {
        /* check if should stick */
        if (shouldStick(grid, gridSize, x, y)) {
            return;
        }

        int newX;
        int newY;
        /* generate next move */
        // #pragma omp critical
        // {
            char tempChar = 1;
            do {
            const std::tuple<int, int> direction = nextMove(generator);
            const int dx = std::get<0>(direction);
            const int dy = std::get<1>(direction);
            newX = x + dx;
            newY = y + dy;
            const bool inBounds = newX >= 0 && newX < gridSize && newY >= 0 && newY < gridSize;
            if (inBounds) {
                // #pragma omp atomic read
                tempChar = grid[newX][newY];
            }
            } while (tempChar != 0);
            // grid[x][y] = 0;
            // if (newX >= 0 && newX < gridSize && newY >= 0 && newY < gridSize) {
            //     grid[newX][newY] = 'p';
            // }
        // }

        x = newX;
        y = newY;
    }
}

/**********************************************************************
 * main function to manage crystal, lattice, and particles sequentially
***********************************************************************/
int main(int argc, char* argv[]) {
    /* check for proper arguments */
    if (argc != 3) {
        std::cerr << "Requires one argument\n\nUsage:\n\t./sequential <grid_size> <num_particles>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string gridSizeStr = argv[1];
    std::string numParticlesStr = argv[2];

    /* check if strings match desired regex pattern */
    if (!std::regex_match(gridSizeStr, std::regex("[0-9]+")) || !std::regex_match(numParticlesStr, std::regex("[0-9]+"))) {
        std::cerr << "Grid Size and Number of Particles must be positive integers" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* attempt to parse gridSize and numParticles */
    int gridSize;
    unsigned long numParticles;
    try {
        gridSize = std::stoi(gridSizeStr);
        numParticles = std::stoul(numParticlesStr);
    } catch (...) {
        std::cerr << "Grid Size and Number of Particles must be positive integers" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* fail if gridSize is even */
    if (gridSize % 2 == 0) {
        std::cerr << "Grid Size must be odd" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<std::vector<char>> grid(gridSize, std::vector<char>(gridSize));

    /* initialize radius and center */
    int radius = 0;
    const int center = gridSize / 2;

    /* place starting crystal */
    grid[center][center] = 'X';

    /* create random number generator */
    std::default_random_engine generator;

    /* seed generator with system clock */
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    generator.seed(seed);

    #pragma omp parallel
    {
        int threadId = omp_get_thread_num();
        int numThreads = omp_get_num_threads();

        unsigned long sectionSize = numParticles / numThreads;
        int remaining = numParticles % numThreads;
        if (threadId < remaining) {
            sectionSize++;
        }

        // for (unsigned long p = 0; p < numParticles; p++) {
        for (unsigned long p = 0; p < sectionSize; p++) {
            /* check if radius is the entire grid */
            if (radius >= gridSize / 2 - 1) {
                std::cout << "radius is " << radius << ", which means no room is left, finished at particle " << p + 1 << std::endl;
                break;
            } else {

                /* generate point */
                const auto point = generatePoint(generator, grid, gridSize, center, radius);
                int x = std::get<0>(point);
                int y = std::get<1>(point);

                /* walk particle until it leaves lattice or sticks to the crystal */
                walkParticle(generator, grid, gridSize, x, y);

                /* check if particle stuck, if it did update radius if necessary */
                if (x >= 0 && x < gridSize && y >= 0 && y < gridSize) {
                    const int distance = std::max(std::abs(center - x), std::abs(center - y));
                    if (distance > radius) {
                        radius = distance;
                    }
                }
            }
        }
    } 

    /* print crude depiction of final crystal inside lattice */
    // for (int i = 0; i < gridSize; i++) {
    //     for (int k = 0; k < gridSize; k++) {
    //         char value = grid[i][k];
    //         if (value == 0) {
    //             value = '-';
    //         }
    //         std::cout << value << " ";
    //     }
    //     std::cout << std::endl;
    // }
}