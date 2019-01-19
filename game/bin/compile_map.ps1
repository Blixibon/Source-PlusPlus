param(
    [parameter(mandatory)]
    [Alias("VProject")]
    [string]$game,
    [parameter(mandatory)]
    [string]$vmf,
    [parameter()]
    [string[]]$global_args,
    [parameter()]
    [string[]]$bsp_args,
    [parameter()]
    [string[]]$vis_args,
    [parameter()]
    [string[]]$rad_args=("-both")
)

function Get-ScriptDirectory
{
  $Invocation = (Get-Variable MyInvocation -Scope 1).Value
  Split-Path $Invocation.MyCommand.Path
}

$toolsdir = Get-ScriptDirectory

Start-Process -FilePath $toolsdir\vbsp.exe -ArgumentList "-game $game $bsp_args $vmf" -NoNewWindow -Wait

Start-Process -FilePath $toolsdir\vvis.exe -ArgumentList "-game $game $vis_args $vmf" -NoNewWindow -Wait

Start-Process -FilePath $toolsdir\vrad.exe -ArgumentList "-game $game $rad_args $vmf" -NoNewWindow -Wait

$filename_base = [System.IO.Path]::GetFileNameWithoutExtension($vmf)
$filename_path = [System.IO.Path]::GetDirectoryName($vmf)
$srcbsp_path = $filename_path + "\" + $filename_base + ".bsp"
$dstbsp_path = $game + "\maps\" + $filename_base + ".bsp"

Move-Item -Path $srcbsp_path -Destination $dstbsp_path

Start-Process -FilePath $toolsdir\vbspinfo.exe -ArgumentList "-X0 $dstbsp_path" -NoNewWindow -Wait