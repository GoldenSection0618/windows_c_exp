param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Binary,

    [Parameter(Mandatory = $true, Position = 1)]
    [string]$TestDir,

    [Parameter(Mandatory = $true, Position = 2)]
    [string]$SuitesFile,

    [Parameter(Mandatory = $true, Position = 3)]
    [string]$OutputDir,

    [Parameter(Position = 4)]
    [string]$SingleRunner = "scripts\run_smoke_test.ps1"
)

$ErrorActionPreference = "Stop"
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = $Utf8NoBom
[Console]::OutputEncoding = $Utf8NoBom

function Resolve-ExistingPath([string]$Path, [string]$Kind) {
    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction SilentlyContinue
    if (-not $resolved) {
        throw "[test-batch] error: $Kind not found: $Path"
    }
    return $resolved.ProviderPath
}

$binaryPath = Resolve-ExistingPath $Binary "binary"
$testDirPath = Resolve-ExistingPath $TestDir "test dir"
$suitesPath = Resolve-ExistingPath $SuitesFile "suites file"
$runnerPath = Resolve-ExistingPath $SingleRunner "runner"
$outputDirPath = [System.IO.Path]::GetFullPath($OutputDir)

New-Item -ItemType Directory -Force -Path $outputDirPath | Out-Null

$failed = $false
$suites = [System.IO.File]::ReadAllLines($suitesPath, [System.Text.Encoding]::UTF8)

foreach ($rawSuite in $suites) {
    $suite = $rawSuite.Trim()
    if ($suite.Length -eq 0 -or $suite.StartsWith("#")) {
        continue
    }

    $inputFile = [System.IO.Path]::Combine($testDirPath, "$suite.input")
    $expectFile = [System.IO.Path]::Combine($testDirPath, "$suite.expect.tsv")
    $outputFile = [System.IO.Path]::Combine($outputDirPath, "test_output_$suite.txt")

    Write-Host "[test-batch] suite=$suite"
    try {
        & powershell -NoProfile -ExecutionPolicy Bypass -File $runnerPath $binaryPath $inputFile $expectFile $outputFile
        if ($LASTEXITCODE -ne 0) {
            $failed = $true
        }
    } catch {
        Write-Error $_.Exception.Message -ErrorAction Continue
        $failed = $true
    }
}

if ($failed) {
    throw "[test-batch] failed. suites file: $SuitesFile"
}

Write-Host "[test-batch] all suites passed ($SuitesFile)"
