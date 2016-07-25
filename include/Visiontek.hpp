#ifndef VISIONTEK_ABSTRACTION_LIBRARY_H
#define VISIONTEK_ABSTRACTION_LIBRARY_H

#include <stdint.h>

namespace lcd
{
    /*
        abstracts this method to allow simple printing
        lk_disptext(unsigned char line_no, unsigned char column, unsigned char *data, unsigned char font)
    */

    void DisplayText(unsigned char line_no, unsigned char column, const char *data, unsigned char font);
}

namespace printer
{
    // int prn_write_text ( unsigned char ∗ text2, int len, int font )
    int WriteText(const char *text2, int len, int font);
}

#endif
