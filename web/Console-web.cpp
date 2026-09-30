// Web backend for RL_shared::Console (replaces RL-Shared/Console/Console.cpp).
// 80x40 cell grid; updateScreen hands the finished grid to JS, readKey waits
// (ASYNCIFY) for a key from the page's queue.
#include "Console/Console.hpp"
#include <emscripten.h>
#include <string.h>
#include <string>

EM_JS(int, web_poll_key, (void), {
  if (Module.rvipPoll) Module.rvipPoll();
  var q = Module.rvipKeys; return (q && q.length) ? q.shift() : -1;
});
// RVIP stage 5: each window gets its own pane from here (rule 6/7). Text panes are lines
// where "\x01<fg><bg>" ('a' + Console::Colour) sets the colour of the text after it.
EM_JS(void, web_map, (const unsigned char* chars, const signed char* fg, const signed char* bg, int w, int h), {
  if (Module.rvipMap) Module.rvipMap(HEAPU8.slice(chars, chars + w*h), HEAP8.slice(fg, fg + w*h), HEAP8.slice(bg, bg + w*h), w, h);
});
EM_JS(void, web_panes, (const char* status, const char* info, const char* popup, int at_cmd), {
  if (Module.rvipPanes) Module.rvipPanes(UTF8ToString(status), UTF8ToString(info), UTF8ToString(popup), !!(at_cmd & 1), !!(at_cmd & 2));
});
EM_JS(void, rvip_pane, (const char* id, const char* text), {
  if (Module.rvipPane) Module.rvipPane(UTF8ToString(id), UTF8ToString(text));
});
EM_JS(void, rvip_msg, (const char* text, int fresh), {
  if (Module.rvipMsg) Module.rvipMsg(UTF8ToString(text), !!fresh);
});
EM_JS(void, rvip_sync, (void), {
  if (Module.rvipSync) Module.rvipSync();
});
EM_JS(void, web_palette, (const char* css), {
  Module.rvipPalette = UTF8ToString(css).split(' ');
});

// Set by the game while auto-explore / a stair walk runs (PlayingGame.cpp).
extern "C" { bool rvip_auto_on = false; }
// RVIP stage 3: rvip_next_key (>= 0, 0x100 = ext) is returned by the next
// readKey: menus queue the CMD key (PlayingGame runs rvip_cmd) or forward a key.
// rvip_numpad_raw is set while a menu wants raw keypad keys (page: 0x200|char).
extern "C" { int rvip_next_key = -1; bool rvip_numpad_raw = false; }
// RVIP stage 5: cells drawn while rvip_base > 0 belong to the main screen (RvipBase in
// PlayingGame/LookMode/TargetSelect::draw); rvip_main_screen = PlayingGame drew it.
extern "C" { int rvip_base = 0; bool rvip_main_screen = false; }
namespace AlienHack { std::string rvip_zone_name; }

// The console's 16 colours (Windows console palette), index = Console::Colour.
static const char* const RVIP_CSS[15] = { "#000000", "#aa0000", "#00aa00", "#0000aa", "#aa5500", "#00aaaa", "#aa00aa", "#aaaaaa",
	"#ff5555", "#55ff55", "#5555ff", "#ffff55", "#55ffff", "#ff55ff", "#ffffff" };
extern "C" const char* rvip_css( int c ) { return RVIP_CSS[(c >= 0 && c < 15) ? c : 7]; }

namespace RL_shared
{

const int CONSOLE_SIZE_X = 80;
const int CONSOLE_SIZE_Y = 40;

struct Console::ConsoleData
{
	unsigned char ch[CONSOLE_SIZE_Y*CONSOLE_SIZE_X];
	signed char fg[CONSOLE_SIZE_Y*CONSOLE_SIZE_X];
	signed char bg[CONSOLE_SIZE_Y*CONSOLE_SIZE_X];
	unsigned char mk[CONSOLE_SIZE_Y*CONSOLE_SIZE_X];	// 0 untouched, 1 main screen, 2 drawn over it
	bool based;
	void clear() { memset(ch, 0, sizeof ch); memset(fg, 0, sizeof fg); memset(bg, 0, sizeof bg); memset(mk, 0, sizeof mk); based = false; rvip_main_screen = false; }
};

Console::Console(void)
{
	m_data.reset( new ConsoleData() ); m_data->clear();
	std::string p;
	for (int i = 0; i < 15; ++i) { if (i) p += ' '; p += RVIP_CSS[i]; }
	web_palette(p.c_str());
}
void Console::clearScreen(void) { m_data->clear(); }
void Console::draw(int nX, int nY, char chr, Colour fore, Colour back)
{
	if ((nX < 0) || (nX >= CONSOLE_SIZE_X) || (nY < 0) || (nY >= CONSOLE_SIZE_Y)) return;
	int i = nY*CONSOLE_SIZE_X + nX;
	m_data->ch[i] = (unsigned char)chr;
	m_data->fg[i] = (signed char)fore;
	m_data->bg[i] = (signed char)back;
	m_data->mk[i] = rvip_base > 0 ? 1 : 2;
	if (rvip_base > 0) m_data->based = true;
}

// One text line of cells [x0, x1] in row y whose mark passes keep(); trailing blanks trimmed.
template< class D, class Keep >
static std::string rvipLine( const D& d, int y, int x0, int x1, Keep keep )
{
	std::string out, pend;
	int cf = -2, cb = -2;
	for (int x = x0; x <= x1; ++x)
	{
		int i = y*CONSOLE_SIZE_X + x;
		bool on = keep(d.mk[i]) && d.ch[i] && (d.ch[i] != ' ' || (d.bg[i] > 0));
		if (!on) { pend += ' '; continue; }
		out += pend; pend.clear();
		int f = d.fg[i] < 0 ? 7 : d.fg[i], b = d.bg[i] < 0 ? 0 : d.bg[i];
		if (f != cf || b != cb) { out += '\x01'; out += (char)('a' + f); out += (char)('a' + b); cf = f; cb = b; }
		unsigned char c = d.ch[i];
		out += (c < 32 || c > 126) ? '#' : (char)c;
	}
	return out;
}
template< class D, class Keep >
static std::string rvipBlock( const D& d, int x0, int y0, int x1, int y1, Keep keep )
{
	std::string out, pend;
	for (int y = y0; y <= y1; ++y)
	{
		std::string l( rvipLine(d, y, x0, x1, keep) );
		if (l.empty()) { pend += '\n'; continue; }
		if (!out.empty()) out += '\n' + pend;
		out += l; pend.clear();
	}
	return out;
}
static bool rvipAny( unsigned char ) { return true; }
static bool rvipMain( unsigned char m ) { return m == 1; }
static bool rvipOver( unsigned char m ) { return m == 2; }
void Console::drawText(int nX, int nY, const char* text, Colour fore, Colour back)
{
	for (; *text; ++nX, ++text) draw(nX, nY, *text, fore, back);
}
void Console::updateScreen(void)
{
	ConsoleData& d( *m_data );
	{	// one-window mode: the whole screen as text rows (all 40 kept in place, trailing blanks trimmed)
		static std::string last("\x02");
		std::string scr, pend;
		for (int y = 0; y < CONSOLE_SIZE_Y; ++y)
		{
			std::string l( rvipLine(d, y, 0, CONSOLE_SIZE_X-1, rvipAny) );
			if (l.empty()) { pend += '\n'; continue; }
			scr += pend + l + '\n'; pend.clear();
		}
		if (!scr.empty()) scr.erase(scr.length()-1);
		if (scr != last) { last = scr; rvip_pane("screen", scr.c_str()); }
	}
	std::string status, info, popup;
	bool over = false;
	int bx0 = CONSOLE_SIZE_X, by0 = CONSOLE_SIZE_Y, bx1 = -1, by1 = -1;
	for (int y = 0; y < CONSOLE_SIZE_Y; ++y)
		for (int x = 0; x < CONSOLE_SIZE_X; ++x)
		{
			int i = y*CONSOLE_SIZE_X + x;
			bool in = d.based ? (d.mk[i] == 2) : (d.ch[i] && d.ch[i] != ' ');
			if (!in) continue;
			over = true;
			if (x < bx0) bx0 = x; if (x > bx1) bx1 = x; if (y < by0) by0 = y; if (y > by1) by1 = y;
		}
	if (over) popup = d.based ? rvipBlock(d, bx0, by0, bx1, by1, rvipOver) : rvipBlock(d, bx0, by0, bx1, by1, rvipAny);
	if (d.based)
	{	// Map = the world view (the game centres it on the player); Status = zone + HUD;
		// info = the look / target description rows under the map (prompt line)
		const int MX0 = 2, MX1 = 41, MY0 = 1, MY1 = 33;
		static unsigned char mc[(MY1-MY0+1)*(MX1-MX0+1)];
		static signed char mf[(MY1-MY0+1)*(MX1-MX0+1)], mb[(MY1-MY0+1)*(MX1-MX0+1)];
		int n = 0;
		for (int y = MY0; y <= MY1; ++y)
			for (int x = MX0; x <= MX1; ++x, ++n)
			{
				int i = y*CONSOLE_SIZE_X + x;
				bool on = d.mk[i] == 1;
				mc[n] = on ? d.ch[i] : 0; mf[n] = on ? d.fg[i] : 0; mb[n] = on ? d.bg[i] : 0;
			}
		web_map(mc, mf, mb, MX1-MX0+1, MY1-MY0+1);
		status = AlienHack::rvip_zone_name + "\n" + rvipBlock(d, 45, 1, 77, 19, rvipMain);
		info = rvipBlock(d, 2, 35, 41, 38, rvipMain);
	}
	web_panes(status.c_str(), info.c_str(), popup.c_str(), (d.based && rvip_main_screen && popup.empty() ? 1 : 0) | (d.based ? 2 : 0));
}
void Console::sleep(int ms) { emscripten_sleep(ms > 0 ? ms : 0); }
KeyCode Console::readKey(void)
{
	int k;
	if (rvip_next_key >= 0) { k = rvip_next_key; rvip_next_key = -1; return KeyCode( (char)(k & 0xff), (k & 0x100) != 0 ); }
	if (rvip_auto_on)
	{	// paint the step, then either a real key cancels (and is dropped) or the next step runs
		emscripten_sleep(40);
		if ((k = web_poll_key()) < 0)
			return KeyCode( (char)1, true );
		rvip_auto_on = false;
	}
	while ((k = web_poll_key()) < 0) emscripten_sleep(16);
	if (k & 0x400)	// Ctrl+letter (page sends 0x400|'a'..'z') -> ext 0xA0+index
		return KeyCode( (char)(0xA0 + ((k & 0xff) - 'a')), true );
	if (k & 0x200)
	{	// keypad: raw (ext char) in menus, else directions / 5 = wait
		char c = (char)(k & 0xff);
		if (rvip_numpad_raw) return KeyCode( c, true );
		switch (c)
		{
		case '8': return KeyCode( 72, true ); case '2': return KeyCode( 80, true );
		case '4': return KeyCode( 75, true ); case '6': return KeyCode( 77, true );
		case '7': return KeyCode( 71, true ); case '9': return KeyCode( 73, true );
		case '1': return KeyCode( 79, true ); case '3': return KeyCode( 81, true );
		case '5': return KeyCode( '.', false );
		default: return KeyCode( c, false );
		}
	}
	return KeyCode( (char)(k & 0xff), (k & 0x100) != 0 );
}
Console::ConsoleDims Console::getConsoleDimensions(void)
{
	ConsoleDims dims = {CONSOLE_SIZE_X, CONSOLE_SIZE_Y};
	return dims;
}

}
