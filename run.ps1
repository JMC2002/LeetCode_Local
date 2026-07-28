param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateRange(1, [int]::MaxValue)]
    [int] $Id,

    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Debug',

    [string] $Distro = 'Ubuntu-26.04'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$workspace = (Resolve-Path $PSScriptRoot).Path
& (Join-Path $workspace 'select.ps1') $Id

$wslWorkspace = (& wsl.exe -d $Distro --cd $workspace pwd).Trim()
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($wslWorkspace)) {
    throw "无法为 WSL 发行版 '$Distro' 转换工作区路径。"
}

$presetSuffix = $Configuration.ToLowerInvariant()
$preset = "wsl-gcc17-cxx29-$presetSuffix"
$executable = "$wslWorkspace/out/build/$preset/lc"

& wsl.exe -d $Distro --cd $wslWorkspace `
    cmake --preset $preset
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& wsl.exe -d $Distro --cd $wslWorkspace `
    cmake --build --preset $preset --target lc
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& wsl.exe -d $Distro --cd $wslWorkspace `
    env LD_LIBRARY_PATH=/opt/gcc-trunk/lib64 $executable
exit $LASTEXITCODE
