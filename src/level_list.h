/*
    Copyright (C) 2005-2007 Tom Beaumont

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/


struct LevelInfo{
	int hintID;
	const char *file, *name;
} levelNames[] = {

/* TRANSLATORS: The following strings (in level_list.h) are level names, which
   should give (in a humerous way) the player an indication about this level. */
{0,		"map_maybe\\map.lev",						_("  Map"),},
{0,		"0_green\\asymmetrix.lev",					_("  Orbital"),},
{0,		"0_green\\hive.lev",						_("  Hive"),},
{0,		"0_green\\there and back.lev",				_("  There  and Back"),},
{0,		"0_green\\triangular.lev",					_("  Triangular"),},
{0,		"1_trampoline\\01.lev",						_("  Mini  Island"),},
{0,		"1_trampoline\\01_b.lev",					_("  Island  Variation"),},
{0,		"1_trampoline\\archipeligo.lev",			_("  Archipelago"),},
{0,		"1_trampoline\\arrow.lev",					_("  Weathervane"),},
{0,		"1_trampoline\\Bridges.lev",				_("  Bridges"),},
{0,		"1_trampoline\\explorer.lev",				_("  Explorer"),},
{0,		"1_trampoline\\test_trampoline.lev",		_("  Trampolines"),},
{0,		"2_greendoor\\Loopy.lev",					_("  Not a Knot"),},
{0,		"2_greendoor\\more mountain.lev",			_("  Another  Mountain"),},
{0,		"2_greendoor\\Mountain.lev",				_("  A  Mountain"),},
{0,		"2_greendoor\\test_green_gate.lev",			_("  Green  Walls"),},
{0,		"2_greendoor\\winding order.lev",			_("  Winding  Order"),},
{0,		"2005_11_15\\boating.lev",					_("  Rental  Boat"),},
{0,		"2005_11_15\\ferry.lev",					_("  Ferrying"),},
{0,		"2005_11_15\\HUB.LEV",						_("  Transport  Hub"),},
{0,		"2005_11_15\\learn lift.lev",				_("  Lifts"),},
{0,		"2005_11_15\\leftovers.lev",				_("  Leftovers"),},
{0,		"2005_11_15\\lumpy.lev",					_("  Trampoline  Retrieval"),},
{0,		"2005_11_15\\rolling hexagons.lev",			_("  Rolling  Hexagons"),},
{0,		"2005_11_15\\telephone.lev",				_("  Telephone"),},
{0,		"2005_11_16\\breakthrough.lev",				_("  Breakthrough"),},
{0,		"2005_11_19\\aa.lev",						_("  Laser  Safety"),},
{0,		"2005_11_19\\branches.lev",					_("  Branching  Pathway"),},
{0,		"2005_11_19\\one way up.lev",				_("  Only One  Way Up"),},
{0,		"2005_11_19\\outposts.lev",					_("  Outposts"),},
{0,		"2005_11_19\\turntables.lev",				_("  Roundabouts"),},
{0,		"2005_11_19\\two fish.lev",					_("  Two  Fish"),},
{0,		"3_2hitfloor\\all wound up.lev",			_("All  Wound  Up"),},
{0,		"3_2hitfloor\\collapse2.lev",				_("  Toughened  Tiles"),},
{0,		"3_2hitfloor\\Island.lev",					_("  Island"),},
{0,		"3_2hitfloor\\more stripes.lev",			_("  More  Stripes"),},
{0,		"3_2hitfloor\\Stripey.lev",					_("  Stripes"),},
{0,		"3_2hitfloor\\test_2hit_floor.lev",			_("  One Two  One Two"),},
{0,		"3_2hitfloor\\Turtle.lev",					_("  Turtle"),},
{0,		"3_2hitfloor\\Wand.lev",					_("  Wand"),},
{0,		"4_gun\\deathtrap.lev",						_("  Deathtrap"),},
{0,		"4_gun\\eagerness.lev",						_("  Eagerness"),},
{0,		"4_gun\\gun platform.lev",					_("  Gun  Platform"),},
{0,		"4_gun\\Nucleus.lev",						_("  Nucleus"),},
{0,		"4_gun\\Sniper.lev",						_("  Sniper"),},
{0,		"4_gun\\snowflake 2.lev",					_("  Deadly  Snowflake"),},
{0,		"4_gun\\snowflake.lev",						_("  Snowflake"),},
{0,		"4_gun\\Test_gun.lev",						_("  Laser  Tiles"),},
{0,		"4_gun\\trigger happy.lev",					_("  Trigger  Happy"),},
{0,		"5_spinner\\lure.lev",						_("  Lure"),},
{0,		"5_spinner\\Maxe.lev",						_("  Maze"),},
{0,		"5_spinner\\Motion.lev",					_("  Motion  Sickness"),},
{0,		"5_spinner\\preperation mk 3.lev",			_("  All About  Preparation"),},
{0,		"5_spinner\\revolver cannon.lev",			_("  Revolver  Cannon"),},
{0,		"5_spinner\\small cog.lev",					_("  Small  Cog"),},
{0,		"5_spinner\\Sprocket.lev",					_("  Sprocket"),},
{0,		"5_spinner\\switch.lev",					_("  Switch"),},
{0,		"5_spinner\\test_spinner.lev",				_("  Spinner  Tiles"),},
{0,		"5_spinner\\three more ways.lev",			_("  Three  More Ways"),},
{0,		"5_spinner\\three ways mk 2.lev",			_("  Three  Ways To Go"),},
{0,		"6_ice\\oo.lev",							_("Please  Skate  Safely"),},
{0,		"6_ice\\refraction.lev",					_("  Refraction"),},
{0,		"6_ice\\route finder.lev",					_("  Route  Finder"),},
{0,		"6_ice\\slippy.lev",						_("A  Slippery  Situation"),},
{0,		"7_item\\crooked.lev",						_("  Crooked"),},
{0,		"7_item\\green honey.lev",					_("  Green  Honey"),},
{0,		"7_item\\kx.lev",							_("  Carefully  Does It"),},
{0,		"7_item\\radioactive ice.lev",				_("  Radioactive  Ice"),},
{0,		"7_item\\slider.lev",						_("  Pro  Skater"),},
{0,		"7_item\\spinners mk2.lev",					_("  Spinners  II"),},
{0,		"7_item\\spinners.lev",						_("  Spinners"),},
/* TRANSLATORS: There is a special kind of pickup, which will turn ice
   plates into "normal" plates (you won't slip on them). So the name
   is (probably) related to "Make less slippery". */
{0,		"7_item\\test_ice.lev",						_("  Deslippify"),},
{0,		"7_item\\tt.lev",							_("  Tri Mesh"),},
{0,		"7_item\\Wheel.lev",						_("  Wheel"),},
{0,		"8_item2\\finishing strike.lev",			_("  Finishing  Strike"),},
{0,		"8_item2\\p2.lev",							_("  Big  Jumps"),},
{0,		"8_item2\\wave cannon.lev",					_("  Wave  Cannon"),},
{0,		"9_boat\\clearance.lev",					_("  Clearance"),},
{0,		"9_boat\\floating.lev",						_("  Floating"),},
{0,		"9_boat\\forced fire.lev",					_("  Forced  Fire"),},
{0,		"9_boat\\no swimming allowed.lev",			_("No  Swimming  Allowed"),},
{0,		"a.lev",									_("A Little  Light  Lifting"),},
{0,		"A_Lift\\house.lev",						_("  House"),},
{0,		"A_Lift\\hunting.lev",						_("  Hunting"),},
{0,		"A_Lift\\Lifting.lev",						_("  More  Lifting"),},
{0,		"A_Lift\\opportunist_mini.lev",				_("  Opportunist"),},
{0,		"A_Lift\\test_lift.lev",					_("  Demolition"),},
{0,		"A_Lift\\upper.lev",						_("  Upper"),},
{0,		"b.lev",									_("Beware  Feedback  Loops"),},
{0,		"B_Builder\\airlock ending.lev",			_("  Somewhat  Constructive"),},
{0,		"B_Builder\\overbuild.lev",					_("  Overbuild"),},
{0,		"B_Builder\\reversing space.lev",			_("  Reversing  Space"),},
{0,		"B_Builder\\test_builder.lev",				_("Burn  Your  Bridges"),},
{0,		"c.lev",									_("A  Strange  Place"),},
{0,		"commute.lev",								_("  Commute"),},
{0,		"d.lev",									_("  Bouncing  Required"),},
{0,		"de-icing.lev",								_("  Fetch  Quest"),},
{0,		"e.lev",									_("  Laser  Surgery"),},
{0,		"ice intro.lev",							_("  Icy  Tiles"),},
{0,		"icy road.lev",								_("  Icy  Road"),},
{0,		"invertor.lev",								_("  Inversion"),},

{0,		"_20",										_("Complete __ levels to unlock")},
{0,		"_35",										_("Complete __ levels to unlock")},
{0,		"_55",										_("Complete __ levels to unlock")},
{0,		"_75",										_("Complete __ levels to unlock")},
{0,		"_90",										_("Complete __ levels to unlock")},

};
