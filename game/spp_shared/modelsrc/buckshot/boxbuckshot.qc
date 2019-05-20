// Created by Crowbar 0.58

$ModelName "items/boxbuckshot.mdl"

$StaticProp

$BodyGroup "boxbuckshot"
{
	studio "boxbuckshot_full.dmx"
	studio "boxbuckshot_most.dmx"
	studio "boxbuckshot_low.dmx"
}

$Include "boxbuckshot_lod.qci"

$SurfaceProp "item"

$Contents "solid"

$IllumPosition 0.37 -0.046 5.83

$Opaque

$CDMaterials "models\items\"

$Include "boxbuckshot_box.qci"

$Include "boxbuckshot_bone.qci"

$Include "boxbuckshot_animation.qci"

$Include "boxbuckshot_collision.qci"

$KeyValues
{
	prop_data 
	{
		"base" "item.small"
		"health" "0"
		"physicsmode" "0"
		"dmg.bullets" "0.0"
		"dmg.club" "0.0"
		"dmg.explosive" "0.0"
		"explosive_radius" "0"
		"explosive_damage" "0.0"
	}
}
