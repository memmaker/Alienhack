// Web backend for RL_shared::Console (replaces RL-Shared/Console/Console.cpp).
// 80x40 cell grid; updateScreen hands the finished grid to JS, readKey waits
// (ASYNCIFY) for a key from the page's queue.
#include "Console/Console.hpp"
#include <emscripten.h>
#include <string.h>

EM_JS(void, web_update_screen, (const unsigned char* chars, const signed char* fg, const signed char* bg, int w, int h), {
  if (Module.rvipScreen) Module.rvipScreen(HEAPU8.subarray(chars, chars + w*h), HEAP8.subarray(fg, fg + w*h), HEAP8.subarray(bg, bg + w*h), w, h);
});
EM_JS(int, web_poll_key, (void), {
  var q = Module.rvipKeys; return (q && q.length) ? q.shift() : -1;
});

namespace RL_shared
{

const int CONSOLE_SIZE_X = 80;
const int CONSOLE_SIZE_Y = 40;

struct Console::ConsoleData
{
	unsigned char ch[CONSOLE_SIZE_Y*CONSOLE_SIZE_X];
	signed char fg[CONSOLE_SIZE_Y*CONSOLE_SIZE_X];
	signed char bg[CONSOLE_SIZE_Y*CONSOLE_SIZE_X];
	void clear() { memset(ch, 0, sizeof ch); memset(fg, 0, sizeof fg); memset(bg, 0, sizeof bg); }
};

Console::Console(void) { m_data.reset( new ConsoleData() ); m_data->clear(); }
void Console::clearScreen(void) { m_data->clear(); }
void Console::draw(int nX, int nY, char chr, Colour fore, Colour back)
{
	if ((nX < 0) || (nX >= CONSOLE_SIZE_X) || (nY < 0) || (nY >= CONSOLE_SIZE_Y)) return;
	int i = nY*CONSOLE_SIZE_X + nX;
	m_data->ch[i] = (unsigned char)chr;
	m_data->fg[i] = (signed char)fore;
	m_data->bg[i] = (signed char)back;
}
void Console::drawText(int nX, int nY, const char* text, Colour fore, Colour back)
{
	for (; *text; ++nX, ++text) draw(nX, nY, *text, fore, back);
}
void Console::updateScreen(void)
{
	web_update_screen(m_data->ch, m_data->fg, m_data->bg, CONSOLE_SIZE_X, CONSOLE_SIZE_Y);
}
void Console::sleep(int ms) { emscripten_sleep(ms > 0 ? ms : 0); }
KeyCode Console::readKey(void)
{
	int k;
	while ((k = web_poll_key()) < 0) emscripten_sleep(16);
	return KeyCode( (char)(k & 0xff), (k & 0x100) != 0 );
}
Console::ConsoleDims Console::getConsoleDimensions(void)
{
	ConsoleDims dims = {CONSOLE_SIZE_X, CONSOLE_SIZE_Y};
	return dims;
}

}
