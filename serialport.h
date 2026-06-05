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
    SerialPort(std::string serialPortName, DWORD baud = CBR_9600);

    ~SerialPort();

    bool Accept(std::string errorMessage, const std::vector<char> & data);

    bool Open(std::string errorMessage);

    bool StartReading(std::string errorMessage);

    void Stop();

    void SetCallbaclFunction(std::function<void(const std::vector<char>&)> callback);

private:
    HANDLE hComPort;
    std::thread readThread;
    std::atomic<bool> isRunning;
    std::string portName;
    DWORD baudRate;

    std::function<void(const std::vector<char>&)> dataCallback;
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
                    break;
            }

            if (targetPort != nullptr)
            {
                std::string errorMessage;
                targetPort->Accept(errorMessage, buffer);
            }

            // Small sleep to prevent CPU spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

    }

};

#endif // SERIALPORT_H
