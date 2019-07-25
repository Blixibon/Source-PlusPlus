// Created by Crowbar 0.62

$ModelName "weapons/tfa_bms/w_rpg_bms.mdl"

$BodyGroup "w_rpg"
{
	studio "w_rpg.smd"
}

$Include "w_rpg_lod.qci"

$SurfaceProp "weapon"

$Contents "solid"

$IllumPosition -1.795 1.571 0.756

$CDMaterials "bms\models\world_models\"

$Attachment "muzzle" "rocket" 0 0 0 rotate 0 0 0
$attachment "missile" "rocket" 12.00 0.00 0.00 rotate 0 0 0
$attachment "backblast" "w_rpg" -24.00 0.00 0.00 rotate 0 -180 0
$attachment "laser" "w_rpg" 18.50 2.00 3.50 rotate 0 -0 0
$attachment "laser_end" "w_rpg" 52.00 2.00 3.50 rotate 0 -0 0

$Include "w_rpg_box.qci"

$Include "w_rpg_bone.qci"

$Include "w_rpg_animation.qci"

$Include "w_rpg_collision.qci"

$KeyValues
{
	prop_data 
	{
		"base" "metal.small"
	}
}
