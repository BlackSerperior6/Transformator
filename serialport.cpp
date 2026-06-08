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
        errorCallback(connectionId, 0, std::to_string(GetLastError()));
        return false;
    }

    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(hComPort, &dcb))
    {
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
        errorCallback(connectionId, 0, "Failed to set COM port timeouts");
        CloseHandle(hComPort);
        return false;
    }

    PurgeComm(hComPort, PURGE_RXCLEAR | PURGE_TXCLEAR);

    StartReading();
}

void SerialPort::SetCallbackFunction(std::function<void (const std::vector<char> &)> callback)
{
    dataCallback = callback;
}

bool SerialPort::Accept(const std::vector<char> & data)
{
    if (!isRunning)
        return false;

    DWORD bytesWritten;

    if (!WriteFile(hComPort, data.data(), static_cast<DWORD>(data.size()), &bytesWritten, NULL))
    {
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

bool SerialPort::StartReading()
{
    if (hComPort == INVALID_HANDLE_VALUE)
    {
        errorCallback(connectionId, 0, "COM port not open");
        return false;
    }

    isRunning = true;
    readThread = std::thread(&SerialPort::ReadLoop, this);
}

void SerialPort::Stop()
{
    isRunning = false;

    if (readThread.joinable())
    {
        readThread.join();
    }

    if (hComPort != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hComPort);
        hComPort = INVALID_HANDLE_VALUE;
    }
}
