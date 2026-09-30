#include "PlayingGame.hpp"
#include "RvipSound.hpp"
#include "story.hpp"
#include "draw.hpp"
#include "TargetSelect.hpp"
#include "LookMode.hpp"
#include "MessageBox.hpp"
#include "ChoiceDialog.hpp"
#include "GetDialog.hpp"
#include "DropDialog.hpp"
#include "GetWeaponDialog.hpp"
#include "CharScreen.hpp"
#include "StoryText.hpp"
#include "HelpScreen.hpp"
#include "Kaboom.hpp"
#include "NameEntry.hpp"
#include "BuyPerks.hpp"
#include "ViewFile.hpp"
#include "GameEvents.hpp"
#include "RvipMenus.hpp"
#include "../../Model/save.hpp"
#include "../../Model/AHGameModel.hpp"
#include "../../Model/FindNearest.hpp"
#include "../../Model/OverWorld.hpp"
#include "../../Model/Objects/ObjectType.hpp"
#include "../../Model/Objects/TerrainType.hpp"
#include "../../Model/Objects/PlayerCharacter.hpp"
#include "../../Model/Objects/Alien.hpp"
#include "../../Model/Objects/Pickup.hpp"
#include "../../Model/Objects/Armour.hpp"
#include "../../Model/Objects/Weapon.hpp"
#include "../../Model/Objects/WeaponData.hpp"
#include "../../Model/Objects/Terminal.hpp"
#include "../../Model/Objects/Explosion.hpp"
#include "../../Model/Objects/Fire.hpp"
#include "../../Model/Actions/WaitAction.hpp"
#include "../../Model/Actions/DiscreteMoveAction.hpp"
#include "../../Model/Actions/DiscreteTurnAction.hpp"
#include "../../Model/Actions/OpenDoorAction.hpp"
#include "../../Model/Actions/CloseDoorAction.hpp"
#include "../../Model/Actions/WeaponAttackAction.hpp"
#include "../../Model/Actions/PickupAction.hpp"
#include "../../Model/Actions/PickupArmourAction.hpp"
#include "../../Model/Actions/DropArmourAction.hpp"
#include "../../Model/Actions/PickupWeaponAction.hpp"
#include "../../Model/Actions/DropWeaponAction.hpp"
#include "../../Model/Actions/DropItemAction.hpp"
#include "../../Model/Actions/SwitchWeaponAction.hpp"
#include "../../Model/Actions/ReloadWeaponAction.hpp"
#include "../../Model/Actions/ThrowGrenadeAction.hpp"
#include "../../Model/Actions/UseItemAction.hpp"
#include "../../Model/Actions/UseTerminalAction.hpp"
#include "../../Model/Actions/TakeAmmoAction.hpp"
#include "../../Model/Actions/SetChargeAction.hpp"
#include "../../Model/MapCreator.hpp"
#include "World-2DTiles/World.hpp"
#include "World-2DTiles/Zone.hpp"
#include "ActionEngine/ActionEngine.hpp"
#include "Input/IFunctionMap.hpp"
#include "ConsoleView/KeyPress.hpp"
#include "assert.hpp"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <stdlib.h>
#include <fstream>
#include <sstream>


namespace AlienHack
{

using namespace RL_shared;
using namespace boost;
#ifdef __EMSCRIPTEN__
extern "C" bool rvip_auto_on;
#else
static bool rvip_auto_on = false;
#endif


namespace
{

	using namespace story_text;

	const std::string drop_armour_AYS_msg = 
		" Are you sure that you \n"
		" want to drop your armour? \n";
	const int drop_armour_AYS_x_low = 25;
	const int drop_armour_AYS_y_low = 17;
	const int drop_armour_AYS_x_high = 54;
	const int drop_armour_AYS_y_high = 22;

	const std::string drop_weapon_AYS_msg = 
		" Are you sure that you \n"
		" want to drop your weapon? \n";
	const int drop_weapon_AYS_x_low = 25;
	const int drop_weapon_AYS_y_low = 17;
	const int drop_weapon_AYS_x_high = 54;
	const int drop_weapon_AYS_y_high = 22;



class GetWeaponChoice : public IChoiceDialogSelect
{
	shared_ptr< PlayerCharacter > m_player; 
	shared_ptr< Weapon > m_weapon;
	AHGameModel& m_model;
	bool m_committed;

public: 

	GetWeaponChoice( AHGameModel& model, shared_ptr< PlayerCharacter > player, shared_ptr< Weapon > weapon )
		: m_model(model), m_player(player), m_weapon(weapon), m_committed(false)
	{
	}

	virtual bool select( int option )
	{
		if (GetWeaponDialog::TakeAmmo == option)
		{
			shared_ptr< TakeAmmoAction > action( new TakeAmmoAction(m_player, m_weapon, 
				m_player->getActionTime(m_model, player_actions::TakeAmmo)) );
			m_model.actionEngine().addAction( action );
			m_committed = true;
		}
		else if (GetWeaponDialog::TakeWeapon == option)
		{
			shared_ptr< PickupWeaponAction > action( new PickupWeaponAction(m_player, m_weapon, 
				m_player->getActionTime(m_model, player_actions::DropWeapon), 
				m_player->getActionTime(m_model, player_actions::PickupWeapon)) );
			m_model.actionEngine().addAction( action );
			m_committed = true;
		}
		return m_committed;
	}

	bool committed() const	{ return m_committed; }
};




class GetItemChoice : public IGetDialogSelect
{
	shared_ptr< IFunctionMap > m_keys;
	shared_ptr< PlayerCharacter > m_player; 
	AHGameModel& m_model;
	InterfaceState& m_is;

public:

	GetItemChoice( 
		InterfaceState& is, 
		shared_ptr< IFunctionMap > keys, 
		AHGameModel& model, 
		shared_ptr< PlayerCharacter > player 
		)
		: m_keys(keys), m_player(player), m_model(model), m_is(is)
	{
	}

	virtual bool select( boost::shared_ptr< AHGameObject > game_obj )
	{
		WorldObjectType obj_type( game_obj->type() );
		if (objects::Pickup == obj_type)
		{
			shared_ptr< Pickup > pickup( dynamic_pointer_cast< Pickup >( game_obj ) );
			shared_ptr< PickupAction > action( new PickupAction(m_player, pickup, m_player->getActionTime(m_model, player_actions::PickupObject)) );
			m_model.actionEngine().addAction( action );
			return true;
		}
		else if (objects::Armour == obj_type)
		{
			shared_ptr< Armour > armour( dynamic_pointer_cast< Armour >( game_obj ) );
			shared_ptr< PickupArmourAction > action( new PickupArmourAction(m_player, armour, 
				m_player->getActionTime(m_model, player_actions::DropArmour), 
				m_player->getActionTime(m_model, player_actions::PickupArmour)) );
			m_model.actionEngine().addAction( action );
			return true;
		}
		else if (objects::Weapon == obj_type)
		{
			shared_ptr< Weapon > weapon( dynamic_pointer_cast< Weapon >( game_obj ) );
			shared_ptr< GetWeaponChoice > weapon_choice( new GetWeaponChoice(m_model, m_player, weapon) );
			shared_ptr< GetWeaponDialog > weapon_dialog( 
				new GetWeaponDialog(m_is.interfaceStateMachine(), weapon, m_keys, weapon_choice, m_model.isCountdownActive() ? ChoiceDialog::Red : ChoiceDialog::Green ) );
			m_is.setNextState( weapon_dialog );
		}
		return false;
	}
};




bool doGetPickupsMenu(InterfaceState& is, shared_ptr< IFunctionMap > keys, AHGameModel& model, MessageDisplay& msgs)
{
	World& world( model.world() );
	if (!world.objectExists(model.avatar()))
		return false;
	shared_ptr<PlayerCharacter> player( dynamic_pointer_cast<PlayerCharacter>(world.objectPtr(model.avatar())) );
	WorldObject::WorldLocation player_loc( player->location() );
	if (!world.zoneExists(player_loc.zone))
		return false;
	Zone& zone( world.zone(player_loc.zone) );
	if (!zone.isWithin(player_loc.x, player_loc.z))
		return false;

	const ObjectList& objects( zone.objectsAt( player_loc.x, player_loc.z ) );

	shared_ptr< std::vector< shared_ptr< AHGameObject > > > pickups( new std::vector< shared_ptr< AHGameObject > >() );

	BOOST_FOREACH( RL_shared::DBKeyValue obj_key, objects )
	{
		if (world.objectExists( obj_key ))
		{
			shared_ptr<AHGameObject> game_obj( dynamic_pointer_cast<AHGameObject>( world.objectPtr( obj_key ) ) );
			WorldObjectType obj_type( game_obj->type() );
			if ((objects::Pickup == obj_type) || (objects::Armour == obj_type) || (objects::Weapon == obj_type))
				pickups->push_back( game_obj );
		}
	}

	if (pickups->empty())
	{
		msgs.addString("There's nothing here to get.");
		return false;
	}

	shared_ptr< GetItemChoice > item_choice( new GetItemChoice(is, keys, model, player) );

	if (1 == pickups->size())
		return item_choice->select( (*pickups)[0] );

	shared_ptr< GetDialog > get_dialog( new GetDialog(pickups, is.interfaceStateMachine(), keys, item_choice, model.isCountdownActive() ? GetDialog::Red : GetDialog::Green) );
	is.setNextState( get_dialog );

	return false;
}

}


class SelectFireTarget : public ITargetSelectCallback
{
	RL_shared::MessageDisplay& m_msgs;

public:

	explicit SelectFireTarget( RL_shared::MessageDisplay& msgs )
		: m_msgs(msgs)
	{
	}

	virtual std::pair< bool, bool > select( AHGameModel& model, int x, int z )
	{
		if ((0==x) && (0==z))
		{
			m_msgs.addString( "Invalid target\nSelect target:" );
			return std::pair< bool, bool >( false, false );
		}

		World& world( model.world() );
		if (world.objectExists( model.avatar() ))
		{
			shared_ptr< PlayerCharacter > player_obj( dynamic_pointer_cast< PlayerCharacter >( world.objectPtr(model.avatar()) ) );

			if (!player_obj->isValidTargetOffset( x, z ))
			{
				m_msgs.addString( "Invalid target\nSelect target:" );
				return std::pair< bool, bool >( false, false );
			}

			DBKeyValue weapon_key( player_obj->weapon( player_obj->currentWeapon() ) );
			if (world.objectExists(weapon_key))
			{
				shared_ptr< Weapon > weapon_obj( dynamic_pointer_cast<Weapon>( world.objectPtr( weapon_key ) ) );
				if (0 < weapon_obj->getAmmo())
				{
					shared_ptr< WeaponAttackAction > action( 
						new WeaponAttackAction( model, model.world(), player_obj, weapon_obj, x, z ) );
					model.actionEngine().addAction( action );

					m_msgs.addString("You open fire!");

					return std::pair< bool, bool >( true, true );
				}
			}
		}
		return std::pair< bool, bool >( false, false );
	}
};

class SelectGrenadeTarget : public ITargetSelectCallback
{
	RL_shared::MessageDisplay& m_msgs;
	pickup::Type m_grenade_type;

public:

	SelectGrenadeTarget( RL_shared::MessageDisplay& msgs, pickup::Type type )
		: m_msgs(msgs), m_grenade_type(type)
	{
	}

	virtual std::pair< bool, bool > select( AHGameModel& model, int x, int z )
	{
		World& world( model.world() );
		if (world.objectExists( model.avatar() ))
		{
			shared_ptr< PlayerCharacter > player_obj( dynamic_pointer_cast< PlayerCharacter >( world.objectPtr(model.avatar()) ) );

			shared_ptr< ThrowGrenadeAction > action( 
				new ThrowGrenadeAction( model, player_obj, m_grenade_type, x, z, 
					player_obj->getActionTime(model, player_actions::ThrowGrenade) ) );
			model.actionEngine().addAction( action );

			return std::pair< bool, bool >( true, true );
		}
		return std::pair< bool, bool >( false, false );
	}
};


class SelectDropItem : public IDropDialogSelect
{
	RL_shared::MessageDisplay& m_msgs;
	AHGameModel& m_model;
	World& m_world;
	OverWorld& m_overworld;
	shared_ptr< PlayerCharacter > m_player_obj;

public:

	SelectDropItem( 
	    RL_shared::MessageDisplay& msgs, 
	    AHGameModel& model, 
	    World& world, 
	    OverWorld& overworld, 
	    shared_ptr< PlayerCharacter > player_obj
	    ) : m_msgs(msgs), m_model(model), m_world(world), m_overworld(overworld), m_player_obj(player_obj)
	{
	}

	virtual bool select( const std::string& action )
	{
	    if ("Sidearm" == action)
	    {
	        return dropWeapon( PlayerCharacter::Sidearm, "sidearm" );
	    }
	    else if ("Primary" == action)
	    {
	        return dropWeapon( PlayerCharacter::Primary, "primary weapon" );
	    }
	    else if ("Armour" == action)
	    {
		    if (m_world.objectExists(m_player_obj->armour()))
		    {
			    shared_ptr< DropArmourAction > drop_action( 
				    new DropArmourAction(m_player_obj, 
					    m_player_obj->getActionTime(m_model, player_actions::DropArmour)) );
			    m_model.actionEngine().addAction( drop_action );
			    return true;
		    }
		    else
		    {
			    m_msgs.addString("You're not wearing any armour.");
		    }
	    }
	    else if ("Stun" == action)
	    {
	        return dropItem( pickup::StunGrenade, "stun grenades" );
	    }
	    else if ("Inc" == action)
	    {
	        return dropItem( pickup::IncGrenade, "inc grenades" );
	    }
	    else if ("Frag" == action)
	    {
	        return dropItem( pickup::FragGrenade, "frag grenades" );
	    }
	    else if ("Krak" == action)
	    {
	        return dropItem( pickup::KrakGrenade, "krak grenades" );
	    }
	    else if ("Medkit" == action)
	    {
	        return dropItem( pickup::Medkit, "medkits" );
	    }
	    else if ("Neutraliser" == action)
	    {
	        return dropItem( pickup::Neutraliser, "neutralisers" );
	    }
	    else if ("Demolition" == action)
	    {
	        return dropItem( pickup::DemoCharge, "demolition charges" );
	    }

        return false;
	}

private:

    bool dropWeapon( PlayerCharacter::WeaponSlot slot, const std::string& name )
    {
	    if (m_world.objectExists(m_player_obj->weapon( slot )))
	    {
		    shared_ptr< DropWeaponAction > drop_action( 
			    new DropWeaponAction(m_player_obj, slot, 
				    m_player_obj->getActionTime(m_model, player_actions::DropWeapon)) );
		    m_model.actionEngine().addAction( drop_action );
		    return true;
	    }
	    else
	    {
	        std::string msg;
            msg += "You don't have a " + name + ".";
		    m_msgs.addString(msg.c_str());
	    }
        return false;
    }

    bool dropItem( pickup::Type type, const std::string& name )
    {
        if (m_player_obj->num(type) > 0)
        {
		    shared_ptr< DropItemAction > drop_action( 
			    new DropItemAction(m_player_obj, type, 
				    m_player_obj->getActionTime(m_model, player_actions::DropItem)) );
		    m_model.actionEngine().addAction( drop_action );
		    return true;
        }
        else
        {
            std::string msg;
            msg += "You don't have any " + name + ".";
    	    m_msgs.addString(msg.c_str());
        }
        return false;
    }
};


PlayingGame::PlayingGame( 
	boost::weak_ptr< RL_shared::InterfaceStateMachine > ism, 
	boost::shared_ptr< RL_shared::IFunctionMap > keys, 
	bool loaded 
	)
: InterfaceState(ism)
, m_msgs( MessageDisplay::WindowParams(MSGS_X_LOW, MSGS_X_HI, MSGS_Y_LOW, MSGS_Y_HI) )
, m_key_map(keys)
, m_last_input(0)
, m_quit(false)
, m_loaded_game(loaded)
, m_move_only(false)
, m_turn_only(false)
, m_move_lock(false)
, m_turn_lock(false)
, m_activating_object(false)
, m_placing_charge(false)
, m_first_look_mode(true)
, m_shown_story_text_1(false)
, m_shown_story_text_2(false)
, m_shown_story_text_3(false)
, m_shown_intro_text(false)
, m_shown_master_text(false)
, m_shown_found_queen_text(false)
, m_shown_killed_queen_text(false)
, m_shown_countdown_start_text(false)
, m_shown_ending_text_1(false)
, m_good_ending(false)
, m_shown_bad_ending_text(false)
, m_shown_good_ending_text_1(false)
, m_shown_good_ending_text_2(false)
, m_shown_death_msg(false)
, m_shown_mortem(false)
, m_auto(0), m_is_cmd(false), m_child_opened(false), m_auto_zone(INVALID_KEY), m_auto_px(0), m_auto_pz(0), m_auto_tx(0), m_auto_tz(0)
, m_auto_moved(false), m_auto_door(false), m_auto_aliens(0)
, m_rvip_save_due(true), m_rvip_player_saved(false)
{
	ASSERT( m_key_map );
}


void PlayingGame::enterFromParent( AGameModel& in_model )
{
	AHGameModel& model( dynamic_cast<AHGameModel&>(in_model) );

	if (!m_loaded_game)
	{
		AHGameModel::GameType game_type = model.gameType();
		model.clear(); //GameTypeMenu will have cleared it already. This is just safety.
		model.setGameType(game_type);

		initialiseWorld( model );

		//TODO make it unnecessary to do these here
		model.updateVision();
		model.updateHearing();

		shared_ptr< NameEntry > newstate( new NameEntry(interfaceStateMachine(), m_key_map) );
		setNextState( newstate );
	}
	else
	{
		m_first_look_mode = false;
		m_shown_story_text_1 = true;
		m_shown_story_text_2 = true;
		m_shown_story_text_3 = true;
		m_shown_intro_text = true;
	}

	m_game_events = makeGameEvents(m_msgs);
	model.setGameEventsObserver( m_game_events );

	m_msgs.addString( "Welcome to AlienHack!" );
	m_msgs.addString( "Press ? for help, l for look mode, c for the character screen." );
}
void PlayingGame::exitToParent( AGameModel& )
{
}
void PlayingGame::enterFromChild( AGameModel& in_model )
{
	AHGameModel& model( dynamic_cast< AHGameModel& >( in_model ) );

	m_activating_object = false;
	m_placing_charge = false;
	m_move_only = false;
	m_turn_only = false;
	m_move_lock = false;
	m_turn_lock = false;

	if (m_quit)
	{
		showMortem();
		return;
	}

	if (!m_shown_story_text_1)
	{
		m_shown_story_text_1 = true;
		shared_ptr< StoryText > newstate( new StoryText(interfaceStateMachine(), background_text_1, 8) );
		setNextState( newstate );
		return;
	}
	if (!m_shown_story_text_2)
	{
		m_shown_story_text_2 = true;
		shared_ptr< StoryText > newstate( new StoryText(interfaceStateMachine(), background_text_2, 8) );
		setNextState( newstate );
		return;
	}
	else if (!m_shown_story_text_3)
	{
		m_shown_story_text_3 = true;
		shared_ptr< StoryText > newstate( new StoryText(interfaceStateMachine(), background_text_3, 7) );
		setNextState( newstate );
		return;
	}
	else if (!m_shown_intro_text)
	{
		m_shown_intro_text = true;
		shared_ptr< MessageBox > newstate;
		newstate.reset( new MessageBox(interfaceStateMachine(), m_key_map, intro_text, 17, 12, 62, 28, MessageBox::Green, 1) );
		setNextState( newstate );
		return;
	}

	if (m_shown_ending_text_1)
	{
		if (m_good_ending)
		{
			if (!m_shown_good_ending_text_1)
			{
				m_shown_good_ending_text_1 = true;
				shared_ptr< StoryText > newstate( new StoryText(interfaceStateMachine(), ending_text_good_1, 8) );
				setNextState( newstate );
				return;
			}
			else if (!m_shown_good_ending_text_2)
			{
				m_shown_good_ending_text_2 = true;
				shared_ptr< StoryText > newstate( new StoryText(interfaceStateMachine(), ending_text_good_2, 8) );
				setNextState( newstate );
				return;
			}
			else
			{
				m_quit = true;
				showMortem();
			}
		}
		else 
		{
			if (!m_shown_bad_ending_text)
			{
				m_shown_bad_ending_text = true;
				shared_ptr< StoryText > newstate( new StoryText(interfaceStateMachine(), ending_text_bad, 8) );
				setNextState( newstate );
				return;
			}
			else
			{
				m_quit = true;
				showMortem();
			}
		}
	}
}
void PlayingGame::exitToChild( AGameModel& )
{
	m_child_opened = true;
	if ((RVIP_CMD_KEY == rvip_next_key) && ("Inventory?" == rvip_cmd))
		rvip_next_key = -1;	// the action opened a prompt: no inventory reopen
	m_auto = 0;
	rvip_auto_on = false;
}


// ---- RVIP auto-explore / stair walk -------------------------------------
// The web console's readKey() returns the synthetic key AUTO_KEY (after a
// ~40 ms paint delay) while rvip_auto_on is set; a real key clears the flag.
namespace { const RL_shared::KeyCode AUTO_KEY( (char)1, true ); }

int PlayingGame::scanView( AHGameModel& model, std::string* alien_name, std::string* new_item, bool record_items )
{
	World& world( model.world() );
	const WorldObject& pobj( world.object( model.avatar() ) );
	DBKeyValue zk( pobj.location().zone );
	const Zone& zone( world.zone( zk ) );
	int aliens = 0, best = 1<<30;
	for (int z=0; z < zone.sizeZ(); ++z)
		for (int x=0; x < zone.sizeX(); ++x)
		{
			if (!model.isVisible(zk, x, z))
				continue;
			BOOST_FOREACH( DBKeyValue ok, zone.objectsAt(x, z) )
			{
				if ((ok == model.avatar()) || !world.objectExists(ok))
					continue;
				const AHGameObject& o( dynamic_cast< const AHGameObject& >( world.object(ok) ) );
				if (objects::Alien == o.type())
				{
					if (!o.shouldDraw())
						continue;
					++aliens;
					int d = (x-pobj.location().x)*(x-pobj.location().x) + (z-pobj.location().z)*(z-pobj.location().z);
					if (alien_name && d < best) { best = d; *alien_name = o.getSelectName(false); }
				}
				else if ((objects::Pickup == o.type()) || (objects::Armour == o.type()) || (objects::Weapon == o.type()))
				{
					if (m_ex_items.insert(ok).second && !record_items && new_item && new_item->empty())
						*new_item = o.getSelectName(false);
				}
			}
		}
	return aliens;
}

void PlayingGame::startAuto( AHGameModel& model, int mode )
{
	const WorldObject& pobj( model.world().object( model.avatar() ) );
	m_auto = mode;
	m_auto_zone = pobj.location().zone;
	m_auto_moved = m_auto_door = false;
	m_auto_aliens = scanView( model, 0, 0, true );
	rvip_auto_on = true;
}

void PlayingGame::stopAuto( const std::string& msg )
{
	m_auto = 0;
	rvip_auto_on = false;
	if (!msg.empty())
		m_msgs.addString( msg.c_str() );
}

// One step of explore / stair walk. Returns true with a direction to move,
// false when it stopped (message already given).
bool PlayingGame::autoStep( AHGameModel& model, int& mx, int& mz )
{
	World& world( model.world() );
	const WorldObject& pobj( world.object( model.avatar() ) );
	WorldObject::WorldLocation loc( pobj.location() );
	if (loc.zone != m_auto_zone) { stopAuto(""); return false; }
	Zone& zone( world.zone( loc.zone ) );
	std::set<int>& visited( m_ex_visited[loc.zone] );
	std::set<int>& skip( m_ex_skip[loc.zone] );
	const int W = zone.sizeX(), H = zone.sizeZ();
	const bool explore = (1 == m_auto);

	// results of the previous step
	if (m_auto_door)
	{
		if (!terrain::isOpen( zone.terrainAt(m_auto_tx, m_auto_tz) ) && !terrain::isBroken( zone.terrainAt(m_auto_tx, m_auto_tz) ))
			skip.insert( m_auto_tz*W + m_auto_tx );
	}
	else if (m_auto_moved && (loc.x == m_auto_px) && (loc.z == m_auto_pz))
	{
		skip.insert( m_auto_tz*W + m_auto_tx );
		stopAuto("Something is in the way.");
		return false;
	}
	visited.insert( loc.z*W + loc.x );

	std::string alien, item;
	int aliens = scanView( model, &alien, &item, false );
	if (explore ? (aliens > 0) : (aliens > m_auto_aliens))
	{
		stopAuto( "In view: " + alien + "." );
		return false;
	}
	if (explore && !item.empty())
	{
		stopAuto( "In view: " + item + "." );
		return false;
	}

	const TerrainType stair_type = (2 == m_auto) ? (TerrainType)terrain::StairsUp : (TerrainType)terrain::StairsDown;
	if (!explore && (zone.terrainAt(loc.x, loc.z) == stair_type))
	{
		stopAuto( (2 == m_auto) ? "You reach the stairs up. Press it again to climb." : "You reach the stairs down. Press it again to descend." );
		return false;
	}

	// BFS over what the player knows (recorded terrain), 8 directions.
	std::vector<int> prev( W*H, -2 );
	std::vector<int> queue;
	int start = loc.z*W + loc.x;
	prev[start] = -1;
	queue.push_back(start);
	int goal = -1;
	bool blocked_by_skip = false;
	for (size_t qi = 0; (qi < queue.size()) && (goal < 0); ++qi)
	{
		int c = queue[qi], cx = c % W, cz = c / W;
		if (c != start)
		{
			TerrainType rec( zone.recordedTerrainAt(cx, cz) );
			if (explore)
			{
				bool target = false;
				if (!visited.count(c))
				{
					for (int dz=-1; dz<=1 && !target; ++dz)
						for (int dx=-1; dx<=1; ++dx)
							if (zone.isWithin(cx+dx, cz+dz) && (0 == zone.recordedObjectAt(cx+dx, cz+dz)))
								{ target = true; break; }
					if (!target && model.isVisible(loc.zone, cx, cz))
						BOOST_FOREACH( DBKeyValue ok, zone.objectsAt(cx, cz) )
							if (world.objectExists(ok))
							{
								WorldObjectType t( dynamic_cast< const AHGameObject& >( world.object(ok) ).type() );
								if ((objects::Pickup == t) || (objects::Armour == t) || (objects::Weapon == t))
									target = true;
							}
				}
				if (target) { goal = c; break; }
			}
			else if (rec == stair_type)
			{
				goal = c; break;
			}
		}
		bool here_door = (terrain::Door == terrain::getType( zone.recordedTerrainAt(cx, cz) ));
		for (int dz=-1; dz<=1; ++dz)
			for (int dx=-1; dx<=1; ++dx)
			{
				if (!dx && !dz) continue;
				int nx = cx+dx, nz = cz+dz;
				if (!zone.isWithin(nx, nz)) continue;
				int n = nz*W + nx;
				if (prev[n] != -2) continue;
				TerrainType rec( zone.recordedTerrainAt(nx, nz) );
				if (0 == rec) continue;
				if (!terrain::isPassable(rec, true)) continue;
				bool door = (terrain::Door == terrain::getType(rec));
				if ((door || here_door) && dx && dz) continue;
				if (skip.count(n)) { blocked_by_skip = true; continue; }
				prev[n] = c;
				queue.push_back(n);
			}
	}
	if (goal < 0)
	{
		if (explore)
			stopAuto( blocked_by_skip ? "Nothing reachable left to explore: blocked doors or obstacles cut the way." : "Nothing left to explore." );
		else
			stopAuto( (2 == m_auto) ? "You don't know of any reachable stairs up." : "You don't know of any reachable stairs down." );
		return false;
	}
	int step = goal;
	while (prev[step] != start) step = prev[step];
	m_auto_tx = step % W; m_auto_tz = step / W;
	m_auto_px = loc.x; m_auto_pz = loc.z;
	mx = m_auto_tx - loc.x; mz = m_auto_tz - loc.z;
	TerrainType real( zone.terrainAt(m_auto_tx, m_auto_tz) );
	m_auto_door = (terrain::Door == terrain::getType(real)) && !terrain::isOpen(real) && !terrain::isBroken(real);
	m_auto_moved = !m_auto_door;
	return true;
}

bool PlayingGame::isFn( const AUserInputItem& in, const char* fn ) const
{
	return m_is_cmd ? (m_cmd == fn) : m_key_map->isFunction(in, fn);
}

PlayingGame::CommandResult PlayingGame::interpretInput( const AUserInputItem& input, AGameModel& in_model )
{
	m_is_cmd = (dynamic_cast<const KeyPress&>(input).value == RL_shared::KeyCode( (char)(RVIP_CMD_KEY & 0xff), true ));
	m_cmd = m_is_cmd ? rvip_cmd : std::string();
	bool reopen = false;
	std::string::size_type bar( m_cmd.find("|reopen") );
	if (std::string::npos != bar) { reopen = true; m_cmd.erase(bar); }
	m_child_opened = false;
#ifdef __EMSCRIPTEN__
	AHGameModel& am( dynamic_cast<AHGameModel&>(in_model) );
	if (m_rvip_save_due && !m_quit && !rvip_auto_on && (rvip_next_key < 0) && am.world().objectExists(am.avatar()))
	{	// idle command prompt: autosave (atomic, synced to IndexedDB by saveGame); no screen change
		m_rvip_save_due = false;
		m_rvip_save_name = getSaveFileName(am);
		saveGame(am, m_rvip_save_name.c_str());
	}
	DBKeyValue zone_before( am.world().objectExists(am.avatar()) ? am.world().object(am.avatar()).location().zone : INVALID_KEY );
#endif
	CommandResult res( interpretKey( input, in_model ) );
#ifdef __EMSCRIPTEN__
	if (am.world().objectExists(am.avatar()) && (am.world().object(am.avatar()).location().zone != zone_before))
		m_rvip_save_due = true;	// new floor: autosave at the next idle prompt
#endif
	m_is_cmd = false;
	if (reopen && !m_child_opened && !m_quit)
		rvipRunCommand("Inventory?");	// list reopens after the action unless an alien is in view
	return res;
}

PlayingGame::CommandResult PlayingGame::interpretKey( const AUserInputItem& input, AGameModel& in_model )
{
	AHGameModel& model( dynamic_cast<AHGameModel&>(in_model) );
	World& world( model.world() );

	if (!world.objectExists( model.avatar() ))
	{
		m_quit = true;
		showMortem();
		return CommandResult( false, true );
	}

	const bool auto_key = (dynamic_cast<const KeyPress&>(input).value == AUTO_KEY);
	const bool had_new_msg = m_msgs.hasNewMessage();
	const bool was_door = m_auto_door;
	if (m_auto && (!auto_key || !rvip_auto_on))
		stopAuto("");	// a real key interrupted the walk

	m_msgs.beginNewMessage();

	if (m_is_cmd && ("Inventory?" == m_cmd))
	{
		if (scanView(model, 0, 0, true) > 0)
			return CommandResult( false, true );
		m_cmd = "Inventory";
	}

	OverWorld& overworld( model.overworld() );

	ASSERT( world.objectExists( model.avatar() ) );
	shared_ptr< PlayerCharacter > player_obj( dynamic_pointer_cast<PlayerCharacter>( world.objectPtr( model.avatar() ) ) );
	ASSERT( player_obj );
	WorldObject::WorldLocation loc( player_obj->location() );

	m_last_input = model.gameTime();

	bool advance(false);
	bool move(false);
	int mx(0), mz(0);


	//if (RL_shared::KeyCode('v',false) == (dynamic_cast<const RL_shared::KeyPress&>(input)).value)
	//{
	//}

	if (auto_key)
	{
		if (!m_auto)
			return CommandResult( false, true );
		if (had_new_msg && !was_door)
		{
			stopAuto("");
			return CommandResult( false, true );
		}
		if (!autoStep(model, mx, mz))
			return CommandResult( false, true );
		move = true;
		m_move_only = m_turn_only = m_activating_object = m_placing_charge = false;
	}
	else if (isFn(input, "OK"))
	{
		setNextState( shared_ptr< CommandMenu >( new CommandMenu( interfaceStateMachine(), m_key_map, model.isCountdownActive() ) ) );
		return CommandResult( false, true );
	}
	else if (isFn(input, "Inventory"))
	{
		setNextState( shared_ptr< InventoryMenu >( new InventoryMenu( interfaceStateMachine(), m_key_map, model.isCountdownActive(), false ) ) );
		return CommandResult( false, true );
	}
	else if (m_is_cmd && (0 == m_cmd.compare(0, 5, "Drop:")))
	{
		SelectDropItem selector(m_msgs, model, world, overworld, player_obj);
		advance = selector.select(m_cmd.substr(5));
	}
	else if (m_is_cmd && (0 == m_cmd.compare(0, 8, "Examine:")))
	{
		std::string fn( m_cmd.substr(8) ), text;
		if ("Armour" == fn && world.objectExists(player_obj->armour()))
			text = "Armour: " + dynamic_cast<const AHGameObject&>(world.object(player_obj->armour())).getSelectName(true) + ".";
		else if (("Sidearm" == fn) || ("Primary" == fn))
		{
			PlayerCharacter::WeaponSlot slot( ("Sidearm" == fn) ? PlayerCharacter::Sidearm : PlayerCharacter::Primary );
			if (world.objectExists(player_obj->weapon(slot)))
				text = fn + ": " + dynamic_cast<const AHGameObject&>(world.object(player_obj->weapon(slot))).getSelectName(true)
					+ ((player_obj->currentWeapon() == slot) ? ", in hand." : ".");
		}
		else
		{
			static const struct { const char* fn; pickup::Type t; const char* name; } pk[] = {
				{ "Frag", pickup::FragGrenade, "Frag grenades" }, { "Krak", pickup::KrakGrenade, "Krak grenades" },
				{ "Stun", pickup::StunGrenade, "Stun grenades" }, { "Inc", pickup::IncGrenade, "Inc grenades" },
				{ "Medkit", pickup::Medkit, "Medkits" }, { "Neutraliser", pickup::Neutraliser, "Neutraliser" },
				{ "Demolition", pickup::DemoCharge, "Demolition charges" } };
			for (size_t i = 0; i < sizeof pk / sizeof pk[0]; ++i)
				if (fn == pk[i].fn)
					text = std::string(pk[i].name) + ": " + std::to_string(player_obj->num(pk[i].t)) + " of " + std::to_string(player_obj->max(pk[i].t)) + ".";
		}
		if (!text.empty())
			m_msgs.addString(text.c_str());
	}
	else if (isFn(input, "Explore"))
	{
		startAuto( model, 1 );
		return CommandResult( false, true );
	}
	else if (isFn(input, "Help"))
	{
		shared_ptr< HelpScreen > newstate( new HelpScreen(interfaceStateMachine(), m_key_map, model.isCountdownActive() ? HelpScreen::Red : HelpScreen::Normal) );
		setNextState( newstate );
		return CommandResult( false, true );
	}
	else if (isFn(input, "Left"))
	{
		move = true;
		mx = -1;
	}
	else if (isFn(input, "Right"))
	{
		move = true;
		mx = 1;
	}
	else if (isFn(input, "Up"))
	{
		move = true;
		mz = 1;
	}
	else if (isFn(input, "Down"))
	{
		move = true;
		mz = -1;
	}
	else if (isFn(input, "UpAndLeft"))
	{
		move = true;
		mx = -1;
		mz = 1;
	}
	else if (isFn(input, "UpAndRight"))
	{
		move = true;
		mx = 1;
		mz = 1;
	}
	else if (isFn(input, "DownAndLeft"))
	{
		move = true;
		mx = -1;
		mz = -1;
	}
	else if (isFn(input, "DownAndRight"))
	{
		move = true;
		mx = 1;
		mz = -1;
	}
	else if (isFn(input, "Wait"))
	{
		advance = true;
		shared_ptr< WaitAction > wait_action( new WaitAction(player_obj, player_obj->getActionTime(model, player_actions::Wait)) );
		model.actionEngine().addAction( wait_action );
	}
	else if (isFn(input, "Strafe"))
	{
		m_move_only = true;
		m_turn_only = false;
		m_move_lock = false;
		m_turn_lock = false;
		m_activating_object = false;
		m_placing_charge = false;
	}
	else if (isFn(input, "StrafeLock"))
	{
		m_move_only = true;
		m_turn_only = false;
		m_move_lock = true;
		m_turn_lock = false;
		m_activating_object = false;
		m_placing_charge = false;
	}
	else if (isFn(input, "Turn"))
	{
		m_move_only = false;
		m_turn_only = true;
		m_move_lock = false;
		m_turn_lock = false;
		m_activating_object = false;
		m_placing_charge = false;
	}
	else if (isFn(input, "TurnLock"))
	{
		m_move_only = false;
		m_turn_only = true;
		m_move_lock = false;
		m_turn_lock = true;
		m_activating_object = false;
		m_placing_charge = false;
	}
	else if (isFn(input, "Back"))
	{
		m_activating_object = false;
		m_placing_charge = false;
		m_move_only = false;
		m_turn_only = false;
		m_move_lock = false;
		m_turn_lock = false;
	}
	else if (isFn(input, "Get"))
	{
		advance = doGetPickupsMenu(*this, m_key_map, model, m_msgs);
	}
	else if (isFn(input, "Fire"))
	{
		DBKeyValue weapon_key( player_obj->weapon( player_obj->currentWeapon() ) );
		if (!world.objectExists(weapon_key))
		{
			m_msgs.addString("You aren't holding a weapon!");
		}
		else
		{
			shared_ptr< Weapon > weapon_obj( dynamic_pointer_cast<Weapon>( world.objectPtr( weapon_key ) ) );
			if (0 >= weapon_obj->getAmmo())
			{
				m_msgs.addString("Your weapon is empty!");
			}
			else
			{
				m_msgs.addString( "Select target:" );
				shared_ptr< SelectFireTarget > action( new SelectFireTarget(m_msgs) );
				shared_ptr< TargetSelect > newstate( new TargetSelect( interfaceStateMachine(), m_key_map, action, m_msgs, KeyCode('f', false) ) );

				DBKeyValue jump_to( findNearestInterestingThing(model, true) );
				newstate->jumpToObject(model, jump_to);

				setNextState( newstate );
				return CommandResult( false, true );
			}
		}
	}
	else if (isFn(input, "Look"))
	{
		if (m_first_look_mode)
			m_msgs.addString("Use movement controls to move the cursor, and look at the bottom left of the screen to see what is under it. Press Esc to return to the game.");

		m_first_look_mode = false;

		WorldObject::WorldLocation look_loc( loc );
		DBKeyValue look_obj( findNearestInterestingThing(model, false) );
		if (world.objectExists(look_obj))
			look_loc = world.object(look_obj).location();

		OverWorld::BlockAndFloor bnf( overworld.getBlockAndFloor( look_loc.zone ) );

		shared_ptr< LookMode > newstate( new LookMode(interfaceStateMachine(), m_key_map, m_msgs, look_loc.x, look_loc.z, bnf.block, bnf.floor) );
		setNextState( newstate );
		return CommandResult( false, true );
	}
	else if (isFn(input, "Sidearm"))
	{
		if (PlayerCharacter::Sidearm != player_obj->currentWeapon())
		{
			advance = true;
			shared_ptr< SwitchWeaponAction > action( new SwitchWeaponAction(player_obj, PlayerCharacter::Sidearm, 
				player_obj->getActionTime(model, player_actions::SwitchToSidearm)) );
			model.actionEngine().addAction( action );
		}
		else
			m_msgs.addString("You're already holding your sidearm!");
	}
	else if (isFn(input, "Primary"))
	{
		if (world.objectExists(player_obj->weapon( PlayerCharacter::Primary )))
		{
			if (PlayerCharacter::Primary != player_obj->currentWeapon())
			{
				advance = true;
				shared_ptr< SwitchWeaponAction > action( new SwitchWeaponAction(player_obj, PlayerCharacter::Primary, 
					player_obj->getActionTime(model, player_actions::SwitchToPrimary)) );
				model.actionEngine().addAction( action );
			}
			else
				m_msgs.addString("You're already holding your primary weapon!");
		}
		else
			m_msgs.addString("You don't have a primary weapon!");
	}
	else if (isFn(input, "Reload"))
	{
		DBKeyValue weapon_key( player_obj->weapon( player_obj->currentWeapon() ) );
		if (world.objectExists(weapon_key))
		{
			Weapon& weapon_obj( dynamic_cast< Weapon& >( world.object( weapon_key ) ) );
			if (weapon_obj.getAmmo() >= weapon_obj.weaponData().clip_size )
			{
				m_msgs.addString("Your weapon is already loaded!");
			}
			else
			{
				advance = true;
				shared_ptr< ReloadWeaponAction > action( new ReloadWeaponAction(player_obj, 
					player_obj->getActionTime(model, player_actions::ReloadWeapon)) );
				model.actionEngine().addAction( action );
			}
		}
		else
		{
			m_msgs.addString("You're not holding a weapon.");
		}
	}
	//TODO merge code for the below two commands
	else if (isFn(input, "FloorUp"))
	{
		if (world.zoneExists(loc.zone))
		{
			Zone& zone( world.zone( loc.zone ) );
			if (terrain::StairsUp != zone.terrainAt(loc.x, loc.z))
			{
				startAuto( model, 2 );
				return CommandResult( false, true );
			}
			else
			{
				OverWorld::BlockAndFloor bnf = overworld.getBlockAndFloor(loc.zone);
				if (bnf.floor < OverWorld::MAX_FLOORS)
				{
					DBKeyValue target_floor = overworld.getFloorKey(bnf.block, bnf.floor+1);
					if (world.zoneExists(target_floor))
					{
						//TODO add action for this
						if (player_obj->moveTo( model, WorldObject::WorldLocation( target_floor, loc.x, loc.z ), true ))
						{
							RVIP_SOUND("stairs");
							model.updateVision();
							model.updateHearing();
						}
					}
				}
			}
		}
	}
	else if (isFn(input, "FloorDown"))
	{
		if (world.zoneExists(loc.zone))
		{
			Zone& zone( world.zone( loc.zone ) );
			if (terrain::StairsDown != zone.terrainAt(loc.x, loc.z))
			{
				startAuto( model, 3 );
				return CommandResult( false, true );
			}
			else
			{
				OverWorld::BlockAndFloor bnf = overworld.getBlockAndFloor(loc.zone);
				if (bnf.floor > OverWorld::FLOOR_MIN)
				{
					DBKeyValue target_floor = overworld.getFloorKey(bnf.block, bnf.floor-1);
					if (world.zoneExists(target_floor))
					{
						//TODO add action for this
						if (player_obj->moveTo( model, WorldObject::WorldLocation( target_floor, loc.x, loc.z ), true ))
						{
							RVIP_SOUND("stairs");
							model.updateVision();
							model.updateHearing();
						}
					}
				}
			}
		}
	}
	//TODO merge code from the below 4 grenade-throwing commands
	else if (isFn(input, "Frag"))
	{
		if (player_obj->num( pickup::FragGrenade ) < 1)
		{
			m_msgs.addString("You don't have any frag grenades!");
		}
		else
		{
			m_msgs.addString( "Select target:" );
			shared_ptr< SelectGrenadeTarget > action( new SelectGrenadeTarget(m_msgs, pickup::FragGrenade) );
			shared_ptr< TargetSelect > newstate( new TargetSelect( interfaceStateMachine(), m_key_map, action, m_msgs, KeyCode('r', false) ) );
			setNextState( newstate );
			return CommandResult( false, true );
		}
	}
	else if (isFn(input, "Krak"))
	{
		if (player_obj->num( pickup::KrakGrenade ) < 1)
		{
			m_msgs.addString("You don't have any krak grenades!");
		}
		else
		{
			m_msgs.addString( "Select target:" );
			shared_ptr< SelectGrenadeTarget > action( new SelectGrenadeTarget(m_msgs, pickup::KrakGrenade) );
			shared_ptr< TargetSelect > newstate( new TargetSelect( interfaceStateMachine(), m_key_map, action, m_msgs, KeyCode('k', false) ) );
			setNextState( newstate );
			return CommandResult( false, true );
		}
	}
	else if (isFn(input, "Stun"))
	{
		if (player_obj->num( pickup::StunGrenade ) < 1)
		{
			m_msgs.addString("You don't have any stun grenades!");
		}
		else
		{
			m_msgs.addString( "Select target:" );
			shared_ptr< SelectGrenadeTarget > action( new SelectGrenadeTarget(m_msgs, pickup::StunGrenade) );
			shared_ptr< TargetSelect > newstate( new TargetSelect( interfaceStateMachine(), m_key_map, action, m_msgs, KeyCode('t', false) ) );
			setNextState( newstate );
			return CommandResult( false, true );
		}
	}
	else if (isFn(input, "Inc"))
	{
		if (player_obj->num( pickup::IncGrenade ) < 1)
		{
			m_msgs.addString("You don't have any inc grenades!");
		}
		else
		{
			m_msgs.addString( "Select target:" );
			shared_ptr< SelectGrenadeTarget > action( new SelectGrenadeTarget(m_msgs, pickup::IncGrenade) );
			shared_ptr< TargetSelect > newstate( new TargetSelect( interfaceStateMachine(), m_key_map, action, m_msgs, KeyCode('i', false) ) );
			setNextState( newstate );
			return CommandResult( false, true );
		}
	}
	else if (isFn(input, "Demolition"))
	{
		if (player_obj->num( pickup::DemoCharge ) < 1)
		{
			m_msgs.addString("You don't have any demo charges!");
		}
		else
		{
			m_msgs.addString("Press direction to blast:");
			m_placing_charge = true;
			m_activating_object = false;
		}
	}
	//else if (isFn(input, "Armour"))
	//{
	//}
	else if (isFn(input, "Drop"))
	{
        // RVIP: the drop prompt is the inventory list with a cursor (item keys drop as before)
        setNextState( shared_ptr< InventoryMenu >( new InventoryMenu( interfaceStateMachine(), m_key_map, model.isCountdownActive(), true ) ) );
		return CommandResult( false, true );
	}
	else if (isFn(input, "Neutraliser"))
	{
		if (player_obj->canUseItem(pickup::Neutraliser))
		{
			if (player_obj->isAcidSplashed(model))
			{
				advance = true;
				shared_ptr< UseItemAction > use_action( new UseItemAction(player_obj, pickup::Neutraliser, 
					player_obj->getActionTime(model, player_actions::UseNeutraliser)) );
				model.actionEngine().addAction( use_action );
			}
			else
				m_msgs.addString("There's nothing to neutralise.");
		}
		else
		{
			m_msgs.addString("You don't have any neutraliser.");
		}
	}
	else if (isFn(input, "Medkit"))
	{
		if (player_obj->canUseItem(pickup::Medkit))
		{
			if (player_obj->getHP() < player_obj->getTopHP())
			{
				advance = true;
				bool plus = player_obj->getHP() == player_obj->getMaxHP();
				GameTimeCoordinate use_time = player_obj->getActionTime(model, plus ? player_actions::UseMedkit : player_actions::UseMedkitPlus);
				if (player_obj->hasPerk(player_perks::LoseLessMaxHP))
					use_time /= 2;
				shared_ptr< UseItemAction > use_action( new UseItemAction(player_obj, pickup::Medkit, use_time) );
				model.actionEngine().addAction( use_action );
			}
			else
				m_msgs.addString("You'd rather not use that stuff if you don't have to...");
		}
		else
		{
			m_msgs.addString("You don't have a medkit.");
		}
	}
	else if (isFn(input, "ScrollDown"))
	{
		m_msgs.scrollDown();
	}
	else if (isFn(input, "ScrollUp"))
	{
		m_msgs.scrollUp();
	}
	else if (isFn(input, "Operate"))
	{
		//TODO move this block of code out.
		bool found_terminal( false );
		if (world.zoneExists(loc.zone))
		{
			const Zone& player_zone( world.zone( loc.zone ) );
			if (player_zone.isWithin( loc.x, loc.z ))
			{
				const ObjectList& objects( player_zone.objectsAt( loc.x, loc.z ) );
				BOOST_FOREACH( DBKeyValue obj_key, objects )
				{
					if (world.objectExists(obj_key))
					{
						const AHGameObject& game_obj( dynamic_cast< const AHGameObject& >( world.object(obj_key) ) );
						if (game_obj.type() == objects::Terminal)
						{
							found_terminal = true;
							const Terminal& terminal( dynamic_cast< const Terminal& >(game_obj) );
							if (!terminal.isBroken())
							{
								if (terminal::Info == terminal.terminalType())
								{
									OverWorld::BlockAndFloor bnf = overworld.getBlockAndFloor(loc.zone);
									if (!overworld.isMapped(bnf.block, bnf.floor))
									{
										shared_ptr< UseTerminalAction > action( new UseTerminalAction( player_obj, terminal::Info, 
											player_obj->getActionTime(model, player_actions::UseTerminal) ) );
										model.actionEngine().addAction( action );
										advance = true;
										m_msgs.addString("Downloading map...");
									}
									else
									{
										m_msgs.addString("You have already downloaded the map for this floor.");
									}
								}
								else if (terminal::Master == terminal.terminalType())
								{
									if (model.usedMasterTerminal())
									{
										m_msgs.addString("You have already accessed the master terminal... There's nothing more you can do with it.");
									}
									else
									{
										shared_ptr< UseTerminalAction > action( new UseTerminalAction( player_obj, terminal::Master, 
											player_obj->getActionTime(model, player_actions::UseTerminal) ) );
										model.actionEngine().addAction( action );
										advance = true;
										m_msgs.addString("Accessing master terminal...");
									}
								}
							}
							else
							{
								m_msgs.addString("You can't use this terminal; it's totalled.");
							}
						}
					}
				}
			}
		}

		if (!found_terminal)
		{
			if (!m_activating_object)
				m_msgs.addString("Press direction of object to operate:");
			m_activating_object = true;
		}
	}
	else if (isFn(input, "Char"))
	{
		shared_ptr< CharScreen > newstate( new CharScreen( interfaceStateMachine(), m_key_map ) );
		setNextState( newstate );
		return CommandResult( false, true );
	}
	else
	if (isFn(input, "Buy"))
	{
		shared_ptr< BuyPerks > newstate( new BuyPerks( interfaceStateMachine(), m_key_map ) );
		setNextState( newstate );
		return CommandResult( false, true );
	}
	else if (isFn(input, "Save"))
	{
		if (saveGame(model, getSaveFileName(model)))
		{
			m_rvip_player_saved = true;
			m_quit = true;
		}
	}

	if (move)
	{
		advance = true;
		if (m_activating_object || m_placing_charge)
		{
			WorldObject::WorldLocation tloc = loc;
			tloc.x += mx;
			tloc.z += mz;

			TerrainType there( world.zone( tloc.zone ).terrainAt( tloc.x, tloc.z ) );

			if (m_activating_object)
			{
				if ((terrain::Door == terrain::getType(there)) && !terrain::isBroken(there))
				{
					if (0 == (there & terrain::Open))
					{
						shared_ptr< OpenDoorAction > open_action( new OpenDoorAction(player_obj, tloc, 
							player_obj->getActionTime(model, player_actions::OpenDoor)) );
						model.actionEngine().addAction( open_action );
						m_msgs.addString("You activate the door.");
					}
					else
					{
						shared_ptr< CloseDoorAction > close_action( new CloseDoorAction(player_obj, tloc, 
							player_obj->getActionTime(model, player_actions::CloseDoor)) );
						model.actionEngine().addAction( close_action );
						m_msgs.addString("You activate the door.");
					}
				}
				else
				{
					m_msgs.addString("There's nothing to activate.");
				}
			}
			else if (m_placing_charge)
			{
				if ((mx != 0) && (mz != 0))
				{
					m_msgs.addString("You can't blast diagonally.");
				}
				else if ((terrain::Wall == terrain::getType(there)) && !terrain::isBroken(there))
				{
					advance = true;
					shared_ptr< SetChargeAction > use_action( new SetChargeAction(
						player_obj, player_obj->location(), mx, mz, 
						player_obj->getActionTime(model, player_actions::SetCharge)) );
					model.actionEngine().addAction( use_action );
				}
				else
				{
					m_msgs.addString("There's nothing there to blast.");
				}
			}
		}
		else
		{
			if (!m_turn_only)
			{
				if (INVALID_KEY != loc.zone)
				{
					WorldObject::WorldLocation tloc = loc;
					tloc.x += mx;
					tloc.z += mz;

					const Zone& target_zone( world.zone( tloc.zone ) );
					bool cancel( false );
					//TODO this block of code should REALLY not be in here. Move out.
					if (!target_zone.isWithin( tloc.x, tloc.z ))
					{
						cancel = true;
						OverWorld::BlockAndFloor bnf = overworld.getBlockAndFloor(tloc.zone);
						int search_x(0), search_z(0);
						if ((-1 != bnf.block) && (-1 != bnf.floor))
						{
							if (tloc.z < 0)
							{	//south
								int tblock = overworld.exitsToBlock(bnf.block, bnf.floor, OverWorld::South);
								tloc.zone = overworld.getFloorKey(tblock, bnf.floor);
								if (world.zoneExists(tloc.zone))
								{
									tloc.z = world.zone(tloc.zone).sizeZ()-1;
									search_x = 1;
									cancel = false;
								}
							}
							else if (tloc.z >= target_zone.sizeZ())
							{	//north
								int tblock = overworld.exitsToBlock(bnf.block, bnf.floor, OverWorld::North);
								tloc.zone = overworld.getFloorKey(tblock, bnf.floor);
								if (world.zoneExists(tloc.zone))
								{
									tloc.z = 0;
									search_x = 1;
									cancel = false;
								}
							}
							else if (tloc.x < 0)
							{	//west
								int tblock = overworld.exitsToBlock(bnf.block, bnf.floor, OverWorld::West);
								tloc.zone = overworld.getFloorKey(tblock, bnf.floor);
								if (world.zoneExists(tloc.zone))
								{
									tloc.x = world.zone(tloc.zone).sizeX()-1;
									search_z = 1;
									cancel = false;
								}
							}
							else if (tloc.x >= target_zone.sizeX())
							{	//east
								int tblock = overworld.exitsToBlock(bnf.block, bnf.floor, OverWorld::East);
								tloc.zone = overworld.getFloorKey(tblock, bnf.floor);
								if (world.zoneExists(tloc.zone))
								{
									tloc.x = 0;
									search_z = 1;
									cancel = false;
								}
							}
						}

						if (!cancel)
						{	//the bridge openings might not line up, so find the nearest space the player can move to.
							const Zone& new_target_zone( world.zone( tloc.zone ) );
							for (int i=0; ; ++i)
							{
								WorldObject::WorldLocation try_loc1( tloc.zone, tloc.x + (i*search_x), tloc.z + (i*search_z) );
								WorldObject::WorldLocation try_loc2( tloc.zone, tloc.x - (i*search_x), tloc.z - (i*search_z) );
								if (!new_target_zone.isWithin(try_loc1.x, try_loc1.z))
									if (!new_target_zone.isWithin(try_loc2.x, try_loc2.z))
									{
										cancel = true;
										break;
									}
								TerrainType there1( new_target_zone.terrainAt( try_loc1.x, try_loc1.z ) );
								if (player_obj->canOverlap( there1 ))
								{
									tloc = try_loc1;
									break;
								}
								TerrainType there2( new_target_zone.terrainAt( try_loc2.x, try_loc2.z ) );
								if (player_obj->canOverlap( there2 ))
								{
									tloc = try_loc2;
									break;
								}
							}
						}
					}

					if (cancel)
					{
						advance = !m_move_only;
					}
					else
					{
						TerrainType there( world.zone( tloc.zone ).terrainAt( tloc.x, tloc.z ) );
						if (player_obj->canOverlap( there ))
						{
							GameTimeCoordinate move_time( (mx != 0) && (mz != 0) ? 
								player_obj->getActionTime(model, player_actions::MoveDiagonal) : 
								player_obj->getActionTime(model, player_actions::Move) );
							shared_ptr< DiscreteMoveAction > move_action( new DiscreteMoveAction(player_obj, tloc, move_time) );
							model.actionEngine().addAction( move_action );
						}
						else
						{
							if ((terrain::Door == terrain::getType(there)) && !terrain::isBroken(there))
							{
								shared_ptr< OpenDoorAction > open_action( new OpenDoorAction(player_obj, tloc, 
									player_obj->getActionTime(model, player_actions::OpenDoor)) );
								model.actionEngine().addAction( open_action );
								m_msgs.addString("You activate the door.");
							}
						}
					}
				}
			}
			if (!m_move_only)
			{
				const int16_t angles[3][3] = { {-135, -90, -45}, {180, 0, 0}, {135, 90, 45} };
				int16_t heading = angles[mz+1][mx+1];
				shared_ptr< DiscreteTurnAction > turn_action( 
					new DiscreteTurnAction(player_obj, heading, 
						player_obj->getActionTime(model, player_actions::Turn)) );
				model.actionEngine().addAction( turn_action );
			}
		}
		if (!m_turn_lock)
			m_turn_only = false;
		if (!m_move_lock)
			m_move_only = false;
		m_activating_object = false;
		m_placing_charge = false;
	}

	return CommandResult( advance, true );
}

void PlayingGame::notifyAHGameModelAdvance( RL_shared::AGameModel& in_model, RL_shared::GameTimeCoordinate dt, bool is_current_state )
{
	if (is_current_state)
	{
		AHGameModel& model( dynamic_cast<AHGameModel&>(in_model) );
		World& world( model.world() );

		if (!world.objectExists(model.avatar()))
		{
			if (!m_shown_death_msg)
			{
				m_shown_death_msg = true;
				m_msgs.beginNewMessage();
				m_msgs.addString("You are dead. Press any key to continue.");

				writeMortem(in_model);
			}
			return;
		}

		if (model.worldExplodes())
		{
			shared_ptr< Kaboom > newstate( new Kaboom( interfaceStateMachine() ) );
			setNextState( newstate );
			m_quit = true;

			if (world.objectExists(model.avatar()))
			{
				PlayerCharacter& player( dynamic_cast< PlayerCharacter& >( world.object(model.avatar()) ) );
				player.writeOutcome(model, "Obliterated in a massive nuclear explosion");
			}

			writeMortem(in_model);

			return;
		}
		if (model.usedMasterTerminal() && 
			(!m_shown_master_text))
		{
			m_shown_master_text = true;
			shared_ptr< MessageBox > newstate( new MessageBox(interfaceStateMachine(), m_key_map, used_master_terminal_text, 13, 10, 66, 30, model.isCountdownActive() ? MessageBox::Red : MessageBox::Green, 1) );
			setNextState( newstate );
			return;
		}
		if (model.isQueenFound() && 
			(!m_shown_found_queen_text))
		{
			m_shown_found_queen_text = true;
			shared_ptr< MessageBox > newstate( new MessageBox(interfaceStateMachine(), m_key_map, found_queen_text, 17, 14, 63, 25, model.isCountdownActive() ? MessageBox::Red : MessageBox::Green, 1) );
			setNextState( newstate );
			return;
		}
		if (model.isQueenDead() && 
			(!m_shown_killed_queen_text))
		{
			m_shown_killed_queen_text = true;
			shared_ptr< MessageBox > newstate( new MessageBox(interfaceStateMachine(), m_key_map, killed_queen_text, 11, 14, 69, 25, model.isCountdownActive() ? MessageBox::Red : MessageBox::Green, 1) );
			setNextState( newstate );

			m_msgs.addString("Return to the colony entrance to escape.");

			return;
		}
		if (model.isCountdownActive() && 
			(!m_shown_countdown_start_text))
		{
			m_shown_countdown_start_text = true;
			shared_ptr< MessageBox > newstate( new MessageBox(interfaceStateMachine(), m_key_map, started_countdown_text, 14, 12, 64, 27, MessageBox::Red, 1) );
			setNextState( newstate );

			m_msgs.addString("Return to the colony entrance to escape.");

			return;
		}
		if (model.hasPlayerEscaped())
		{
			if (!m_shown_ending_text_1)
			{
				m_good_ending = model.doesPlayerGetTheGoodEnding();

				if (world.objectExists(model.avatar()))
				{
					PlayerCharacter& player( dynamic_cast< PlayerCharacter& >( world.object(model.avatar()) ) );
					if (m_good_ending)
						player.writeOutcome(model, "Escaped the infested colony and then... who knows?");
					else
						player.writeOutcome(model, "Escaped, but obliterated in the reactor explosion");
				}

				writeMortem(in_model);

				m_shown_ending_text_1 = true;

				shared_ptr< StoryText > newstate( new StoryText(interfaceStateMachine(), ending_text_1, 8) );
				setNextState( newstate );

				return;
			}
		}
	}
}

bool PlayingGame::finished(void)
{
#ifdef __EMSCRIPTEN__
	if (m_quit && !m_rvip_player_saved && !m_rvip_save_name.empty())
	{	// the run ended (death, win, explosion): the autosave must not bring it back
		std::remove(m_rvip_save_name.c_str());
		m_rvip_save_name.clear();
		rvip_sync();
	}
#endif
	return m_quit;
}

void PlayingGame::draw( AOutputWindow& window, AGameModel& in_model ) const
{
	Console& console( dynamic_cast<Console&>(window) );
	const AHGameModel& model( dynamic_cast<AHGameModel&>(in_model) );
	const World& world( model.world() );

	console.clearScreen();
#ifdef __EMSCRIPTEN__
	RvipBase rvip_base_guard( true );	// the main screen: routed to the Map/Status windows
	rvipSidePanes( model, *m_key_map );
#endif

	drawFrame(console, model.isCountdownActive());

	if (model.isCountdownActive())
		m_msgs.draw(console, Console::BrightRed, Console::Red);
	else
		m_msgs.draw(console, Console::BrightGreen, Console::Green);

	DBKeyValue player_key( model.avatar() );
	if (!world.objectExists(player_key))
		return;

	const PlayerCharacter& player_obj( dynamic_cast<const PlayerCharacter&>( world.object( player_key ) ) );
	WorldObject::WorldLocation loc( player_obj.location() );

	drawWorld(console, model, loc.zone, loc.x, loc.z, loc.x, loc.z, 
		WORLD_VIEW_X_LOW, WORLD_VIEW_X_HI, WORLD_VIEW_Y_LOW, WORLD_VIEW_Y_HI, 
		cursor_type::None, 0, 0, false, 
		player_obj.hasMotionDetection(model) );

	drawZoneName(console, model, loc.zone, model.isCountdownActive());

	GameTimeCoordinate time_step = 0;//model.gameTime() - m_last_input;
	drawHUD(console, model, HUD_X_LOW, HUD_Y_LOW, HUD_X_HI, HUD_Y_HI, time_step);
}

bool PlayingGame::drawsWholeWindow(void) const
{
	return true;
}

void PlayingGame::writeMortem( AGameModel& in_model )
{
	AHGameModel& model( dynamic_cast< AHGameModel& >( in_model ) );

	try
	{
		std::ofstream fout( "mortem.txt" );
		if (fout.good())
		{
			fout << model.mortem();
		}
	}
	catch(...) //TODO signal in some fashion if there was a file exception
	{
	}
}

void PlayingGame::showMortem()
{
	if (m_shown_mortem)
		return;

	try
	{
		std::ifstream fin( "mortem.txt" );
		if (fin.good())
		{
			//ugh.
			std::string temp((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
			std::istringstream issin(temp);

			shared_ptr< ViewFile > newstate( new ViewFile( 
					interfaceStateMachine(), m_key_map, issin, ViewFile::Normal 
					) 
				);
			setNextState( newstate );
		}
	}
	catch(...)
	{
	}

	m_shown_mortem = true;
}

}
