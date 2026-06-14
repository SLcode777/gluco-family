#include "Ecran/pageAbout.h"
#include "Ecran/Gestion.h"
#include <U8g2lib.h>
#include "Config.h"
#include "Langues/License.h"


static int16_t idxVisu = 0;
void Impression(int16_t delta);
void pageAboutSetup()
{
    PageActu = pageAbout;
    idxVisu=0;
    Impression(0);
}
// Re-flow text to a max number of characters per line, breaking on spaces only
// (never mid-word) and preserving existing newlines. Operates on already-
// converted (single-byte) text so the char count matches the displayed glyphs.
static String wrapToWidth(const String &src, int maxChars)
{
    String out = "";
    String line = "";
    int n = src.length(), i = 0;
    while (i < n)
    {
        if (src[i] == '\n')
        {
            out += line + "\n";
            line = "";
            i++;
            continue;
        }
        int j = i;
        while (j < n && src[j] != ' ' && src[j] != '\n')
            j++;
        String word = src.substring(i, j);
        if (line.length() == 0)
            line = word;
        else if ((int)(line.length() + 1 + word.length()) <= maxChars)
            line += " " + word;
        else
        {
            out += line + "\n";
            line = word;
        }
        i = j;
        while (i < n && src[i] == ' ') // collapse runs of spaces
            i++;
    }
    out += line;
    return out;
}

// Build + word-wrap the page once and cache it (rebuilt only if the language
// changes). Re-wrapping the whole licence on every scroll tick was the main
// cause of the sluggish feel.
static String cachedText = "";
static int8_t cachedLang = -1;

static const String &aboutText()
{
    if (cachedText.length() == 0 || cachedLang != LaLangue)
    {
        String t;
        if (LaLangue == LANG_FR)
            t = String(Avertissement) + "       \n\n=== Licence ===\n" + License;
        else
            t = String(Disclaimer) + "     \n\n=== License ===\n" + License;
        cachedText = wrapToWidth(utf8ToLatin15(t), (EcranW - 2) / 8);
        cachedLang = LaLangue;
    }
    return cachedText;
}

// Returns the char index of the line start `n` lines away from `pos`
// (n > 0 moves forward / down, n < 0 backward / up).
static int moveLines(const String &s, int pos, int n)
{
    if (n > 0)
    {
        for (int k = 0; k < n; k++)
        {
            int p = s.indexOf('\n', pos);
            if (p < 0)
                break; // already on the last line
            pos = p + 1;
        }
    }
    else
    {
        for (int k = 0; k < -n; k++)
        {
            if (pos <= 0)
            {
                pos = 0;
                break;
            }
            int p = s.lastIndexOf('\n', pos - 2);
            pos = (p < 0) ? 0 : p + 1;
        }
    }
    return pos;
}

void Impression(int16_t delta)
{
    const String &Texte = aboutText();

    // Scroll by whole lines, proportional to the swipe (delta = pixels / 2).
    // delta < 0 (finger up) scrolls down into the text. Tune the "/ 2" to make
    // scrolling faster (smaller divisor) or slower (larger divisor).
    int step = -delta / 2;
    if (delta != 0 && step == 0)
        step = (delta > 0) ? -1 : 1;
    if (step > 25)
        step = 25;
    if (step < -25)
        step = -25;
    idxVisu = moveLines(Texte, idxVisu, step);

    // Don't scroll so far that the screen goes (almost) empty.
    int16_t Lmax = Texte.length() - 100;
    if (Lmax < 0)
        Lmax = 0;
    if (idxVisu > Lmax)
        idxVisu = Lmax;
    if (idxVisu < 0)
        idxVisu = 0;

    CanvaBase->setTextColor(RGB565_WHITE);
    CanvaBase->setFont(u8g2_font_8x13_tf); // monospace font for long text
    CanvaBase->fillScreen(RGB565_NAVY);
    CanvaBase->setCursor(0, 0);
    CanvaBase->print(Texte.substring(idxVisu, idxVisu + 2600));
    CanvaBase->flush();
}
void handleTouch_About(uint16_t touchX, uint16_t touchY, int16_t DeltaTouchY)
{
    int16_t delta = DeltaTouchY / 2;

    if (delta != 0)
    {

        Impression(delta);
    }
}