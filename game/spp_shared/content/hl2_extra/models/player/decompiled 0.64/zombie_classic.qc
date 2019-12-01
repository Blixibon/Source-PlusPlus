// Created by Crowbar 0.64

$ModelName "player/zombie_classic.mdl"

$BodyGroup "studio"
{
	studio "Zombie_reference.smd"
}
$BodyGroup "headcrab1"
{
	blank
	studio "Headcrab_reference.smd"
}
$BodyGroup "studio"
{
	studio "zombie_cop_vest_reference.smd"
}
$BodyGroup "studio"
{
	studio "zombie_rebel_reference.smd"
}
$BodyGroup "studio"
{
	studio "zombie_medic_reference.smd"
}
$BodyGroup "studio"
{
	studio "zombie_cop_reference.smd"
}

$Include "classic_new_lod.qci"

$SurfaceProp "zombieflesh"

$Contents "solid"

$IllumPosition -2.887 -4.81 33.483

$EyePosition 0 0 64

$AmbientBoost

$MostlyOpaque

$CDMaterials "models\zombie_classic_new\"
$CDMaterials "models\headcrab_classic\"
$CDMaterials "models\police\"

$TextureGroup "skinfamilies"
{
	{ "Zombie_Classic_sheet"            "blank"                 "blankc"             "blankd"             "blankb"                "blanke"               }
	{ "zombie_classic_sheet_citizen"    "blank"                 "blankc"             "blankd"             "blankb"                "blanke"               }
	{ "zombie_classic_sheet_rebelgreen" "metrocop_sheet_zombie" "rebel_sheet_zombie" "blankd"             "blankb"                "blanke"               }
	{ "zombie_classic_sheet_rebelblue"  "metrocop_sheet_zombie" "rebel_sheet_zombie" "blankd"             "blankb"                "blanke"               }
	{ "zombie_classic_sheet_medicgrey"  "metrocop_sheet_zombie" "blankc"             "rebel_sheet_zombie" "blankb"                "blanke"               }
	{ "zombie_classic_sheet_medicblue"  "metrocop_sheet_zombie" "blankc"             "rebel_sheet_zombie" "blankb"                "blanke"               }
	{ "zombie_classic_sheet_cop"        "metrocop_sheet_zombie" "blankc"             "blankd"             "metrocop_sheet_zombie" "combine_elite_zombie" }
}

$Attachment "headcrab" "ValveBiped.HC_Body_Bone" -4.49 -1.32 -0.02 rotate 0 20.25 29.25
$Attachment "eyes" "ValveBiped.HC_Body_Bone" 0 0 0 rotate 0 0 -50
$Attachment "head" "ValveBiped.HC_Body_Bone" 0 0 0 rotate 0 0 -50
$Attachment "chest" "ValveBiped.Bip01_Spine4" -3 6 0 rotate 0 60 90
$Attachment "maw" "ValveBiped.Bip01_Spine2" 0 8 0 rotate 0 90 90
$Attachment "Blood_Left" "ValveBiped.Bip01_L_Finger2" 0 0 0 rotate 0 0 0
$Attachment "Blood_Right" "ValveBiped.Bip01_R_Finger2" 0 0 0 rotate 0 0 0

$Include "zombie_classic_box.qci"

$Include "zombie_classic_bone.qci"

$Include "zombie_classic_animation.qci"

$Include "zombie_classic_collision.qci"
