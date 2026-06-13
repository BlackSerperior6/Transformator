#ifndef SERIALPORT_H
#define SERIALPORT_H

#include "abstractport.h"
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
    SerialPort(std::string serialPortName, int conId, PortType type, DWORD baud = CBR_9600, AbstractPort* target = nullptr);

    ~SerialPort();

    bool Accept(const std::vector<char> & data);

    bool Start();

    void Stop();

    void SetCallbackFunction(std::function<void(const std::vector<char>&)> callback);

    std::string GetPortName();

private:
    HANDLE hComPort;
    std::mutex mtx;
    std::thread readThread;
    std::vector<std::thread> writeThreads;
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
