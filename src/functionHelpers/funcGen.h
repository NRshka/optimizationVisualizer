#include <vector>
#include <algorithm>
#include <cmath>
#include <concepts>


struct Vertex3D {float x, y, z;};
struct Smooth3DFunction {
    __uint32_t nx;
    __uint32_t nz;
    std::vector<Vertex3D> vertices;
    std::vector<__uint32_t> indices;
};

// Just random smooth function
// TODO: find something nice to look at
template<typename T>
constexpr T randomFunction(T x, T z) {
    return std::pow(std::sin(x), 2) - std::cos(z);
}


inline __uint32_t id(__uint32_t i, __uint32_t j, __uint32_t nx) { return i + j * (nx + 1); }


// Returns a 1D vector of real numbers that scales from `start` to `end` with length `count`.
// Basically, a very simple version of numpy's linspace(...)
template<typename T>
inline std::vector<T> linspace(T start, T end, __uint32_t count) {
    std::vector<T> result(count);
    T step = (end - start) / count;
    for (T x = start; x < end; x += step) {
        result.push_back(x);
    }
    return result;
}


Smooth3DFunction getRandomFunction(__uint32_t nx, __uint32_t nz) {
    auto xPoints = linspace(0.0f, 10.0f, nx);
    auto zPoints = linspace(0.0f, 10.0f, nz);
    std::vector<Vertex3D> vertices(nx * nz);
    std::vector<__uint32_t> indices(nx * nz);

    __uint32_t j = 0;
    for (float z: zPoints) {
        __uint32_t i = 0;
        for (float x: xPoints) {
            float y = randomFunction(x, z);
            vertices.push_back({x, y, z});

            __uint32_t a = id(i, j, nx);
            __uint32_t b = id(i + 1, j, nx);
            __uint32_t c = id(i + 1, j + 1, nx);
            __uint32_t d = id(i, j + 1, nx);

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);

            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(d);
        }
    }

    return {nx, nz, vertices, indices};
}