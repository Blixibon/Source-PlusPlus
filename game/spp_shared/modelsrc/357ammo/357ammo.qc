// Created by Crowbar 0.58

$ModelName "items/357ammo.mdl"

$BodyGroup "357box"
{
	studio "357ammo_box.dmx"
}

$BodyGroup "357ammo"
{
	studio "357ammo_full.dmx"
	studio "357ammo_toprow.dmx"
	studio "357ammo_topleft.dmx"
}

$Include "357ammo_lod.qci"

$SurfaceProp "item"

$Contents "solid"

$IllumPosition -0.23 0.001 4.36

$Opaque

$CDMaterials "models\items\"

$Include "357ammo_box.qci"

$Include "357ammo_bone.qci"

$Include "357ammo_animation.qci"

$Include "357ammo_collision.qci"

$KeyValues
{
	prop_data 
	{
		"base" "Item.Small"
	}
}
