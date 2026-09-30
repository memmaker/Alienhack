#include "RvipMenus.hpp"
#include "Console/Console.hpp"
#include "ConsoleView/KeyPress.hpp"
#include "Input/IFunctionMap.hpp"
#include "../../Model/AHGameModel.hpp"
#include "../../Model/Objects/PlayerCharacter.hpp"
#include "../../Model/Objects/Pickup.hpp"
#include "../../Model/Objects/Armour.hpp"
#include "../../Model/Objects/Weapon.hpp"
#include <algorithm>
#include <cctype>

#ifndef __EMSCRIPTEN__
extern "C" { int rvip_next_key = -1; bool rvip_numpad_raw = false; }
#endif

namespace AlienHack
{

using namespace RL_shared;
using namespace boost;

std::string rvip_cmd;

void rvipRunCommand( const std::string& cmd )
{
	rvip_cmd = cmd;
	rvip_next_key = RVIP_CMD_KEY;
}

namespace
{
	bool isKey( const KeyCode& k, int c, bool ext ) { return (k.second == ext) && ((unsigned char)k.first == (unsigned char)c); }
	bool keyUp( const KeyCode& k )    { return isKey(k, 72, true) || isKey(k, '8', true); }
	bool keyDown( const KeyCode& k )  { return isKey(k, 80, true) || isKey(k, '2', true); }
	bool keyPgUp( const KeyCode& k )  { return isKey(k, 73, true) || isKey(k, '9', true); }
	bool keyPgDn( const KeyCode& k )  { return isKey(k, 81, true) || isKey(k, '3', true); }
	bool keyClose( const KeyCode& k ) { return isKey(k, 27, false) || isKey(k, '0', true) || isKey(k, '.', true); }
	bool keyChoose( const KeyCode& k ){ return isKey(k, 13, false) || isKey(k, '5', true); }
}

RvipMenuBase::RvipMenuBase( weak_ptr< InterfaceStateMachine > ism, shared_ptr< IFunctionMap > keys, bool red )
: InterfaceState( ism ), m_key_map( keys ), m_red( red ), m_done( false ), m_top( 0 )
{
}
void RvipMenuBase::enterFromParent( AGameModel& ) { rvip_numpad_raw = true; }
void RvipMenuBase::exitToParent( AGameModel& )   { rvip_numpad_raw = false; }

std::string RvipMenuBase::keyName( const std::string& fn ) const
{
	shared_ptr< const IFunctionMap::FunctionToControlsMap > map( m_key_map->getFunctions() );
	IFunctionMap::FunctionToControlsMap::const_iterator it( map->find(fn) );
	if ((map->end() == it) || it->second.empty())
		return "";
	return it->second.front();
}

void RvipMenuBase::drawList( AOutputWindow& window, const std::string& title, const std::vector<Row>& rows, int cursor, int& top, int yhint ) const
{
	Console& console( dynamic_cast<Console&>(window) );
	Console::ConsoleDims dims( console.getConsoleDimensions() );
	Console::Colour frame_col = m_red ? Console::Red : Console::Cyan;
	Console::Colour text_col  = m_red ? Console::Red : Console::Green;
	Console::Colour hl_col    = m_red ? Console::BrightRed : Console::BrightGreen;

	int lw = 0, kw = 0;
	for (size_t i = 0; i < rows.size(); ++i)
	{
		lw = (std::max)(lw, (int)rows[i].label.length() + (rows[i].header ? 0 : 2));
		kw = (std::max)(kw, (int)rows[i].key.length());
	}
	int inner = lw + (kw ? kw + 1 : 0);
	inner = (std::max)(inner, (int)title.length() + 2);
	int w = inner + 4;	// border + one space each side
	int vis = (std::min)((int)rows.size(), dims.y - 2);
	int h = vis + 2;
	if (cursor < top) top = cursor;
	if (cursor >= top + vis) top = cursor - vis + 1;
	top = (std::max)(0, (std::min)(top, (int)rows.size() - vis));

	int x0 = (dims.x - w) / 2;
	int y0 = (yhint >= 0) ? (std::min)(yhint, dims.y - h) : (dims.y - h) / 2;
	int x1 = x0 + w - 1, y1 = y0 + h - 1;

	for (int y = y0; y <= y1; ++y)
		for (int x = x0; x <= x1; ++x)
			console.draw(x, y, ' ', Console::Black);
	for (int x = x0+1; x < x1; ++x) { console.draw(x, y0, '-', frame_col); console.draw(x, y1, '-', frame_col); }
	for (int y = y0+1; y < y1; ++y) { console.draw(x0, y, '|', frame_col); console.draw(x1, y, '|', frame_col); }
	console.draw(x0, y0, '/', frame_col);  console.draw(x1, y0, '\\', frame_col);
	console.draw(x0, y1, '\\', frame_col); console.draw(x1, y1, '/', frame_col);
	console.drawText(x0 + (w - (int)title.length()) / 2, y0, title.c_str(), hl_col);
	if (top > 0) console.draw(x1, y0+1, '^', hl_col);
	if (top + vis < (int)rows.size()) console.draw(x1, y1-1, 'v', hl_col);

	for (int i = 0; i < vis; ++i)
	{
		const Row& r( rows[top + i] );
		int y = y0 + 1 + i;
		bool sel = (top + i == cursor);
		if (r.header)
		{
			console.drawText(x0 + 2, y, r.label.c_str(), frame_col);
			continue;
		}
		if (sel)
			for (int x = x0+1; x < x1; ++x) console.draw(x, y, ' ', Console::Black, Console::Green);
		Console::Colour bg = sel ? Console::Green : Console::Black;
		Console::Colour fg = sel ? Console::Black : text_col;
		console.drawText(x0 + 2, y, sel ? "> " : "  ", sel ? fg : hl_col, bg);
		console.drawText(x0 + 4, y, r.label.c_str(), fg, bg);
		console.drawText(x1 - 1 - (int)r.key.length(), y, r.key.c_str(), sel ? fg : hl_col, bg);
	}
}


// ---- Enter menu: every command, grouped as the help screen groups them ----
CommandMenu::CommandMenu( weak_ptr< InterfaceStateMachine > ism, shared_ptr< IFunctionMap > keys, bool red )
: RvipMenuBase( ism, keys, red ), m_cursor( 0 )
{
	static const char* const table[][2] = {
		{ "", "Actions" },
		{ "Wait", "Wait" }, { "FloorUp", "Up one floor (or walk to >)" }, { "FloorDown", "Down one floor (or to <)" },
		{ "Strafe", "Strafe" }, { "StrafeLock", "Strafe lock" }, { "Turn", "Turn" }, { "TurnLock", "Turn lock" },
		{ "Get", "Get items" }, { "Operate", "Operate (door/terminal)" }, { "Explore", "Auto-explore" },
		{ "", "Weapons and items" },
		{ "Inventory", "Inventory" }, { "Sidearm", "Select sidearm" }, { "Primary", "Select primary" },
		{ "Reload", "Reload weapon" }, { "Drop", "Drop something" }, { "Fire", "Fire" },
		{ "Frag", "Frag grenade" }, { "Stun", "Stun grenade" }, { "Inc", "Inc grenade" }, { "Krak", "Krak grenade" },
		{ "Medkit", "Use medkit" }, { "Neutraliser", "Use neutraliser" }, { "Demolition", "Use demo charge" },
		{ "", "Other" },
		{ "ScrollUp", "Scroll messages up" }, { "ScrollDown", "Scroll messages down" }, { "Help", "Help" },
		{ "Look", "Look" }, { "Char", "Character screen" }, { "Buy", "Buy perks" }, { "Save", "Save and quit" },
	};
	for (size_t i = 0; i < sizeof table / sizeof table[0]; ++i)
	{
		Row r; r.header = !*table[i][0]; r.label = table[i][1];
		if (!r.header) r.key = keyName(table[i][0]);
		if (!r.header && r.key.empty()) continue;
		m_rows.push_back(r); m_fn.push_back(table[i][0]);
	}
	move(1);
}

void CommandMenu::move( int d )
{
	int n = (int)m_rows.size(), c = m_cursor;
	for (int i = 0; i < n; ++i)
	{
		c = (c + d + n) % n;
		if (!m_rows[c].header) { m_cursor = c; return; }
	}
}

CommandMenu::CommandResult CommandMenu::interpretInput( const AUserInputItem& input, AGameModel& )
{
	const KeyCode k( dynamic_cast<const KeyPress&>(input).value );
	for (size_t i = 0; i < m_fn.size(); ++i)	// a command's own key runs it (before cursor keys)
		if (!m_fn[i].empty() && m_key_map->isFunction(input, m_fn[i]))
		{
			rvipRunCommand(m_fn[i]); m_done = true;
			return CommandResult( false, true );
		}
	if (keyUp(k)) move(-1);
	else if (keyDown(k)) move(1);
	else if (keyPgUp(k)) { for (int i = 0; i < 10; ++i) move(-1); }
	else if (keyPgDn(k)) { for (int i = 0; i < 10; ++i) move(1); }
	else if (keyChoose(k) || isKey(k, '6', true)) { rvipRunCommand(m_fn[m_cursor]); m_done = true; }
	else if (keyClose(k) || isKey(k, '4', true)) m_done = true;
	else return CommandResult( false, false );
	return CommandResult( false, true );
}

void CommandMenu::draw( AOutputWindow& window, AGameModel& ) const
{
	drawList(window, "Commands", m_rows, m_cursor, m_top, -1);
}


// ---- Inventory with cursor and item menus ----
InventoryMenu::InventoryMenu( weak_ptr< InterfaceStateMachine > ism, shared_ptr< IFunctionMap > keys, bool red, bool drop_prompt )
: RvipMenuBase( ism, keys, red ), m_drop( drop_prompt ), m_cursor( 0 ), m_act( -1 ), m_top2( 0 )
{
}

void InventoryMenu::enterFromParent( AGameModel& in_model )
{
	RvipMenuBase::enterFromParent(in_model);
	AHGameModel& model( dynamic_cast<AHGameModel&>(in_model) );
	World& world( model.world() );
	shared_ptr< PlayerCharacter > pc( dynamic_pointer_cast<PlayerCharacter>( world.objectPtr( model.avatar() ) ) );
	m_items.clear();
	if (!pc) return;

	struct Add
	{
		InventoryMenu& m;
		Add( InventoryMenu& im ) : m(im) {}
		void operator()( const std::string& fn, const std::string& name, const std::string& main_label, const std::string& main_cmd, bool reload )
		{
			std::string key( m.keyName(fn) );
			Item it; it.fn = fn; it.name = name; it.key = (1 == key.length()) ? key[0] : 0;
			char lo = it.key, up = (char)std::toupper((unsigned char)it.key);
			bool letter = std::isalpha((unsigned char)lo) && std::islower((unsigned char)lo);
			Action a;
			a.label = main_label; a.cmd = main_cmd; a.key = lo; a.ext = false; it.actions.push_back(a);
			if (reload) { std::string rk( m.keyName("Reload") ); a.label = "Reload"; a.cmd = "Reload"; a.key = (1 == rk.length()) ? rk[0] : 0; a.ext = false; it.actions.push_back(a); }
			a.label = "Drop"; a.cmd = "Drop:" + fn; a.key = letter ? up : 0; a.ext = false; it.actions.push_back(a);
			if (main_cmd.compare(0, 8, "Examine:"))
			{ a.label = "Examine"; a.cmd = "Examine:" + fn; a.key = letter ? (char)(0xA0 + (lo - 'a')) : 0; a.ext = true; it.actions.push_back(a); }
			m.m_items.push_back(it);
		}
	} add(*this);

	if (world.objectExists(pc->armour()))
		add("Armour", dynamic_cast<const Armour&>(world.object(pc->armour())).getSelectName(true), "Examine", "Examine:Armour", false);
	for (int s = 0; s < 2; ++s)
	{
		PlayerCharacter::WeaponSlot slot = (PlayerCharacter::WeaponSlot)s;
		if (!world.objectExists(pc->weapon(slot))) continue;
		bool held = (pc->currentWeapon() == slot);
		std::string name( dynamic_cast<const Weapon&>(world.object(pc->weapon(slot))).getSelectName(true) );
		std::string fn( s ? "Primary" : "Sidearm" );
		add(fn, name + (held ? " (in hand)" : ""), held ? "Fire" : "Ready", held ? "Fire" : fn, held);
	}
	static const struct { pickup::Type t; const char* fn; const char* name; const char* verb; } pk[] = {
		{ pickup::FragGrenade, "Frag", "Frag grenades", "Throw" },
		{ pickup::KrakGrenade, "Krak", "Krak grenades", "Throw" },
		{ pickup::StunGrenade, "Stun", "Stun grenades", "Throw" },
		{ pickup::IncGrenade, "Inc", "Inc grenades", "Throw" },
		{ pickup::Medkit, "Medkit", "Medkits", "Use" },
		{ pickup::Neutraliser, "Neutraliser", "Neutraliser", "Use" },
		{ pickup::DemoCharge, "Demolition", "Demolition charges", "Place" },
	};
	for (size_t i = 0; i < sizeof pk / sizeof pk[0]; ++i)
	{
		int n = pc->num(pk[i].t);
		if (n <= 0) continue;
		add(pk[i].fn, std::string(pk[i].name) + " x" + std::to_string(n), pk[i].verb, pk[i].fn, false);
	}
	if (m_cursor >= (int)m_items.size()) m_cursor = 0;
}

void InventoryMenu::choose( const std::string& cmd, bool reopen )
{
	rvipRunCommand( (reopen && !m_drop) ? cmd + "|reopen" : cmd );
	m_done = true;
}

InventoryMenu::CommandResult InventoryMenu::interpretInput( const AUserInputItem& input, AGameModel& )
{
	const KeyCode k( dynamic_cast<const KeyPress&>(input).value );
	if (m_items.empty()) { m_done = true; return CommandResult( false, true ); }

	if (m_act >= 0)
	{	// item menu
		const Item& it( m_items[m_cursor] );
		for (size_t i = 0; i < it.actions.size(); ++i)
			if (it.actions[i].key && isKey(k, it.actions[i].key, it.actions[i].ext)) { choose(it.actions[i].cmd, true); return CommandResult( false, true ); }
		int n = (int)it.actions.size();
		if (keyUp(k)) m_act = (m_act + n - 1) % n;
		else if (keyDown(k)) m_act = (m_act + 1) % n;
		else if (keyChoose(k) || isKey(k, '6', true) || isKey(k, ' ', false)) choose(it.actions[m_act].cmd, true);
		else if (keyClose(k) || isKey(k, '4', true)) m_act = -1;
		else return CommandResult( false, false );
		return CommandResult( false, true );
	}

	for (size_t i = 0; i < m_items.size(); ++i)
	{	// letter = main action (drop in the drop prompt), Shift = drop, Ctrl = examine
		const Item& it( m_items[i] );
		if (it.key && k.second && ((unsigned char)k.first == (unsigned char)(0xA0 + (it.key - 'a'))))
		{
			choose("Examine:" + it.fn, true);
			return CommandResult( false, true );
		}
		for (size_t a = 0; a < it.actions.size(); ++a)
			if (it.actions[a].key && isKey(k, it.actions[a].key, it.actions[a].ext))
			{
				std::string cmd( it.actions[a].cmd );
				if (m_drop && (0 == a)) cmd = "Drop:" + it.fn;
				choose(cmd, true);
				return CommandResult( false, true );
			}
	}
	const Item& it( m_items[m_cursor] );
	int n = (int)m_items.size();
	if (keyUp(k)) m_cursor = (m_cursor + n - 1) % n;
	else if (keyDown(k)) m_cursor = (m_cursor + 1) % n;
	else if (keyChoose(k) || isKey(k, '6', true) || isKey(k, ' ', false))
	{
		if (m_drop) choose("Drop:" + it.fn, false);
		else m_act = 0;
	}
	else if (isKey(k, '+', true) || isKey(k, '+', false)) choose(m_drop ? "Drop:" + it.fn : it.actions[0].cmd, true);
	else if (isKey(k, '-', true) || isKey(k, '-', false)) choose("Drop:" + it.fn, true);
	else if (isKey(k, '*', true) || isKey(k, '*', false)) choose("Examine:" + it.fn, true);
	else if (keyClose(k) || isKey(k, '4', true)) m_done = true;
	else
	{	// any other key is a normal command
		m_done = true;
		rvip_next_key = (k.second ? 0x100 : 0) | (unsigned char)k.first;
	}
	return CommandResult( false, true );
}

void InventoryMenu::draw( AOutputWindow& window, AGameModel& ) const
{
	std::vector<Row> rows;
	if (m_items.empty())
	{
		Row r; r.header = true; r.label = "You carry nothing."; rows.push_back(r);
		drawList(window, m_drop ? "Drop what?" : "Inventory", rows, -1, m_top, -1);
		return;
	}
	for (size_t i = 0; i < m_items.size(); ++i)
	{
		Row r; r.header = false; r.label = m_items[i].name;
		if (m_items[i].key) r.key = std::string(1, m_items[i].key);
		if (!m_drop) r.key = m_items[i].actions[0].label + " " + r.key;
		rows.push_back(r);
	}
	drawList(window, m_drop ? "Drop what?" : "Inventory", rows, m_cursor, m_top, -1);
	if (m_act >= 0)
	{
		const Item& it( m_items[m_cursor] );
		std::vector<Row> acts;
		for (size_t i = 0; i < it.actions.size(); ++i)
		{
			const Action& a( it.actions[i] );
			Row r; r.header = false; r.label = a.label;
			if (a.key) r.key = a.ext ? std::string("^") + (char)std::toupper('a' + ((unsigned char)a.key - 0xA0)) : std::string(1, a.key);
			acts.push_back(r);
		}
		drawList(window, it.name, acts, m_act, m_top2, 3);
	}
}

}

#ifdef __EMSCRIPTEN__
#include "draw.hpp"
#include "World-2DTiles/World.hpp"
#include "World-2DTiles/Zone.hpp"
#include "../../Model/Objects/ObjectType.hpp"
#include <boost/foreach.hpp>
extern "C" void rvip_pane( const char* id, const char* text );   // web/Console-web.cpp
extern "C" const char* rvip_css( int colour );

namespace AlienHack
{

// RVIP stage 5: the Inventory and Visible windows, built from the game's data at the
// command prompt (PlayingGame::draw). Inventory rows "a) <glyph> name" in the item's
// colour ("\x01<fg><bg>" = colour of the text after it); Visible = RvipWM.visible lines.
static std::string last_inv("\x02"), last_vis("\x02");   // sentinels: an empty list is sent too
// Back on the title: empty every side window (Title::draw); the next game sends them anew.
void rvipClearPanes()
{
	if (last_inv.empty() && last_vis.empty()) return;
	last_inv.clear(); last_vis.clear();
	rvip_pane("inv", ""); rvip_pane("vis", ""); rvip_pane("stat", ""); rvip_pane("msg", "");
}
void rvipSidePanes( const AHGameModel& model, const IFunctionMap& keys )
{
	const World& world( model.world() );
	if (!world.objectExists(model.avatar())) return;
	const PlayerCharacter& pc( dynamic_cast<const PlayerCharacter&>( world.object( model.avatar() ) ) );
	shared_ptr< const IFunctionMap::FunctionToControlsMap > map( keys.getFunctions() );

	std::string inv;
	struct Row
	{
		std::string& out; const IFunctionMap::FunctionToControlsMap& m;
		Row( std::string& o, const IFunctionMap::FunctionToControlsMap& mm ) : out(o), m(mm) {}
		void operator()( const char* fn, char glyph, int col, const std::string& name )
		{
			IFunctionMap::FunctionToControlsMap::const_iterator it( m.find(fn) );
			std::string k( ((m.end() != it) && !it->second.empty() && (1 == it->second.front().length())) ? it->second.front() : " " );
			out += "\x01ha" + k + ") \x01"; out += (char)('a' + col); out += 'a'; out += glyph; out += " " + name + "\n";
		}
	} row( inv, *map );
	int col;
	char g;
	if (world.objectExists(pc.armour()))
	{
		const AHGameObject& o( dynamic_cast<const AHGameObject&>(world.object(pc.armour())) );
		g = rvipObjectGlyph(o, &col);
		row("Armour", g, col, dynamic_cast<const Armour&>(o).getSelectName(true));
	}
	for (int s = 0; s < 2; ++s)
	{
		PlayerCharacter::WeaponSlot slot = (PlayerCharacter::WeaponSlot)s;
		if (!world.objectExists(pc.weapon(slot))) continue;
		const AHGameObject& o( dynamic_cast<const AHGameObject&>(world.object(pc.weapon(slot))) );
		g = rvipObjectGlyph(o, &col);
		row(s ? "Primary" : "Sidearm", g, col, dynamic_cast<const Weapon&>(o).getSelectName(true) + ((pc.currentWeapon() == slot) ? " (in hand)" : ""));
	}
	static const struct { pickup::Type t; const char* fn; const char* name; } pk[] = {
		{ pickup::FragGrenade, "Frag", "Frag grenades" }, { pickup::KrakGrenade, "Krak", "Krak grenades" },
		{ pickup::StunGrenade, "Stun", "Stun grenades" }, { pickup::IncGrenade, "Inc", "Inc grenades" },
		{ pickup::Medkit, "Medkit", "Medkits" }, { pickup::Neutraliser, "Neutraliser", "Neutraliser" },
		{ pickup::DemoCharge, "Demolition", "Demolition charges" },
	};
	for (size_t i = 0; i < sizeof pk / sizeof pk[0]; ++i)
	{
		int n = pc.num(pk[i].t);
		if (n <= 0) continue;
		g = rvipPickupGlyph(pk[i].t, &col);
		row(pk[i].fn, g, col, std::string(pk[i].name) + " x" + std::to_string(n));
	}
	while (!inv.empty() && '\n' == inv[inv.length()-1]) inv.erase(inv.length()-1);
	if (inv != last_inv) { last_inv = inv; rvip_pane("inv", inv.c_str()); }

	// Visible: aliens, then items, in view (nearest first)
	const WorldObject::WorldLocation loc( pc.location() );
	const Zone& zone( world.zone( loc.zone ) );
	std::vector< std::pair<int, std::string> > mons, items;
	for (int z=0; z < zone.sizeZ(); ++z)
		for (int x=0; x < zone.sizeX(); ++x)
		{
			if (!model.isVisible(loc.zone, x, z)) continue;
			int d = (x-loc.x)*(x-loc.x) + (z-loc.z)*(z-loc.z);
			BOOST_FOREACH( DBKeyValue ok, zone.objectsAt(x, z) )
			{
				if ((ok == model.avatar()) || !world.objectExists(ok)) continue;
				const AHGameObject& o( dynamic_cast< const AHGameObject& >( world.object(ok) ) );
				bool mon = (objects::Alien == o.type());
				if (!mon && (objects::Pickup != o.type()) && (objects::Armour != o.type()) && (objects::Weapon != o.type())) continue;
				if (!o.shouldDraw()) continue;
				g = rvipObjectGlyph(o, &col);
				std::string l( std::string(mon ? "M" : "I") + g + o.getSelectName(false) + "\t" + rvip_css(col) );
				(mon ? mons : items).push_back( std::make_pair(d, l) );
			}
		}
	std::stable_sort(mons.begin(), mons.end()); std::stable_sort(items.begin(), items.end());
	std::string vis;
	for (size_t i = 0; i < mons.size(); ++i) vis += mons[i].second + "\n";
	for (size_t i = 0; i < items.size(); ++i) vis += items[i].second + "\n";
	if (vis != last_vis) { last_vis = vis; rvip_pane("vis", vis.c_str()); }
}

}
#endif
