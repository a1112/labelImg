param(
    [string]$Configuration = "Release",
    [string]$BuildDir = "",
    [string]$QtBin = "D:\Qt\6.11.0\msvc2022_64\bin",
    [string]$Version = "0.1.0"
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$cppRoot = Resolve-Path (Join-Path $scriptRoot "..")
$repoRoot = Resolve-Path (Join-Path $cppRoot "..")
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $cppRoot "build"
}

$distRoot = Join-Path $cppRoot "dist"
$deployName = "labelImgCpp-$Version-win64"
$deployRoot = Join-Path $distRoot $deployName
$installerWork = Join-Path $distRoot "installer-work"
$payloadZip = Join-Path $installerWork "payload.zip"
$installerExe = Join-Path $distRoot "$deployName-installer.exe"
$portableZip = Join-Path $distRoot "$deployName.zip"

if (!(Test-Path (Join-Path $BuildDir "CMakeCache.txt"))) {
    $qtPrefix = Split-Path -Parent $QtBin
    & cmake -S $cppRoot -B $BuildDir -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="$qtPrefix"
}

& cmake --build $BuildDir --config $Configuration --target labelImgCpp

$builtExe = Join-Path $BuildDir "$Configuration\labelImgCpp.exe"
if (!(Test-Path $builtExe)) {
    throw "Built executable was not found: $builtExe"
}

Remove-Item -LiteralPath $deployRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $installerWork -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $installerExe -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $portableZip -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $deployRoot, $installerWork | Out-Null

Copy-Item -LiteralPath $builtExe -Destination $deployRoot
$windDeployQt = Join-Path $QtBin "windeployqt.exe"
if (!(Test-Path $windDeployQt)) {
    throw "windeployqt was not found: $windDeployQt"
}
& $windDeployQt --release --dir $deployRoot (Join-Path $deployRoot "labelImgCpp.exe")

New-Item -ItemType Directory -Force -Path (Join-Path $deployRoot "resources"), (Join-Path $deployRoot "data") | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot "resources\icons") -Destination (Join-Path $deployRoot "resources\icons") -Recurse
Copy-Item -LiteralPath (Join-Path $repoRoot "resources\strings") -Destination (Join-Path $deployRoot "resources\strings") -Recurse
Copy-Item -LiteralPath (Join-Path $repoRoot "data\predefined_classes.txt") -Destination (Join-Path $deployRoot "data\predefined_classes.txt")
Copy-Item -LiteralPath (Join-Path $repoRoot "LICENSE") -Destination (Join-Path $deployRoot "LICENSE.txt")
Copy-Item -LiteralPath (Join-Path $cppRoot "README.md") -Destination (Join-Path $deployRoot "README.txt")

@'
$ErrorActionPreference = "Stop"
$installRoot = Join-Path $env:LOCALAPPDATA "Programs\labelImgCpp"
$startMenu = [Environment]::GetFolderPath("Programs")
$desktop = [Environment]::GetFolderPath("DesktopDirectory")
$shortcutPaths = @(
    (Join-Path $startMenu "labelImgCpp.lnk"),
    (Join-Path $desktop "labelImgCpp.lnk")
)
foreach ($shortcut in $shortcutPaths) {
    Remove-Item -LiteralPath $shortcut -Force -ErrorAction SilentlyContinue
}
Remove-Item -LiteralPath $installRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $installRoot | Out-Null
Expand-Archive -LiteralPath (Join-Path $PSScriptRoot "payload.zip") -DestinationPath $installRoot -Force

$exe = Join-Path $installRoot "labelImgCpp.exe"
$shell = New-Object -ComObject WScript.Shell
foreach ($shortcut in $shortcutPaths) {
    $link = $shell.CreateShortcut($shortcut)
    $link.TargetPath = $exe
    $link.WorkingDirectory = $installRoot
    $link.IconLocation = "$exe,0"
    $link.Save()
}
'@ | Set-Content -LiteralPath (Join-Path $installerWork "install.ps1") -Encoding UTF8

@'
$ErrorActionPreference = "SilentlyContinue"
$installRoot = Join-Path $env:LOCALAPPDATA "Programs\labelImgCpp"
Remove-Item -LiteralPath (Join-Path ([Environment]::GetFolderPath("Programs")) "labelImgCpp.lnk") -Force
Remove-Item -LiteralPath (Join-Path ([Environment]::GetFolderPath("DesktopDirectory")) "labelImgCpp.lnk") -Force
Remove-Item -LiteralPath $installRoot -Recurse -Force
'@ | Set-Content -LiteralPath (Join-Path $deployRoot "uninstall.ps1") -Encoding UTF8

Compress-Archive -Path (Join-Path $deployRoot "*") -DestinationPath $payloadZip -Force
Compress-Archive -Path (Join-Path $deployRoot "*") -DestinationPath $portableZip -Force

$sedPath = Join-Path $installerWork "labelImgCpp.sed"
$installCommand = 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File install.ps1'
@"
[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=1
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=N
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=<None>
AdminQuietInstCmd=%AppLaunched%
UserQuietInstCmd=%AppLaunched%
SourceFiles=SourceFiles

[Strings]
InstallPrompt=
DisplayLicense=
FinishMessage=labelImgCpp has been installed.
TargetName=$installerExe
FriendlyName=labelImgCpp
AppLaunched=$installCommand
FILE0=payload.zip
FILE1=install.ps1

[SourceFiles]
SourceFiles0=$installerWork

[SourceFiles0]
%FILE0%=
%FILE1%=
"@ | Set-Content -LiteralPath $sedPath -Encoding ASCII

& "$env:SystemRoot\System32\iexpress.exe" /N /Q $sedPath
if (!(Test-Path $installerExe)) {
    throw "Installer was not created: $installerExe"
}

Write-Host "Deploy directory: $deployRoot"
Write-Host "Portable zip: $portableZip"
Write-Host "Installer: $installerExe"
