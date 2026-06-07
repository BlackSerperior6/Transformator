#ifndef SERIALPORT_H
#define SERIALPORT_H

#include "abstractport.h"
#include <windows.h>
#include "string"
#include <thread>
#include <atomic>
#include <functional>
#include <vector>

class SerialPort : public AbstractPort
{
public:
    SerialPort(std::string serialPortName, int conId, PortType type, DWORD baud = CBR_9600, AbstractPort* target = nullptr);

    ~SerialPort();

    bool Accept(const std::vector<char> & data);

    bool Start();

    bool StartReading();

    void Stop();

    void SetCallbackFunction(std::function<void(const std::vector<char>&)> callback);

    void SetErrorCallbackFunction(std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> callback);

private:
    HANDLE hComPort;
    std::thread readThread;
    std::atomic<bool> isRunning;
    std::string portName;
    DWORD baudRate;

    std::function<void(const std::vector<char>&)> dataCallback;
    std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> errorCallback;
    std::string serialPortName;

    void ReadLoop()
    {
        const int BUFFER_SIZE = 4096;
        std::vector<char> buffer(BUFFER_SIZE);

        while (isRunning)
        {
            DWORD bytesRead = 0;

            if (!ReadFile(hComPort, buffer.data(), BUFFER_SIZE - 1, &bytesRead, NULL))
            {
                DWORD error = GetLastError();

                if (error != ERROR_IO_PENDING && error != ERROR_SUCCESS)
                {
                    errorCallback(connectionId, 0, std::to_string(error));
                    break;
                }
            }

            if (targetPort != nullptr)
                targetPort->Accept(buffer);

            // Small sleep to prevent CPU spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

    }

};

#endif // SERIALPORT_H
