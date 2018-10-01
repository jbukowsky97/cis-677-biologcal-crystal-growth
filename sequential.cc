#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <random>
#include <regex>
#include <tuple>

/**********************************************************************
 * generates a random point outside of the radius of the crystal
***********************************************************************/
std::tuple<int, int> generatePoint(std::default_random_engine& generator, const int gridSize, const int center, const int radius) {
    std::uniform_int_distribution<int> distribution(0, gridSize - 1);
    int x, y;
    do {
        x = distribution(generator);
        y = distribution(generator);
    } while (abs(center - x) <= radius + 1 && abs(center - y) <= radius + 1);
    return std::make_tuple(x, y);
}

/**********************************************************************
 * calculates the next random move for a particle
 * 
 * note: the next move could cause particle to leave the lattice
***********************************************************************/
std::tuple<int, int> nextMove(std::default_random_engine& generator) {
    std::uniform_int_distribution<int> distribution(-1, 1);
    int dx = distribution(generator);
    int dy = distribution(generator);
    return std::make_tuple(dx, dy);
}

/* determines if the current particle should stick to the crystal */
bool shouldStick(std::vector<std::vector<char>>& grid, const int gridSize, const int x, const int y) {
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            const int newX = x + dx;
            const int newY = y + dy;
            if (newX >= 0 && newX < gridSize && newY >= 0 && newY < gridSize) {
                if (grid[newX][newY] == 'X') {
                    return true;
                }
            }
        }
    }
    return false;
}

/**********************************************************************
 * walks particle until it leaves lattice or sticks to the crystal
***********************************************************************/
void walkParticle(std::default_random_engine& generator, std::vector<std::vector<char>>& grid, const int gridSize, int& x, int& y) {
    while (x >= 0 && x < gridSize && y >= 0 && y < gridSize) {
        /* check if should stick */
        if (shouldStick(grid, gridSize, x, y)) {
            grid[x][y] = 'X';
            return;
        }

        /* generate next move */
        const std::tuple<int, int> direction = nextMove(generator);
        const int dx = std::get<0>(direction);
        const int dy = std::get<1>(direction);

        x += dx;
        y += dy;
    }
}

/**********************************************************************
 * write result to file
***********************************************************************/
void writeToFile(const std::vector<std::vector<char>>& grid, const int gridSize) {
    std::ofstream myfile;
    myfile.open("sequential_result.txt");
    for (int i = 0; i < gridSize; i++) {
        for (int k = 0; k < gridSize; k++) {
            int value = 0;
            if (grid[i][k] != 0) {
                value = 1;
            }
            if (k != 0) {
                myfile << ",";
            }
            myfile << value;
        }
        if (i != gridSize - 1) {
            myfile << "\n";
        }
    }
    myfile.close();
}

/**********************************************************************
 * print crude result visual to console
***********************************************************************/
void consoleVisual(const std::vector<std::vector<char>>& grid, const int gridSize) {
    /* print crude depiction of final crystal inside lattice */
    for (int i = 0; i < gridSize; i++) {
        for (int k = 0; k < gridSize; k++) {
            char value = grid[i][k];
            if (value == 0) {
                value = '-';
            }
            std::cout << value << " ";
        }
        std::cout << std::endl;
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
    int tempSize;
    unsigned long tempParticles;
    try {
        tempSize = std::stoi(gridSizeStr);
        tempParticles = std::stoul(numParticlesStr);
    } catch (...) {
        std::cerr << "Grid Size and Number of Particles must be positive integers" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* fail if gridSize is even */
    if (tempSize % 2 == 0) {
        std::cerr << "Grid Size must be odd" << std::endl;
        exit(EXIT_FAILURE);
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    const int gridSize = tempSize;
    const unsigned long numParticles = tempParticles;

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

    /* sequentially run each particle through its journey in the lattice */
    for (unsigned long p = 0; p < numParticles; p++) {
        /* check if radius is the entire grid */
        if (radius >= gridSize / 2 - 1) {
            break;
        }

        /* generate point */
        const auto point = generatePoint(generator, gridSize, center, radius);
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

    auto end_time = std::chrono::high_resolution_clock::now();
    
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0 << " s" << std::endl;

    writeToFile(grid, gridSize);
}