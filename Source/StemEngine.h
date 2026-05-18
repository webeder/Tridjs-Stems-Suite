#pragma once

#include <string>
#include <functional>

class StemEngine {
public:
    static StemEngine& getInstance();

    // Engine statuses
    enum class Status {
        Idle,
        Booting,
        CudaLoading,
        ModelLoading,
        WarmingUp,
        Ready,
        Failed
    };

    // Lifecycle
    void initialize();
    void loadModel(const std::string& modelPath);
    void warmupCUDA();
    void shutdown();

    // Process a track and trigger callbacks for progress or check for cancel requests
    bool processTrack(const std::string& inputPath, 
                      const std::string& outputDir, 
                      std::function<void(float)> progressCallback,
                      std::function<bool()> shouldCancelCallback);

    bool isInitialized() const { return initialized; }
    bool isModelLoaded() const { return modelLoaded; }
    bool isWarmedUp() const { return warmedUp; }
    
    Status getStatus() const { return currentStatus; }
    std::string getStatusString() const;

private:
    StemEngine() = default;
    ~StemEngine() { shutdown(); }
    StemEngine(const StemEngine&) = delete;
    StemEngine& operator=(const StemEngine&) = delete;

    bool initialized = false;
    bool modelLoaded = false;
    bool warmedUp = false;
    Status currentStatus = Status::Idle;
    std::string errorMessage;

    // Opaque private implementation wrapper to completely isolate PyTorch headers
    struct Impl;
    Impl* impl = nullptr;
};
