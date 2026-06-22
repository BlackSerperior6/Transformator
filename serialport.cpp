#include "serialport.h"

SerialPort::SerialPort(std::string serialPortName, int conId, PortType type, DWORD baud, void* parentConnection, AbstractPort* target) :
    hComPort(INVALID_HANDLE_VALUE), portName(serialPortName), baudRate(baud),
    AbstractPort(conId, type, parentConnection, target), acceptThreadsPool(new ThreadPool(2))  {}

SerialPort::~SerialPort()
{
    Stop();
}

bool SerialPort::Start()
{
    std::string fullPortName = "\\\\.\\" + portName;

    if (portName.empty())
    {
        errorCallback(connectionId, 0, "Empty port name");
        return false;
    }

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

    COMSTAT comStat;
    DWORD errors;

    if (!ClearCommError(hComPort, &errors, &comStat))
    {
        errorCallback(connectionId, 0, "Failed to clear comm errors");
        CloseHandle(hComPort);
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

    DCB originalDcb = dcb;

    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    dcb.fOutxCtsFlow = TRUE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fOutxDsrFlow = FALSE;

    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fNull = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fAbortOnError = FALSE;

    if (!SetCommState(hComPort, &dcb))
    {
        SetCommState(hComPort, &originalDcb);

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

    if (!PurgeComm(hComPort, PURGE_RXCLEAR | PURGE_TXCLEAR))
        errorCallback(connectionId, 0, "Failed to purge buffers");

    if (targetPort != nullptr)
        StartReading();
    else
        isRunning = true;

    return true;
}

void SerialPort::Accept(const std::vector<char>& data)
{
    acceptThreadsPool->AddTask([this, data]{this->AcceptThread(data);});
}

void SerialPort::Stop()
{
    isRunning = false;

    delete acceptThreadsPool;

    CancelIoEx(hComPort, NULL);

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

void SerialPort::AcceptThread(const std::vector<char>& data)
{
    DWORD bytesWritten;

    {
        std::lock_guard<std::mutex> lock(transferMutex);

        if (!isRunning)
        {
            errorCallback(connectionId, 0, "COM not running!");
            return;
        }

        if (!WriteFile(hComPort, data.data(), static_cast<DWORD>(data.size()), &bytesWritten, NULL))
        {
            DWORD error = GetLastError();
            errorCallback(connectionId, 0, "WriteFile failed: " + std::to_string(error));
            return;
        }
    }


    if (bytesWritten == 0)
    {
        errorCallback(connectionId, 0, "Write stalled - no bytes written");
        return;
    }

    if (bytesWritten != data.size())
    {
        errorCallback(connectionId, 0, "Failed to write all data!");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(baseAcceptMutex);
        AbstractPort::Accept(data);
    }
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

            if (error != ERROR_SUCCESS && error != ERROR_OPERATION_ABORTED)
            {
                if (errorCallback)
                    errorCallback(connectionId, 0, std::to_string(error));

                break;
            }
        }

        if (bytesRead > 0)
            targetPort->Accept(buffer);
    }
}

std::string SerialPort::GetPortName()
{
    return portName;
}
