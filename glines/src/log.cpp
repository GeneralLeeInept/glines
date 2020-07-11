#include "log.h"

#include <vector>
#include <cstdarg>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>


void logf(const char* format, ...)
{
    static constexpr int log_buffer_size = 256;
    static char log_buffer[log_buffer_size];
    std::va_list args;
    va_start(args, format);
    int len = std::vsnprintf(log_buffer, log_buffer_size, format, args);
    va_end(args);

    if (len < (log_buffer_size - 1))
    {
        logm(log_buffer);
    }
    else
    {
        std::vector<char> buffer(len + 1);
        std::vsnprintf(buffer.data(), buffer.size(), format, args);
        logm(buffer.data());
    }
}


void logm(const char* message)
{
    OutputDebugStringA(message);
}
