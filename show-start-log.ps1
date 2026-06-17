$LogPath = Join-Path $PSScriptRoot 'Saved\Logs\TryComboWepon.log'
if (-not (Test-Path -LiteralPath $LogPath)) {
    Write-Host "Log file not found: $LogPath" -ForegroundColor Yellow
    exit 1
}
Select-String -LiteralPath $LogPath -Pattern 'LogCombatStartFlow|LogCombatStartMenuWidget|LogCombatGameStart|StartCombatGame|Playing background music|Applied AI state|Loaded default music' | Select-Object -Last 160 | ForEach-Object { $_.Line }
