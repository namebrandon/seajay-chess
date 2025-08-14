#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

int main() {
    // Simulate the bug scenario
    milliseconds elapsed(0);  // Very fast search, shows as 0ms
    milliseconds softLimit(490);
    milliseconds hardLimit(981);
    
    // Predict next iteration time
    int iterationTime = 204;  // Last iteration took 204ms (from logs)
    double ebf = 10.19;  // From logs
    
    // This is the calculation from predictNextIterationTime
    double clampedEBF = ebf;
    if (clampedEBF < 1.5) clampedEBF = 1.5;
    if (clampedEBF > 10.0) clampedEBF = 10.0;
    
    double depthFactor = 1.0;  // depth 6, no adjustment yet
    double predictedTime = iterationTime * clampedEBF * depthFactor;
    predictedTime *= 1.2;  // Safety margin
    
    milliseconds predicted(static_cast<int64_t>(predictedTime));
    
    cout << "Elapsed: " << elapsed.count() << "ms\n";
    cout << "Predicted: " << predicted.count() << "ms\n";
    cout << "Soft limit: " << softLimit.count() << "ms\n";
    cout << "Hard limit: " << hardLimit.count() << "ms\n";
    cout << "Elapsed + predicted: " << (elapsed + predicted).count() << "ms\n";
    cout << "Exceeds soft? " << ((elapsed + predicted) > softLimit) << "\n";
    cout << "Exceeds hard? " << ((elapsed + predicted) > hardLimit) << "\n";
    
    // The real issue: iterationTime passed to predictNextIterationTime
    // Let's check what happens if iterationTime is wrong
    cout << "\n--- If iterationTime is wrong ---\n";
    iterationTime = 0;  // If search reported 0ms
    predictedTime = iterationTime * clampedEBF * depthFactor * 1.2;
    predicted = milliseconds(static_cast<int64_t>(predictedTime));
    cout << "Predicted with 0ms iter time: " << predicted.count() << "ms\n";
    
    // Or if we use a fallback
    if (iterationTime <= 0) {
        predicted = milliseconds(1000000);  // Fallback from predictNextIterationTime
        cout << "Fallback predicted: " << predicted.count() << "ms\n";
        cout << "Exceeds hard? " << ((elapsed + predicted) > hardLimit) << "\n";
    }
    
    return 0;
}
