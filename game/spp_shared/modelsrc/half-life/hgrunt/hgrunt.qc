// Created by Crowbar 0.58

$ModelName "half-life/hgrunt.mdl"

$BodyGroup "studio"
{
	studio "hgrunt_soldier_torso_ref.smd"
}
$BodyGroup "heads"
{
	studio "hgrunt_soldier_head_gasmask_ref.smd"
	studio "hgrunt_soldier_head_commander_ref.smd"
	studio "hgrunt_soldier_head_shotgun_ref.smd"
	//studio "hgrunt_soldier_head_black_ref.smd"
	studio "hgrunt_soldier_head_rocket_ref.smd"
}
	//
$BodyGroup "weapons"
{
	studio "hgrunt_weapon_m16_pack.smd"
	studio "hgrunt_weapon_spas12_pack.smd"
	studio "hgrunt_weapon_unarmed_pack.smd"
	//studio "hgrunt_weapon_rpg_pack"
	//studio "hgrunt_weapon_mp5_pack"
	//studio "hgrunt_weapon_m40a1_pack"
}

$SurfaceProp "flesh"

$Contents "solid"

$EyePosition 0 0 73

$CDMaterials "models\half-life\hgrunt\"

$MostlyOpaque

$TextureGroup "skinfamilies"
{
	{ "Grunt_Arm_WHT" "Grunt_Hand_WHT" "Grunt_Gasmask_WHT" "Grunt_Head_CO_WHT" "Grunt_Head_SGun_WHT" "Grunt_Head_OP4_CO_WHT" }
	{ "Grunt_Arm_BLK" "Grunt_Hand_BLK" "Grunt_Gasmask_BLK" "Grunt_Head_CO_BLK" "Grunt_Head_SGun_BLK" "Grunt_Head_OP4_CO_BLK" }
}

$Attachment "muzzle" "Bip01 R Hand" 38 2.5 8.5 rotate 0 0 0
$Attachment "r_hand" "Bip01 R Hand" 0 0 0 rotate 0 0 0
$attachment "eyes" "Bip01 Head" 7.00 6.50 -0.00 rotate -0 90 90
$Attachment "mouth" "Bip01 Head" 3.3 8 -1 rotate -0 90 90

$Include "hgrunt_box.qci"
$Include "hgrunt_collision.qci"
$Include "hgrunt_animation.qci"
