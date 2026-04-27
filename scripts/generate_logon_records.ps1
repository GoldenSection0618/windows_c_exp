param(
    [string]$OutputDir = "data",
    [int]$CardCount = 100,
    [int]$SessionsPerCard = 3
)

function Format-Time($timestamp) {
    $epoch = Get-Date '1970-01-01 00:00:00'
    return (Get-Date ($epoch.AddSeconds([double]$timestamp)) -Format 'yyyy-MM-dd HH:mm:ss')
}

function Get-RandomUniqueOffsets($min, $max, $count) {
    $range = $min..$max
    $shuffled = $range | Get-Random -Count $range.Count
    return $shuffled[0..($count-1)]
}

if ($CardCount -lt 1) { Write-Error 'card_count must be a positive integer'; exit 1 }
if ($SessionsPerCard -lt 1) { Write-Error 'sessions_per_card must be a positive integer'; exit 1 }

$RequiredSlots = $CardCount * $SessionsPerCard
$MinOffsetMinutes = 180
$MaxOffsetMinutes = 365 * 24 * 60 - 1
if ($RequiredSlots -gt ($MaxOffsetMinutes - $MinOffsetMinutes + 1)) {
    Write-Error "not enough unique time slots in the past week for $RequiredSlots billing records"
    exit 1
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$cards = @()
$billings = @()
$now = [int][double]::Parse((Get-Date -UFormat %s))
$offsets = Get-RandomUniqueOffsets $MinOffsetMinutes $MaxOffsetMinutes $RequiredSlots
$slotIndex = 0

for ($cardIndex = 1; $cardIndex -le $CardCount; $cardIndex++) {
    $cardName = ('stats{0:D3}' -f $cardIndex)
    $password = ('pw{0:D6}' -f $cardIndex)
    $totalUseCent = 0
    $earliestStart = 0
    $latestEnd = 0
    for ($sessionIndex = 1; $sessionIndex -le $SessionsPerCard; $sessionIndex++) {
        $offsetMinutes = $offsets[$slotIndex]
        $slotIndex++
        $startTs = $now - $offsetMinutes * 60
        $durationMinutes = Get-Random -Minimum 10 -Maximum 180
        if ($durationMinutes -ge $offsetMinutes) { $durationMinutes = $offsetMinutes - 1 }
        if ($durationMinutes -lt 1) { $durationMinutes = 1 }
        $endTs = $startTs + $durationMinutes * 60
        if ($endTs -gt $now) { $endTs = $now - 60 }
        $amountCent = $durationMinutes
        $totalUseCent += $amountCent
        if ($earliestStart -eq 0 -or $startTs -lt $earliestStart) { $earliestStart = $startTs }
        if ($endTs -gt $latestEnd) { $latestEnd = $endTs }
        $billings += "{0}|{1}|{2}|{3}|1|0" -f $cardName, (Format-Time $startTs), (Format-Time $endTs), $amountCent
    }
    $extraBalanceCent = Get-Random -Minimum 1000 -Maximum 10000
    $balanceCent = $totalUseCent + $extraBalanceCent
    $issueTs = $earliestStart - 7 * 24 * 60 * 60
    $expiryTs = $issueTs + 365 * 24 * 60 * 60
    $cards += "{0}|{1}|0|{2}|{3}|{4}|{5}|{6}|{7}|0" -f $cardName, $password, (Format-Time $issueTs), (Format-Time $expiryTs), $totalUseCent, (Format-Time $latestEnd), $SessionsPerCard, $balanceCent
}

$cards | Sort-Object | Set-Content -Encoding UTF8 "$OutputDir/cards.txt"
$billings | Sort-Object | Set-Content -Encoding UTF8 "$OutputDir/billings.txt"
Write-Host "generated $CardCount cards and $($CardCount * $SessionsPerCard) billing records in $OutputDir"