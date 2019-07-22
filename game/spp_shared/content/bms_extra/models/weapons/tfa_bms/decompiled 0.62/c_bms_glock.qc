// Created by Crowbar 0.62

$ModelName "weapons\tfa_bms\c_bms_glock.mdl"

$BodyGroup "studio"
{
	studio "c_bms_glock_reference.smd"
}

$Include "c_bms_glock_lod.qci"

$SurfaceProp "default"

$Contents "solid"

$IllumPosition 1.12 -4.689 -3.292

$CDMaterials "bms\models\weapons\v_glock\"
$CDMaterials "bms\models\Weapons\V_hand\"

$Attachment "1" "bone.muzzle" 0 0 0 rotate 0 0 0
$Attachment "3" "bone.muzzle" 1 0 0 rotate 0 0 0
$Attachment "2" "ValveBiped.eject" 0 0 0 rotate 0 0 0

$Include "c_bms_glock_box.qci"

$Include "c_bms_glock_bone.qci"

$Include "c_bms_glock_animation.qci"
