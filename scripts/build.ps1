#requires -Version 7.0
[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$SkipDeploy,
    [switch]$RebuildQWindowKit
)

$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$QtRoot = 'D:\Library\Qt\Qt'
$QtVersion = '6.12.0'
$QtDir = Join-Path $QtRoot "$QtVersion\mingw_64"
$MingwBin = Join-Path $QtRoot 'Tools\mingw1510_64\bin'
$CMake = Join-Path $QtRoot 'Tools\CMake_64\bin\cmake.exe'
$Ninja = Join-Path $QtRoot 'Tools\Ninja\ninja.exe'
$Git = 'D:\Library\Git\bin\git.exe'

$QWindowKitSource = Join-Path $RepoRoot 'qwindowkit'
$QWindowKitBuild = Join-Path $RepoRoot 'build-qwindowkit'
$QWindowKitInstall = Join-Path $RepoRoot 'third_party\QWindowKit'
$QWindowKitConfig = Join-Path $QWindowKitInstall 'lib\cmake\QWindowKit\QWindowKitConfig.cmake'

$BuildDir = Join-Path $RepoRoot 'build'
$OutputDir = Join-Path $RepoRoot 'output'

foreach ($path in @($QtDir, $MingwBin, $CMake, $Ninja, $QWindowKitSource)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required path not found: $path"
    }
}

$env:Path = "$MingwBin;$(Join-Path $QtDir 'bin');$env:Path"
$env:CC = Join-Path $MingwBin 'gcc.exe'
$env:CXX = Join-Path $MingwBin 'g++.exe'

function Invoke-Checked {
    param(
        [Parameter(Mandatory)] [scriptblock]$Command,
        [Parameter(Mandatory)] [string]$Description
    )
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed with exit code $LASTEXITCODE"
    }
}

function Ensure-QWindowKitDependencies {
    $qmsetup = Join-Path $QWindowKitSource 'qmsetup'
    $stdcore = Join-Path $qmsetup 'src\stdcorelib\CMakeLists.txt'

    if (-not (Test-Path -LiteralPath (Join-Path $qmsetup 'CMakeLists.txt'))) {
        $gitExe = if (Test-Path -LiteralPath $Git) { $Git } else { 'git' }
        Write-Host 'Cloning qmsetup ...'
        Invoke-Checked { & $gitExe clone --depth 1 https://github.com/stdware/qmsetup.git $qmsetup } 'git clone qmsetup'
    }

    if (-not (Test-Path -LiteralPath $stdcore)) {
        $gitExe = if (Test-Path -LiteralPath $Git) { $Git } else { 'git' }
        Write-Host 'Updating qmsetup submodules ...'
        Invoke-Checked { & $gitExe -C $qmsetup submodule update --init --recursive --depth 1 } 'git submodule update'
    }
}

function Build-QWindowKit {
    if ((Test-Path -LiteralPath $QWindowKitConfig) -and -not $RebuildQWindowKit -and -not $Clean) {
        Write-Host "Using existing QWindowKit: $QWindowKitInstall"
        return
    }

    Ensure-QWindowKitDependencies

    if ($Clean -and (Test-Path -LiteralPath $QWindowKitBuild)) {
        Remove-Item -LiteralPath $QWindowKitBuild -Recurse -Force
    }

    Write-Host 'Configuring QWindowKit (static, MinGW) ...'
    Invoke-Checked {
        & $CMake -S $QWindowKitSource -B $QWindowKitBuild -G Ninja `
            -DCMAKE_MAKE_PROGRAM="$Ninja" `
            -DCMAKE_C_COMPILER="$MingwBin\gcc.exe" `
            -DCMAKE_CXX_COMPILER="$MingwBin\g++.exe" `
            -DCMAKE_PREFIX_PATH="$QtDir" `
            -DCMAKE_INSTALL_PREFIX="$QWindowKitInstall" `
            -DCMAKE_BUILD_TYPE=Release `
            -DQWINDOWKIT_BUILD_STATIC=ON `
            -DQWINDOWKIT_BUILD_WIDGETS=ON `
            -DQWINDOWKIT_BUILD_QUICK=OFF `
            -DQWINDOWKIT_BUILD_EXAMPLES=OFF `
            -DQWINDOWKIT_BUILD_TESTS=OFF `
            -DQWINDOWKIT_INSTALL=ON
    } 'QWindowKit configure'

    Write-Host 'Building QWindowKit ...'
    Invoke-Checked { & $CMake --build $QWindowKitBuild --config Release } 'QWindowKit build'
    Invoke-Checked { & $CMake --install $QWindowKitBuild --config Release } 'QWindowKit install'
}

function Build-Application {
    if ($Clean -and (Test-Path -LiteralPath $BuildDir)) {
        Remove-Item -LiteralPath $BuildDir -Recurse -Force
    }

    Write-Host 'Configuring JfzQtRDP ...'
    Invoke-Checked {
        & $CMake -S $RepoRoot -B $BuildDir -G Ninja `
            -DCMAKE_MAKE_PROGRAM="$Ninja" `
            -DCMAKE_C_COMPILER="$MingwBin\gcc.exe" `
            -DCMAKE_CXX_COMPILER="$MingwBin\g++.exe" `
            -DCMAKE_PREFIX_PATH="$QtDir" `
            -DQWindowKit_DIR="$QWindowKitInstall\lib\cmake\QWindowKit" `
            -DCMAKE_BUILD_TYPE=Release
    } 'Application configure'

    Write-Host 'Building JfzQtRDP ...'
    Invoke-Checked { & $CMake --build $BuildDir --config Release } 'Application build'
}

function Deploy-Application {
    $exe = Join-Path $BuildDir 'JfzQtRDP.exe'
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "Built executable not found: $exe"
    }

    if (Test-Path -LiteralPath $OutputDir) {
        $resolvedOutput = (Resolve-Path -LiteralPath $OutputDir).Path
        if ($resolvedOutput -ne [System.IO.Path]::GetFullPath($OutputDir)) {
            throw "Unexpected output path: $resolvedOutput"
        }
        Remove-Item -LiteralPath $OutputDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $OutputDir | Out-Null

    Copy-Item -LiteralPath $exe -Destination $OutputDir -Force

    $windeployqt = Join-Path $QtDir 'bin\windeployqt.exe'
    Write-Host 'Deploying Qt runtime ...'
    Invoke-Checked {
        & $windeployqt --release --no-translations --no-opengl-sw --compiler-runtime `
            --dir $OutputDir (Join-Path $OutputDir 'JfzQtRDP.exe')
    } 'windeployqt'

    foreach ($runtime in @('libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll')) {
        $source = Join-Path $QtDir "bin\$runtime"
        if (Test-Path -LiteralPath $source) {
            Copy-Item -LiteralPath $source -Destination $OutputDir -Force
        }
    }

    Remove-Item -LiteralPath (Join-Path $OutputDir 'JfzQtRDP.log') -Force -ErrorAction SilentlyContinue
    Write-Host "Output: $OutputDir"
    Get-ChildItem -LiteralPath $OutputDir | Select-Object Name, Length
}

Build-QWindowKit
Build-Application
if (-not $SkipDeploy) {
    Deploy-Application
}

Write-Host 'Build complete.'
