#ifndef SERIALPORT_H
#define SERIALPORT_H

#include "abstractport.h"
#include "threadpool.h"
#include <windows.h>
#include "string"
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <mutex>

class SerialPort : public AbstractPort
{
public:
    SerialPort(std::string serialPortName, int conId, PortType type, DWORD baud = CBR_9600, void* parentConnection = nullptr, AbstractPort* target = nullptr);

    ~SerialPort();

    void Accept(const std::vector<char> & data) override;

    bool Start() override;

    void Stop() override;

    void SetCallbackFunction(std::function<void(const std::vector<char>&)> callback);

    std::string GetPortName();

private:
    HANDLE hComPort;
    std::mutex mtx;
    std::thread readThread;
    ThreadPool* acceptThreadsPool;
    std::atomic<bool> isRunning;
    std::string portName;
    DWORD baudRate;

    std::function<void(const std::vector<char>&)> dataCallback;
    std::string serialPortName;

    bool StartReading();

    void AcceptThread(const std::vector<char> & data);

    void ReadLoop();
};

#endif // SERIALPORT_H
