// Created by Crowbar 0.56

$ModelName "half-life\scientist.mdl"

$BodyGroup "body"
{
	studio "scientist_body_male_ref.smd"
}
$BodyGroup "heads"
{
	studio "scientist_head_Walter_ref.smd"
	studio "scientist_head_Einstein_ref.smd"
	studio "scientist_head_Luther_ref.smd"
	studio "scientist_head_Slick_ref.smd"
}
$BodyGroup "needle"
{
	blank
	studio "scientist_needle_ref.smd"
}

$Include "scientist_lod.qci"

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition -1.289 -1.075 35.211

$EyePosition 0 0 60

$CDMaterials "models\half-life\scientist\"

$TextureGroup "skinfamilies"
{
	{ "scientist_hands_wht" }
	{ "scientist_hands_blk" }
}

$MostlyOpaque

// This list shows the VMT file names used in the SMD files.
// "scientist_hands_wht.vmt"
// "scientist_coat.vmt"
// "scientist_tie.vmt"
// "scientist_shirt.vmt"
// "scientist_trousers.vmt"
// "scientist_head_Walter.vmt"
// "scientist_glasses_lens_Chrome.vmt"
// "scientist_glasses.vmt"
// "scientist_mouth.vmt"
// "scientist_ID_Walter.vmt"
// "scientist_head_Einstein.vmt"
// "scientist_ID_Einstein.vmt"
// "scientist_head_Luther.vmt"
// "scientist_ID_Luther.vmt"
// "scientist_head_Slick.vmt"
// "scientist_ID_Slick.vmt"
// "scientist_needle.vmt"
// "scientist_hands_blk.vmt"

$Attachment "eyes" "Bip01 Head" 5.05 1.82 0.21 rotate 0 80 90
$Attachment "forward" "Bip01 Spine1" 0 0 0 rotate 0 90 90

$Include "scientist_box.qci"

$Include "scientist_bone.qci"

$Include "scientist_animation.qci"

$Include "scientist_collision.qci"
