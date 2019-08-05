// Created by Crowbar 0.63

$ModelName "weapons/tfa_bms/w_357_bms.mdl"

$BodyGroup "Body"
{
	studio "w_357_bms_reference.smd"
}

$Include "w_357_bms_lod.qci"

$SurfaceProp "weapon"

$Contents "solid"

$IllumPosition 1.428 -0.063 0.307

$CDMaterials "bms\models\world_models\"

$Attachment "muzzle" "muzzle" 0 0 0 rotate 0 0 0

$Include "w_357_bms_box.qci"

$Include "w_357_bms_bone.qci"

$Include "w_357_bms_animation.qci"

$Include "w_357_bms_collision.qci"


$KeyValues
{
	prop_data 
	{
		"base" "metal.small"
	}
}
