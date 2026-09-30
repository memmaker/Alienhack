// RVIP stage 3: Enter command menu and inventory / item menus.
// Both run the chosen command through PlayingGame: they set rvip_cmd and queue
// the synthetic CMD key (web Console::readKey), so every command keeps its
// own checks, prompts and target selection.
#ifndef ALIENHACK_RVIP_MENUS_HPP
#define	ALIENHACK_RVIP_MENUS_HPP

#include "Interface/InterfaceState.hpp"
#include <string>
#include <vector>

namespace RL_shared { class IFunctionMap; }

extern "C" { extern int rvip_next_key; extern bool rvip_numpad_raw; }

namespace AlienHack
{

extern std::string rvip_cmd;		// command for the CMD key: a keys.txt function, "Drop:<fn>", "Examine:<fn>", "Inventory?"
const int RVIP_CMD_KEY = 0x102;	// ext char 2
void rvipRunCommand( const std::string& cmd );

class RvipMenuBase : public RL_shared::InterfaceState
{
public:
	RvipMenuBase( boost::weak_ptr< RL_shared::InterfaceStateMachine > ism, boost::shared_ptr< RL_shared::IFunctionMap > keys, bool red );
	virtual void enterFromParent( RL_shared::AGameModel& );
	virtual void exitToParent( RL_shared::AGameModel& );
	virtual void enterFromChild( RL_shared::AGameModel& ) {}
	virtual void exitToChild( RL_shared::AGameModel& ) {}
	virtual void notifyAHGameModelAdvance( RL_shared::AGameModel&, RL_shared::GameTimeCoordinate, bool ) {}
	virtual bool finished(void) { return m_done; }
	virtual bool drawsWholeWindow(void) const { return false; }
	virtual bool needsInput(void) const { return true; }
protected:
	struct Row { std::string label, key; bool header; };
	// Box sized to content (title counts), centred, scrolls when taller than the screen.
	void drawList( RL_shared::AOutputWindow&, const std::string& title, const std::vector<Row>& rows, int cursor, int& top, int yhint ) const;
	std::string keyName( const std::string& fn ) const;
	boost::shared_ptr< RL_shared::IFunctionMap > m_key_map;
	bool m_red, m_done;
	mutable int m_top;
};

class CommandMenu : public RvipMenuBase
{
public:
	CommandMenu( boost::weak_ptr< RL_shared::InterfaceStateMachine > ism, boost::shared_ptr< RL_shared::IFunctionMap > keys, bool red );
	virtual CommandResult interpretInput( const RL_shared::AUserInputItem&, RL_shared::AGameModel& );
	virtual void draw( RL_shared::AOutputWindow&, RL_shared::AGameModel& ) const;
private:
	std::vector<Row> m_rows;
	std::vector<std::string> m_fn;	// per row, "" for headers
	int m_cursor;
	void move( int d );
};

class InventoryMenu : public RvipMenuBase
{
public:
	InventoryMenu( boost::weak_ptr< RL_shared::InterfaceStateMachine > ism, boost::shared_ptr< RL_shared::IFunctionMap > keys, bool red, bool drop_prompt );
	virtual void enterFromParent( RL_shared::AGameModel& );
	virtual CommandResult interpretInput( const RL_shared::AUserInputItem&, RL_shared::AGameModel& );
	virtual void draw( RL_shared::AOutputWindow&, RL_shared::AGameModel& ) const;
private:
	struct Action { std::string label, cmd; char key; bool ext; };
	struct Item { std::string fn, name; char key; std::vector<Action> actions; };
	std::vector<Item> m_items;
	bool m_drop;
	int m_cursor, m_act;	// m_act >= 0: item menu open
	mutable int m_top2;
	void choose( const std::string& cmd, bool reopen );
};

}

#endif
