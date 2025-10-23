#include <JuceHeader.h>

// Include all test files
#include "RaopClientTests.cpp"
#include "StreamBufferTests.cpp"
#include "AudioEncoderTests.cpp"
#include "AirPlayDeviceTests.cpp"
#include "ErrorHandlingTests.cpp"

int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    // Print results summary
    int numTests = 0;
    int numPasses = 0;
    int numFailures = 0;
    
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        numTests += result->passes + result->failures;
        numPasses += result->passes;
        numFailures += result->failures;
    }
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << numTests << std::endl;
    std::cout << "Passed: " << numPasses << std::endl;
    std::cout << "Failed: " << numFailures << std::endl;
    std::cout << "===================" << std::endl;
    
    return numFailures > 0 ? 1 : 0;
}