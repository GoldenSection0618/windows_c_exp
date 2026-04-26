param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Binary,

    [Parameter(Mandatory = $true, Position = 1)]
    [string]$InputFile,

    [Parameter(Mandatory = $true, Position = 2)]
    [string]$ExpectFile,

    [Parameter(Mandatory = $true, Position = 3)]
    [string]$OutputFile
)

$ErrorActionPreference = "Stop"
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = $Utf8NoBom
[Console]::OutputEncoding = $Utf8NoBom

function Resolve-ExistingPath([string]$Path, [string]$Kind) {
    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction SilentlyContinue
    if (-not $resolved) {
        throw "[test] error: $Kind not found: $Path"
    }
    return $resolved.ProviderPath
}

function Convert-ToBashPath([string]$Path, [bool]$UseWsl) {
    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if ($UseWsl -and $fullPath -match '^([A-Za-z]):\\(.*)$') {
        $drive = $matches[1].ToLowerInvariant()
        $rest = $matches[2] -replace '\\', '/'
        return "/mnt/$drive/$rest"
    }
    return ($fullPath -replace '\\', '/')
}

function Quote-Bash([string]$Text) {
    return "'" + ($Text -replace "'", "'\''") + "'"
}

function Invoke-OptionalScript([string]$BasePath, [string[]]$Arguments) {
    $psScript = "$BasePath.ps1"
    $shScript = "$BasePath.sh"

    if (Test-Path -LiteralPath $psScript) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File $psScript @Arguments
        if ($LASTEXITCODE -ne 0) {
            throw "[test] script failed: $psScript"
        }
        return
    }

    if (-not (Test-Path -LiteralPath $shScript)) {
        return
    }

    $bash = Get-Command bash.exe -ErrorAction SilentlyContinue
    if (-not $bash) {
        throw "[test] error: setup/verify script requires bash, but bash.exe was not found: $shScript"
    }

    $uname = (& $bash.Source -lc "uname -a" 2>$null)
    $useWsl = ($uname -match "Microsoft|WSL|Linux")
    $cwd = Convert-ToBashPath (Get-Location).Path $useWsl
    $script = Convert-ToBashPath $shScript $useWsl
    $bashArgs = $Arguments | ForEach-Object { Convert-ToBashPath $_ $useWsl }
    $quotedArgs = ($bashArgs | ForEach-Object { Quote-Bash $_ }) -join " "
    $command = "cd $(Quote-Bash $cwd) && bash $(Quote-Bash $script) $quotedArgs"

    & $bash.Source -lc $command
    if ($LASTEXITCODE -ne 0) {
        throw "[test] script failed: $shScript"
    }
}

function Backup-DataFile([string]$RelativePath, [string]$BackupPath, [string]$MissingMarker) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $BackupPath) | Out-Null
    Remove-Item -LiteralPath $BackupPath, $MissingMarker -Force -ErrorAction SilentlyContinue

    if (Test-Path -LiteralPath $RelativePath) {
        Copy-Item -LiteralPath $RelativePath -Destination $BackupPath -Force
    } else {
        New-Item -ItemType File -Path $MissingMarker -Force | Out-Null
    }
}

function Restore-DataFile([string]$RelativePath, [string]$BackupPath, [string]$MissingMarker) {
    if (Test-Path -LiteralPath $BackupPath) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $RelativePath) | Out-Null
        Move-Item -LiteralPath $BackupPath -Destination $RelativePath -Force
    } elseif (Test-Path -LiteralPath $MissingMarker) {
        Remove-Item -LiteralPath $RelativePath -Force -ErrorAction SilentlyContinue
    }

    Remove-Item -LiteralPath $BackupPath, $MissingMarker -Force -ErrorAction SilentlyContinue
}

function Invoke-TestBinary([string]$ExePath, [string]$InputPath, [string]$OutputPath) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $OutputPath) | Out-Null
    $stderrPath = "$OutputPath.stderr"
    Remove-Item -LiteralPath $OutputPath, $stderrPath -Force -ErrorAction SilentlyContinue

    $process = Start-Process -FilePath $ExePath `
        -WorkingDirectory (Get-Location).Path `
        -RedirectStandardInput $InputPath `
        -RedirectStandardOutput $OutputPath `
        -RedirectStandardError $stderrPath `
        -NoNewWindow `
        -Wait `
        -PassThru

    if (Test-Path -LiteralPath $stderrPath) {
        $stderr = [System.IO.File]::ReadAllText($stderrPath, [System.Text.Encoding]::UTF8)
        if ($stderr.Length -gt 0) {
            [System.IO.File]::AppendAllText($OutputPath, $stderr, $Utf8NoBom)
        }
        Remove-Item -LiteralPath $stderrPath -Force -ErrorAction SilentlyContinue
    }

    if ($process.ExitCode -ne 0) {
        throw "[test] binary exited with code $($process.ExitCode): $ExePath"
    }
}

$binaryPath = Resolve-ExistingPath $Binary "binary"
$inputPath = Resolve-ExistingPath $InputFile "input file"
$expectPath = Resolve-ExistingPath $ExpectFile "expect file"
$outputPath = [System.IO.Path]::GetFullPath($OutputFile)
$suiteBase = [System.IO.Path]::Combine(
    [System.IO.Path]::GetDirectoryName($inputPath),
    [System.IO.Path]::GetFileNameWithoutExtension($inputPath)
)
$backupStateDir = [System.IO.Path]::Combine("build", "test_state")
$backupName = [System.IO.Path]::GetFileName($suiteBase)

$dataFiles = @(
    @{ Path = "data\cards.txt"; Backup = "$backupName.cards.txt.bak"; Missing = "$backupName.cards.txt.missing" },
    @{ Path = "data\billings.txt"; Backup = "$backupName.billings.txt.bak"; Missing = "$backupName.billings.txt.missing" },
    @{ Path = "data\money.txt"; Backup = "$backupName.money.txt.bak"; Missing = "$backupName.money.txt.missing" }
)

foreach ($file in $dataFiles) {
    Backup-DataFile $file.Path ([System.IO.Path]::Combine($backupStateDir, $file.Backup)) ([System.IO.Path]::Combine($backupStateDir, $file.Missing))
}

try {
    Invoke-OptionalScript "$suiteBase.setup" @($binaryPath, $inputPath, $expectPath, $outputPath)
    Invoke-TestBinary $binaryPath $inputPath $outputPath

    $failed = $false
    $lineNo = 0
    $outputLines = [System.IO.File]::ReadAllLines($outputPath, [System.Text.Encoding]::UTF8)
    $expectLines = [System.IO.File]::ReadAllLines($expectPath, [System.Text.Encoding]::UTF8)

    foreach ($line in $expectLines) {
        $lineNo++
        if ([string]::IsNullOrWhiteSpace($line) -or $line.TrimStart().StartsWith("#")) {
            continue
        }

        $parts = $line -split "`t", 2
        if ($parts.Count -ne 2 -or $parts[0] -notmatch '^\d+$') {
            throw "[test] error: invalid expect format at line $lineNo`: $line"
        }

        $expected = [int]$parts[0]
        $pattern = $parts[1]
        $actual = @($outputLines | Where-Object { $_.Contains($pattern) }).Count
        if ($actual -ne $expected) {
            Write-Error "[test] mismatch at line $lineNo`: expected=$expected, actual=$actual, pattern=$pattern" -ErrorAction Continue
            $failed = $true
        }
    }

    if ($failed) {
        throw "[test] failed. output: $OutputFile"
    }

    Invoke-OptionalScript "$suiteBase.verify" @($binaryPath, $inputPath, $expectPath, $outputPath)
    Write-Host "[test] passed ($InputFile -> $OutputFile)"
} finally {
    foreach ($file in $dataFiles) {
        Restore-DataFile $file.Path ([System.IO.Path]::Combine($backupStateDir, $file.Backup)) ([System.IO.Path]::Combine($backupStateDir, $file.Missing))
    }
}
