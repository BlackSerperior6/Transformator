#include "serialport.h"

SerialPort::SerialPort(std::string serialPortName, int conId, PortType type, DWORD baud, AbstractPort* target) :
    hComPort(INVALID_HANDLE_VALUE), portName(serialPortName), baudRate(baud),
    AbstractPort(conId, type, target) {}

SerialPort::~SerialPort()
{
    Stop();
}

bool SerialPort::Start()
{
    std::string fullPortName = "\\\\.\\" + portName;
            hComPort = CreateFileA(
                fullPortName.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                NULL
            );

    if (hComPort == INVALID_HANDLE_VALUE)
    {
        if (errorCallback)
            errorCallback(connectionId, 0, std::to_string(GetLastError()));

        return false;
    }

    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(hComPort, &dcb))
    {
        if (errorCallback)
            errorCallback(connectionId, 0, "Failed to get COM port state");

        CloseHandle(hComPort);
        return false;
    }

    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(hComPort, &dcb))
    {
        if (errorCallback)
            errorCallback(connectionId, 0, "Failed to set COM port state");

        CloseHandle(hComPort);
        return false;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hComPort, &timeouts))
    {
        if (errorCallback)
            errorCallback(connectionId, 0, "Failed to set COM port timeouts");

        CloseHandle(hComPort);
        return false;
    }

    PurgeComm(hComPort, PURGE_RXCLEAR | PURGE_TXCLEAR);

    if (targetPort != nullptr)
        StartReading();

    return true;
}

bool SerialPort::Accept(const std::vector<char> & data)
{
    if (!isRunning)
        return false;

    DWORD bytesWritten;

    if (!WriteFile(hComPort, data.data(), static_cast<DWORD>(data.size()), &bytesWritten, NULL))
    {
        if (errorCallback)
            errorCallback(connectionId, 0, std::to_string(GetLastError()));

        return false;
    }

    if (bytesWritten != data.size())
    {
        errorCallback(connectionId, 0, "Failed to write all data!");
        return false;
    }

    return true;
}

void SerialPort::Stop()
{
    isRunning = false;

    if (readThread.joinable())
        readThread.join();

    if (hComPort != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hComPort);
        hComPort = INVALID_HANDLE_VALUE;
    }
}

void SerialPort::SetCallbackFunction(std::function<void (const std::vector<char> &)> callback)
{
    dataCallback = callback;
}

bool SerialPort::StartReading()
{
    if (hComPort == INVALID_HANDLE_VALUE)
    {
        if (errorCallback)
            errorCallback(connectionId, 0, "COM port not open");

        return false;
    }

    isRunning = true;
    readThread = std::thread(&SerialPort::ReadLoop, this);
    return true;
}

void SerialPort::ReadLoop()
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
                if (errorCallback)
                    errorCallback(connectionId, 0, std::to_string(error));

                break;
            }
        }

        if (targetPort != nullptr)
            targetPort->Accept(buffer);
    }
}

std::string SerialPort::GetPortName()
{
    return portName;
}
