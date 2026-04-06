#include <cmath>

// Calculate Mean Squared Error for two flat vectors
template<typename T>
T mse_loss(std::vector<T>& groundTruth, std::vector<T>& prediction) {
    static_assert(groundTruth.size() == prediction.size(), "Vectors should be of same length");
    T mse = 0.0f;
    for (__uint32_t i = 0; i < groundTruth.size(); ++i) {
        mse += std::pow(groundTruth[i] - prediction[i], 2) / groundTruth.size();
    }
    return mse;
}