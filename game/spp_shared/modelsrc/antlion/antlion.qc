// Created by Crowbar 0.58

$ModelName "Antlion.mdl"

$BodyGroup "studio"
{
	studio "AntLion_reference.smd"
}
$BodyGroup "wings"
{
	blank
	studio "antlion_wings_reference.smd"
}

$Include "antlion_lod.qci"

$SurfaceProp "antlion"

$Contents "solid"

$IllumPosition -0.14 -4.591 31.629

$EyePosition 0 0 24

$MostlyOpaque

$CDMaterials "models\antlion\"

$TextureGroup "skinfamilies"
{
	{ "antlionhigh_sheet"  }
	{ "antlionhigh_sheet2" }
	{ "antlionhigh_sheet3" }
	{ "antlionhigh_sheet4" }
}

$Attachment "leftfront" "Antlion.LegR3_Bone" 46 0 0 rotate 0 0 0
$Attachment "rightfront" "Antlion.LegL3_Bone" 46 0 0 rotate 0 0 0
$Attachment "leftrear" "Antlion.LegMidL3_Bone" 30 0 0 rotate 0 0 0
$Attachment "rightrear" "Antlion.LegMidR3_Bone" 30 0 0 rotate 0 0 0

$Include "antlion_box.qci"

$Include "antlion_bone.qci"

$Include "antlion_animation.qci"

$Include "antlion_collision.qci"

$Include "antlion_di.qci"
