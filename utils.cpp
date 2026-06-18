#include <tcpstatuscode.h>
#include <string>

static class Utils
{
public:

    static TcpStatusCode ParseStatusCode(const std::string& response)
    {
        size_t statusPos = response.find("Status:");

        if (statusPos != std::string::npos)
        {
            try {
                size_t startPos = statusPos + 7;
                while (startPos < response.length() && isspace(response[startPos]))
                    startPos++;

                int code = std::stoi(response.substr(startPos));
                return static_cast<TcpStatusCode>(code);
            }
            catch (...)
            {
                for (size_t i = 0; i < response.length() - 2; ++i)
                {
                    if (isdigit(response[i]) && isdigit(response[i+1]) && isdigit(response[i+2]))
                    {
                        int code = (response[i] - '0') * 100 +
                                  (response[i+1] - '0') * 10 +
                                  (response[i+2] - '0');

                        return static_cast<TcpStatusCode>(code);
                    }
                }
            }
        }

        if (response.find("SUCCESS") != std::string::npos)
            return TcpStatusCode::SUCCESS;

        return TcpStatusCode::UNKNOWN;
    }
};
