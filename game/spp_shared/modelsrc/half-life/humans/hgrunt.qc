// Created by Crowbar 0.56

$ModelName "half-life/hgrunt.mdl"

$BodyGroup "body"
{
	studio "hgrunt_hl_body.smd"
}
$BodyGroup "heads"
{
	studio "hgrunt_hl__mask.smd"
	studio "hgrunt_hl__comm.smd"
	studio "hgrunt_hl__soldier.smd"
	studio "hgrunt_hl__black.smd"
}
$BodyGroup "weapons"
{
	studio "hgrunt_hl_m16.smd"
	studio "hgrunt_hl_spas.smd"
	studio "hgrunt_SAW_reference.smd"
	studio "hgrunt_p_desert_eagle.smd"
	studio "hgrunt_p_beretta.smd"
	blank
}

$Include "hgrunt_lod.qci"

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition -3.441 10.717 36.65

$EyePosition 0 0 73

$CDMaterials "models\half-life\hgrunt\"

$TextureGroup "skinfamilies"
{
	{ "PC_soldier_skin_bronze" "PC_soldier_mask_face"     "PC_commander_face"     }
	{ "PC_soldier_skin_blk"    "PC_soldier_mask_face_blk" "PC_commander_face_blk" }
}

$Attachment "muzzle_m4" "Bip01 R Hand" 35 1.3 7.5 rotate 0 0 0
$Attachment "muzzle_shotgun" "Bip01 R Hand" 34.6 1.4 6.8 rotate 0 0 0
$Attachment "muzzle_saw" "Bip01 R Hand" 27.8 2.3 4.3 rotate 0 0 0
$Attachment "muzzle_pistol_9mm" "Bip01 R Hand" 11 1.35 4.6 rotate 0 0 0
$Attachment "muzzle_pistol_deagle" "Bip01 R Hand" 12 1.75 4.26 rotate 0 0 0
$Attachment "l_hand" "Bip01 L Hand" 4 1 -3.4 rotate 0 0 0
$Attachment "r_hand" "Bip01 R Hand" 0 0 0 rotate 0 0 0
$Attachment "possession_cam" "Bip01 Head" 0 0 0 rotate 0 0 0

// $Include "hgrunt_box.qci"

$Include "hgrunt_bone.qci"

$Include "hgrunt_animation.qci"

$Include "hgrunt_collision.qci"
