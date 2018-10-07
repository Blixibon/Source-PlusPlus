// Created by Crowbar 0.56

$ModelName "combine_helicopter/helicopter_bomb02.mdl"

$StaticProp

$BodyGroup "Body"
{
	studio "helicopter_bomb01_Reference.smd"
}

$Include "helicopter_bomb01_lod.qci"

$SurfaceProp "metal"

$Contents "solid"

$IllumPosition 0 0.086 0

$NoForcedFade

$CDMaterials "models\combine_helicopter\ep2\"

$TextureGroup "skinfamilies"
{
	{ "helicopter_bomb01"     }
	{ "helicopter_bomb_off01" }
}

$Attachment "bottom" "static_prop" 0 0 -16.4 rotate 90 90 0
$Attachment "top" "static_prop" 0 0 16.4 rotate -90 90 0

$Include "helicopter_bomb01_box.qci"

$Include "helicopter_bomb01_bone.qci"

$Include "helicopter_bomb01_animation.qci"

$Include "helicopter_bomb01_collision.qci"

$KeyValues
{
	physgun_interactions 
	{
		"preferred_carryangles"		"-12 98 55"
		"allow_overhead"		"yes"
	}
}
