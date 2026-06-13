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
                FILE_ATTRIBUTE_NORMAL,
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
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;

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
    timeouts.WriteTotalTimeoutConstant = 5000;
    timeouts.WriteTotalTimeoutMultiplier = 0;

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
    else
        isRunning = true;

    return true;
}

bool SerialPort::Accept(const std::vector<char>& data)
{
    writeThreads.push_back((std::thread(&SerialPort::AcceptThread, this, data)));
    return true;
}

void SerialPort::Stop()
{
    isRunning = false;

    if (readThread.joinable())
        readThread.join();

    for (auto& i : writeThreads)
    {
        if (i.joinable())
            i.join();
    }

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

void SerialPort::AcceptThread(const std::vector<char>& data)
{
    std::lock_guard<std::mutex> lock(mtx);

    if (!isRunning)
        errorCallback(connectionId, 0, "COM not running!");

    DWORD bytesWritten;

    if (!WriteFile(hComPort, data.data(), static_cast<DWORD>(data.size()), &bytesWritten, NULL))
    {
        DWORD error = GetLastError();
        errorCallback(connectionId, 0, "WriteFile failed: " + std::to_string(error));
        return;
    }

    if (bytesWritten == 0)
    {
        errorCallback(connectionId, 0, "Write stalled - no bytes written");
        return;
    }

    if (bytesWritten != data.size())
        errorCallback(connectionId, 0, "Failed to write all data!");
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

            if (error != ERROR_SUCCESS)
            {
                if (errorCallback)
                    errorCallback(connectionId, 0, std::to_string(error));

                break;
            }
        }

        if (bytesRead > 0 && targetPort != nullptr)
            targetPort->Accept(buffer);
    }
}

std::string SerialPort::GetPortName()
{
    return portName;
}
