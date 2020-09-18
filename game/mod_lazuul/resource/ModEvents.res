//=========== (C) Copyright 2005 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//=============================================================================

// No spaces in event names, max length 32
// All strings are case sensitive
//
// valid data key types are:
//   string : a zero terminated string
//   bool   : unsigned int, 1 bit
//   byte   : unsigned int, 8 bit
//   short  : signed int, 16 bit
//   long   : signed int, 32 bit
//   float  : float, 32 bit
//   local  : any data, but not networked to clients
//
// following key names are reserved:
//   local      : if set to 1, event is not networked to clients
//   unreliable : networked, but unreliable
//   suppress   : never fire this event
//   time	: firing server time
//   eventid	: holds the event ID

"ModEvents"
{
	

	"intro_finish"
	{
		"player"	"short"		// entindex of the player
	}

	"intro_nextcamera"
	{
		"player"	"short"		// entindex of the player
	}	

	"mm_lobby_chat"
	{
		"steamid"	"string"	// steamID (64-bit value converted to string) of user who said the thing
		"text"	"string"	// Their chat message
		"type"	"short"	// What sort of message?  (Some "system" messages are sent by lobby leader)
	}
	"mm_lobby_member_join"
	{
		"steamid"	"string"	// steamID (64-bit value converted to string) of user who joined
	}
	"mm_lobby_member_leave"
	{
		"steamid"	"string"	// steamID (64-bit value converted to string) of user who joined
		"flags"	"long"	// Bitfield of EChatMemberStateChange flags describing who entered or left
	}
	"player_changeclass"
	{
		"userid"	"short"		// user ID who changed class
		"class"		"short"		// class that they changed to
	}

	"player_death"		// a game event, name may be 32 charaters long
	{
		// this extends the original player_death 
		"userid"	"short"   	// user ID who died
		"victim_entindex"	"long"
		"inflictor_entindex"	"long"	// ent index of inflictor (a sentry, for example)
		"attacker"	"short"	 	// user ID who killed
		"weapon"	"string" 	// weapon name killer used 
		"weaponid"	"short"		// ID of weapon killed used
		"damagebits1"	"long"	// bits of type of damage
		"damagebits2"	"long"	// bits of type of damage
		"customkill"	"short"	// type of custom kill
		"assister"	"short"		// user ID of assister
		"dominated"	"short"		// did killer dominate victim with this kill
		"assister_dominated" "short"	// did assister dominate victim with this kill
		"revenge"	"short"		// did killer get revenge on victim with this kill
		"assister_revenge" "short"	// did assister get revenge on victim with this kill
		"victim_index"	"short"	// entindex who died
		"attacker_index"	"short"		// entindex that killed
		"attacker_name"		"string"	// classname of attacker
		"attacker_team"		"byte"		// team of attacker
		"assister_index"	"short"		// entindex of assister
		"assister_name"		"string"	// classname of assister
		"assister_team"		"byte"		// team of assister
		"weapon_logclassname"	"string" 	// weapon name that should be printed on the log
		"weapon_index"	"short"	// Entindex of weapon
		"stun_flags"	"short"	// victim's stun flags at the moment of death
		"death_flags"	"short" //death flags.
		"silent_kill"	"bool"
		"playerpenetratecount"	"short"
		"assister_fallback"	"string"	// contains a string to use if "assister" is -1
		"kill_streak_total" 	"short"	// Kill streak count (level)
		"kill_streak_wep" 	"short"	// Kill streak for killing weapon
		"kill_streak_assist" "short"	// Kill streak for assister count
		"kill_streak_victim" "short"	// Victims kill streak
                "ducks_streaked"	"short" // Duck streak increment from this kill
		"duck_streak_total"	"short" // Duck streak count for attacker
		"duck_streak_assist"	"short" // Duck streak count for assister
		"duck_streak_victim"	"short" // (former) duck streak count for victim
		"rocket_jump"		"bool"		// was the victim rocket jumping
		
		"victim_faction"		"byte"
		"attacker_faction"	"byte"
		"assister_faction"	"byte"
		
	//	"dominated"	"short"		// did killer dominate victim with this kill
	//	"assister_dominated" "short"	// did assister dominate victim with this kill
	//	"revenge"	"short"		// did killer get revenge on victim with this kill
	//	"assister_revenge" "short"	// did assister get revenge on victim with this kill
	//	"first_blood"	"bool"		// was this a first blood kill
	//	"feign_death"	"bool"	// the victim is feign death
	}

	"tf_map_time_remaining"
	{
		"seconds"	"long"
	}

	"tf_game_over"
	{
		"reason"	"string"	// why the game is over ( timelimit, winlimit )
	}
	"ctf_flag_captured"
	{
		"capping_team"			"short"
		"capping_team_score"	"short"
	}
	"controlpoint_initialized"
	{
	}
	"controlpoint_updateimages"
	{
		"index"		"short"		// index of the cap being updated
	}
	"controlpoint_updatelayout"
	{
		"index"		"short"		// index of the cap being updated
	}
	"controlpoint_updatecapping"
	{
		"index"		"short"		// index of the cap being updated
	}
	"controlpoint_updateowner"
	{
		"index"		"short"		// index of the cap being updated
	}
	"controlpoint_starttouch"
	{
		"player"	"short"		// entindex of the player
		"area"		"short"		// index of the control point area
	}

	"controlpoint_endtouch"
	{
		"player"	"short"		// entindex of the player
		"area"		"short"		// index of the control point area
	}
	
	"controlpoint_pulse_element"
	{
		"player"	"short"		// entindex of the player
	}

	"controlpoint_fake_capture"
	{
		"player"	"short"		// entindex of the player
		"int_data"	"short"
	}

	"controlpoint_fake_capture_mult"
	{
		"player"	"short"		// entindex of the player
		"int_data"	"short"
	}
	
	"teamplay_round_selected"
	{
		"round"		"string"	// name of the round selected
	}

	"teamplay_round_start"			// round restart
	{
		"full_reset"	"bool"		// is this a full reset of the map
	}
	
	"teamplay_round_active"			// called when round is active, players can move
	{
		// nothing for now
	}

	"teamplay_waiting_begins"
	{
		// nothing for now
	}
	
	"teamplay_waiting_ends"
	{
		// nothing for now
	}
	
	"teamplay_waiting_abouttoend"
	{
	}

	"teamplay_restart_round"
	{
		// nothing for now
	}

	"teamplay_ready_restart"
	{
		// nothing for now
	}

	"teamplay_round_restart_seconds"
	{
		"seconds"	"short"
	}

	"teamplay_team_ready"
	{
		"team"		"byte"		// which team is ready
	}

	"teamplay_round_win"
	{
		"team"		"byte"		// which team won the round
		"winreason"	"byte"		// the reason the team won
		"flagcaplimit"	"short"		// if win reason was flag cap limit, the value of the flag cap limit
		"full_round"	"short"		// was this a full round or a mini-round
		"round_time"	"float"		// elapsed time of this round
		"losing_team_num_caps"	"short"	// # of caps this round by losing team
		"was_sudden_death" "byte"	// did a team win this after entering sudden death
	}

	"teamplay_update_timer"
	{
	}

	"teamplay_round_stalemate"
	{
		"reason"	"byte"		// why the stalemate is occuring
	}
	
	"teamplay_overtime_begin"
	{
		// nothing for now
	}	
	
	"teamplay_overtime_end"
	{
		// nothing for now
	}		
	
	"teamplay_suddendeath_begin"
	{
		// nothing for now
	}
	
	"teamplay_suddendeath_end"
	{
		// nothing for now
	}	
	
	"teamplay_game_over"
	{
		"reason"	"string"	// why the game is over ( timelimit, winlimit )
	}

	"teamplay_map_time_remaining"
	{
		"seconds"	"short"
	}

	"teamplay_broadcast_audio"
	{
		"team"				"byte"		// which team should hear the broadcast. 0 will make everyone hear it.
		"sound"				"string"	// sound to play
		"additional_flags"	"short"		// additional sound flags to pass through to sound system
	}

	"teamplay_timer_flash"
	{
		"time_remaining"	"short"	// how many seconds until the round ends
	}	

	"teamplay_timer_time_added"
	{
		"timer"	"short"		// entindex of the timer	
		"seconds_added"	"short"		// how many seconds were added to the round timer	
	}

	"teamplay_point_startcapture"
	{
		"cp"		"byte"			// index of the point being captured
		"cpname"	"string"		// name of the point
		"team"		"byte"			// which team currently owns the point
		"capteam"	"byte"			// which team is capping
		"cappers"	"string"		// string where each character is a player index of someone capping
		"captime"	"float"			// time between when this cap started and when the point last changed hands
	}

	"teamplay_point_captured"
	{
		"cp"		"byte"			// index of the point that was captured
		"cpname"	"string"		// name of the point
		"team"		"byte"			// which team capped
		"cappers"	"string"		// string where each character is a player index of someone that capped
	}

	"teamplay_point_locked"
	{
		"cp"		"byte"			// index of the point being captured
		"cpname"	"string"		// name of the point
		"team"		"byte"			// which team currently owns the point
	}

	"teamplay_point_unlocked"
	{
		"cp"		"byte"			// index of the point being captured
		"cpname"	"string"		// name of the point
		"team"		"byte"			// which team currently owns the point
	}
	
	"teamplay_capture_broken"
	{
		"cp"		"byte"
		"cpname"	"string"
		"time_remaining" "float"
	}

	"teamplay_capture_blocked"
	{
		"cp"		"byte"			// index of the point that was blocked
		"cpname"	"string"		// name of the point
		"blocker"	"byte"			// index of the player that blocked the cap
	}
	"teamplay_flag_event"
	{
		"player"	"short"			// player this event involves
		"carrier"	"short"			// the carrier if needed
		"eventtype"	"short"			// pick up, capture, defend, dropped
		"home"		"byte"			// whether or not the flag was home (only set for TF_FLAGEVENT_PICKUP) 
		"team"		"byte"			// which team the flag belongs to
	}
	"teamplay_win_panel"		
	{
		"panel_style"		"byte"		// for client to determine layout		
		"winning_team"		"byte"		
		"winreason"		"byte"		// the reason the team won
		"cappers"		"string"	// string where each character is a player index of someone that capped
		"flagcaplimit"		"short"		// if win reason was flag cap limit, the value of the flag cap limit
		"blue_score"		"short"		// red team score
		"red_score"		"short"		// blue team score
		"blue_score_prev"	"short"		// previous red team score
		"red_score_prev"	"short"		// previous blue team score
		"round_complete"	"short"		// is this a complete round, or the end of a mini-round
		"rounds_remaining"	"short"		// # of rounds remaining for wining team, if mini-round
		"player_1"		"short"
		"player_1_points"	"short"
		"player_1_kills"	"short"
		"player_1_deaths"	"short"
		"player_2"		"short"
		"player_2_points"	"short"
		"player_2_kills"	"short"
		"player_2_deaths"	"short"
		"player_3"		"short"
		"player_3_points"	"short"
		"player_3_kills"	"short"
		"player_3_deaths"	"short"
		"killstreak_player_1"		"short"
		"killstreak_player_1_count"	"short"
	}
	"teamplay_teambalanced_player"
	{
		"player"	"short"		// entindex of the player
		"team"		"byte"		// which team the player is being moved to
	}

	"teamplay_setup_finished"
	{
	}
	"teamplay_alert"
	{
		"alert_type"		"short"		// which alert type is this (scramble, etc)?
	}

	"show_freezepanel"
	{
		"killer"	"short"		// entindex of the killer entity
	}

	"hide_freezepanel"
	{
	}

	"freezecam_started"
	{
	}

	"localplayer_changeteam"
	{
	}

	"localplayer_score_changed"
	{
		"score"		"short"
	}
	
	"localplayer_respawn"
	{
	}

	"flagstatus_update"
	{
		"userid"	"short"		// user ID of the player who now has the flag
		"entindex"	"long"	// ent index of flag
	}

	"player_stats_updated"
	{
		"forceupload"	"bool"
	}
	"playing_commentary"
	{
	}

	"npc_death"
	{ 
		"victim_index"	"short"	// entindex who died
		"victim_name"	"string"// classname of victim
		"victim_team"	"byte"	// team of victim
		"attacker_index"	"short"		// entindex that killed
		"attacker_name"		"string"	// classname of attacker
		"attacker_team"		"byte"		// team of attacker
		"weapon"	"string" 	// weapon name killer used
		"weapon_index"	"short"	// Entindex of weapon
		"weapon_logclassname"	"string" 	// weapon name that should be printed on the log
		"damagebits1"	"long"	// bits of type of damage
		"damagebits2"	"long"	// bits of type of damage
		"customkill"	"short"	// type of custom kill
		"assister_index"	"short"		// entindex of assister
		"assister_name"		"string"	// classname of assister
		"assister_team"		"byte"		// team of assister
		"show_notice"		"bool"	// show in deathnotice
		
		"victim_faction"		"byte"
		"attacker_faction"	"byte"
		"assister_faction"	"byte"
	}

	"achievement_earned"
	{
		"player"	"byte"		// entindex of the player
		"achievement"	"short"		// achievement ID
	}
	
	"spec_target_updated"
	{
	}

	"localplayer_becameobserver"
	{
	}
	
	"escort_speed"
	{
		"team"		"byte"			// which team
		"speed"		"byte"
		"players"	"byte"
	}
	
	"escort_progress"
	{
		"team"		"byte"			// which team
		"progress"	"float"
		"reset"		"bool"
	}

	"escort_recede"
	{
		"team"			"byte"		// which team
		"recedetime"	"float"
	}

	"gameui_activated"
	{
	}
	
	"gameui_hidden"
	{
	}
	
	"player_escort_score"
	{
		"player"	"byte"
		"points"	"byte"
	}

	"player_damaged"
	{
		"amount"		"short"
		"type"			"long"
	}
	
	"player_hurt"
	{
		"userid" "short"
		"health" "short"
		"attacker" "short"
		"damageamount" "short"
		"custom"	"short"
		"showdisguisedcrit" "bool"	// if our attribute specifically crits disguised enemies we need to show it on the client
		"crit" "bool"
		"minicrit" "bool"
		"allseecrit" "bool"
		"weaponid" "short"
		"victim_index"	"short"	// entindex who died
		"attacker_index"	"short"		// entindex that killed		
		"bonuseffect" "byte"
	}
	
	"npc_hurt"
	{
		"victim_index" "short"
		"health" "short"
		"attacker_index" "short"
		"damageamount" "short"
		"custom"	"short"
		"crit" "bool"
		"minicrit" "bool"
        "weaponid" "short"
	}
	
	"player_spawn"
	{
		"userid"	"short"		// user ID who spawned
		"team"		"short"		// team they spawned on
		"class"		"short"		// class they spawned as
	}	
	
	"controlpoint_unlock_updated"
	{
		"index"	"short"		// index of the cap being updated
		"time"	"float"		// time
	}
	
	"overtime_nag"
	{
	}
	
	"teams_changed"
	{
	}

	"localplayer_healed"
	{
		"amount"	"short"
	}
	
	"nav_blocked"
	{
		"area"		"long"
		"blocked"	"bool"
	}
	
	"path_track_passed"
	{
		"index"	"short"		// index of the node being passed
	}
	
	"num_cappers_changed"
	{
		"index"		"short"		// index of the trigger
		"count"		"byte"		// number of cappers (-1 for blocked)
	}
	
	"achievement_earned_local"
	{
		"achievement"	"short"
	}

	"player_healed"
	{
		"patient"	"short"
		"healer"	"short"
		"amount"	"short"
	}

	"npc_healed"
	{
		"patient"	"short"
		"healer"	"short"
		"amount"	"short"
	}
	
	"item_pickup"
	{
		"userid"	"short"
		"item"		"string"
	}
	
	"controlpoint_timer_updated"
	{
		"index"	"short"		// index of the cap being updated
		"time"	"float"		// time
	}
	
	"player_killed_achievement_zone"
	{
		"attacker"	"short"		// entindex of the attacker
		"victim"	"short"		// entindex of the victim
		"zone_id"	"short"		// type of area (0 for general, 1 for capture zone)
	}

	"teamplay_pre_round_time_left"
	{
		"time"		"short"
	}

	"player_initial_spawn"
	{
		"index"	"short"		// entindex of the player
	}


	// put new events here
	"antlionguard_spawned"
	{
	}

	"antlionguard_killed"
	{
	}

	"helicopter_spawned"
	{
	}

	"helicopter_destoryed"
	{
	}

	"gunship_spawned"
	{
	}

	"gunship_destoryed"
	{
	}

	"strider_spawned"
	{
	}

	"strider_killed"
	{
	}
	
	"portal_player_touchedground"	// player landed
	{
		"userid"	"short"		// user ID on server
	}
	
	"portal_player_portaled"		// player traveled through a portal
	{
		"userid"		"short"		// user ID on server
		"pairid"		"byte"		// portal linkage group ID
		"portal2"	"bool"		// false for portal1 (blue)
	}
	
	"portal_enabled"
	{
		"userid" "short"
		"leftportal" "bool"
	}
	
	"portal_fired"
	{
		"userid" "short"
		"leftportal" "bool"
	}
	
	"player_use"
	{
		"userid"		"short" // The player that did the using
		"entity"		"short" // The entity that they used
	}
	
	"turret_hit_turret"
	{
	}
	
	"security_camera_detached"
	{
	}
	"challenge_map_complete"
	{
		"numbronze"	"short"
		"numsilver"	"short"
		"numgold"	"short"
	}
	"advanced_map_complete"
	{
		"numadvanced"	"short"
	}
	"dinosaur_signal_found"
	{
		"id"	"short"
	}
}