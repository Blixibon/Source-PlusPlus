// Created by Crowbar 0.64

$ModelName "Vehicle.mdl"

$BodyGroup "Body"
{
	studio "Vehicle_reference.smd"
}
$BodyGroup "radar"
{
	blank
	studio "radar.smd"
}
$BodyGroup "magnusson_holder"
{
	blank
	studio "magnusson_holder.smd"
}
$BodyGroup "car_bar"
{
	blank
	studio "car_bar.smd"
}

$Include "vehicle_lod.qci"

$SurfaceProp "jalopy"

$Contents "solid"

$IllumPosition -22.801 0 75.047

$MaxEyeDeflection 90

$NoForcedFade

$CDMaterials "models\vehicle\"

$Attachment "vehicle_driver_eyes" "Rig_Buggy.view" 0 0 1 rotate 0 0 0
$Attachment "enter1" "Rig_Buggy.Frame" 69.43 19.95 -1.98 rotate 0 -180 90
$Attachment "enter2" "Rig_Buggy.Frame" -69.43 19.95 -1.98 rotate 0 0 -90
$Attachment "enter3" "Rig_Buggy.Frame" 5.61 62.95 81.37 rotate 55.44 -90 180
$Attachment "enter4" "Rig_Buggy.Frame" 0 100.95 -9.51 rotate -90 -90 0
$Attachment "vehicle_feet_passenger1" "Rig_Buggy.Frame" -19.68 -34.05 14.33 rotate -90 -90 0
$Attachment "vehicle_feet_passenger2" "Rig_Buggy.Frame" -14.46 -23.28 9.39 rotate -90 -90 0
$Attachment "passenger_zombie1" "Rig_Buggy.Frame" 13.4 6.31 57.31 rotate 90 90 0
$Attachment "vehicle_feet_passenger0" "Rig_Buggy.view" 0 0 -32 rotate 0 0 0
$Attachment "vehicle_engine" "Rig_Buggy.Frame" 0 6 100 rotate 0 0 0
$Attachment "controlpanel0_ll" "Rig_Buggy.Frame" 2.2 3.69 39.45 rotate 0 -180 169
$Attachment "controlpanel0_ur" "Rig_Buggy.Frame" -3.55 9.13 40.45 rotate 0 -180 169
$Attachment "wheel_fr" "Rig_Buggy.Wheel_FR_Rotate" 0 -1 -1 rotate 0 0 0
$Attachment "wheel_fl" "Rig_Buggy.Wheel_FL_Rotate" 0 -1 -1 rotate 0 0 0
$Attachment "wheel_rr" "Rig_Buggy.Wheel_RR" 5 0 0 rotate 0 0 0
$Attachment "wheel_rl" "Rig_Buggy.Wheel_RL" 5 0 0 rotate 0 0 0
//$Attachment "exhaust" "Rig_Buggy.Frame" -26 35 34 rotate -90 -90 0
$Attachment "cargo" "Rig_Buggy.Frame" 0 9.5 -119 rotate 90 0 0
$Attachment "headlight_r" "Rig_Buggy.Frame" -36 -5.5 112 rotate -90 0 0
$Attachment "headlight_l" "Rig_Buggy.Frame" 36 -5.5 112 rotate -90 0 0
$Attachment "rearlight_r" "Rig_Buggy.Frame" -34 18.5 -102 rotate 90 0 0
$Attachment "rearlight_l" "Rig_Buggy.Frame" 34 18.5 -102 rotate 90 0 0
$Attachment "headlight" "Rig_Buggy.Frame" -40 4 102 rotate -82 -90 0
$Attachment "Steering_LHand" "Rig_Buggy.Steer_Wheel" 9.00 3.00 2.00 rotate -5 -90 20
$Attachment "Steering_RHand" "Rig_Buggy.Steer_Wheel" -9.00 3.00 2.00 rotate -5 -90 160

$Include "vehicle_box.qci"

$Include "vehicle_bone.qci"

$Include "vehicle_animation.qci"

$Include "vehicle_collision.qci"

$KeyValues
{
	vehicle_entry 
	{
		"enter1" "0"
		"enter2" "1"
		"enter3" "2"
		"enter4" "3"
	}
	vehicle_exit 
	{
		"exit1" "upright"
		"exit2" "upright"
	}
	vehicle_escape_exit 
	{
		"exit3" "upright"
		"exit4" "upright"
		"exit5" "upright"
	}
	vehicle_npc_passengers 
	{
		"passenger" 
		{
			"target_attachment" "vehicle_feet_passenger1"
			 "entry" 
			{
				"priority" "1"
				"animation" "buggy_enter1"
			}
			"exit" 
			{
				"priority" "1"
				"animation" "buggy_exit1"
			}
			"exit" 
			{
				"priority" "2"
				"animation" "buggy_exit3"
			}
			"entry" 
			{
				"priority" "2"
				"animation" "buggy_enter2"
			}
			"exit" 
			{
				"priority" "2"
				"animation" "buggy_exit2"
			}
		}
		"passenger_zombie" 
		{
			"target_attachment" "passenger_zombie1"
			 "entry" 
			{
				"priority" "1"
				"animation" "buggy_fast_dynamic01"
			}
			"exit" 
			{
				"priority" "1"
				"animation" "buggy_dismount"
			}
			"metadata" 
			{
				"side" "front"
			}
		}
	}
}
