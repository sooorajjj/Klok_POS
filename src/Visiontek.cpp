#include "Visiontek.hpp"
#include <cstdlib>
#include <cstring>

extern "C"
{
    #include <0202lcd.h>
    #include <printer.h>
}

namespace lcd
{
    void DisplayText(unsigned char line_no, unsigned char column, const char *data, unsigned char font)
    {
        static unsigned char toDisplayBuffer[500] = {0};

        const int toCopy = std::strlen(data);
        if(toCopy > 0)
        {
            std::memset(toDisplayBuffer, 0, sizeof(toDisplayBuffer));
            std::strncpy((char*)toDisplayBuffer, (const char* )data, sizeof(toDisplayBuffer) - 1);
            lk_disptext(line_no, column, toDisplayBuffer, font);
        }
    }
}

namespace printer
{
    int WriteText(const char* text2, int len, int font)
    {
        static unsigned char toDisplayBuffer[25000] = {0};

        const int toCopy = len;
        if(toCopy > 0)
        {
            std::memset(toDisplayBuffer, 0, sizeof(toDisplayBuffer));
            std::strncpy((char*)toDisplayBuffer, (const char*)text2, sizeof(toDisplayBuffer) - 1);
            prn_write_text(toDisplayBuffer, len, font);
        }
    }
}
