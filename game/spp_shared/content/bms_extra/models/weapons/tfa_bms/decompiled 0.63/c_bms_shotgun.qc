// Created by Crowbar 0.63

$ModelName "weapons\tfa_bms\c_bms_shotgun.mdl"

$BodyGroup "shotgun"
{
	studio "c_bms_shotgun_reference.smd"
}

$Include "c_bms_shotgun_lod.qci"

$SurfaceProp "default"

$Contents "solid"

$IllumPosition -3.395 -18.825 -9.144

$CDMaterials "bms\models\Weapons\V_hand\"
$CDMaterials "bms\models\weapons\v_shotgun\"
$CDMaterials "bms\models\weapons\shells\"

$Attachment "muzzle" "ValveBiped.Gun" 0 0 0 rotate 0 0 0
$Attachment "ejectbrass" "ShellEject" 0 0 0 rotate 0 0 90

$Include "c_bms_shotgun_box.qci"

$Include "c_bms_shotgun_bone.qci"

$Include "c_bms_shotgun_animation.qci"
