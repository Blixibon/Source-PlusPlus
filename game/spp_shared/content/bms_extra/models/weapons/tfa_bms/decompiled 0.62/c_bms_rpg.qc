// Created by Crowbar 0.62

$ModelName "weapons\tfa_bms\c_bms_rpg.mdl"

$BodyGroup "body"
{
	studio "c_bms_rpg_body0_model0.smd"
}
$BodyGroup "rocket"
{
	blank
	studio "c_bms_rpg_body1_model1.smd"
}

$Include "c_bms_rpg_lod.qci"

$SurfaceProp "default"

$Contents "solid"

$IllumPosition -0.222 -9.65 -5.808

$CDMaterials "bms\models\weapons\v_rpg\"
$CDMaterials "bms\models\Weapons\V_hand\"

$Attachment "muzzle" "V_RPG" 0 -10 0 rotate 0 -90 0
$attachment "controlpanel0_ll" "Screen" 4.10 0.60 -4.00 rotate -14 92 0
$attachment "laser" "V_RPG" 3.00 -10.00 -1.60 rotate -0 -92 0
$attachment "laser_end" "V_RPG" 3.00 -110.00 -1.60 rotate -0 -92 0
$attachment "controlpanel0_ur" "Screen" 1.80 0.50 -2.20 rotate -14 92 0

$Include "c_bms_rpg_box.qci"

$Include "c_bms_rpg_bone.qci"

$Include "c_bms_rpg_animation.qci"
