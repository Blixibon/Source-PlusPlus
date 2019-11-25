// Created by Crowbar 0.63

$ModelName "buggy.mdl"

$BodyGroup "Body"
{
	studio "buggy_reference.smd"
}
$BodyGroup "tau_cannon"
{
	blank
	studio "buggy_tau_reference.smd"
}

$Include "buggy_lod.qci"

$SurfaceProp "metal"

$Contents "solid"

$IllumPosition 0.775 0 51.547

$MaxEyeDeflection 90

$NoForcedFade

$CDMaterials "models\buggy\"

$Attachment "levitate_target" "Rig_Buggy.Gun_Parent" 0 0 0 rotate 0 0 0
$Attachment "gun_ref" "Rig_Buggy.Gun" 0 0 0 rotate 0 0 0
$Attachment "wheel_fl" "Rig_Buggy.Wheel_FL_Rotate" 0 0 0 rotate 0 0 0
$Attachment "wheel_fr" "Rig_Buggy.Wheel_FR_Rotate" 0 0 0 rotate 0 0 0
$Attachment "wheel_rl" "Rig_Buggy.Wheel_RL" 0 0 0 rotate 0 0 0
$Attachment "wheel_rr" "Rig_Buggy.Wheel_RR" 0 0 0 rotate 0 0 0
$Attachment "vehicle_driver_eyes" "Rig_Buggy.view" 5 0 0 rotate 0 0 0
$Attachment "vehicle_feet_passenger0" "Rig_Buggy.view" 0 0 -32 rotate 0 0 0
$Attachment "vehicle_engine" "Rig_Buggy.Frame" 0 0 -40 rotate 0 0 0
$Attachment "exit1" "Rig_Buggy.Frame" 69.43 27.95 28.02 rotate -90 -90 0
$Attachment "exit2" "Rig_Buggy.Frame" -69.43 27.95 28.02 rotate -90 -90 0
$Attachment "exit3" "Rig_Buggy.Frame" 5.61 70.95 52.93 rotate -90 -90 0
$Attachment "exit4" "Rig_Buggy.Frame" 0 27.95 -110.09 rotate -90 -90 0
$Attachment "exit5" "Rig_Buggy.Frame" 0 -74.18 -11.98 rotate -67.83 90 0
$Attachment "exit6" "Rig_Buggy.Frame" 0 -74.18 -11.98 rotate -67.83 90 0
$Attachment "exit7" "Rig_Buggy.Frame" 70.21 -14.14 -1.04 rotate -51.46 -165.29 -104.71
$Attachment "exit8" "Rig_Buggy.Frame" -70.21 -14.14 3.99 rotate -49.87 -14.71 104.71
$Attachment "enter1" "Rig_Buggy.Frame" 69.43 19.95 -1.98 rotate 0 180 90
$Attachment "enter2" "Rig_Buggy.Frame" -69.43 19.95 -1.98 rotate 0 0 -90
$Attachment "enter3" "Rig_Buggy.Frame" 5.61 62.95 81.37 rotate 55.44 -90 180
$Attachment "enter4" "Rig_Buggy.Frame" 0 19.95 -110.09 rotate -90 -90 0
$Attachment "beam_damage" "Rig_Buggy.Frame" 10 7 29 rotate 0 0 0
$Attachment "seagull_perch1" "Rig_Buggy.Frame" -18 20 53 rotate 0 180 90
$Attachment "seagull_perch2" "Rig_Buggy.Frame" 10 5 88 rotate -90 -90 0
$Attachment "seagull_perch3" "Rig_Buggy.Frame" -8 50 -46 rotate 90 90 0
$Attachment "taucannon" "Rig_Buggy.Frame" -18 10 53 rotate 90 90 0
$Attachment "headlight" "Rig_Buggy.Frame" 0 5 100 rotate -90 -90 0
$Attachment "Muzzle" "Rig_Buggy.Gun" 12.63 0 0 rotate 0 0 0
$Attachment "Steering_LHand" "Rig_Buggy.Steer_Wheel" 9.00 3.00 2.00 rotate -5 -90 20
$Attachment "Steering_RHand" "Rig_Buggy.Steer_Wheel" -9.00 3.00 2.00 rotate -5 -90 160

$Include "buggy_box.qci"

$Include "buggy_bone.qci"

$Include "buggy_animation.qci"

$Include "buggy_collision.qci"

$KeyValues
{
	vehicle_entry 
	{
		"enter1" "1"
		"enter2" "2"
		"enter3" "3"
	}
	vehicle_exit 
	{
		"exit1" "upright"
		"exit2" "upright"
		"exit7" "upsidedown"
		"exit8" "upsidedown"
	}
	vehicle_escape_exit 
	{
		"exit3" "upright"
		"exit4" "upright"
		"exit5" "upsidedown"
		"exit6" "upsidedown"
	}
}
